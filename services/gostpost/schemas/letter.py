from app import ma
from models.letter import Letter


class LetterSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = Letter
        include_relationships = True
        load_instance = True
        dump_only = ("id",)
