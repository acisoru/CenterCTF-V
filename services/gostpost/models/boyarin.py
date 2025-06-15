from app import db
from sqlalchemy.orm import Mapped, mapped_column


class Boyarin(db.Model):
    id: Mapped[int] = mapped_column(primary_key=True)
    uid: Mapped[str] = mapped_column()
    name: Mapped[str] = mapped_column(unique=True)
    passphrase: Mapped[str] = mapped_column()
    son_id: Mapped[int | None] = mapped_column(db.ForeignKey("boyarin.id"))

    son: Mapped["Boyarin | None"] = db.relationship(remote_side=[id])
