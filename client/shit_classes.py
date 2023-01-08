import os
import platform
from enum import Enum

clear = None
if platform.system() == "Linux":
    clear = lambda: os.system('clear')
else:
    clear = lambda: os.system('cls')

class Shit_Colors(Enum):
    RED = "\033[91m"
    GREEN = "\033[32m"
    YELLOW = "\033[33m"
    BLUE = "\033[94m"
    PURPLE = "\033[95m"
    CYAN = "\033[36m"
    WHITE = "\033[97m"
    GRAY = "\033[90m"
    BLACK = "\033[30m"
    END = "\033[0m"

    def colored(self, text):
        return f"{self.value}{text}{self.END.value}"

class Shit_Player:
    def __init__(self, nick) -> None:
        self.nick = nick
        self.hand = 0
        self.face_up = [0,0,0]
        self.face_down = [1,1,1]

        self.on_turn = False
        self.done = False
        self.disconnected = False

    card_names = ["2","3","4","5","6","7","8","9","10","J","Q","K","A"]
    uncard = staticmethod(lambda c: 'X' if c==0 else Shit_Player.card_names[c-2])
    down_mask = staticmethod(lambda c: 'X' if c==0 else 'O')
    gray_out = lambda self, c, otherwise: Shit_Colors.GRAY.colored(c) if self.disconnected else otherwise.colored(c)

    def print(self):
        self._resolve_states()
        suf = Shit_Colors.GREEN.colored(" (DONE)") if self.done else ""
        suf += " (DISCONNECTED)" if self.disconnected else ""
        suf = Shit_Colors.BLUE.colored(" (ON TURN)") if self.on_turn else suf
        print(Shit_Colors.RED.colored(self.nick), suf ,":")
        print(f"\tHand: {self.gray_out(self.hand, Shit_Colors.RED)}")
        print(f"\tFace Up: {self.gray_out(', '.join(map(self.uncard,self.face_up)),Shit_Colors.RED)}")
        print(f"\tFace Down: {self.gray_out(', '.join(map(self.down_mask,self.face_down)),Shit_Colors.RED)}")

    def is_done(self):
        ret = self.hand == 0 and self.face_up.count(0) == 3 and self.face_down.count(0) == 3
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

    hand_mask = staticmethod(lambda c: 'X' if c==0 else str(c))

    def print(self):
        self._resolve_states()
        self.update_play_from()
        if self.done:
            print(Shit_Colors.RED.colored(self.nick), f" {Shit_Colors.YELLOW.colored('(YOU)')} {Shit_Colors.GREEN.colored('(DONE)')}:")
            print("\tCongrats! You won!")
            return
        
        suf = Shit_Colors.BLUE.colored(' (ON TURN)') if self.on_turn else ""
        print(Shit_Colors.GREEN.colored(self.nick), f" {Shit_Colors.YELLOW.colored('(YOU)')}{suf}:")
        print(f"\tPlay from: {Shit_Colors.BLUE.colored(self.play_from.upper())}")
        self.print_hand()
        self.print_face_up()
        self.print_face_down()

    def print_hand(self):
        if self.play_from != "hand":
            print(f"\tHand:\t{ Shit_Colors.GRAY.colored( '|'.join(map(lambda n: f'{n:>2}',self.card_names)))}")
            print(f"\t\t {Shit_Colors.GRAY.colored('| '.join(map(self.hand_mask,self.hand)))}")
            return

        cards_list = [Shit_Colors.YELLOW.colored(self.card_names[c]) if c+2 and self.hand[c] != 0
                      else Shit_Colors.GRAY.colored(self.card_names[c]) 
                      for c in range(len(self.card_names))]
        num_list =   [Shit_Colors.YELLOW.colored(str(c[1])) if c[0]+2 and c[1] != 0
                      else Shit_Colors.GRAY.colored(str(c[1])) 
                      for c in enumerate(self.hand)]

        print(f"\tHand:\t{'|'.join(map(lambda n: f'{n:>11}',cards_list))}")
        print(f"\t\t {'| '.join(map(self.hand_mask,num_list))}")

    def print_face_up(self):
        if self.play_from != "face up":
            print(f"\tFace Up: {Shit_Colors.GRAY.colored(', '.join(map(self.uncard,self.face_up)))}")
            return

        f_u = [Shit_Colors.YELLOW.colored(self.card_names[c-2]) if c != 0
               else Shit_Colors.GRAY.colored('X')
               for c in self.face_up]
        print(f"\tFace Up: {', '.join(f_u)}")

    def print_face_down(self):
        if self.play_from != "face down":
            print(f"\tFace Down: {Shit_Colors.GRAY.colored(', '.join(map(self.down_mask,self.face_down)))}")
            return

        f_d = [Shit_Colors.YELLOW.colored(c) if c == 'O'
               else Shit_Colors.GRAY.colored(c)
               for c in map(self.down_mask,self.face_down)]
        print(f"\tFace Down: {', '.join(f_d)}")

    def clear(self):
        self.hand = [0 for _ in range(13)]
        self.face_up = [0,0,0]
        self.face_down = [1,1,1]

        self.on_turn = False
        self.done = False
        #self.recon = False

    def trade(self, cards):
        for i in range(3):
            if self.hand[cards[i]-2]:
                self.hand[self.face_up[i]-2] += 1
                self.hand[cards[i]-2] -= 1
                self.face_up[i] = cards[i]
            if cards[i] in self.face_up:
                c = self.face_up.index(cards[i])
                self.face_up[c] = self.face_up[i]
                self.face_up[i] = cards[i]



    def update_play_from(self) -> None:
        if self.hand.count(0) == 13:
            self.play_from = "face up"
        else: 
            self.play_from = "hand"
            return

        if self.face_up.count(0) == 3:
            self.play_from = "face down"
        else: 
            self.play_from = "face up"
            return

        if self.face_down.count(0) == 3:
            self.play_from = None
        else: 
            self.play_from = "face down"
            return

    def has_cards(self, card, count, play_from=None):
        self.update_play_from()
        if play_from is None: play_from = self.play_from

        if play_from == "hand":
            return self.hand[card-2] >= count
        elif play_from == "face up":
            return self.face_up.count(card) >= count
        elif play_from == "face down":
            return True

    def is_done(self):
        self.update_play_from()
        self.done = self.play_from is None
        return self.done

    def play_cards(self, card, cnt):
        self.update_play_from()
        if self.play_from == "hand":
            self.hand[card-2] -= cnt
        elif self.play_from == "face up":
            self.face_up = list(map(lambda c: 0 if c==card else c, self.face_up))
        elif self.play_from == "face down":
            self.face_down[card] = 0

