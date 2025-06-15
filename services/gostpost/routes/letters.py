from flask import Blueprint, request, session
from app import db
from models.boyarin import Boyarin
from models.letter import Letter
from schemas.letter import LetterSchema
from sqlalchemy import or_

letters = Blueprint("letters", __name__)


@letters.route("/send", methods=["POST"])
def send_letter():
    data = request.get_json()
    data["sender"] = session["user_id"]
    letter = LetterSchema().load(data)
    db.session.add(letter)
    db.session.commit()
    return LetterSchema().dump(letter)


@letters.route("/get", methods=["GET"])
def get_letters():
    boyarin = Boyarin.query.get(session["user_id"])
    letters_query = Letter.query.filter(
        or_(Letter.recipient_id == boyarin.id, Letter.sender_id == boyarin.id)
    )
    if boyarin.son:
        son_letters_query = Letter.query.filter(
            or_(
                Letter.recipient_id == boyarin.son.id,
                Letter.sender_id == boyarin.son.id,
            )
        )
        letters_query = letters_query.union(son_letters_query)
    letters = letters_query.all()
    return LetterSchema(many=True).dump(letters)
