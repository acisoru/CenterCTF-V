import checklib
from requests import Session
from dataclasses import dataclass
from gostcrypto import gostcipher
from secrets import token_bytes
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from checker import Checker


@dataclass
class Boyarin:
    id: int
    name: str


@dataclass
class Letter:
    sender: Boyarin
    recipient: Boyarin
    content: str
    is_encrypted: bool


class ApiClient(object):
    def __init__(self, checker: "Checker"):
        self.checker = checker
        self.url = self.checker.url
        self.sess: Session = checker.get_initialized_session()
        self.me = None
        self.cache = {}

    def assert_schema(
        self,
        data: dict,
        schema: "dict[str, type | dict]",
        suffix: str,
        status=checklib.Status.MUMBLE,
    ):
        self.checker.assert_(isinstance(data, dict), f"Malformed data" + suffix)
        for k, t in schema.items():
            self.checker.assert_in(k, data, f"Missing {k}" + suffix)
            self.checker.assert_(isinstance(data[k], t), f"Malformed {k}" + suffix)

    def login(self, name, passphrase):
        r = self.sess.post(
            self.url + "/auth/login", json={"name": name, "passphrase": passphrase}
        )
        self.checker.check_response(r, "Error on login")
        self.me = name

    def register(self, name, passphrase, parent=None):
        r = self.sess.post(
            self.url + "/auth/register",
            json={"name": name, "passphrase": passphrase, "parent": parent},
        )
        self.checker.check_response(r, "Error on register")
        self.me = name

    def get_boyarin(self, id=None, name=None) -> Boyarin:
        cache_key = (id, name)
        if cache_key in self.cache:
            return self.cache[cache_key]
        data = {}
        if id is not None:
            data["id"] = id
        if name is not None:
            data["name"] = name
        r = self.sess.post(self.url + "/boyarin/get", json=data)
        self.checker.check_response(r, "Error on /boyarin/get")
        data = self.checker.get_json(r, "Malformed /boyarin/get response")
        self.assert_schema(
            data, {"id": object, "name": str}, " in /boyarin/get response"
        )
        self.checker.assert_(
            isinstance(data["id"], (int, str)), "Malformed id in /boyarin/get response"
        )
        boyarin = Boyarin(id=data["id"], name=data["name"])
        self.cache[cache_key] = boyarin
        return boyarin

    def get_letters(self) -> list[Letter]:
        r = self.sess.get(self.url + "/letters/get")
        self.checker.check_response(r, "Error on /letters/get")
        data = self.checker.get_json(r, "Malformed /letters/get response")
        self.checker.assert_(isinstance(data, list), "Malformed /letters/get response")
        letters = []
        for e in data:
            self.assert_schema(
                e,
                {
                    "sender": object,
                    "recipient": object,
                    "content": object,
                    "is_encrypted": object,
                },
                " in a letter",
            )
            letters.append(
                Letter(
                    sender=self.get_boyarin(id=e["sender"]),
                    recipient=self.get_boyarin(id=e["recipient"]),
                    content=e["content"],
                    is_encrypted=e["is_encrypted"],
                )
            )
        shown_letters = []
        for letter in letters:
            if not letter.is_encrypted:
                shown_letters.append(letter)
                continue
            other = None
            if letter.sender.name == self.me:
                other = letter.recipient
            elif letter.recipient.name == self.me:
                other = letter.sender
            else:
                continue
            try:
                letter.content = self.decrypt_letter(other, letter.content)
                shown_letters.append(letter)
            except Exception as e:
                pass
        self.cache.clear()
        return shown_letters

    def send_letter(self, recipient_name, content, do_encrypt=False):
        recipient = self.get_boyarin(name=recipient_name)
        data = {"recipient": recipient.id, "is_encrypted": do_encrypt}
        if do_encrypt:
            data["content"] = self.encrypt_letter(recipient, content)
        else:
            data["content"] = content
        r = self.sess.post(self.url + "/letters/send", json=data)
        self.checker.check_response(r, "Error on /letters/send")

    def get_shared_key(self, other: Boyarin) -> bytes:
        cache_key = other.id
        if cache_key in self.cache:
            return self.cache[cache_key]
        data = {"other_id": other.id}
        r = self.sess.post(self.url + "/secrets/shared_key", json=data)
        self.checker.check_response(r, "Error on /secrets/shared_key")
        data = self.checker.get_json(r, "Malformed /secrets/shared_key response")
        self.assert_schema(
            data, {"shared_key": str}, " in /secrets/shared_key response"
        )
        try:
            result = bytes.fromhex(data["shared_key"])
        except ValueError as e:
            self.checker.cquit(
                checklib.Status.MUMBLE,
                "Malformed hex in shared_key",
                f"`{e}` in shared_key",
            )
        self.cache[cache_key] = result
        return result

    def encrypt_letter(self, other: Boyarin, content: str) -> str:
        key = self.get_shared_key(other)
        self.checker.assert_eq(len(key), 32, "Wrong key length")
        iv = token_bytes(8)
        cipher_obj = gostcipher.new("kuznechik", key, gostcipher.MODE_CTR, init_vect=iv)
        ct = cipher_obj.encrypt(content.encode())
        return (iv + ct).hex()

    def decrypt_letter(self, other: Boyarin, content: str) -> str:
        key = self.get_shared_key(other)
        data = bytes.fromhex(content)
        iv, ct = data[:8], data[8:]
        cipher_obj = gostcipher.new("kuznechik", key, gostcipher.MODE_CTR, init_vect=iv)
        pt = cipher_obj.decrypt(ct).decode()
        return pt
