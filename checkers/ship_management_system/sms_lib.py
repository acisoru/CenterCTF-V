import requests
import json
import base64
import binascii
from checklib import *

PORT = 8080


def _encode(s: str) -> str:
    if not isinstance(s, str):
        s = str(s)
    return base64.b64encode(s.encode('utf-8')).decode('utf-8')

def _decode(encoded_str: str | None) -> str:
    if encoded_str is None:
        return ""
    try:
        missing_padding = len(encoded_str) % 4
        if missing_padding:
            encoded_str += '=' * (4 - missing_padding)
        decoded_bytes = base64.b64decode(encoded_str)
        return decoded_bytes.decode('utf-8')
    except (binascii.Error, UnicodeDecodeError, TypeError) as e:
        return ""

class CheckMachine:
    @property
    def url(self):
        host_url = f'http://{self.c.host}:{self.port}'
        return host_url

    def __init__(self, checker: BaseChecker):
        self.port = PORT
        self.c = checker


    def register(self, session: requests.Session, firstname: str, secondname: str, thirdname: str, password: str):
        data = {
            'firstname': _encode(firstname),
            'secondname': _encode(secondname),
            'thirdname': _encode(thirdname),
            'password': _encode(password)
        }
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/user/register",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def login(self, session: requests.Session, firstname: str, secondname: str, thirdname: str, password: str):
        data = {
            'firstname': _encode(firstname),
            'secondname': _encode(secondname),
            'thirdname': _encode(thirdname),
            'password': _encode(password)
        }
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/user/login",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def get_all_fleets(self, session: requests.Session):
        ans = session.get(f"{self.url}/user/get_all_fleets")
        return ans

    def get_command_for_fleet(self, session: requests.Session, fleet_id: int):
        params = {'fleet_id': fleet_id}
        ans = session.get(f"{self.url}/user/get_command_for_fleet", params=params)
        return ans

    def create_request(self, session: requests.Session, accepter_id: int, role_to_request: str):
        data = {
            'accepterId': accepter_id,
            'role': _encode(role_to_request)
        }
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/user/create_request",
            data=json.dumps(data),
            headers=headers
        )
        return ans


    def admiral_create_fleet(self, session: requests.Session, country: str, description: str):
        data = {
            'country': _encode(country),
            'description': _encode(description),
        }
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/create_fleet",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_get_all_captains_requests(self, session: requests.Session):
        ans = session.get(f"{self.url}/admiral/get_all_captains_requests")
        return ans

    def admiral_accept_captain_request(self, session: requests.Session, captain_id: int):
        data = {'captain_id': captain_id}
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/accept_captain_request",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_decline_captain_request(self, session: requests.Session, captain_id: int):
        data = {'captain_id': captain_id}
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/decline_captain_request",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_create_ship(self, session: requests.Session, ship_name: str):
        data = {'ship_name': _encode(ship_name)}
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/create_ship",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_remove_ship(self, session: requests.Session, ship_id: int):
        data = {'ship_id': ship_id}
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/remove_ship",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_get_all_ships(self, session: requests.Session):
        ans = session.get(f"{self.url}/admiral/get_all_ships")
        return ans

    def admiral_get_all_captains(self, session: requests.Session):
        ans = session.get(f"{self.url}/admiral/get_all_captains")
        return ans

    def admiral_assign_captain_to_ship(self, session: requests.Session, captain_id: int, ship_id: int):
        data = {
            'captain_id': captain_id,
            'ship_id': ship_id
        }
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/assign_captain_to_ship",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_remove_captain_from_ship(self, session: requests.Session, captain_id: int):
        data = {'captain_id': captain_id}
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/remove_captain_from_ship",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_assign_mission_to_ship(self, session: requests.Session, ship_id: int, description: str):
        data = {
            'ship_id': ship_id,
            'description': _encode(description)
        }
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/assign_mission_to_ship",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_remove_mission_to_ship(self, session: requests.Session, mission_id: int):
        data = {'mission_id': mission_id}
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/remove_mission_to_ship",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_get_all_missions(self, session: requests.Session):
        ans = session.get(f"{self.url}/admiral/get_all_missions")
        return ans

    def admiral_change_captain_description(self, session: requests.Session, captain_id: int, new_description: str):
        data = {
            'captain_id': captain_id,
            'new_description': _encode(new_description)
        }
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/admiral/change_captain_description",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def admiral_get_my_fleet_info(self, session: requests.Session):
        ans = session.get(f"{self.url}/admiral/my_fleet")
        return ans


    def captain_get_all_sailors_requests(self, session: requests.Session):
        ans = session.get(f"{self.url}/captain/get_all_sailors_requests")
        return ans

    def captain_accept_sailor_request(self, session: requests.Session, sailor_id: int):
        data = {'sailor_id': sailor_id}
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/captain/accept_sailor_request",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def captain_decline_sailor_request(self, session: requests.Session, sailor_id: int):
        data = {'sailor_id': sailor_id}
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/captain/decline_sailor_request",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def captain_check_mission(self, session: requests.Session):
        ans = session.get(f"{self.url}/captain/check_mission")
        return ans

    def captain_mark_mission_as_complete(self, session: requests.Session):
        ans = session.get(f"{self.url}/captain/mark_mission_as_complete")
        return ans

    def captain_change_sailor_description(self, session: requests.Session, sailor_id: int, new_description: str):
        data = {
            'sailor_id': sailor_id,
            'new_description': _encode(new_description)
        }
        headers = {'Content-Type': 'application/json'}
        ans = session.post(
            f"{self.url}/captain/change_sailor_description",
            data=json.dumps(data),
            headers=headers
        )
        return ans

    def captain_get_sailors_for_ship(self, session: requests.Session):
        ans = session.get(f"{self.url}/captain/get_sailors_for_ship")
        return ans

    def captain_get_my_ship_info(self, session: requests.Session):
        ans = session.get(f"{self.url}/captain/my_ship")
        return ans