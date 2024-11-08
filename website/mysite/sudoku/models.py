from django.db import models, signals
from django.contrib.auth.models import User
from django.dispatch import receiver
from django.utils import timezone

from django.db import IntegrityError, transaction

import sudoku_utils.python_grids

#################################################

class SudokuError(Exception):
    pass


#################################################

possible_game_durations = [3, 5, 10, 15, 30]

class Game(models.Model):

    game_type = models.CharField(max_length=30, null = False)
    game_duration = models.IntegerField(null = False)
    game_level = models.IntegerField(null = False)


    player_1 = models.ForeignKey(User,
                            on_delete=models.CASCADE,
                            null=True,
                            related_name='+')

    player_2 = models.ForeignKey(User,
                            on_delete=models.CASCADE,
                            null=True,
                            related_name='+')

    player_1_ready = models.BooleanField( default=False,
                                          null=False )

    player_2_ready = models.BooleanField( default=False,
                                          null=False )



    creation_date = models.DateTimeField(null=False)
    start_date = models.DateTimeField(null=True)
    end_date = models.DateTimeField(null=True)

    player_1_position = models.BinaryField(null=False)
    player_2_position = models.BinaryField(null=False)

    # 1 for player_1
    # 2 for player_2
    winner = models.PositiveSmallIntegerField(null=True)


    def save(self, *args, **kwargs):
        """this is used to deal with creation time"""

        if not self.id:
            self.creation_date = timezone.now()

        return super().save(*args, **kwargs)


    def make_move(self, row, col, val, main_player_in_game_id):
        """this method is called to update current position
        after a player write a value

        return True if other player position was updated

        in the code below , main player is the one who made
        the action , other is the other one"""

        if self.game_type == "GAME_TYPE_SOLO":
            multiplayer = False
        else:
            multiplayer = True

        if main_player_in_game_id == 1:
            other_player_in_game_id = 2

            main_player_origin_code  = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_PLAYER_1
            other_player_origin_code = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_PLAYER_2
        else:
            other_player_in_game_id = 1

            main_player_origin_code  = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_PLAYER_2
            other_player_origin_code = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_PLAYER_1


        with transaction.atomic():
            self.refresh_from_db()

            if main_player_in_game_id == 1:
                main_position_bytes = self.player_1_position
                if multiplayer:
                    other_position_bytes = self.player_2_position
            else:
                main_position_bytes = self.player_2_position
                if multiplayer:
                    other_position_bytes = self.player_1_position

            main_position = sudoku_utils.python_grids.bytes_to_current_position(
                                                    main_position_bytes)
            if multiplayer:
                other_position = sudoku_utils.python_grids.bytes_to_current_position(
                                                    other_position_bytes)

            # allow the change if value is not
            # known by main , or if he wrote it
            if( not (
                main_position.position.squares[row][col] == 0  or
                main_position.origins[row][col] == main_player_origin_code.value
                    )):
                raise Exception()

            # change the value in main
            if val == 0:
                main_position.origins[row][col] = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_START
            else:
                main_position.origins[row][col] = main_player_origin_code
            main_position.position.squares[row][col] = val

            if main_player_in_game_id == 1:
                self.player_1_position = bytes(main_position)
            else:
                self.player_2_position = bytes(main_position)


            # if move is correct , and its multiplayer
            # and other player does not yet have the value:
            #
            # change other position
            if( not (
                    multiplayer and

                    main_position.solution.squares[row][col] == 
                    main_position.position.squares[row][col]    and

                    other_position.position.squares[row][col] != 
                    other_position.solution.squares[row][col]
                    )):
                self.save()
                return False


            # update the other position
            other_position.origins[row][col] = main_player_origin_code
            other_position.position.squares[row][col] = val

            if other_player_in_game_id  == 1:
                self.player_1_position = bytes(other_position)
            else:
                self.player_2_position = bytes(other_position)

            self.save()
            return True


    def get_player_score(self, in_game_id):

        # count number of squares set but the player
        # in his position

        if in_game_id == 1:
            position_bytes = self.player_2_position
            origin_code = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_PLAYER_1
        else:
            position_bytes = self.player_2_position
            origin_code = sudoku_utils.python_grids.GRID_VALUE_ORIGIN_PLAYER_2

        position = sudoku_utils.python_grids.bytes_to_current_position(position_bytes)

        score = 0
        for row in range(9):
            for col in range(9):
                if( position.origins[row][col] == origin_code.value  and

                    position.position.squares[row][col] ==
                    position.solution.squares[row][col]
                        ):
                    score += 1
        return score
 

    def set_winner(self, winner = None):
        """called when the game is over to set the value of the winner.
        if winner is given , the scores are ignored
        """

        if self.game_type == "GAME_TYPE_SOLO":
            self.winner = 1

        if winner is None:
            score_1 = self.get_player_score(1)
            score_2 = self.get_player_score(2)

            if score_1 < score_2:
                self.winner = 2
            elif score_1 == score_2:
                pass
            else:
                self.winner = 1
        else:
            self.winner = winner

        if self.game_type == "GAME_TYPE_MULTIPLAYER":
            update_ratings(self.player_1, self.player_2, self.winner)


    def get_in_game_id(self, player):
        """given a user object , return its in game id"""

        if player == self.player_1:
            return 1
        if player == self.player_2:
            return 2
        return None


def update_ratings(player_1, player_2, winner):
    """updates the rating of the players given the outcome"""
    
    # the k factor
    k = 20

    # compute from the perspective of player_1
    # then do the reverse for player_2

    expected = 1 / (10**((player_2.userprofile.rating - player_1.userprofile.rating) / 400) + 1)

    if winner == 1:
        result = 1
    elif winner == 2:
        result = 0
    else:
        result = 0.5

    delta = k * ( result - expected )

    player_1.userprofile.rating = max(0, player_1.userprofile.rating + delta)
    player_2.userprofile.rating = max(0, player_2.userprofile.rating - delta)



#################################################

class UserProfile(models.Model): 
    user = models.OneToOneField(User, on_delete=models.CASCADE)

    # FIXME: use a trigger to update it
    #
    # this is used to make sure a player has at most
    # one game at a time
    current_game = models.ForeignKey(Game,
                            on_delete=models.SET_NULL,
                            default=None,
                            null=True,
                            related_name='+')

    rating = models.FloatField(default=1500, null=False)

# this class is used to store
# the users currently waiting
# for a multiplayer game
#
# rating is used to avoid looking 
# for it during list sorting
class MatchWaitingList(models.Model): 
    user = models.OneToOneField(User, on_delete=models.CASCADE)
    rating = models.FloatField(null=False)


    game_duration = models.IntegerField(null = False)
    game_level = models.IntegerField(null = False)

    def save(self, *args, **kwargs):
        """this is used to cache the value of the rating"""

        if not self.id:
            self.rating = self.user.userprofile.rating

        return super().save(*args, **kwargs)




#################################################
################################################# SIGNALS
#################################################
# FIXME: move signal handling to sudoku/signals.py
#        and change the app config to include it


from django.db.models.signals import post_save

@receiver(post_save, sender=User)
def create_UserProfile(sender, instance, created, **kwargs):
    if created:
        UserProfile.objects.create(user=instance)
