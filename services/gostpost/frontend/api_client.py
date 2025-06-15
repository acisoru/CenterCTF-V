from aiohttp import ClientSession, CookieJar
from dataclasses import dataclass
from gostcrypto import gostcipher
from secrets import token_bytes


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


class ApiClient:
    def __init__(self, base_url, session, me):
        self.base_url = base_url
        self.session: ClientSession = session
        self.me = me
        self.cache = {}
        self.prev_letters = []

    @staticmethod
    async def login(base_url, name, passphrase) -> "ApiClient | str":
        cookie_jar = CookieJar(unsafe=True)
        sess = ClientSession(cookie_jar=cookie_jar)
        try:
            async with sess.post(
                base_url + "/auth/login", json={"name": name, "passphrase": passphrase}
            ) as r:
                if r.status != 200:
                    return "Error"
                with open("hello.txt", "w") as f:
                    f.write("...\n")
                    f.write(str(r.raw_headers))
                    f.write("...\n")
        except Exception as e:
            return str(e)
        return ApiClient(base_url, sess, name)

    @staticmethod
    async def register(base_url, name, passphrase, parent=None) -> "ApiClient | str":
        cookie_jar = CookieJar(unsafe=True)
        sess = ClientSession(cookie_jar=cookie_jar)
        try:
            async with sess.post(
                base_url + "/auth/register",
                json={"name": name, "passphrase": passphrase, "parent": parent},
            ) as r:
                if r.status != 200:
                    return "Error"
        except Exception as e:
            return str(e)
        return ApiClient(base_url, sess, name)

    async def get_boyarin(self, id=None, name=None) -> Boyarin:
        cache_key = (id, name)
        if cache_key in self.cache:
            return self.cache[cache_key]
        data = {}
        if id is not None:
            data["id"] = id
        if name is not None:
            data["name"] = name
        async with self.session.post(self.base_url + "/boyarin/get", json=data) as r:
            if r.status != 200:
                return None
            data = await r.json()
            boyarin = Boyarin(id=data["id"], name=data["name"])
            self.cache[cache_key] = boyarin
            return boyarin

    async def get_letters(self) -> list[Letter]:
        async with self.session.get(self.base_url + "/letters/get") as r:
            result = [
                Letter(
                    sender=await self.get_boyarin(id=e["sender"]),
                    recipient=await self.get_boyarin(id=e["recipient"]),
                    content=e["content"],
                    is_encrypted=e["is_encrypted"],
                )
                for e in await r.json()
            ]
            shown_letters = []
            for letter in result:
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
                letter.content = await self.decrypt_letter(other, letter.content)
                shown_letters.append(letter)
            result = shown_letters
            self.cache.clear()
            if result == self.prev_letters:
                return result, False
            else:
                self.prev_letters = result
                return result, True

    async def send_letter(self, recipient_name, content, do_encrypt=False) -> None:
        recipient = await self.get_boyarin(name=recipient_name)
        if recipient is None:
            return False
        data = {"recipient": recipient.id, "is_encrypted": do_encrypt}
        if do_encrypt:
            data["content"] = await self.encrypt_letter(recipient, content)
        else:
            data["content"] = content
        async with self.session.post(self.base_url + "/letters/send", json=data) as r:
            return r.status == 200

    async def get_shared_key(self, other: Boyarin) -> bytes:
        cache_key = other.id
        if cache_key in self.cache:
            return self.cache[cache_key]
        data = {"other_id": other.id}
        async with self.session.post(
            self.base_url + "/secrets/shared_key", json=data
        ) as r:
            data = await r.json()
            result = bytes.fromhex(data["shared_key"])
            self.cache[cache_key] = result
            return result

    async def encrypt_letter(self, other: Boyarin, content: str) -> str:
        key = await self.get_shared_key(other)
        iv = token_bytes(8)
        cipher_obj = gostcipher.new("kuznechik", key, gostcipher.MODE_CTR, init_vect=iv)
        ct = cipher_obj.encrypt(content.encode())
        return (iv + ct).hex()

    async def decrypt_letter(self, other: Boyarin, content: str) -> str:
        try:
            key = await self.get_shared_key(other)
            data = bytes.fromhex(content)
            iv, ct = data[:8], data[8:]
            cipher_obj = gostcipher.new(
                "kuznechik", key, gostcipher.MODE_CTR, init_vect=iv
            )
            pt = cipher_obj.decrypt(ct).decode()
            return pt
        except Exception:
            return content

    def close(self):
        self.session._connector.close()
