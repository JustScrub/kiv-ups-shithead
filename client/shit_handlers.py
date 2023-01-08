import socket
from pytimedinput import timedInput
from datetime import datetime, timedelta
import shit_classes as sc

CLI_TO = 21
ack_send = ["ACKN", "QUIT"]

def sendall(sock: socket, msg: str):
    sock.sendall((msg+"\x0A").encode())

def inputto(prompt, till):
    timeout = (till - datetime.now()).seconds
    if timeout < 1:
        return "", True
    return timedInput(prompt, timeout, resetOnInput=False, allowCharacters="yYnN1234567890JQKAjka q")

def handle_main_menu(sock: socket, nick):
    inp = ""
    with sock.makefile("r") as f:
        inp = f.readline()
    #print(inp)
    inp = inp.split('^')

    if inp[0] != "MAIN MENU":
        sock.close()
        raise ValueError("Invalid server response")
    
    sendall(sock, ack_send[0])
    sendall(sock, f"NICK^{nick}")
    return int(inp[2]), int(inp[1])

def handle_mm_choice( game, inp):
    if game.me.recon:
        cache = sc.Shit_Game.get_cache()
        if not cache:
            game.serv_msg = "Could not read cache"
            game.print()
            #exit(0)
            game.me.recon = False
            game.del_cache()
            return ["IGNR"]

        #game.me.recon = False
        game.state = sc.Shit_State.RECON_WAIT
        return ["RECON", *tuple(map(str, cache))]
    #game.serv_msg = ""

    till = datetime.now() + timedelta(seconds=CLI_TO)
    rcache = sc.Shit_Game.get_cache()
    while 1:
        if rcache: 
            game.serv_msg = "Cache found. Would you like to reconnect? (Y/y) "
        game.print()
        ret, to = inputto("Lobby number to connect to or N/n for new lobby: ", till)
        if to:
            return ["IGNR"]
        if ret == "q":
            return ["QUIT"]

        if ret.upper() == "N":
            return ["LOBBY", "0"]
        else:
            try:
                ret = int(ret)
            except ValueError:
                return ["IGNR"]
            if ret < 1 or ret > game.lobby_cnt:
                game.serv_msg = "Invalid lobby number"
                continue
            return ["LOBBY", str(ret)]

def handle_recon( game, inp):
    if not inp[0] in ["I", "F", "R"]:
        raise ValueError(f"Invalid recon data: {inp[0]}")
    if inp[0] == "I":
        game.serv_msg +="\nServer rejected cache. You may try again (after a while)."
        game.print()
        game.me.recon = False
        game.state = sc.Shit_State.MAIN_MENU

    elif inp[0] == "F":
        game.serv_msg = "Old game finished. Choose new lobby."
        game.print()
        game.del_cache()
        game.me.recon = False
        game.state = sc.Shit_State.MAIN_MENU

    elif inp[0] == "R":
        game.state = sc.Shit_State.PLAYING_WAITING
        game.serv_msg = "Reconnected to game."
        game.print()
        game.me.recon = False
        game.me.serv_nick, game.me.id, game.id = inp[1], int(inp[2]), int(inp[3])
        game.cache_player()

    return None

def handle_write( game, inp):
    game.serv_msg = inp[0]
    game.print()
    return None

def handle_on_turn( game, inp):
    if inp[0] == game.me.serv_nick:
        game.state = sc.Shit_State.PLAYING_ON_TURN
        game.me.on_turn = True
        for pl in game:
            pl.on_turn = False

    else:
        game.state = sc.Shit_State.PLAYING_WAITING
        game.me.on_turn = False
        for pl in game:
            pl.on_turn = pl.nick == inp[0]

    game.print()
    return None

def handle_lobbies( game, inp):
    game.clear()
    if game.state.name.startswith("PLAYING"):
        game.del_cache()
        game.serv_msg = ""
    game.state = sc.Shit_State.MAIN_MENU
    game.lobbies = [x.split("`") for x in inp]
    #print([x.split("`") for x in inp])
    game.print()
    return None

def handle_lobby_state( game, inp):
    game.state = sc.Shit_State.LOBBY if game.state != sc.Shit_State.LOBBY_OWNER else sc.Shit_State.LOBBY_OWNER
    game.id = int(inp[0])
    game.players = {x: None for x in inp[1:]}
    game.serv_msg = ""
    game.print()
    #try:
    #    game.players.pop(game.me.serv_nick)
    #except KeyError:
    #    raise ValueError("Self not in player list")
    return None

mask_str2tuple = lambda y: list(ord(x)-0x30 for x in y)
def handle_game_state( game, inp):
    if not game.state.name.startswith("PLAYING"):
        game.cache_player()
        game.clear()
        #game.me.recon = False
    game.state = sc.Shit_State.PLAYING_WAITING if game.state != sc.Shit_State.PLAYING_DONE else sc.Shit_State.PLAYING_DONE
    plinfos = inp[:-1]
    plinfos = {x.split('`')[0]: x.split('`')[1:] for x in plinfos}
    #print(plinfos)
    if game.me.serv_nick not in plinfos.keys():
        raise ValueError("Self not in player list")
    game.me.face_up = mask_str2tuple(plinfos[game.me.serv_nick][1])
    game.me.face_down = mask_str2tuple(plinfos[game.me.serv_nick][2])
    game.me.hand = list(map(lambda x: int(x), plinfos[game.me.serv_nick][0].split(',')))
    game.me.update_play_from()
    if not game.me.play_from: 
        game.state = sc.Shit_State.PLAYING_DONE
    plinfos.pop(game.me.serv_nick)

    for pl in game:
        if not pl.nick in plinfos.keys():
            pl.disconnected = True
            continue
        pl.disconnected = False
        pl.hand = plinfos[pl.nick][0]
        pl.face_up = mask_str2tuple(plinfos[pl.nick][1])
        pl.face_down =  mask_str2tuple(plinfos[pl.nick][2])
        plinfos.pop(pl.nick)

    for plnick in plinfos.keys():
        pl = sc.Shit_Player(plnick)
        pl.hand = plinfos[plnick][0]
        pl.face_up = mask_str2tuple(plinfos[plnick][1])
        pl.face_down =  mask_str2tuple(plinfos[plnick][2])
        game.add_player(pl)

    infos = inp[-1].split("`")
    game.top_card = int(infos[0])
    game.play_deck = int(infos[1])
    game.draw_pile = int(infos[2])

    game.print()

