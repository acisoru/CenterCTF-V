from app import ma
from models.boyarin import Boyarin


class BoyarinSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = Boyarin
        include_relationships = True
        load_instance = True
        load_only = ("passphrase",)
        dump_only = ("id",)
