from concurrent.futures import thread
import socket
import threading
from datetime import datetime
from typing import Any, Callable, TypeVar
Shit_Game = TypeVar("Shit_Game")
Request = str

class Shit_Cache:
    def __init__(self) -> None:
        pass


class Shit_Game:
    req_set = {"PING", "LOBBY", "CARDS"}

    def __init__(
        self, 
        serv_info: tuple(str, int) = ("127.0.0.1",4242), 
        timeout: float = 10.0,
        err_log: str = "game.log"
        ) -> None:
        
        self._serv_sock: socket.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._serv_sock.settimeout(timeout)
        self._serv_sock.connect(serv_info)
        self._serv_file = None

        self._err_log_path = err_log

        self._req_handler_t: threading.Thread = threading.Thread(target=self._req_handler,args=(self,))
        self._cache: Shit_Cache = None
        self._lobbies = None

        self._req_handler_t.run()

    def send_request(self, data_parser: Callable[[Shit_Game, list[Any]], bool], req: Request, req_data = None) -> bool:
        if not req in Shit_Game.req_set:
            return False

        self._serv_send(req)
        if req_data is not None: self._serv_send(req_data)
        self._serv_send(b"\n")

        data_parser(self, self._sock_readline())
        pass

    def _load_lobby_list(self, data):
        pass

    def _load_cache(self, data):
        pass

    def _sock_readline(self) -> str:
        if self._serv_file is None:
            self._serv_file = self._serv_sock.makefile()
        return self._serv_file.readline() # may raise timeout err

    def _serv_send(self, data: bytes):
        self._serv_sock.send(data)

    def _log_err(self, errmsg: str):
        with open(self._err_log_path, "a") as logf:
            logf.write(f"[ERR] {datetime.now()}: {errmsg}")

    def _req_handler(self):
        handlers = {
            "MAIN MENU": None,
            "GIMME CARD": None,
            "TRADE NOW": None,
            "YOUR TURN": None,
            "TOP CARD": None,
            "WRITE": None
        }

        while True:
            req = self._sock_readline()
            for prefix, handler in handlers.items():
                if req.startswith(prefix):
                    self._serv_send(b"ACK\n")

                    reply = handler(req.lstrip(prefix)) # reply contains newline

                    self._serv_send(reply)
                    reply = self._sock_readline()
                    if reply != b"ACK":
                        pass
                    break
            else:
                self._log_err(f"Unknown request: {req}")