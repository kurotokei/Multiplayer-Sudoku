from django.urls import path

from . import views

urlpatterns = [
    path("", views.home, name="home"),

    path('login/', views.user_login, name='login'),
    path('signup/', views.user_signup, name='signup'),
    path('logout/', views.user_logout, name='logout'),

    path("game/<int:game_id>", views.game, name="game"),

    path("api/create_game/", views.api__create_game),
    path("api/notify_player_is_ready/", views.api__notify_player_is_ready),
    path("api/end_game/", views.api__end_game),
    path("api/cancel_match_search/", views.api__cancel_match_search),
]

