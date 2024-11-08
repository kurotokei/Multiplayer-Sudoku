from django.shortcuts import render, redirect
from django.contrib.auth import authenticate, login, logout 
from django.contrib.auth.decorators import login_required
from django.http import HttpResponse, JsonResponse
from .forms import SignupForm, LoginForm

import datetime
from django.utils import timezone

import json

import sudoku.models
import sudoku_utils.python_grids

from django.core.cache import cache

from channels.layers import get_channel_layer
from asgiref.sync import async_to_sync


def home(request):

    if not request.user.is_authenticated:
        return render(request, 'sudoku/home.html', dict())

    current_game_id = getattr(request.user.userprofile.current_game, 'id', None)

    waiting_for_match = hasattr(request.user, 'matchwaitinglist') 

    params = {
                "current_game_id": json.dumps(current_game_id),
                "waiting_for_match": json.dumps(waiting_for_match)
            }

    return render(request, 'sudoku/home.html', params)


# signup page
def user_signup(request):
    if request.method == 'POST':
        form = SignupForm(request.POST)
        if form.is_valid():
            form.save()

            username = form.cleaned_data['username']
            password = form.cleaned_data['password1']
            user = authenticate(request, username=username, password=password)
            if user:
                login(request, user)
                return redirect('home')

            # this is never reached unless there is a bug

    else:
        form = SignupForm()
    return render(request, 'sudoku/signup.html', {'form': form})

# login page
def user_login(request):
    if request.method == 'POST':
        form = LoginForm(request.POST)
        if form.is_valid():
            username = form.cleaned_data['username']
            password = form.cleaned_data['password']
            user = authenticate(request, username=username, password=password)
            if user:
                login(request, user)
                return redirect('home')
    else:
        form = LoginForm()
    return render(request, 'sudoku/login.html', {'form': form})

# logout page
def user_logout(request):
    logout(request)
    return redirect('home')

#################################################
from django.db import IntegrityError, transaction

def get_game_params(game, user):
    """NO CHECK IS MADE BY THIS FUNCTION !
    a helper function to get the params to render
    the game template page
    """

    if game.player_1 == user:
        in_game_id = 1
    else:
        in_game_id = 2


    player_1_str = json.dumps({
                            "id": game.player_1.id,
                            "name": game.player_1.username,
                            "rating": game.player_1.userprofile.rating
                            })

    if game.player_2 == None:
        player_2_str = json.dumps(None)
    else:
        player_2_str = json.dumps({
                            "id": game.player_2.id,
                            "name": game.player_2.username,
                            "rating": game.player_2.userprofile.rating
                            })


    creation_date_str = game.creation_date.isoformat()

    if game.start_date == None:
        start_date_str = None
    else:
        start_date_str  = game.start_date.isoformat()

    if game.end_date == None:
        end_date_str = None
    else:
        end_date_str = game.end_date.isoformat()



    params = {
                "in_game_id": in_game_id,
                "game": json.dumps({
                            "id": game.id,

                            "level": game.game_level,
                            "type": game.game_type,
                            "duration": game.game_duration,

                            "creation_date": creation_date_str,
                            "start_date": start_date_str,
                            "end_date": end_date_str,

                            "winner": game.winner
                        }),

                "player_1": player_1_str,
                "player_2": player_2_str
            }

    return params



@login_required
def game(request, game_id):
    """return the page of the game if the user is allowed
    """

    game_objs = sudoku.models.Game.objects.filter(id=game_id)
    if len(game_objs) == 0:
        return HttpResponse(status=404)

    game_obj = game_objs[0] 

    # potentioally end the game
    helper__end_game(game_obj, only_on_timeout = True)


    if game_obj.game_type == "GAME_TYPE_SOLO":
        # if the right user render the page

        if game_obj.player_1_id != request.user.id:
            return HttpResponse(status=403)

        params = get_game_params(game_obj, request.user)
        return render(request, "sudoku/game.html", params)

    elif game_obj.game_type == "GAME_TYPE_WITH_FRIEND":

        # if first or second player just render the page
        if( request.user == game_obj.player_1 or
            request.user == game_obj.player_2
                ):
            params = get_game_params(game_obj, request.user)
            return render(request, "sudoku/game.html", params)

        # if second player not known
        # set it to current user
        with transaction.atomic():
            game_obj.refresh_from_db()

            if game_obj.player_2 == None:

                # first check that user is not already playing a game
                # or waiting for multiplayer match
                if( request.user.userprofile.current_game != None  or
                    hasattr(request.user, 'matchwaitinglist')
                        ):
                    return redirect('home')

                game_obj.player_2 = request.user
                request.user.userprofile.current_game = game_obj

                game_obj.save()
                request.user.userprofile.save()

                params = get_game_params(game_obj, request.user)

                async_to_sync(get_channel_layer().group_send)(
                    f"game_group_{game_obj.id}",
                    {
                        "type": "GameConsumer.send.object",
                        "object": {
                                    "type": "player_2",
                                    "player_2": params["player_2"]
                        }
                    }
                )

                return render(request, "sudoku/game.html", params)

            else:
                return HttpResponse(status=400)



    elif game_obj.game_type == "GAME_TYPE_MULTIPLAYER":
        # if first or second player just render the page
        if( request.user == game_obj.player_1 or
            request.user == game_obj.player_2
                ):
            params = get_game_params(game_obj, request.user)
            return render(request, "sudoku/game.html", params)

        else:
            return HttpResponse(status=400)
 
    else:
        # invalid game type
        return HttpResponse(status=400)


