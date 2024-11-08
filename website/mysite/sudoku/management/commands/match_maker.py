from django.core.management.base import BaseCommand, CommandError
from django.db import IntegrityError, transaction

from channels.layers import get_channel_layer
from asgiref.sync import async_to_sync

import json
import time


import sudoku.models
import sudoku_utils.python_grids

def create_match(elt1, elt2):
    """helper function , it tries to create a game.
    elt1 and elt2 must be of type sudoku.models.MatchWaitingList"""

    if( type(elt1) != sudoku.models.MatchWaitingList or
        type(elt2) != sudoku.models.MatchWaitingList
            ):
        raise TypeError()

    new_position = sudoku_utils.python_grids.get_new_game_position(elt1.game_level)

    try:
        with transaction.atomic():
            new_game = sudoku.models.Game.objects.create(
                                            game_type = "GAME_TYPE_MULTIPLAYER",
                                            game_duration = elt1.game_duration,
                                            game_level = elt1.game_level,

                                            player_1 = elt1.user,
                                            player_2 = elt2.user,

                                            player_1_position = bytes(new_position),
                                            player_2_position = bytes(new_position) )

            print("new game id:", new_game.id)

            if( elt1.user.userprofile.current_game != None or
                elt2.user.userprofile.current_game != None or
                not hasattr(elt1.user, 'matchwaitinglist') or
                not hasattr(elt2.user, 'matchwaitinglist')
                    ):
                raise sudoku.models.SudokuError()

            elt1.user.userprofile.current_game = new_game
            elt2.user.userprofile.current_game = new_game

            elt1.user.userprofile.save()
            elt2.user.userprofile.save()

            elt1.delete()
            elt2.delete()

        # notify the users
        # no need to put it in the atomic block
        async_to_sync(get_channel_layer().group_send)(
            f"user_group_{elt1.user.id}",
            {
                "type": "UserConsumer.notify",
                "message": json.dumps({
                                    "type":"new_game",
                                    "game_id":new_game.id
                                    })      
            }
        )

        async_to_sync(get_channel_layer().group_send)(
            f"user_group_{elt2.user.id}",
            {
                "type": "UserConsumer.notify",
                "message": json.dumps({
                                    "type":"new_game",
                                    "game_id":new_game.id
                                    })      
            }
        )

        

    # just ignore the errors
    # and move to next pair
    except (sudoku.models.SudokuError, IntegrityError):
        pass


def create_matches():
    """called periodically to make the matches"""

    # (game_level, game_duration) -> [waiting users]
    waiting_lists = dict()

    # create the lists here to avoid the
    # check below to know if key exists
    for game_level in range(sudoku_utils.python_grids.NB_LEVELS):
        for game_duration in sudoku.models.possible_game_durations:
            waiting_lists[ (game_level, game_duration) ] = []

    for elt in sudoku.models.MatchWaitingList.objects.all():
        waiting_lists[ (elt.game_level, elt.game_duration) ].append(elt)

    for waiting_list in waiting_lists.values():

        # sort element by rating
        def key(elt):
            return elt.rating

        waiting_list.sort(key = key)

        for pos in range(len(waiting_list) // 2):
            e1 = waiting_list[2*pos]
            e2 = waiting_list[2*pos+1]

            create_match(e1, e2)




class Command(BaseCommand):
    help = "create the multiplayer games"

    def handle(self, *args, **options):
        print("make matches ...")

        while True:
            create_matches()
            time.sleep(10)



