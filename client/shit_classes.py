import os
import platform
from enum import Enum

clear = None
if platform.system() == "Linux":
    clear = lambda: os.system('clear')
else:
    clear = lambda: os.system('cls')

class Shit_Player:
    def __init__(self, nick) -> None:
        self.nick = nick
        self.hand = 0
        self.face_up = (0,0,0)
        self.face_down = (1,1,1)

        self.on_turn = False
        self.done = False
        self.disconnected = False

    uncard = staticmethod(lambda c: 'X' if c==0 else str(c))
    down_mask = staticmethod(lambda c: 'X' if c==0 else 'O')

    def print(self):
        self._resolve_states()
        suf = " (DONE)" if self.done else ""
        suf += " (DISCONNECTED)" if self.disconnected else ""
        suf = " (ON TURN)" if self.on_turn else suf
        print(self.nick, suf ,":")
        print(f"\tHand: {self.hand}")
        print(f"\tFace Up: {', '.join(map(self.uncard,self.face_up))}")
        print(f"\tFace Down: {', '.join(map(self.down_mask,self.face_down))}")

    def is_done(self):
        ret = self.hand == 0 and self.face_up == (0,0,0) and self.face_down == (0,0,0)
        self.done = ret
        return ret

    def _resolve_states(self):
        if self.on_turn:
            self.done = False
            self.disconnected = False
            return
        self.is_done()


class Shit_Me(Shit_Player):
    def __init__(self,nick,id) -> None:
        super().__init__(nick)
        self.serv_nick = None
        self.hand = [0 for _ in range(13)]
        self.play_from = "hand"
        self.id = id

        self.recon = False

    card_names = ["2","3","4","5","6","7","8","9","10","J","Q","K","A"]

    def print(self):
        self._resolve_states()
        if self.done:
            print(self.nick, " (YOU) (DONE):")
            print("\tCongrats! You won!")
            return
        suf = " (DONE)" if self.done else ""
        suf = " (ON TURN)" if self.on_turn else suf
        print(self.nick, f" (YOU){suf}:")
        print(f"\tHand:\t{'|'.join(map(lambda n: f'{n:>2}',self.card_names))}")
        print(f"\t\t {'| '.join(map(self.uncard,self.hand))}")
        print(f"\tFace Up: {', '.join(map(self.uncard,self.face_up))}")
        print(f"\tFace Down: {', '.join(map(self.down_mask,self.face_down))}")

    def clear(self):
        self.hand = [0 for _ in range(13)]
        self.face_up = (0,0,0)
        self.face_down = (1,1,1)

        self.on_turn = False
        self.done = False
        self.recon = False

    def update_play_from(self) -> None:
        if self.hand.count(0) == 13:
            self.play_from = "face_up"
        else: 
            self.play_from = "hand"
            return

        if self.face_up.count(0) == 3:
            self.play_from = "face_down"
        else: 
            self.play_from = "face_up"
            return

        if self.face_down.count(0) == 3:
            self.play_from = None
        else: 
            self.play_from = "face_down"
            return

    def has_cards(self, card, count):
        self.update_play_from()
        if self.play_from == "hand":
            return self.hand[card-2] >= count
        elif self.play_from == "face_up":
            return self.face_up.count(card) >= count
        elif self.play_from == "face_down":
            return True

    def is_done(self):
        self.update_play_from()
        self.done = self.play_from is None
        return self.done

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

class Shit_State(Enum):
    MAIN_MENU = 0
    LOBBY = 1
    LOBBY_OWNER = 2
    PLAYING_TRADING = 3
    PLAYING_WAITING = 4
    PLAYING_ON_TURN = 5
    PLAYING_DONE = 6

class Shit_Game:
    cache_name = "shit_cache"
    def __init__(self, me, players_cnt=4, packs_cnt=1):
        self.id = -1
        self.state = Shit_State.MAIN_MENU
        
        self.players_cnt = players_cnt
        self.packs_cnt = packs_cnt
        self.clear()
        self.me = me

        if self.draw_pile < 0:
            raise ValueError("Not enough cards for players")

        self.serv_msg = None
        self.lobby_cnt = 0

    def clear(self):
        self.players = {}
        self.me.clear()
        self.top_card = 0
        self.play_deck = 0
        self.draw_pile = self.packs_cnt*52 - self.players_cnt*9

    def add_player(self, player):
        self.players[player.nick] = player

    def __getitem__(self, key):
        return self.players[key]

    def __iter__(self):
        return iter(self.players.values())

    def is_legal(self, card):
        self.me.update_play_from()
        if self.me.play_from == "face_down":
            return True
        if self.top_card == 18:
            return card == 8
        if card in {2, 3, 10}:
            return True
        if self.top_card == 7:
            return card <= 7
        return card >= self.top_card

    def print(self,lobbies=None):
        clear()
        if self.state in {Shit_State.MAIN_MENU}:
            self._print_lobbies(lobbies)
        elif self.state in {Shit_State.LOBBY, Shit_State.LOBBY_OWNER}:
            self._print_lobby()
        else:
            self._print_state()

    def _print_state(self): # GAME screen
        if self.top_card == 18:
            print(f"Top Card: 8 ({self.play_deck})", "ACTIVE")
        else:
            print(f"Top Card: {Shit_Player.uncard(self.top_card)} ({self.play_deck})")
        print("Draw Pile Height:", self.draw_pile)
        for player in filter(lambda p: p.nick != self.me.nick, self.players.values()):
            player.print()
        self.me.print()
        print(self.serv_msg or "")

    def _print_lobbies(self, lobbies): # MM screen
        if lobbies is None:
            return
        print("n.\towner\tplayers")
        for i, lobby in enumerate(lobbies, start=1):
            print(f"{i}.\t{lobby[0]}\t{lobby[1]}/{self.players_cnt}")
        self.lobby_cnt = len(lobbies)
        print(self.serv_msg or "")

    def _print_lobby(self): # LOBBY screen
        print("n.\tnick")
        for i, player in enumerate(self.players.keys(), start=1):
            print(f"{i}.\t{player}", " (OWNER)" if i==0 else " (YOU)" if player==self.me.serv_nick else "")
        print(self.serv_msg or "")

    def cache_player(self):
        with open(self.cache_name, "w") as f:
            f.write(f"""
            {self.me.nick}
            {self.me.id}
            {self.id}
            """)

    def del_cache(self):
        try:
            os.remove(self.cache_name)
        except FileNotFoundError:
            pass

    def get_cache():
        with open(Shit_Game.cache_name, "r") as f:
            nick, id, game_id = f.read().split()
            try: 
                return (nick, int(id), int(game_id))
            except:
                raise ValueError("Invalid cache file")



if __name__ == "__main__":
    exit()