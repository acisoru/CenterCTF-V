#!/usr/bin/env python3

import sys
import requests
import json
import time
import binascii
import base64

from checklib import *
try:
    from sms_lib import CheckMachine, _decode
except ImportError:
    print("ERROR: Failed to import CheckMachine or _decode from sms_lib.py.", file=sys.stderr)
    print("Ensure sms_lib.py is in the same directory or Python path.", file=sys.stderr)
    sys.exit(1)


class Checker(BaseChecker):
    vulns: int = 1
    timeout: int = 30
    uses_attack_data: bool = False

    def __init__(self, *args, **kwargs):
        super(Checker, self).__init__(*args, **kwargs)
        self.mch = CheckMachine(self)
        self.decode = _decode

    def action(self, action, *args, **kwargs):
        try:
            super(Checker, self).action(action, *args, **kwargs)
        except requests.exceptions.ConnectionError:
            self.cquit(Status.DOWN, 'Connection error', 'Got requests connection error')

    def generate_user(self):
        firstname = rnd_string(10)
        secondname = rnd_string(12)
        thirdname = rnd_string(11)
        password = rnd_password()
        return firstname, secondname, thirdname, password

    def _assert_status_and_decode(self, ans, expected_status, context=""):
        prefix = f"[{context}] " if context else ""
        self.assert_eq(expected_status, ans.status_code, f"{prefix}Unexpected status code. Response: {ans.text[:500]}")
        try:
            if ans.content:
                 try:
                     return json.loads(ans.content.decode(ans.encoding or 'utf-8'), **{})
                 except ImportError:
                     import complexjson
                     return complexjson.loads(ans.content.decode(ans.encoding or 'utf-8'), **{})
            else:
                 return None
        except json.JSONDecodeError:
            if expected_status < 400:
                self.cquit(Status.MUMBLE, f"{prefix}Invalid JSON response", f"Could not decode JSON: {ans.text[:500]}")
            else:
                return None
        except Exception as e:
             self.cquit(Status.MUMBLE, f"{prefix}Error processing response", f"Exception during JSON decode or processing: {e}")


    def _get_message(self, decoded_json, context=""):
        prefix = f"[{context}] " if context else ""
        if decoded_json is None:
             self.cquit(Status.MUMBLE, f"{prefix}Expected JSON response but got None", "Cannot get message from None")
        self.assert_in("message", decoded_json, f"{prefix}Response missing 'message' field")
        msg_encoded = decoded_json.get("message")
        msg = self.decode(msg_encoded)
        return msg

    def __check_register(self, session, firstname, secondname, thirdname, password):
        ans = self.mch.register(session, firstname, secondname, thirdname, password)
        resp_json = self._assert_status_and_decode(ans, 200, "Register")
        message = self._get_message(resp_json, "Register")
        self.assert_eq("User registered successfully", message, "[Register] Unexpected registration message")
        ans_dup = self.mch.register(session, firstname, secondname, thirdname, password)
        resp_json_dup = self._assert_status_and_decode(ans_dup, 400, "Register Duplicate")
        message_dup = self._get_message(resp_json_dup, "Register Duplicate")
        self.assert_eq("User already exist", message_dup, "[Register Duplicate] Unexpected duplicate registration message")

    def __check_login(self, session, firstname, secondname, thirdname, password):
        ans = self.mch.login(session, firstname, secondname, thirdname, password)
        resp_json = self._assert_status_and_decode(ans, 200, "Login")
        message = self._get_message(resp_json, "Login")
        self.assert_eq("Login successful", message, "[Login] Unexpected login message")
        self.assert_in("token", resp_json, "[Login] Response missing 'token'")
        token = self.decode(resp_json.get("token"))
        self.assert_neq(token, "", "[Login] Received empty token")

        auth_session = self.get_initialized_session()
        auth_session.headers.update({'Authorization': f'Bearer {token}'})

        ans_fail = self.mch.login(session, firstname, secondname, thirdname, "wrongpassword")
        self.assert_eq(401, ans_fail.status_code, "[Login Fail] Unexpected status code for failed login")
        return auth_session

    def __check_get_all_fleets(self, session):
        ans = self.mch.get_all_fleets(session)
        resp_json = self._assert_status_and_decode(ans, 200, "Get All Fleets")
        self.assert_in("fleets", resp_json, "[Get All Fleets] Response missing 'fleets'")
        fleets = resp_json["fleets"]
        self.assert_(isinstance(fleets, list), "[Get All Fleets] Response 'fleets' key is not a list")
        return fleets

    def __check_get_command_for_fleet(self, session, fleet_id):
        ans = self.mch.get_command_for_fleet(session, fleet_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Get Command (Fleet {fleet_id})")
        self.assert_in("command", resp_json, "[Get Command] Response missing 'command'")
        self.assert_in("fleet_id", resp_json, "[Get Command] Response missing 'fleet_id'")
        self.assert_eq(fleet_id, resp_json["fleet_id"], "[Get Command] Fleet ID mismatch in response")
        command = resp_json["command"]
        self.assert_(isinstance(command, list), "[Get Command] Response 'command' key is not a list")
        return command

    def __check_create_request(self, requester_session, accepter_id, role_to_request):
        ans = self.mch.create_request(requester_session, accepter_id, role_to_request)
        resp_json = self._assert_status_and_decode(ans, 200, f"Create Request ({role_to_request})")
        message = self._get_message(resp_json, f"Create Request ({role_to_request})")
        self.assert_eq("Request created successfully", message, f"[Create Request ({role_to_request})] Unexpected success message")

    def __check_admiral_create_fleet(self, s_unauth, admiral_session, country, description, adm_fn, adm_sn, adm_tn, adm_pw):
        """Creates fleet, verifies, re-logs in admiral, returns fleet_id and new session."""
        ans = self.mch.admiral_create_fleet(admiral_session, country, description)
        resp_json = self._assert_status_and_decode(ans, 200, "Admiral Create Fleet")
        message = self._get_message(resp_json, "Admiral Create Fleet")
        self.assert_(message.startswith("Fleet created successfully"),
                     "[Admiral Create Fleet] Unexpected success message format",
                     f"Expected message to start with 'Fleet created successfully', got: '{message}'")

        time.sleep(0.5)

        new_admiral_session = self.__check_login(s_unauth, adm_fn, adm_sn, adm_tn, adm_pw)

        fleet_id, r_country, r_desc = self.__check_admiral_get_my_fleet_info(new_admiral_session)
        self.assert_eq(country, r_country, "[Admiral Create Fleet] Created fleet country mismatch after retrieval")
        self.assert_eq(description, r_desc, "[Admiral Create Fleet] Created fleet description mismatch after retrieval")

        return fleet_id, new_admiral_session

    def __check_admiral_get_my_fleet_info(self, admiral_session):
        ans = self.mch.admiral_get_my_fleet_info(admiral_session)
        resp_json = self._assert_status_and_decode(ans, 200, "Admiral Get My Fleet")
        self.assert_in("id", resp_json, "[Admiral Get My Fleet] Response missing 'id'")
        self.assert_in("country", resp_json, "[Admiral Get My Fleet] Response missing 'country'")
        self.assert_in("description", resp_json, "[Admiral Get My Fleet] Response missing 'description'")
        return resp_json['id'], self.decode(resp_json['country']), self.decode(resp_json['description'])

    def _get_requests(self, session, request_type="captain"):
        if request_type == "captain":
            ans = self.mch.admiral_get_all_captains_requests(session)
        elif request_type == "sailor":
            ans = self.mch.captain_get_all_sailors_requests(session)
        else:
            raise ValueError(f"Invalid request type '{request_type}' for _get_requests")

        resp_json = self._assert_status_and_decode(ans, 200, f"Get {request_type} Requests")

        if "requests" not in resp_json:
             message = self._get_message(resp_json, f"Get {request_type} Requests (Empty?)")
             if "No requests found" not in message and "Success" not in message:
                 self.cquit(Status.MUMBLE, f"Get {request_type} Requests: Unexpected empty response format", f"Message: {message}")
             return []

        requests_list = resp_json.get("requests", [])
        self.assert_(isinstance(requests_list, list), f"[Get {request_type} Requests] Response 'requests' key is not a list")
        return requests_list

    def _find_user_id_from_requests(self, session, request_type="captain"):
        requests_list = self._get_requests(session, request_type)
        if not requests_list:
             self.cquit(Status.MUMBLE, f"Finding {request_type} ID failed", f"No {request_type} requests found when expecting one.")
        requester_id = requests_list[0].get("asker_id")
        self.assert_(requester_id is not None, f"[Find User ID] Could not extract asker_id from {request_type} request")
        return requester_id

    def __check_admiral_accept_captain_request(self, admiral_session, captain_id):
        ans = self.mch.admiral_accept_captain_request(admiral_session, captain_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Accept Captain {captain_id}")
        message = self._get_message(resp_json, f"Accept Captain {captain_id}")
        self.assert_eq("Successfuly approve the request.", message, "[Accept Captain] Unexpected success message")

    def __check_admiral_decline_captain_request(self, admiral_session, captain_id):
        ans = self.mch.admiral_decline_captain_request(admiral_session, captain_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Decline Captain {captain_id}")
        message = self._get_message(resp_json, f"Decline Captain {captain_id}")
        self.assert_eq("Successfuly decline the request.", message, "[Decline Captain] Unexpected success message")
        ans_fail = self.mch.admiral_decline_captain_request(admiral_session, 99999)
        resp_fail = self._assert_status_and_decode(ans_fail, 404, "Decline Non-existent Captain")
        self._get_message(resp_fail, "Decline Non-existent Captain")

    def __check_admiral_create_ship(self, admiral_session, ship_name):
        ans = self.mch.admiral_create_ship(admiral_session, ship_name)
        resp_json = self._assert_status_and_decode(ans, 201, f"Create Ship {ship_name}")
        message = self._get_message(resp_json, f"Create Ship {ship_name}")
        self.assert_(message.startswith("Ship created and assigned"),
                     f"[Create Ship {ship_name}] Unexpected success message format",
                     f"Expected message to start with 'Ship created and assigned', got: '{message}'")
        self.assert_in("ship_id", resp_json, f"[Create Ship {ship_name}] Response missing 'ship_id'")
        ship_id = resp_json['ship_id']
        self.assert_(isinstance(ship_id, int), f"[Create Ship {ship_name}] Ship ID is not an integer")
        return ship_id

    def __check_admiral_remove_ship(self, admiral_session, ship_id):
        ans = self.mch.admiral_remove_ship(admiral_session, ship_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Remove Ship {ship_id}")
        message = self._get_message(resp_json, f"Remove Ship {ship_id}")
        self.assert_eq("Successfully deleted", message, "[Remove Ship] Unexpected success message")
        ans_fail = self.mch.admiral_remove_ship(admiral_session, ship_id)
        resp_fail = self._assert_status_and_decode(ans_fail, 404, "Remove Non-existent Ship")
        self._get_message(resp_fail, "Remove Non-existent Ship")

    def __check_admiral_get_all_ships(self, admiral_session):
        ans = self.mch.admiral_get_all_ships(admiral_session)
        resp_json = self._assert_status_and_decode(ans, 200, "Admiral Get Ships")
        self.assert_in("ships", resp_json, "[Admiral Get Ships] Response missing 'ships'")
        ships = resp_json["ships"]
        self.assert_(isinstance(ships, list), "[Admiral Get Ships] Response 'ships' key is not a list")
        return ships

    def __check_admiral_get_all_captains(self, admiral_session):
        ans = self.mch.admiral_get_all_captains(admiral_session)
        resp_json = self._assert_status_and_decode(ans, 200, "Admiral Get Captains")
        self.assert_in("captains", resp_json, "[Admiral Get Captains] Response missing 'captains'")
        captains = resp_json["captains"]
        self.assert_(isinstance(captains, list), "[Admiral Get Captains] Response 'captains' key is not a list")
        return captains

    def __check_admiral_assign_captain_to_ship(self, admiral_session, captain_id, ship_id):
        ans = self.mch.admiral_assign_captain_to_ship(admiral_session, captain_id, ship_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Assign Captain {captain_id} to Ship {ship_id}")
        message = self._get_message(resp_json, f"Assign Captain {captain_id} to Ship {ship_id}")
        self.assert_eq("Successfully assign captain to ship.", message, "[Assign Captain] Unexpected success message")

    def __check_admiral_remove_captain_from_ship(self, admiral_session, captain_id):
        ans = self.mch.admiral_remove_captain_from_ship(admiral_session, captain_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Remove Captain {captain_id} from Ship")
        message = self._get_message(resp_json, f"Remove Captain {captain_id} from Ship")
        self.assert_eq("Successfully delete captain from ship.", message, "[Remove Captain] Unexpected success message")
        ans_fail = self.mch.admiral_remove_captain_from_ship(admiral_session, captain_id)
        self.assert_eq(500, ans_fail.status_code, "[Remove Non-assigned Captain] Unexpected status code")

    def __check_admiral_assign_mission(self, admiral_session, ship_id, description):
        ans = self.mch.admiral_assign_mission_to_ship(admiral_session, ship_id, description)
        if ans.status_code == 404:
             resp_json_404 = self._assert_status_and_decode(ans, 404, f"Assign Mission (Ship {ship_id}) - Already Exists?")
             msg_404 = self._get_message(resp_json_404, "Assign Mission (Already Exists?)")
             self.assert_eq("Ship already has mission.", msg_404, "[Assign Mission] Unexpected 404 message")
             time.sleep(1)
             missions = self.__check_admiral_get_missions(admiral_session)
             for m_id, m_desc, m_active, m_ship_id_found in missions:
                 if m_ship_id_found == ship_id and m_active:
                     return m_id
             self.cquit(Status.MUMBLE, "[Assign Mission] Ship has mission (404), but couldn't find an active one", f"Failed for ship {ship_id}")

        resp_json = self._assert_status_and_decode(ans, 200, f"Assign Mission (Ship {ship_id})")
        message = self._get_message(resp_json, f"Assign Mission (Ship {ship_id})")
        self.assert_eq("Successfuly added the mission.", message, "[Assign Mission] Unexpected success message")
        time.sleep(0.5)
        missions = self.__check_admiral_get_missions(admiral_session)

        for m_id, m_desc, m_active, m_ship_id_found in missions:
            if m_ship_id_found == ship_id and m_desc == description and not m_active:
                return m_id
        self.cquit(Status.MUMBLE, "[Assign Mission] Assigned mission not found shortly after creation", f"Failed for ship {ship_id}")

    def __check_admiral_remove_mission(self, admiral_session, mission_id):
        ans = self.mch.admiral_remove_mission_to_ship(admiral_session, mission_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Remove Mission {mission_id}")
        message = self._get_message(resp_json, f"Remove Mission {mission_id}")
        self.assert_eq("Successfuly deleted the mission.", message, "[Remove Mission] Unexpected success message")
        ans_fail = self.mch.admiral_remove_mission_to_ship(admiral_session, mission_id)
        self.assert_in(ans_fail.status_code, [400, 500], "[Remove Non-existent Mission] Unexpected status code")

    def __check_admiral_get_missions(self, admiral_session):
        ans = self.mch.admiral_get_all_missions(admiral_session)
        resp_json = self._assert_status_and_decode(ans, 200, "Admiral Get Missions")
        self.assert_in("missions", resp_json, "[Admiral Get Missions] Response missing 'missions'")
        missions_list = resp_json["missions"]
        self.assert_(isinstance(missions_list, list), "[Admiral Get Missions] Response 'missions' key is not a list")
        missions_data = []
        for mission in missions_list:
            self.assert_in("id", mission, "[Admiral Get Missions] Mission object missing 'id'")
            self.assert_in("description", mission, "[Admiral Get Missions] Mission object missing 'description'")
            self.assert_in("active", mission, "[Admiral Get Missions] Mission object missing 'active'")
            self.assert_in("ship_id", mission, "[Admiral Get Missions] Mission object missing 'ship_id'")
            m_id = mission["id"]
            m_desc = self.decode(mission["description"])
            m_active = bool(mission["active"])
            m_ship_id = mission["ship_id"]
            missions_data.append((m_id, m_desc, m_active, m_ship_id))
        return missions_data

    def __check_admiral_change_captain_description(self, admiral_session, captain_id, new_desc):
        ans = self.mch.admiral_change_captain_description(admiral_session, captain_id, new_desc)
        resp_json = self._assert_status_and_decode(ans, 200, f"Change Captain Desc {captain_id}")
        message = self._get_message(resp_json, f"Change Captain Desc {captain_id}")
        self.assert_eq("Successfully changed the description.", message, "[Change Captain Desc] Unexpected success message")

    def __check_captain_get_my_ship_info(self, captain_session):
        ans = self.mch.captain_get_my_ship_info(captain_session)
        resp_json = self._assert_status_and_decode(ans, 200, "Captain Get My Ship")
        self.assert_in("id", resp_json, "[Captain Get My Ship] Response missing 'id'")
        self.assert_in("name", resp_json, "[Captain Get My Ship] Response missing 'name'")
        ship_id = resp_json['id']
        ship_name = self.decode(resp_json['name'])
        self.assert_(isinstance(ship_id, int), "[Captain Get My Ship] Ship ID is not an integer")
        return ship_id, ship_name

    def __check_captain_accept_sailor_request(self, captain_session, sailor_id):
        ans = self.mch.captain_accept_sailor_request(captain_session, sailor_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Accept Sailor {sailor_id}")
        message = self._get_message(resp_json, f"Accept Sailor {sailor_id}")
        self.assert_eq("Successfuly approve the request.", message, "[Accept Sailor] Unexpected success message")

    def __check_captain_decline_sailor_request(self, captain_session, sailor_id):
        ans = self.mch.captain_decline_sailor_request(captain_session, sailor_id)
        resp_json = self._assert_status_and_decode(ans, 200, f"Decline Sailor {sailor_id}")
        message = self._get_message(resp_json, f"Decline Sailor {sailor_id}")
        self.assert_eq("Successfuly decline the request.", message, "[Decline Sailor] Unexpected success message")
        ans_fail = self.mch.captain_decline_sailor_request(captain_session, 99998)
        resp_fail = self._assert_status_and_decode(ans_fail, 404, "Decline Non-existent Sailor")
        self._get_message(resp_fail, "Decline Non-existent Sailor")

    def __check_captain_check_mission(self, captain_session):
        ans = self.mch.captain_check_mission(captain_session)
        resp_json = self._assert_status_and_decode(ans, 200, "Captain Check Mission")
        message = self._get_message(resp_json, "Captain Check Mission")
        if "No mission was assign" in message:
            return None
        self.assert_eq("Successfuly check mission.", message, "[Captain Check Mission] Unexpected success message")
        self.assert_in("mission_id", resp_json, "[Captain Check Mission] Response missing 'mission_id'")
        self.assert_in("description", resp_json, "[Captain Check Mission] Response missing 'description'")
        self.assert_in("isComplete", resp_json, "[Captain Check Mission] Response missing 'isComplete'")
        m_id = resp_json['mission_id']
        m_desc = self.decode(resp_json['description'])
        m_complete = bool(resp_json['isComplete'])
        self.assert_(isinstance(m_id, int), "[Captain Check Mission] Mission ID is not an integer")
        return m_id, m_desc, m_complete

    def __check_captain_mark_mission_as_complete(self, captain_session):
        ans = self.mch.captain_mark_mission_as_complete(captain_session)
        resp_json = self._assert_status_and_decode(ans, 200, "Captain Complete Mission")
        message = self._get_message(resp_json, "Captain Complete Mission")
        self.assert_eq("Successfuly set mission as complete.", message, "[Captain Complete Mission] Unexpected success message")

    def __check_captain_change_sailor_description(self, captain_session, sailor_id, new_desc):
        ans = self.mch.captain_change_sailor_description(captain_session, sailor_id, new_desc)
        resp_json = self._assert_status_and_decode(ans, 200, f"Change Sailor Desc {sailor_id}")
        message = self._get_message(resp_json, f"Change Sailor Desc {sailor_id}")
        self.assert_eq("Successfully changed the description.", message, "[Change Sailor Desc] Unexpected success message")

    def __check_captain_get_sailors_for_ship(self, captain_session):
        ans = self.mch.captain_get_sailors_for_ship(captain_session)
        resp_json = self._assert_status_and_decode(ans, 200, "Captain Get Sailors")
        self.assert_in("sailors", resp_json, "[Captain Get Sailors] Response missing 'sailors'")
        sailors = resp_json["sailors"]
        self.assert_(isinstance(sailors, list), "[Captain Get Sailors] Response 'sailors' key is not a list")
        return sailors

    def check(self):
        s_unauth = self.get_initialized_session()

        adm_fn, adm_sn, adm_tn, adm_pw = self.generate_user()
        cap_fn, cap_sn, cap_tn, cap_pw = self.generate_user()
        sail_fn, sail_sn, sail_tn, sail_pw = self.generate_user()

        self.__check_register(s_unauth, adm_fn, adm_sn, adm_tn, adm_pw)
        self.__check_register(s_unauth, cap_fn, cap_sn, cap_tn, cap_pw)
        self.__check_register(s_unauth, sail_fn, sail_sn, sail_tn, sail_pw)

        adm_sess = self.__check_login(s_unauth, adm_fn, adm_sn, adm_tn, adm_pw)
        cap_sess = self.__check_login(s_unauth, cap_fn, cap_sn, cap_tn, cap_pw)
        sail_sess = self.__check_login(s_unauth, sail_fn, sail_sn, sail_tn, sail_pw)

        fleet_country = rnd_string(15)
        fleet_desc = rnd_string(40)
        fleet_id, adm_sess = self.__check_admiral_create_fleet(
            s_unauth, adm_sess, fleet_country, fleet_desc,
            adm_fn, adm_sn, adm_tn, adm_pw
        )

        command = self.__check_get_command_for_fleet(adm_sess, fleet_id)
        self.assert_(len(command) >= 1, "[Check] Command structure should not be empty after fleet creation")
        admiral_id = None
        for person in command:
             admiral_id = person.get('id')
             break
        self.assert_(admiral_id is not None, "[Check] Could not find Admiral ID in command structure")

        self.__check_create_request(cap_sess, admiral_id, "Captain")

        time.sleep(0.5)
        captain_id = self._find_user_id_from_requests(adm_sess, "captain")
        self.__check_admiral_accept_captain_request(adm_sess, captain_id)

        cap_sess = self.__check_login(s_unauth, cap_fn, cap_sn, cap_tn, cap_pw)

        ship_name = rnd_string(20)
        ship_id = self.__check_admiral_create_ship(adm_sess, ship_name)

        self.__check_admiral_assign_captain_to_ship(adm_sess, captain_id, ship_id)

        time.sleep(0.5)

        cap_ship_id, cap_ship_name = self.__check_captain_get_my_ship_info(cap_sess)
        self.assert_eq(ship_id, cap_ship_id, "[Check] Captain get_my_ship_info returned wrong ship ID")

        self.__check_create_request(sail_sess, captain_id, "Sailor")

        time.sleep(0.5)
        sailor_id = self._find_user_id_from_requests(cap_sess, "sailor")
        self.__check_captain_accept_sailor_request(cap_sess, sailor_id)

        sail_sess = self.__check_login(s_unauth, sail_fn, sail_sn, sail_tn, sail_pw)

        mission_desc = rnd_string(25)
        mission_id = self.__check_admiral_assign_mission(adm_sess, ship_id, mission_desc)

        m_info = self.__check_captain_check_mission(cap_sess)
        self.assert_(m_info is not None, "[Check] Captain has no mission after assignment")
        m_id, m_desc, m_complete = m_info
        self.assert_eq(mission_id, m_id, "[Check] Captain checking wrong mission ID")
        self.assert_eq(mission_desc, m_desc, "[Check] Captain checking wrong mission description")
        self.assert_(not m_complete, "[Check] Mission should be incomplete initially")

        self.__check_captain_mark_mission_as_complete(cap_sess)

        m_info_after = self.__check_captain_check_mission(cap_sess)
        self.assert_(m_info_after is not None, "[Check] Captain has no mission info after completion")
        m_id_after, _, m_complete_after = m_info_after
        self.assert_eq(mission_id, m_id_after, "[Check] Mission ID mismatch after completion check")
        self.assert_(m_complete_after, "[Check] Mission should be complete after marking")

        self.__check_admiral_remove_mission(adm_sess, mission_id)

        self.cquit(Status.OK)

    def put(self, flag_id: str, flag: str, vuln: str):
        s_unauth = self.get_initialized_session()

        adm_fn, adm_sn, adm_tn, adm_pw = self.generate_user()
        cap_fn, cap_sn, cap_tn, cap_pw = self.generate_user()
        self.__check_register(s_unauth, adm_fn, adm_sn, adm_tn, adm_pw)
        self.__check_register(s_unauth, cap_fn, cap_sn, cap_tn, cap_pw)

        adm_sess = self.__check_login(s_unauth, adm_fn, adm_sn, adm_tn, adm_pw)
        cap_sess = self.__check_login(s_unauth, cap_fn, cap_sn, cap_tn, cap_pw)

        fleet_country = rnd_string(15)
        fleet_desc = rnd_string(35)
        fleet_id, adm_sess = self.__check_admiral_create_fleet(
            s_unauth, adm_sess, fleet_country, fleet_desc,
            adm_fn, adm_sn, adm_tn, adm_pw
        )

        command = self.__check_get_command_for_fleet(adm_sess, fleet_id)
        self.assert_eq(len(command), 1, "[Put] Expected only Admiral in command for put")
        admiral_id = command[0]['id']

        self.__check_create_request(cap_sess, admiral_id, "Captain")

        time.sleep(1)
        captain_id = self._find_user_id_from_requests(adm_sess, "captain")
        self.__check_admiral_accept_captain_request(adm_sess, captain_id)

        cap_sess = self.__check_login(s_unauth, cap_fn, cap_sn, cap_tn, cap_pw)

        ship_name = rnd_string(20)
        ship_id = self.__check_admiral_create_ship(adm_sess, ship_name)

        self.__check_admiral_assign_captain_to_ship(adm_sess, captain_id, ship_id)

        mission_id = self.__check_admiral_assign_mission(adm_sess, ship_id, flag)
        self.assert_(mission_id is not None, "[Put] Failed to get mission ID after assigning flag mission")

        flag_id_data = f"{adm_fn}:{adm_sn}:{adm_tn}:{adm_pw}:{mission_id}"
        self.cquit(Status.OK, flag_id_data)

    def get(self, flag_id: str, flag: str, vuln: str):
        s_unauth = self.get_initialized_session()

        try:
            parts = flag_id.split(':')
            if len(parts) != 5:
                 raise ValueError("Incorrect number of parts in flag_id")
            adm_fn, adm_sn, adm_tn, adm_pw, mission_id_str = parts
            mission_id = int(mission_id_str)
        except (ValueError, TypeError) as e:
            self.cquit(Status.ERROR, "[Get] Invalid flag_id format provided", f"flag_id format error: {e} ('{flag_id}')")

        adm_sess = self.__check_login(s_unauth, adm_fn, adm_sn, adm_tn, adm_pw)

        all_missions = self.__check_admiral_get_missions(adm_sess)

        found_mission_desc = None
        for m_id, m_desc, m_active, m_ship_id in all_missions:
            if m_id == mission_id:
                found_mission_desc = m_desc
                break

        self.assert_(found_mission_desc is not None, f"[Get] Failed to find mission with ID {mission_id}")
        self.assert_eq(flag, found_mission_desc, "[Get] Failed to retrieve correct flag from mission description")

        self.cquit(Status.OK)


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print(f"Usage: python {sys.argv[0]} <action> <host> [flag_id] [flag] [vuln]", file=sys.stderr)
        sys.exit(1)

    c = Checker(sys.argv[2])

    try:
        c.action(sys.argv[1], *sys.argv[3:])
    except c.get_check_finished_exception():
        cquit(Status(c.status), c.public, c.private)