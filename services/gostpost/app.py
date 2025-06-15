import os
from flask import Flask
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate
from flask_marshmallow import Marshmallow
from pathlib import Path

app = Flask(__name__)
app.config["SQLALCHEMY_DATABASE_URI"] = os.environ.get("DB_URI", "sqlite:///app.db")
SECRET_KEY = "dev"
if Path("/secret_key").exists():
    print("Loading the secret key from file!")
    SECRET_KEY = Path("/secret_key").read_bytes().hex()
app.secret_key = SECRET_KEY

app.config['MAX_CONTENT_LENGTH'] = 2048

db = SQLAlchemy(app)
migrate = Migrate(app, db)
ma = Marshmallow(app)

from routes.auth import auth
from routes.letters import letters
from routes.secrets import secrets
from routes.boyarin import boyarin

app.register_blueprint(auth, url_prefix="/auth")
app.register_blueprint(letters, url_prefix="/letters")
app.register_blueprint(secrets, url_prefix="/secrets")
app.register_blueprint(boyarin, url_prefix="/boyarin")
