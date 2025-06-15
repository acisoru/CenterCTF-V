#!/usr/bin/env python3

import sys
import requests
import re
import time
import json

from checklib import *
from battleship_lib import *


class Checker(BaseChecker):
    vulns: int = 1
    timeout: int = 15
    uses_attack_data: bool = True

    def __init__(self, *args, **kwargs):
        super(Checker, self).__init__(*args, **kwargs)
        self.mch = CheckMachine(self)

    def action(self, action, *args, **kwargs):
        try:
            super(Checker, self).action(action, *args, **kwargs)
        except requests.exceptions.ConnectionError:
            self.cquit(Status.DOWN, 'Connection error', 'Got requests connection error')
    
    def __check_register(self):
        session = self.get_initialized_session()
        username, password = rnd_username(), rnd_password()

        flag = rnd_string(10)

        register_ans = self.mch.register(session, username, password, flag)
        register_status = int(register_ans.status_code)
        self.assert_eq(200, register_status, "Cannot register")
        
        return username, password, flag
    
    def __check_login(self, username, password):
        session = self.get_initialized_session()
        login_ans = self.mch.login(session, username, password)
        login_status = int(login_ans.status_code)
        self.assert_eq(200, login_status, "Cannot login")
        
        return session
    
    def __check_user_info(self, session):
        user_info_ans = self.mch.get_user_info(session)
        user_info_status = int(user_info_ans.status_code)
        self.assert_eq(200, user_info_status, "Cannot get user info")
        
        user_info = user_info_ans.json()
        self.assert_in("username", user_info, "User info does not contain username")
        self.assert_in("wins", user_info, "User info does not contain wins")
        self.assert_in("losses", user_info, "User info does not contain losses")
        
        return user_info
    
    def __check_user_treasure(self, session, flag):
        treasure_ans = self.mch.get_user_treasure(session)
        treasure_status = int(treasure_ans.status_code)
        self.assert_eq(200, treasure_status, "Cannot get user treasure")
        
        treasure = treasure_ans.json()
        self.assert_in("treasure", treasure, "Treasure response does not contain treasure")
        self.assert_eq(flag, treasure["treasure"], "Treasure does not match flag")
        
        return treasure
    
    def __check_create_game(self, session, opponent_username):
        create_game_ans = self.mch.create_game(session, opponent_username)
        create_game_status = int(create_game_ans.status_code)
        self.assert_eq(200, create_game_status, "Cannot create game")
        
        create_game = create_game_ans.json()
        self.assert_in("game_id", create_game, "Create game response does not contain game_id")
        
        return create_game["game_id"]
    
    def __check_get_game_info(self, session):
        game_info_ans = self.mch.get_game_info(session)
        game_info_status = int(game_info_ans.status_code)
        self.assert_eq(200, game_info_status, "Cannot get game info")
        
        game_info = game_info_ans.json()
        self.assert_in("id", game_info, "Game info does not contain id")
        self.assert_in("status", game_info, "Game info does not contain status")
        
        return game_info
    
    def __check_make_shot(self, session, x, y):
        make_shot_ans = self.mch.make_shot(session, x, y)
        make_shot_status = int(make_shot_ans.status_code)
        self.assert_eq(200, make_shot_status, "Cannot make shot")
        
        make_shot = make_shot_ans.json()
        self.assert_in("hit", make_shot, "Make shot response does not contain hit")
        self.assert_in("message", make_shot, "Make shot response does not contain message")
        
        return make_shot
    
    def __check_win(self, session, opponent_session, opponent_username):
        game_id = self.__check_create_game(session, opponent_username)
        
        active_games_ans = self.mch.get_active_games(opponent_session)
        active_games_status = int(active_games_ans.status_code)
        self.assert_eq(200, active_games_status, "Cannot get active games")
        
        active_games = active_games_ans.json()
        self.assert_in("games", active_games, "Active games response does not contain games")
        
        ship_coords = []
        for game in active_games["games"]:
            if game["game_id"] == game_id:
                for ship in game["ships"]:
                    start_x = ship["Start"]["x"]
                    start_y = ship["Start"]["y"]
                    end_x = ship["End"]["x"]
                    end_y = ship["End"]["y"]
                    
                    if start_x == end_x:  
                        for y in range(start_y, end_y + 1):
                            ship_coords.append((start_x, y))
                    else:  
                        for x in range(start_x, end_x + 1):
                            ship_coords.append((x, start_y))
        
        win_message = None
        treasure = None
        
        for x, y in ship_coords:
            try:
                result = self.__check_make_shot(session, x, y)
                
                if "message" in result and isinstance(result["message"], str) and result["message"].startswith("win: "):
                    win_message = result["message"]
                    treasure = result["message"][5:]
                    break
            except Exception as e:
                continue
        
        self.assert_neq(None, win_message, "Did not get a win message after shooting all ships")
        self.assert_neq(None, treasure, "Did not get a treasure after winning")
        self.assert_neq("", treasure, "Got empty treasure after winning")

        return treasure
    
    def __check_get_active_games(self, session):
        active_games_ans = self.mch.get_active_games(session)
        active_games_status = int(active_games_ans.status_code)
        self.assert_eq(200, active_games_status, "Cannot get active games")
        
        active_games = active_games_ans.json()
        self.assert_in("games", active_games, "Active games response does not contain games")
        
        return active_games
    
    def __check_leaderboard(self, session):
        leaderboard_ans = self.mch.get_leaderboard(session)
        leaderboard_status = int(leaderboard_ans.status_code)
        self.assert_eq(200, leaderboard_status, "Cannot get leaderboard")
        
        leaderboard = leaderboard_ans.json()
        self.assert_in("entries", leaderboard, "Leaderboard response does not contain users")
        
        return leaderboard
    
    def __check_update_profile(self, session, username):
        rand_seed = rnd_string(10, "0123456789")
        
        avatar_url = f"https://api.dicebear.com/9.x/croodles/svg?seed={rand_seed}&size=64&backgroundColor=b6e3f4,c0aede,d1d4f9,ffd5dc,ffdfbf&beardProbability=50&mustacheProbability=50&top=variant23,variant24,variant11,variant18,variant09"
        
        update_ans = self.mch.update_profile(session, username, avatar_url)
        update_status = int(update_ans.status_code)
        self.assert_eq(200, update_status, "Cannot update profile")
        
        user_info_ans = self.mch.get_user_info(session)
        user_info_status = int(user_info_ans.status_code)
        self.assert_eq(200, user_info_status, "Cannot get user info after profile update")
        
        user_info = user_info_ans.json()
        self.assert_in("avatar", user_info, "User info does not contain avatar after update")
        
        return user_info["avatar"]
    
    def __check_logout(self, session):
        logout_ans = self.mch.logout(session)
        logout_status = int(logout_ans.status_code)
        self.assert_eq(200, logout_status, "Cannot logout")
        
        return logout_ans

    def check(self):
        username, password, flag = self.__check_register()
        
        session = self.__check_login(username, password)
        
        user_info = self.__check_user_info(session)
        
        treasure = self.__check_user_treasure(session, flag)
        
        avatar_url = self.__check_update_profile(session, username)
        self.assert_neq(avatar_url, user_info["avatar"], "Avatar was not updated correctly")
        
        opponent_username, opponent_password, opponent_flag = self.__check_register()
        opponent_session = self.__check_login(opponent_username, opponent_password)
        opponent_info = self.__check_user_info(opponent_session)
        
        captured_flag = self.__check_win(session, opponent_session, opponent_info["username"])
        self.assert_eq(opponent_flag, captured_flag, "Captured flag doesn't match opponent's flag")
        
        leaderboard = self.__check_leaderboard(session)
        
        self.__check_logout(session)
        
        self.cquit(Status.OK)

    def put(self, flag_id: str, flag: str, vuln:str):
        session = self.get_initialized_session()
        username, password = rnd_username(), rnd_password()

        register_ans = self.mch.register(session, username, password, flag)
        register_status = int(register_ans.status_code)
        self.assert_eq(200, register_status, "Cannot register")

        self.cquit(Status.OK, username, f'{username}:{password}')

    def get(self, flag_id: str, flag: str, vuln:str):
        session = self.get_initialized_session()
        username, password = flag_id.split(':')
        
        login_ans = self.mch.login(session, username, password)
        login_status = int(login_ans.status_code)
        self.assert_eq(200, login_status, "Cannot login")
        
        treasure_ans = self.mch.get_user_treasure(session)
        treasure_status = int(treasure_ans.status_code)
        self.assert_eq(200, treasure_status, "Cannot get user treasure")
        
        treasure = treasure_ans.json()
        self.assert_in("treasure", treasure, "Treasure response does not contain treasure")
        self.assert_eq(flag, treasure["treasure"], "Treasure does not match flag")
        
        self.cquit(Status.OK)


if __name__ == '__main__':
    c = Checker(sys.argv[2])

    try:
        c.action(sys.argv[1], *sys.argv[3:])
    except c.get_check_finished_exception():
        cquit(Status(c.status), c.public, c.private)

