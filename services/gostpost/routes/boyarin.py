from flask import Blueprint, request
from models.boyarin import Boyarin
from schemas.boyarin import BoyarinSchema

boyarin = Blueprint("boyarin", __name__)


@boyarin.route("/get", methods=["POST"])
def get_boyarin():
    data = request.get_json()
    boyarin = Boyarin.query.filter_by(**data).first()
    if not boyarin:
        return "", 400
    return BoyarinSchema().dump(boyarin)


@boyarin.route("/set_son", methods=["POST"])
def set_son():
    raise NotImplementedError("TODO")
