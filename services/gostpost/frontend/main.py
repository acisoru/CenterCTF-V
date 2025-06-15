from textual.app import App, ComposeResult
from textual.widgets import LoadingIndicator
from textual import work

from auth_screens import ErrorScreen, LoginScreen, RegisterScreen
from letters_screen import LettersScreen
from api_client import ApiClient


class GostPostApp(App):
    SCREENS = {"login": LoginScreen, "register": RegisterScreen}

    def compose(self) -> ComposeResult:
        yield LoadingIndicator()

    def on_mount(self) -> None:
        self.show_login()

    def on_dismiss_login(self, result):
        if isinstance(result, tuple):
            self.login(result)
        elif result is None:
            self.show_register()

    def show_login(self) -> None:
        self.push_screen("login", self.on_dismiss_login)

    def on_dismiss_register(self, result):
        if isinstance(result, tuple):
            self.register(result)
        elif result is None:
            self.show_login()

    def show_register(self) -> None:
        self.push_screen("register", self.on_dismiss_register)

    @work(exclusive=True)
    async def login(self, tup) -> None:
        result = await ApiClient.login(*tup)
        if isinstance(result, str):
            self.push_screen(ErrorScreen(result), lambda _: self.show_login())
        else:
            self.title = "Letters"
            self.sub_title = tup[1]
            self.push_screen(LettersScreen(result))

    @work(exclusive=True)
    async def register(self, tup) -> None:
        result = await ApiClient.register(*tup)
        if isinstance(result, str):
            self.push_screen(ErrorScreen(result), lambda _: self.show_register())
        else:
            self.title = "Letters"
            self.sub_title = tup[1]
            self.push_screen(LettersScreen(result))


if __name__ == "__main__":
    app = GostPostApp()
    app.run()
