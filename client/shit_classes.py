from concurrent.futures import thread
import socket
import threading
from datetime import datetime
from typing import Any, Callable, TypeVar
Shit_Game = TypeVar("Shit_Game")
Request = str

class Shit_Player:
    def __init__(self, nick) -> None:
        self.nick = nick
        self.hand = 0
        self.face_up = (0,0,0)
        self.face_down = (1,1,1)

        self.uncard = lambda c: 'X' if c==0 else str(c)
        self.down_mask = lambda c: 'X' if c==0 else 'O'

    def print(self):
        print(self.nick, ":")
        print(f"\tHand: {self.hand}")
        print(f"\tFace Up: {', '.join(map(self.uncard,self.face_up))}")
        print(f"\tFace Down: {', '.join(map(self.down_mask,self.face_down))}")

class Shit_Me(Shit_Player):
    def __init__(self,nick) -> None:
        super().__init__(nick)
        self.hand = [0 for i in range(13)]
        self.play_from = "hand"

    card_names = ["2","3","4","5","6","7","8","9","10","J","Q","K","A"]

    def print(self):
        print(self.nick, "(YOU):")
        print(f"\tHand:\t{'|'.join(map(lambda n: f'{n:>2}',self.card_names))}")
        print(f"\t\t {'| '.join(map(self.uncard,self.hand))}")
        print(f"\tFace Up: {', '.join(map(self.uncard,self.face_up))}")
        print(f"\tFace Down: {', '.join(map(self.down_mask,self.face_down))}")

    def update_play_from(self) -> None:
        if self.hand.count(0) == 13:
            self.play_from = "face_up"
        if self.face_up.count(0) == 3:
            self.play_from = "face_down"
        if self.face_down.count(0) == 3:
            self.play_from = None

    def play_cards(self) -> int:
        card, amount = 0,0
        print("Play from:",self.play_from)

        while self.play_from != "face_down":
            inp = input("Card, Amount: ")
            if inp == "quit":
                exit(0)
            inp = inp.split()
            if len(inp) != 2:
                print("Invalid input")
                continue
            card, amount = inp
            if card not in self.card_names:
                print("Invalid card")
                continue
            card = self.card_names.index(card)+2
            try:
                amount = int(amount)
            except:
                print("Amount must be an integer")
                continue
            print(card, amount)

            if self.play_from == "hand" and \
                self.hand[card-2] >= amount:
                self.hand[card-2] -= amount
                break

            elif self.play_from == "face_up" and \
                self.face_up.count(card) >= amount:
                self.face_up = list(map(lambda c: 0 if c==card else c, self.face_up))
                break

            else:
                print("You don't have that many cards!")
        
        while self.play_from == "face_down":
            inp = input("Card order: ")
            try:
                card_idx = int(inp)
            except:
                print("Card order must be an integer")
                continue

            if card_idx not in range(1,4):
                print("Invalid order")
                continue
            if self.face_down[card_idx-1] != 0:
                self.face_down[card_idx-1] = 0
                card, amount = self.face_down[card_idx-1], 1
                break
            else:
                print("You don't have that card!")

        self.update_play_from()
        return card,amount

class Shit_Game:
    def __init__(self, players_cnt, packs_cnt=1):
        self.players = {}
        self.me = None
        self.top_card = 0
        self.draw_pile = packs_cnt*13*4 - players_cnt*9
        if self.draw_pile < 0:
            raise ValueError("Not enough cards for players")

    def add_player(self, player):
        self.players[player.nick] = player
        if type(player) == Shit_Me:
            self.me = player

    def rm_player(self, nick):
        del self.players[nick]

    def __getitem__(self, key):
        return self.players[key]

    def print_state(self):
        print("Top Card:", self.top_card)
        print("Draw Pile Height:", self.draw_pile)
        for player in filter(lambda p: p.nick != self.me.nick, self.players.values()):
            player.print()
        self.me.print()



if __name__ == "__main__":
    import random
    import os
    clear = lambda: os.system('clear')

#dict of players, key is nick
    players = {f"player{i}": Shit_Player(nick=f"player{i}") for i in range(1,5)}
    me = Shit_Me(nick="me")

    me.hand = list(0 for i in range(13))
    me.hand[0] = 1
    me.face_up = list(random.randint(2,14) for i in range(3))
    me.face_down = [1,1,1]

    game = Shit_Game(players_cnt=5)
    game.players = players
    game.add_player(me)

    game.print_state()

    while True:
        clear()
        game.print_state()
        print("Your turn!")
        game.me.play_cards()

    exit()



class Shit_Cache_old:
    def __init__(self) -> None:
        pass


class Shit_Game_old:
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
        self._cache: Shit_Cache_old = None
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