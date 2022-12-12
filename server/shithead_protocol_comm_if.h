#include "include/card.h"
#include "include/game.h"
#include "include/comm_if.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define DELIM '^'
typedef comm_flag_t (*proto_fn)(int cd,char *rest, void *data);


comm_flag_t shit_req_send(int cd,server_request_t request, void *data);

/**
 * @brief Handle request
 * 
 * @param cd connection descriptor
 * @param rq_bfield bit field of ORed \c player_request_t
 * @param data data of the request and reply
 * @return comm_flag_t 
    comm_flag_t shit_req_handle(int cd,short rq_bfield, void *data);
 */

#define RQFN(x) comm_flag_t send_##x(int cd, char *rest, void *data)
RQFN(MAIN_MENU);
RQFN(MM_CHOICE);
RQFN(RECON);
RQFN(LOBBIES);
RQFN(LOBBY_STATE);
RQFN(LOBBY_START);
RQFN(TRADE_NOW);
RQFN(ON_TURN);
RQFN(GIMME_CARD);
RQFN(GAME_STATE);
RQFN(WRITE);
#undef RQFN


/*
comm_flag_t send_main_menu(int cd, char *rest, void *data);
comm_flag_t send_lobbies(int cd, char *rest, void *data);
comm_flag_t send_gimme_card(int cd, char *rest, void *data);
comm_flag_t send_on_turn(int cd, char *rest, void *data);
comm_flag_t send_your_turn(int cd, char *rest, void *data);
comm_flag_t send_your_cards(int cd, char *rest, void *data);
comm_flag_t send_trade_now(int cd, char *rest, void *data);
comm_flag_t send_write(int cd, char *rest, void *data);
comm_flag_t send_top_card(int cd, char *rest, void *data);

comm_flag_t recv_mm_choice(int cd, char *rest, void *data);
comm_flag_t recv_game_state(int cd, char *rest, void *data);
comm_flag_t recv_top_card(int cd, char *rest, void *data);
comm_flag_t recv_recon(int cd, char *rest, void *data);
comm_flag_t recv_ping(int cd, char *rest, void *data);
comm_flag_t recv_game_start(int cd, char *rest, void *data);
comm_flag_t recv_quit(int cd, char *rest, void *data);
*/
