import ctypes
import json
import sudoku_utils.python_grids

from channels.generic.websocket import WebsocketConsumer
from asgiref.sync import async_to_sync

from django.db import IntegrityError, transaction
import sudoku.models
import sudoku.views

# the prefix is used to make sure
# there is no name conflict
#
# main attributes attributes:
#   self.GameConsumer_user
#   self.GameConsumer_game
# 
#   self.GameConsumer_group_name = f"game_group_{self.game_id}"
#
class GameConsumer(WebsocketConsumer):
    def connect(self):

        # this is used to avoid errors
        # during disconnection
        self.GameConsumer_connect_aborted = True


        game_id = self.scope["url_route"]["kwargs"]["game_id"]
        user = self.scope["user"]

        if not user.is_authenticated:
            return None


        game_obj_query_set = sudoku.models.Game.objects.filter(id=game_id)

        if len(game_obj_query_set) != 1:
            return None

        game_obj = game_obj_query_set[0]

        # FIXME: no need for this since it is checked by views.game ?
        sudoku.views.helper__end_game(game_obj, only_on_timeout = True)

        if game_obj.player_1 == user:
            self.GameConsumer_in_game_id = 1
            self.GameConsumer_in_game_origin_code = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_PLAYER_1
        else:
            self.GameConsumer_in_game_id = 2
            self.GameConsumer_in_game_origin_code = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_PLAYER_2

        # connect to the group
        # of this game
        self.GameConsumer_user = user
        self.GameConsumer_game = game_obj

        self.GameConsumer_group_name = f"game_group_{self.GameConsumer_game.id}"

        # join the group
        async_to_sync(self.channel_layer.group_add)(
            self.GameConsumer_group_name, self.channel_name
        )

        self.GameConsumer_connect_aborted = False
        self.accept()

        # send the game to the user
        if self.GameConsumer_game.end_date != None:
            self.GameConsumer_send_position(include_solution=True)

        elif self.GameConsumer_game.start_date != None:
            self.GameConsumer_send_position(include_solution=False)




    def disconnect(self, close_code):
        if self.GameConsumer_connect_aborted:
            return None

        async_to_sync(self.channel_layer.group_discard)(
            self.GameConsumer_group_name, self.channel_name
        )

        sudoku.views.helper__end_game(self.GameConsumer_game, only_on_timeout = True)


    def receive(self, text_data):
        # the user request is a json object
        req_json = json.loads(text_data)

        sudoku.views.helper__end_game(self.GameConsumer_game, only_on_timeout = True)
        self.GameConsumer_game.refresh_from_db()

        # empty type is used to end game on timeout
        if req_json["type"] == None:
            pass

        elif req_json["type"] == "get_position":
            if self.GameConsumer_game.end_date != None:
                return self.GameConsumer_send_position(include_solution=True)
            elif self.GameConsumer_game.start_date != None:
                return self.GameConsumer_send_position(include_solution=False)
            else:
                return self.send(text_data=json.dumps({"error": "ERROR_GAME_DID_NOT_START"}))

        elif req_json["type"] == "make_move":
            # can not make a move if game is over
            if self.GameConsumer_game.end_date != None:
                return None

            row = req_json["data"]["row"]
            col = req_json["data"]["col"]
            val = req_json["data"]["val"]

            # catch invalid values
            if(
                type(row) != int or not 0 <= row < 9 or
                type(col) != int or not 0 <= col < 9 or
                type(val) != int or not 0 <= val <= 9
            ):
                return None


            # FIXME: no need to re send entire position
            # FIXME: notify the other user only when his position changed
            self.GameConsumer_game.make_move(row, col, val, self.GameConsumer_in_game_id)

            async_to_sync(self.channel_layer.group_send)(
                self.GameConsumer_group_name,
                {
                    "type": "GameConsumer.send.position",
                    "include_solution": False
                }
            )
            return None

        else:
            raise Exception()


    def GameConsumer_send_position(self, event = None, include_solution = False):

        # Send the position to websocket
        if event != None:
            include_solution = event["include_solution"]

        self.GameConsumer_game.refresh_from_db()

        if self.GameConsumer_in_game_id == 1:
            position_bytes = self.GameConsumer_game.player_1_position
        else:
            position_bytes = self.GameConsumer_game.player_2_position

        position = sudoku_utils.python_grids.bytes_to_current_position(position_bytes)
        position_str = sudoku_utils.python_grids.current_position_to_json_string(
                            ctypes.byref(position),
                            include_solution
                        ).decode('utf-8')

        if self.GameConsumer_game.start_date == None:
            start_date_str = None;
        else:
            start_date_str = self.GameConsumer_game.start_date.isoformat()
        self.send(text_data=json.dumps({
                                        "type":"position",
                                        "position": position_str,
                                        "start_date": start_date_str
                                        }))


    def GameConsumer_end_game(self, event):
        """called when the game ends to notify user
        and update user game.end_date"""

        self.GameConsumer_game.refresh_from_db()

        player_1_rating = self.GameConsumer_game.player_1.userprofile.rating

        if self.GameConsumer_game.player_2 is None:
            player_2_rating = None
        else:
            player_2_rating = self.GameConsumer_game.player_2.userprofile.rating


        self.send(text_data=json.dumps(
            {
                "type": "game_over",
                "end_date": self.GameConsumer_game.end_date.isoformat(),
                "winner": self.GameConsumer_game.winner,

                "player_1_rating": player_1_rating,
                "player_2_rating": player_2_rating,
            }))

        # send the solution since game is over
        self.GameConsumer_send_position(include_solution = True)

    def GameConsumer_send_object(self, event):
        """used to send some object. this avoids creating a method for
        every simple use case"""

        obj = event["object"]

        self.send(text_data=json.dumps(obj))




#################################################
#################################################

class UserConsumer(WebsocketConsumer):
    """this consumer is used to notify a user that something changed.
    GameConsumer used exclusively for things related to a game"""

    def connect(self):
        # this is used to avoid errors
        # during disconnection
        self.UserConsumer_connect_aborted = True

        user = self.scope["user"]

        if not user.is_authenticated:
            return None

        self.UserConsumer_user = user

        self.UserConsumer_group_name = f"user_group_{self.UserConsumer_user.id}"

        # join the group
        async_to_sync(self.channel_layer.group_add)(
            self.UserConsumer_group_name, self.channel_name
        )


        self.UserConsumer_connect_aborted = False
        self.accept()

    def disconnect(self, close_code):
        if self.UserConsumer_connect_aborted:
            return None

        async_to_sync(self.channel_layer.group_discard)(
            self.UserConsumer_group_name, self.channel_name
        )


    # for now no request is made by user
    def receive(self, text_data):
        pass


    def UserConsumer_notify(self, event):
        """send event["message"] to user"""

        self.send(event["message"])

