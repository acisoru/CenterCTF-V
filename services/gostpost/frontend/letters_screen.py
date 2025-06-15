from textual import work, on
from textual.app import ComposeResult, Screen
from textual.widgets import (
    Label,
    ListView,
    ListItem,
    Footer,
    Header,
    LoadingIndicator,
    Input,
    Button,
    TextArea,
)
from textual.containers import Horizontal, Middle, Center
from textual.screen import ModalScreen
from api_client import ApiClient, Letter


class ErrorScreen(ModalScreen):
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

    def on_key(self, _) -> None:
        self.dismiss(None)


class LetterScreen(ModalScreen):
    def __init__(self, letter: Letter):
        self.letter = letter
        super().__init__(None, None, None)

    CSS = """
    Label {
        padding: 0 2;
        border: black;
        height: auto;
    }
    """

    def compose(self) -> ComposeResult:
        with Middle():
            with Center():
                yield Label(f"From: {self.letter.sender.name}")
            with Center():
                yield Label(f"To: {self.letter.recipient.name}")
            with Center():
                yield Label(f"{self.letter.content}")

    def on_key(self, _) -> None:
        self.dismiss(None)


class SendLetterScreen(ModalScreen):
    def __init__(self, client: ApiClient, initial_recipient: str = ""):
        self.client = client
        self.initial_recipient = initial_recipient
        super().__init__(None, None, None)

    CSS = """
    Input {
        width: 0.5fr;
    }

    #send {
        width: 24;
    }

    #send_encrypted {
        border: none;
    }

    TextArea {
        width: 0.5fr;
        height: auto;
    }
    """

    BINDINGS = [("escape", "dismiss", "Cancel")]

    def compose(self) -> ComposeResult:
        with Middle():
            with Center():
                yield Input(
                    self.initial_recipient, placeholder="Reciever name", id="reciever"
                )
            with Center():
                yield TextArea(tooltip="Content", id="content")
            with Center():
                yield Button("Send", id="send")
            with Center():
                yield Button("Send encrypted", id="send_encrypted")

    @on(Button.Pressed)
    def on_send_pressed(self, pressed) -> None:
        reciever = self.query_exactly_one("#reciever", Input).value
        content = self.query_exactly_one("#content", TextArea).text
        self.mount(LoadingIndicator())
        do_encrypt = pressed.button.id == "send_encrypted"
        self.send_letter(reciever, content, do_encrypt)

    @work(exclusive=True)
    async def send_letter(self, reciever, content, do_encrypt):
        self.mount(LoadingIndicator())
        result = await self.client.send_letter(reciever, content, do_encrypt)
        if not result:
            self.app.push_screen(ErrorScreen("Error"), self.dismiss)
            return
        self.dismiss()

    @on(Button.Pressed, "#register")
    def on_register_pressed(self) -> None:
        self.dismiss()


class LetterListItem(ListItem):
    def __init__(
        self,
        letter: Letter,
        name=None,
        id=None,
        classes=None,
        disabled=False,
        markup=True,
    ):
        self.letter = letter
        super().__init__(
            Horizontal(
                Label(f"From: {letter.sender.name}"),
                Label(f"To: {letter.recipient.name}"),
                Label(f"{letter.content}"),
            ),
            name=name,
            id=id,
            classes=classes,
            disabled=disabled,
            markup=markup,
        )


class LettersScreen(Screen):
    CSS = """
    Screen {
        align: center middle;
    }

    ListView {
        width: auto;
        height: auto;
        margin: 2 2;
    }

    Label {
        padding: 0 2;
        height: 3;
        border: black;
    }

    LetterListItem {
        height: 3;
        width: auto;
    }
    """

    BINDINGS = [
        ("escape", "logout", "Logout"),
        ("f5", "reload_letters", "Refresh"),
        ("s", "send_letter", "Send letter"),
        ("d", "details_letter", "See details"),
        ("r", "reply_letter", "Reply"),
    ]

    def __init__(self, client: ApiClient):
        self.client = client
        self.letters_worker = None
        super().__init__(None, None, None)

    def compose(self) -> ComposeResult:
        yield Header()
        yield ListView(id="list")
        yield Footer()

    def on_mount(self):
        self.set_interval(5, self.action_reload_letters)
        self.action_reload_letters()

    def action_reload_letters(self) -> None:
        if self.letters_worker:
            return
        self.letters_worker = self._worker_reload_letters()

    @work
    async def _worker_reload_letters(self) -> None:
        footer = self.query_exactly_one("Footer", Footer)
        footer.mount(LoadingIndicator())
        letters, changed = await self.client.get_letters()
        if changed:
            letters = [LetterListItem(letter) for letter in letters]
            listview = self.query_exactly_one("#list", ListView)
            listview.clear()
            listview.extend(letters)
            listview.refresh(layout=True)
        footer.remove_children("LoadingIndicator")
        self.letters_worker = None

    def action_send_letter(self, initial_recipient="") -> None:
        self.app.push_screen(
            SendLetterScreen(self.client, initial_recipient),
            lambda _: self.action_reload_letters(),
        )

    @on(ListView.Highlighted)
    def on_highlight(self):
        self.refresh_bindings()

    def check_action(self, action: str, parameters: tuple[object, ...]) -> bool | None:
        if action == "details_letter" or action == "reply_letter":
            return self.query_exactly_one("ListView").highlighted_child is not None
        return True

    def action_reply_letter(self) -> None:
        letter = self.query_exactly_one("ListView").highlighted_child.letter
        self.app.push_screen(
            SendLetterScreen(self.client, letter.sender.name),
            lambda _: self.action_reload_letters(),
        )

    def action_details_letter(self) -> None:
        letter = self.query_exactly_one("ListView").highlighted_child.letter
        self.app.push_screen(
            LetterScreen(letter), lambda _: self.action_reload_letters()
        )

    def action_logout(self) -> None:
        self.dismiss()
        self.app.show_login()

    def on_unmount(self):
        self.client.close()
