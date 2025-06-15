import requests
import json

PORT = 15401

class BattleshipLib:
    def __init__(self, url: str):
        self.url = url

    def register(self, session: requests.Session, username: str, password: str, flag: str):
        data = {
            'username': username,
            'password': password,
            'treasure': flag
        }
        
        headers = {
            'Content-Type': 'application/json'
        }
        
        ans = session.post(
            f"{self.url}/api/auth/register", 
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def login(self, session: requests.Session, username: str, password: str):
        data = {
            'username': username,
            'password': password
        }
        
        headers = {
            'Content-Type': 'application/json'
        }
        
        ans = session.post(
            f"{self.url}/api/auth/login", 
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def logout(self, session: requests.Session):
        ans = session.post(f"{self.url}/api/auth/logout")
        return ans

    def get_user_info(self, session: requests.Session):
        ans = session.get(f"{self.url}/api/profile/get-user-info")
        return ans

    def update_avatar(self, session: requests.Session, avatar: str):
        data = {
            'avatar': avatar
        }
        
        headers = {
            'Content-Type': 'application/json'
        }
        
        ans = session.post(
            f"{self.url}/api/profile/update-profile", 
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def create_game(self, session: requests.Session, opponent_username: str):
        data = {
            'opponent_username': opponent_username
        }
        
        headers = {
            'Content-Type': 'application/json'
        }
        
        ans = session.post(
            f"{self.url}/api/game/create-game", 
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def make_shot(self, session: requests.Session, x: int, y: int):
        data = {
            'position': {
                'x': x,
                'y': y
            }
        }
        
        headers = {
            'Content-Type': 'application/json'
        }
        
        ans = session.post(
            f"{self.url}/api/game/make-shot", 
            data=json.dumps(data),
            headers=headers
        )
        return ans