#################################################
################################################# API
#################################################


@login_required
def api__create_game(request):

    if request.method != "POST":
        return HttpResponse(status=400)

    try:
        req_data = json.loads(request.body)

        game_type = req_data["game_type"]
        game_duration = req_data["game_duration"]
        game_level = req_data["game_level"]

        if type(game_type) != str  or  game_type not in {"GAME_TYPE_SOLO",
                                                         "GAME_TYPE_WITH_FRIEND",
                                                         "GAME_TYPE_MULTIPLAYER" }:
            return HttpResponse(status=400)


        if type(game_duration) != int  or  game_duration not in {3, 5, 10, 15, 30}:
            return HttpResponse(status=400)

        NB_LEVELS = sudoku_utils.python_grids.NB_LEVELS
        if type(game_level) != int  or not  0 <= game_level < NB_LEVELS:
            return HttpResponse(status=400)

        if game_type == "GAME_TYPE_MULTIPLAYER":
            try:
                sudoku.models.MatchWaitingList.objects.create(
                                                    user = request.user,
                                                    game_duration = game_duration,
                                                    game_level = game_level)
            # user already in waiting list
            except IntegrityError:
                return HttpResponse(status=400)

            return JsonResponse({'game_id':None})

        # create the game and redirect
        new_position = sudoku_utils.python_grids.get_new_game_position(game_level)

        try:
            with transaction.atomic():
                new_game = sudoku.models.Game.objects.create(
                                            game_type = game_type,
                                            game_duration = game_duration,
                                            game_level = game_level,
                                            player_1 = request.user,
                                            player_1_position = bytes(new_position),
                                            player_2_position = bytes(new_position) )

                if( request.user.userprofile.current_game != None or
                    hasattr(request.user, 'matchwaitinglist')
                        ):
                    raise sudoku.models.SudokuError()

                request.user.userprofile.current_game = new_game
                request.user.userprofile.save()

        except sudoku.models.SudokuError:
            return HttpResponse(status=400)


        return JsonResponse({'game_id':new_game.id})

    except BaseException as err:
        return HttpResponse(status=400)

#################################################

@login_required
def api__notify_player_is_ready(request):

    # if current player is not playing
    # a game , it has nothing to notify
    if request.user.userprofile.current_game == None:
        return HttpResponse(status=400)

    game_obj = request.user.userprofile.current_game

    # if the game already began or is over
    # the client made a mistake
    if game_obj.start_date is not None  or  game_obj.end_date is not None:
        return HttpResponse(status=400)


    if request.user == game_obj.player_1:
       game_obj.player_1_ready = True
    elif request.user == game_obj.player_2:
        game_obj.player_2_ready = True
    else:
        # the data base is in an incorrect state
        return HttpResponse(status=500)

    game_obj.save()


    #  if all players are ready
    # send the game starting position
    # to every body
    start_game = False
    with transaction.atomic():
        game_obj.refresh_from_db()

        # FIXME: remove the solo case from the with ...
        if game_obj.game_type == "GAME_TYPE_SOLO":
            start_game = True
        else:
            if game_obj.player_1_ready  and  game_obj.player_2_ready:
                start_game = True

        if start_game:
            game_obj.start_date = timezone.now()
            game_obj.save()


    if start_game:
        async_to_sync(get_channel_layer().group_send)(
                f"game_group_{game_obj.id}",
                {
                    "type": "GameConsumer.send.position",
                    "include_solution": False
                }
            )


    return JsonResponse({})

#################################################


def helper__end_game(game, aborting_player = None, only_on_timeout = False):
    """this function is seperated from the api__end_game to avoid code duplication.
    no check is made here.
    if only_on_timeout then end the game only if there is a timeout.
    if aborting_player is given , it is set as the loser of the game.

    only_on_timeout and aborting_player should not be given at the same time"""

    with transaction.atomic():
        game.refresh_from_db()

        if only_on_timeout:

            if( game.start_date == None  or
                game.end_date != None
                    ):
                return None

            expected_end = game.start_date + datetime.timedelta(minutes = game.game_duration)

            if timezone.now() < expected_end:
                return None

        game.end_date = timezone.now()

        if aborting_player is None:
            game.set_winner()
        elif aborting_player == 1:
            game.set_winner(winner = 2)
        else:
            game.set_winner(winner = 1)


        game.save()

        game.player_1.userprofile.current_game = None
        game.player_1.userprofile.save()

        if game.player_2 != None:
            game.player_2.userprofile.current_game = None
            game.player_2.userprofile.save()

    # notify users that game is over
    async_to_sync(get_channel_layer().group_send)(
                f"game_group_{game.id}",
                {
                    "type": "GameConsumer.end.game",
                }
            )

    return None


@login_required
def api__end_game(request):
    """this is a very basic api to end a game"""

    # if current player is not playing
    # a game , it has nothing to end
    if request.user.userprofile.current_game == None:
        return HttpResponse(status=400)

    game_obj = request.user.userprofile.current_game

    helper__end_game(game_obj, aborting_player = game_obj.get_in_game_id(request.user))


    return JsonResponse({})


@login_required
def api__cancel_match_search(request):

    try:
        request.user.matchwaitinglist.delete()
    except:
        pass

    return JsonResponse({})
