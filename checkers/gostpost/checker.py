#!/usr/bin/env python3

import sys
import secrets
import json
import checklib
import requests
import random
from checker_api_client import ApiClient


def random_str(prefix="") -> str:
    if random.random() < 0.1:
        return prefix + "Hello world"
    elif random.random() < 0.1:
        return prefix + "How are you?"
    elif random.random() < 0.1:
        return prefix + "I'm fine"
    elif random.random() < 0.1:
        return prefix + "Super secret communication"
    elif random.random() < 0.1:
        return prefix + "There are flags I want to send you"
    else:
        return prefix + checklib.rnd_string(5 + secrets.randbelow(40))


def random_username() -> str:
    if random.random() < 0.8:
        return checklib.rnd_username()
    return checklib.rnd_string(5 + secrets.randbelow(20))


def random_password() -> str:
    if random.random() < 0.8:
        return checklib.rnd_password()
    return checklib.rnd_string(5 + secrets.randbelow(20))


class Checker(checklib.BaseChecker):
    vulns = 1
    timeout = 60
    uses_attack_data: bool = True

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.url = f"http://{self.host}:5454"

    def action(self, action, *args, **kwargs):
        try:
            super().action(action, *args, **kwargs)
        except self.get_check_finished_exception():
            raise
        except requests.RequestException as e:
            self.cquit(
                checklib.Status.DOWN, "Connection error", f"Requests error {repr(e)}"
            )

    def assert_letter_in(self, letters, sender, recipient, content, message):
        for letter in letters:
            if (
                letter.recipient.name == recipient
                and letter.sender.name == sender
                and letter.content == content
            ):
                return
        self.assert_(False, message, skip=4)

    def assert_letter_not_in(self, letters, sender, recipient, content, message):
        for letter in letters:
            if (
                letter.recipient.name == recipient
                and letter.sender.name == sender
                and letter.content == content
            ):
                self.assert_(False, message, skip=4)

    def check(self):
        # - test login/register -
        u1 = random_username()
        p1 = random_password()
        c1 = ApiClient(self)
        c1.register(u1, p1)
        c1 = ApiClient(self)
        c1.login(u1, p1)

        u2 = random_username()
        p2 = random_password()
        c2 = ApiClient(self)
        c2.register(u2, p2)

        # - test sending -
        letter1 = random_str("1 ")
        c1.send_letter(u2, letter1)
        self.assert_letter_in(
            c2.get_letters(), u1, u2, letter1, "Letter didn't come through"
        )
        self.assert_letter_in(
            c1.get_letters(), u1, u2, letter1, "Letter didn't appear for self"
        )

        letter2 = random_str("2 letter ")
        c2.send_letter(u1, letter2)
        self.assert_letter_in(
            c1.get_letters(), u2, u1, letter2, "Letter didn't come through"
        )
        self.assert_letter_in(
            c2.get_letters(), u2, u1, letter2, "Letter didn't appear for self"
        )
        self.assert_letter_in(c2.get_letters(), u1, u2, letter1, "Letter disappeared")

        # same, but encrypted
        letter3 = random_str("letter ")
        c1.send_letter(u2, letter3, True)
        self.assert_letter_in(
            c2.get_letters(), u1, u2, letter3, "Letter didn't come through"
        )
        self.assert_letter_in(
            c1.get_letters(), u1, u2, letter3, "Letter didn't appear for self"
        )

        letter4 = random_str("new letter ")
        c2.send_letter(u1, letter4, True)
        self.assert_letter_in(
            c1.get_letters(), u2, u1, letter4, "Letter didn't come through"
        )
        self.assert_letter_in(
            c2.get_letters(), u2, u1, letter4, "Letter didn't appear for self"
        )
        self.assert_letter_in(c2.get_letters(), u1, u2, letter3, "Letter disappeared")

        # - test parenting -
        u3 = random_username()
        p3 = random_password()
        c3 = ApiClient(self)
        c3.register(u3, p3, u1)

        # send letters
        letter5 = random_str("another letter")
        c2.send_letter(u3, letter5)
        self.assert_letter_in(
            c3.get_letters(), u2, u3, letter5, "Letter didn't come through"
        )
        self.assert_letter_in(
            c2.get_letters(), u2, u3, letter5, "Letter didn't appear for self"
        )
        self.assert_letter_in(
            c1.get_letters(), u2, u3, letter5, "Letter didn't come through to parent"
        )

        letter6 = random_str("hello ")
        c1.send_letter(u3, letter6)
        self.assert_letter_in(
            c3.get_letters(), u1, u3, letter6, "Letter didn't come through"
        )
        self.assert_letter_in(
            c1.get_letters(), u1, u3, letter6, "Letter didn't appear for self"
        )

        letter7 = random_str("Hi ")
        c2.send_letter(u3, letter7, True)
        self.assert_letter_in(
            c3.get_letters(), u2, u3, letter7, "Letter didn't come through"
        )
        self.assert_letter_in(
            c2.get_letters(), u2, u3, letter7, "Letter didn't appear for self"
        )
        self.assert_letter_not_in(
            c1.get_letters(), u2, u3, letter7, "Encrypted letter came through to parent"
        )

        letter8 = random_str("---")
        c3.send_letter(u1, letter8, True)
        self.assert_letter_in(
            c1.get_letters(), u3, u1, letter8, "Letter didn't come through"
        )
        self.assert_letter_in(
            c3.get_letters(), u3, u1, letter8, "Letter didn't appear for self"
        )
        self.assert_letter_not_in(
            c2.get_letters(), u3, u1, letter8, "Letter came through to another person"
        )

        letter_list = [
            ((c1, c2, None), c3, (u1, u2, letter1)),
            ((c2, c1, None), c3, (u2, u1, letter2)),
            ((c1, c2, None), c3, (u1, u2, letter3)),
            ((c2, c1, None), c3, (u2, u1, letter4)),
            ((c2, c3, c1), None, (u2, u3, letter5)),
            ((c1, c3, None), c2, (u1, u3, letter6)),
            ((c2, c3, None), c1, (u2, u3, letter7)),
            ((c3, c1, None), c2, (u3, u1, letter8)),
        ]

        for visibles, not_visible, letter_data in letter_list:
            if visibles[1]:
                self.assert_letter_in(
                    visibles[1].get_letters(), *letter_data, "Letter disappeared"
                )
            if visibles[0]:
                self.assert_letter_in(
                    visibles[0].get_letters(),
                    *letter_data,
                    "Letter disappeared for self",
                )
            if visibles[2]:
                self.assert_letter_in(
                    visibles[2].get_letters(),
                    *letter_data,
                    "Letter disappeared for parent",
                )
            if not_visible:
                self.assert_letter_not_in(
                    not_visible.get_letters(),
                    *letter_data,
                    "Letter came through to another person",
                )

        self.cquit(checklib.Status.OK)

    def put(self, flag_id: str, flag: str, vuln: str):
        u1 = random_username()
        p1 = random_password()
        c1 = ApiClient(self)
        c1.register(u1, p1)

        u2 = random_username()
        p2 = random_password()
        c2 = ApiClient(self)
        c2.register(u2, p2)

        c1.send_letter(u2, "Keep this secret: " + flag, True)

        self.cquit(
            checklib.Status.OK,
            public=json.dumps({"username": u2}),
            private=json.dumps({"username": u2, "password": p2, "other_username": u1}),
        )

    def get(self, flag_id: str, flag: str, vuln: str):
        data = json.loads(flag_id)
        u2, p2, u1 = data["username"], data["password"], data["other_username"]
        c2 = ApiClient(self)
        c2.login(u2, p2)
        self.assert_letter_in(c2.get_letters(), u1, u2, "Keep this secret: " + flag, "No letter with the flag")

        self.cquit(checklib.Status.OK)


if __name__ == "__main__":
    host = sys.argv[2]
    checker = Checker(host)

    try:
        action = sys.argv[1]
        arguments = sys.argv[3:]

        checker.action(action, *arguments)
    except checker.get_check_finished_exception():
        checklib.cquit(
            checklib.Status(checker.status),
            checker.public,
            checker.private,
        )
