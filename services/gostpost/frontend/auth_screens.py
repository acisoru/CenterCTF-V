from textual.app import ComposeResult, Screen
from textual.widgets import Label, Input, Button
from textual.containers import Center, Middle
from textual import on


class LoginScreen(Screen):
    CSS = """
    Input {
        width: 0.5fr;
    }

    #login {
        width: 24;
    }

    #register {
        border: none;
    }
    """

    def compose(self) -> ComposeResult:
        with Middle():
            with Center():
                yield Label("gostpost")
            with Center():
                yield Input("http://localhost:5454/", placeholder="Server", id="server")
                yield Input(placeholder="Name", id="name")
                yield Input(placeholder="Passphrase", id="passphrase")
            with Center():
                yield Button("Login", id="login")
            with Center():
                yield Button("or go register", id="register")

    @on(Button.Pressed, "#login")
    def on_login_pressed(self) -> None:
        base_url = self.query_one("#server", Input).value
        name = self.query_one("#name", Input).value
        passphrase = self.query_one("#passphrase", Input).value
        self.dismiss((base_url, name, passphrase))

    @on(Button.Pressed, "#register")
    def on_register_pressed(self) -> None:
        self.dismiss()


class RegisterScreen(Screen):
    CSS = """
    Input {
        width: 0.5fr;
    }

    #register {
        width: 24;
    }

    #login {
        border: none;
    }
    """

    def compose(self) -> ComposeResult:
        with Middle():
            with Center():
                yield Label("gostpost")
            with Center():
                yield Input("http://localhost:5454/", placeholder="Server", id="server")
                yield Input(placeholder="Name", id="name")
                yield Input(placeholder="Passphrase", id="passphrase")
                yield Input(placeholder="Parent name (optional)", id="parent")
            with Center():
                yield Button("Register", id="register")
            with Center():
                yield Button("or go login", id="login")

    @on(Button.Pressed, "#register")
    def on_register_pressed(self) -> None:
        base_url = self.query_exactly_one("#server", Input).value
        name = self.query_exactly_one("#name", Input).value
        passphrase = self.query_exactly_one("#passphrase", Input).value
        parent = self.query_exactly_one("#parent", Input).value
        self.dismiss((base_url, name, passphrase, parent))

    @on(Button.Pressed, "#login")
    def on_login_pressed(self) -> None:
        self.dismiss(None)


class ErrorScreen(Screen):
    def __init__(self, message: str):
        self.message = message
        super().__init__(None, None, None)

    CSS = """
    Label {
        color: red;
        background: black;
        width: auto;
    }
    """

    def compose(self) -> ComposeResult:
        with Middle():
            with Center():
                yield Label(self.message)
            with Center():
                yield Button("Dismiss", id="dismiss")

    @on(Button.Pressed, "#dismiss")
    def on_dismiss_pressed(self) -> None:
        self.dismiss(None)
