import uuid
from flask import Blueprint, request, session
from app import db
from models.boyarin import Boyarin
from schemas.boyarin import BoyarinSchema

auth = Blueprint("auth", __name__)


@auth.route("/register", methods=["POST"])
def register():
    data = request.get_json()
    if (
        not data["name"]
        or data["name"].isspace()
        or not data["passphrase"]
        or data["passphrase"].isspace()
    ):
        return 400
    data["uid"] = str(uuid.uuid4())
    parent = data["parent"]
    del data["parent"]
    boyarin = BoyarinSchema(exclude=["son"]).load(data)
    db.session.add(boyarin)
    db.session.commit()
    if parent:
        Boyarin.query.filter_by(name=parent).first().son_id = boyarin.id
        db.session.commit()
    session["user_id"] = boyarin.id
    return BoyarinSchema().dump(boyarin)


@auth.route("/login", methods=["POST"])
def login():
    data = request.get_json()
    boyarin = Boyarin.query.filter_by(
        name=data["name"], passphrase=data["passphrase"]
    ).first()
    session["user_id"] = boyarin.id
    return BoyarinSchema().dump(boyarin)
