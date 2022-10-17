import socket
from typing import Any, Callable, TypeVar
Shit_Game = TypeVar("Shit_Game")
Request = str

class Shit_Cache:
    def __init__(self) -> None:
        pass


class Shit_Game:
    req_set = {"PING", "LOBBY", "CARDS"}

    def __init__(self, serv_info: tuple(str, int) = ("127.0.0.1",4242)) -> None:
        
        self._serv_sock: socket.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._serv_sock.connect(serv_info)
        pass

    def send_request(self, data_parser: Callable[[Shit_Game, list[Any]], bool], req: Request) -> bool:
        if not req in Shit_Game.req_set:
            return False

        self._serv_sock.send(req)
        pass

    def _load_cache(self):
        pass

    def __serv_send(self):
        pass