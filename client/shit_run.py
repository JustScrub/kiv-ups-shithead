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
        self._sock.settimeout(CLI_TO+1.0)
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

    def comm_loop(self):
        inp = ""
        ret = None
        shit_patience = 5 
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
                print("Unknown command:", inp[0])
                shit_patience -= 1
                continue
            
            self._quit_thread_ctrl(False)

            if self.quit():
                if handle_quit(): return
                shit_patience = 5
                continue
            self.sendall("ACKN")

            try:
                ret = self.handlers[inp[0]](self.game, inp[1:])
            except Exception as e:
                print(e)
                shit_patience -= 1
            else:
                if ret is not None:
                    if ret[0] == "QUIT":
                        if handle_quit(): return
                        shit_patience = 5
                        continue
                    self.sendall("^".join(ret))
                shit_patience = 5

        raise Exception("Server is not responding")


def main():
    # handle args - ip and port
    ip, port = "127.0.0.1", 4444
    if len(sys.argv) < 3:
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
            if not recon: os.remove(Shit_Game.cache_name)
        except ValueError:
            os.remove(Shit_Game.cache_name)

    if not recon:
        #ask for nick, max len = 12
        print()
        nick = input("Enter your nick: ")
        while len(nick) > 12:
            clear()
            print("Nick too long")
            nick = input("Enter your nick: ")

    try:
        gm = Shit_Comm(nick, (ip, port), recon)
    except:
        clear()
        print("Connection error")
        exit(1)

    try:
        gm.comm_loop()
    except Exception as e:
        print(e)
        time.sleep(1)
    gm.end()
    del gm
    if platform.system() == "Linux":
        os.system("reset")
    print("Bye")
    exit()

if __name__ == "__main__":
    main()