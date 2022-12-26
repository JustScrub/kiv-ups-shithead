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

typedef struct {
   char nick[NIC_LEN+1];
   unsigned id;
   unsigned gid;
} recon_cache_t;

comm_flag_t shit_req_send(int cd,server_request_t request, void *data, int dlen);

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
/** @param data input: player ID, output: player nick*/
RQFN(MAIN_MENU);
/** @param data input: none, output: x<0 (TO), x=0 (new lobby), 0<x<MAX_GAMES (enter lobby), "RECON^cache" (reconnect)*/
RQFN(MM_CHOICE);
/** @param data input: string to send, output: none*/
RQFN(RECON);
/** @param data input: int* ({id0,pl_count0...}), output: none*/
RQFN(LOBBIES);
/** @param data input: game *, output: none*/
RQFN(LOBBY_STATE);
/** @param data input: none, output: 1=start*/
RQFN(LOBBY_START);
/** @param data input: none, output: char[3] (cards)*/
RQFN(TRADE_NOW);
/** @param data input: player nick, output: none*/
RQFN(ON_TURN);
/** @param data input: 1 or 0, whether plays from f-d, output: card,count*/
RQFN(GIMME_CARD);
/** @param data input: game *, output: none*/
RQFN(GAME_STATE);
/** @param data input: str to send, output: none*/
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
