import socket

def handle_main_menu(sock: socket, nick):
    inp = ""
    with sock.makefile("r") as f:
        inp = f.readline()
    print(inp)
    inp = inp.split('^')

    if inp[0] != "MAIN MENU":
        sock.close()
        raise ValueError("Invalid server response")
    
    sock.sendall(b"ACKN\x0A")
    sock.sendall(f"NICK^{nick}\n".encode())

    return int(inp[2]), int(inp[1])