import socket

ack_send = ["ACKN", "QUIT"]

def sendall(sock: socket, msg: str):
    sock.sendall((msg+"\x0A").encode())

def handle_main_menu(sock: socket, nick):
    inp = ""
    with sock.makefile("r") as f:
        inp = f.readline()
    print(inp)
    inp = inp.split('^')

    if inp[0] != "MAIN MENU":
        sock.close()
        raise ValueError("Invalid server response")
    
    sendall(sock, ack_send[0])
    sendall(sock, f"NICK^{nick}")

    return int(inp[2]), int(inp[1])

def handle_write( game, inp):
    game.print_state(inp[0])
    return None