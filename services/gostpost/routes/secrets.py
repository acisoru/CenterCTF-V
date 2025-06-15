from flask import Blueprint, jsonify, request, session
from gostcrypto import gosthash
from app import SECRET_KEY
from models.boyarin import Boyarin

secrets = Blueprint("secrets", __name__)


@secrets.route("/shared_key", methods=["POST"])
def get_shared_key():
    user1 = Boyarin.query.get(session["user_id"])
    data = request.get_json()
    user2 = Boyarin.query.get(data["other_id"])
    uid1, uid2 = sorted((user1.uid, user2.uid))
    streebog = gosthash.new("streebog512")
    streebog.update(SECRET_KEY.encode())
    streebog.update(uid1.encode())
    streebog.update(SECRET_KEY.encode())
    streebog.update(uid2.encode())
    streebog.update(SECRET_KEY.encode())
    key = streebog.hexdigest()[:64]
    return jsonify({"shared_key": key})
