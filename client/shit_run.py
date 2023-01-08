from shit_classes import *
from shit_handlers import *
import socket
import threading
import sys
import os
import time


class Shit_Comm:
    handlers = {
        "MAIN MENU": handle_main_menu,
        "MM CHOICE": handle_mm_choice,
        "RECON": handle_recon,
        "LOBBIES": handle_lobbies,
        "LOBBY STATE": handle_lobby_state,
        "LOBBY START": handle_lobby_start,
        "TRADE NOW": handle_trade_now,
        "ON TURN": handle_on_turn,
        "GIMME CARD": handle_gimme_card,
        "GAME STATE": handle_game_state,
        "WRITE": handle_write
    }

    _reqs_by_states = {
        Shit_State.MAIN_MENU: {"MAIN MENU", "MM CHOICE", "RECON","LOBBIES", "LOBBY STATE"},
        Shit_State.LOBBY: {"LOBBY STATE", "GAME STATE", "LOBBIES"},
        Shit_State.LOBBY_OWNER: {"LOBBY STATE", "LOBBY START", "GAME STATE", "LOBBIES"},
        Shit_State.PLAYING_TRADING: {"TRADE NOW", "GAME STATE", "LOBBIES"},
        Shit_State.PLAYING_WAITING: {"GAME STATE", "ON TURN", "TRADE NOW", "LOBBIES"},
        Shit_State.PLAYING_ON_TURN: {"GIMME CARD", "GAME STATE", "ON TURN", "LOBBIES"},
        Shit_State.PLAYING_DONE: {"GAME STATE", "ON TURN", "LOBBIES"},
        Shit_State.RECON_WAIT: {"RECON"}
    }

    def __init__(self, 
                 nick, 
                 serv_info=("127.0.0.1", 4444),
                 recon=False,
                 log="shit_log"):
        self._pl_quit = False
        self._quit_lock = threading.Lock()
        self._quit_thread = threading.Thread()
        self._quit_thread_cancel = threading.Event()

        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.settimeout(5*(CLI_TO+3.0)) # longer than server timeout
        print(f"Trying to contact server at {serv_info[0]}:{serv_info[1]}...")
        self._sock.connect(serv_info)
        nlen, pid = self.handlers["MAIN MENU"](self._sock, nick)

        #self.serv_nick = nick[:nlen]
        #print(pid, self.serv_nick)

        self.game = Shit_Game(Shit_Me(nick, pid), 4)
        self.game.me.serv_nick = nick[:nlen]
        self.game.me.recon = recon
        self._quit_thread_ctrl(True)

    def __del__(self):
        #print("Closing socket")
        self._quit_thread_ctrl(False)
        try:
            self._sock.close()
        except:
            pass

    def end(self):
        self.__del__()

    def quit(self, val=None):
        with self._quit_lock:
            if val is None:
                return self._pl_quit
            #print("Setting quit to", val)
            self._pl_quit = val
            return val

    def _quit_thread_task(self):
        while True:
            self.game.print()
            try:
                inp, to = timedInput("", 1, False)
            except Exception:
                print("Timed input error")
                return
            else:
                if not to and inp.startswith("q"):
                    self.quit(True)
            
            if self._quit_thread_cancel.is_set():
                #print("Quit thread cancelled")
                return

    def _quit_thread_ctrl(self, on=True):
        if on:
            if self._quit_thread.is_alive():
                #print("Quit thread already running")
                return
            self._quit_thread_cancel.clear()
            self._quit_thread = threading.Thread(target=Shit_Comm._quit_thread_task, args=(self,), daemon=True, name="Quit Thread" )
            self._quit_thread.start()
        else:
            if not self._quit_thread.is_alive():
                return
            self._quit_thread_cancel.set()
            #print("Waiting for quit thread to finish...")
            self._quit_thread.join()

    def sendall(self, msg):
        self._sock.sendall((msg+"\x0A").encode())

    SHIT_PATIENCE = 2

    def comm_loop(self, blackout=False):
        if blackout:
            import random as rnd
        inp = ""
        err_msg = ""
        ret = None
        shit_patience = self.SHIT_PATIENCE
        def handle_quit():
            self.sendall("QUIT")
            self.game.del_cache()
            if(self.game.state == Shit_State.MAIN_MENU):
                return True
            self.quit(False)
            return False

        while shit_patience > 0:
            self._quit_thread_ctrl(True)

            with self._sock.makefile("r") as f:
                inp = f.readline()
            if not inp:
                shit_patience -= 1
                continue
            inp = inp.rstrip("\x0A").split('^')
            #print(inp)

            if inp[0] not in self.handlers.keys():
                err_msg = f"Unknown command: {inp[0]}"
                shit_patience -= 1
                continue

            if blackout and rnd.random() < 0.1:
                raise Exception("Blackout")
            
            self._quit_thread_ctrl(False)

            if self.quit():
                if handle_quit(): return
                shit_patience = self.SHIT_PATIENCE
                continue
            self.sendall("ACKN")

            #if inp[0] not in self._reqs_by_states[self.game.state]:
            #    err_msg = f"Server desynchronization."
            #    shit_patience -= 1
            #    continue

            try:
                ret = self.handlers[inp[0]](self.game, inp[1:])
            except Exception as e:
                #self.game.serv_msg = "Error: " + str(e)
                err_msg = "Error: " + str(e)
                self.game.print()
                shit_patience -= 1
            else:
                if ret is not None:
                    if ret[0] == "QUIT":
                        if handle_quit(): return
                        shit_patience = self.SHIT_PATIENCE
                        continue
                    self.sendall("^".join(ret))
                shit_patience = self.SHIT_PATIENCE

        raise Exception(err_msg)


