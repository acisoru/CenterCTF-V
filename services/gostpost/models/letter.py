from app import db
from sqlalchemy.orm import Mapped, mapped_column
from .boyarin import Boyarin


class Letter(db.Model):
    id: Mapped[int] = mapped_column(primary_key=True)
    sender_id: Mapped[int] = mapped_column(db.ForeignKey("boyarin.id"))
    recipient_id: Mapped[int] = mapped_column(db.ForeignKey("boyarin.id"))
    content: Mapped[str] = mapped_column()
    is_encrypted: Mapped[bool] = mapped_column()

    sender: Mapped[Boyarin] = db.relationship(foreign_keys=[sender_id])
    recipient: Mapped[Boyarin] = db.relationship(foreign_keys=[recipient_id])