def handle_lobby_start( game, inp):
    game.state = sc.Shit_State.LOBBY_OWNER
    till = datetime.now() + timedelta(seconds=CLI_TO)
    ret = ""
    if len(game.players)+1 < 2:
        return ["NO"]
    while ret not in ["y", "n", "Y", "N"]:
        game.print()
        ret, to = inputto("Start game? (y/n): ", till)
        if to:
            return ["NO"]
            #return ["IGNR"]
        if ret == "q":
            return ["QUIT"]
    return ["YES"] if ret in ["y", "Y"] else ["NO"]

_card_names = {
    "2":  chr(0x32),
    "3":  chr(0x33),
    "4":  chr(0x34),
    "5":  chr(0x35),
    "6":  chr(0x36),
    "7":  chr(0x37),
    "8":  chr(0x38),
    "9":  chr(0x39),
    "10": chr(0x3a), # :
    "J":  chr(0x3b), # ;
    "Q":  chr(0x3c), # <
    "K":  chr(0x3d), # =
    "A":  chr(0x3e), # >
}
_card_vals = {k: ord(v)-0x30 for k, v in _card_names.items()}

def handle_trade_now( game, inp):
    game.state = sc.Shit_State.PLAYING_TRADING
    till = datetime.now() + timedelta(seconds=CLI_TO)
    while 1:
        game.print()
        ret, to = inputto("Trade cards (input 3 cards you want to have in face-up deck): ", till)
        if to:
            game.state = sc.Shit_State.PLAYING_WAITING
            return ["IGNR"]
        if ret == "q":
            game.state = sc.Shit_State.PLAYING_WAITING
            return ["QUIT"]

        ret = ret.split()
        if len(ret) != 3:
            game.serv_msg = "Invalid input"
            continue
        if not all([x in _card_names.keys() for x in ret]):
            game.serv_msg = "Invalid card"
            continue
        if not all(map(lambda c: game.me.has_cards(c,1,"hand") or game.me.has_cards(c,1,"face up"), map(lambda c: _card_vals[c],ret))):
            game.serv_msg = "You don't have all of these cards"
            continue

        game.me.trade([_card_vals[x] for x in ret])
        game.state = sc.Shit_State.PLAYING_WAITING
        game.serv_msg = ["TRADE", "".join([_card_names[x] for x in ret])]
        return ["TRADE", "".join([_card_names[x] for x in ret])]

def handle_gimme_card( game, inp):
    till = datetime.now() + timedelta(seconds=CLI_TO)
    game.me.update_play_from()
    while game.me.play_from in {"hand", "face up"}:
        game.print()
        ret, to = inputto("Gimme card (card + count): ", till)
        if to:
            return ["IGNR"]
        if ret == "q":
            return ["QUIT"]
        ret = ret.split()
        if len(ret) != 2:
            game.serv_msg = "Invalid input"
            game.print()
            continue
        ret[0] = ret[0].upper()
        if ret[0] not in _card_names.keys():
            game.serv_msg = "Invalid card name"
            game.print()
            continue
        try:
            if int(ret[1]) < 1: raise ValueError
        except ValueError:
            game.serv_msg = "Amount must be a positive number"
            game.print()
            continue
        # TODO: check if player has enough cards, legality of move and actually do it
        card, cnt = _card_vals[ret[0]], int(ret[1])
        if not game.is_legal(card):
            game.serv_msg = "You can't play this card"
            game.print()
            continue
        if not game.me.has_cards(card, cnt):
            game.serv_msg = "You don't have enough cards"
            game.print()
            continue
        game.me.play_cards(card, cnt)

        ret[0] = _card_names[ret[0]]
        return ["CARD", *ret]

    while game.me.play_from == "face down":
        game.print()
        ret, to = inputto("Gimme card (position of face-down): ", till)
        if to:
            return ["IGNR"]
        if ret == "q":
            return ["QUIT"]
        ret = ret.split()
        if len(ret) != 1:
            game.serv_msg = "Invalid input"
            game.print()
            continue
        try:
            ret[0] = int(ret[0])
        except ValueError:
            game.serv_msg = "Position must be a number"
            game.print()
            continue
        if ret[0] not in {1, 2, 3}:
            game.serv_msg = "Allowed positions are 1, 2 and 3"
            game.print()
            continue
        if game.me.face_down[ret[0]-1] == 0:
            game.serv_msg = "This position is empty"
            game.print()
            continue

        return ["CARD", str(ret[0]-1), "1"]

if __name__ == "__main__":
    game = sc.Shit_Game(sc.Shit_Me("test",0))
    game.me.hand = [0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1]
    game.me.face_up = [2,3,4]
    game.state = sc.Shit_State.PLAYING_WAITING
    i = handle_trade_now(game, None)
    game.print()
    print("^".join(i))