def connect(nick, serv_info, recon):
    conn_patience = 5
    while(conn_patience > 0):
        try:
            recon = os.path.exists(Shit_Game.cache_name)
            gm = Shit_Comm(nick, serv_info , recon)
        except Exception as e:
            clear()
            print("Connection error, retry:", 5-conn_patience)
            time.sleep(1)
            recon = True
            conn_patience -= 1
            continue
        else:
            return gm
    return None

def communicate(gm, blackout):
    comm_patience = 5
    while comm_patience > 0:
        try:
            gm.comm_loop(blackout)
        except Exception as e:
            print(e)
            time.sleep(1)
            gm.end()
            comm_patience -= 1
            continue
        else:
            return True
    return False

def main():

    if platform.system() == "Windows":
        os.system('color')
    # handle args - ip and port
    ip, port = "127.0.0.1", 4444
    if len(sys.argv) < 3 or len(sys.argv) > 4:
        print("Usage: python", sys.argv[0], "<ip> <port>")
        print("Defaulting to localhost:4444")
        time.sleep(1)
    else:
        ip= sys.argv[1]
        try:
            port = int(sys.argv[2])
        except:
            print("Invalid port")
            exit(1)
    
    clear()
    # handle reconnect
    recon = False
    nick = None
    if os.path.exists(Shit_Game.cache_name):
        try:
            nick = Shit_Game.get_cache()[0]
            y = input(f"Found cache. Do you want to reconnect as {nick}? (y/n) ")
            recon = (y.lower() == "y")
            #print("Reconnect:", recon)
            if not recon: os.remove(Shit_Game.cache_name)
        except ValueError:
            os.remove(Shit_Game.cache_name)

    if not recon:
        #ask for nick, max len = 12
        print()
        nick = input("Enter your nick: ")
        while len(nick) > 11 and len(nick) < 3:
            clear()
            print("Nick too long or short. Max 11, min 3.")
            nick = input("Enter your nick: ")

    blackout = len(sys.argv) == 4 and sys.argv[3] == "BL"
    while 1:
        gm = connect(nick, (ip, port), recon)
        if gm is None:
            print("Connection failed")
            break
        if communicate(gm, blackout):
            break
    gm.end()

    if platform.system() == "Linux":
        os.system("reset")

    print("Bye")
    exit()

if __name__ == "__main__":
    main()