class Shit_State(Enum):
    MAIN_MENU = 0
    LOBBY = 1
    LOBBY_OWNER = 2
    PLAYING_TRADING = 3
    PLAYING_WAITING = 4
    PLAYING_ON_TURN = 5
    PLAYING_DONE = 6
    RECON_WAIT = 7

class Shit_Game:
    cache_name = "shit_cache"
    def __init__(self, me, players_cnt=4):
        self.id = -1
        self.state = Shit_State.MAIN_MENU
        
        self.players_cnt = players_cnt
        self.me = me
        self.clear()
        
        self.serv_msg = None
        self.lobby_cnt = 0

    def clear(self):
        self.players = {}
        self.lobbies = []
        self.me.clear()
        self.top_card = 0
        self.play_deck = 0
        self.draw_pile = 0

    def add_player(self, player):
        self.players[player.nick] = player

    def __getitem__(self, key):
        return self.players[key]

    def __iter__(self):
        return iter(self.players.values())

    def _legal(self,card):
        if card not in range(2,15): return False
        if self.top_card == 18:
            return card == 8
        if card in {2, 3, 10}:
            return True
        if self.top_card == 7:
            return card <= 7
        return card >= self.top_card

    def is_legal(self, card):
        self.me.update_play_from()
        if self.me.play_from == "face down":
            return True
        return self._legal(card)

    def print(self):
        clear()
        if self.state in {Shit_State.MAIN_MENU}:
            self._print_lobbies(self.lobbies)
        elif self.state in {Shit_State.LOBBY, Shit_State.LOBBY_OWNER}:
            self._print_lobby()
        else:
            self._print_state()

    def _print_state(self): # GAME screen
        if self.top_card == 18:
            print(f"Top Card: 8 ({self.play_deck})", " ACTIVE")
        else:
            print(f"Top Card: {Shit_Player.uncard(self.top_card)} ({self.play_deck})")
        print("Draw Pile Height:", self.draw_pile)
        for player in filter(lambda p: p.nick != self.me.nick, self.players.values()):
            player.print()
            print()
        self.me.print()
        print(self.serv_msg or "")
        if(self.me.done):
            print("Congrats! You won!")

    def _print_lobbies(self, lobbies): # MM screen
        if not lobbies or len(lobbies) == 0:
            print("No lobbies found")
            print(self.serv_msg or "")
            return
        print("n.\towner\tplayers")
        for i, lobby in enumerate(lobbies, start=1):
            print(f"{i}.\t{lobby[0]}\t{lobby[1]}/{self.players_cnt}")
        self.lobby_cnt = len(lobbies)
        print(self.serv_msg or "")

    def _print_lobby(self): # LOBBY screen
        print("n.\tnick")
        for i, player in enumerate(self.players.keys(), start=1):
            print(f"{i}.\t{player}", Shit_Colors.GREEN.colored(" (OWNER)") if i==1 
                    else Shit_Colors.YELLOW.colored(" (YOU)") if player==self.me.serv_nick 
                    else "")
        print(self.serv_msg or "")

    def cache_player(self):
        with open(self.cache_name, "w") as f:
            f.write(f"{self.me.nick}\n{self.me.id}\n{self.id}")

    def del_cache(self):
        try:
            os.remove(self.cache_name)
        except FileNotFoundError:
            pass

    def get_cache():
        if not os.path.exists(Shit_Game.cache_name):
            return None
        with open(Shit_Game.cache_name, "r") as f:
            nick, id, game_id = f.read().split()
            try: 
                return (nick, int(id), int(game_id))
            except:
                return None



if __name__ == "__main__":
    me = Shit_Me("nick", 0)
    game = Shit_Game(me)
    me.hand = [0,0,1,0,1,0,1,0,0,0,1,0,0]
    me.face_up = [13,5,8]
    game.top_card = 18
    game.state = Shit_State.PLAYING_ON_TURN
    game.print()
    
    exit()