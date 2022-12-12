#ifndef __SHITHEAD_COMM_IF_H__
#define __SHITHEAD_COMM_IF_H__

#include "include/game.h"
#include "include/player.h"

typedef enum {
    PL_CONN_DOWN = 0,
    PL_CONN_UP
} player_conn_state_t;

/*
typedef enum {
    PLRQ_MM_CHOICE  = 1<<0,
    PLRQ_RECON      = 1<<1,
    PLRQ_GAME_STATE = 1<<2,
    PLRQ_GAME_START = 1<<3,
//    PLRQ_TOP_CARD   = 1<<4,
    PLRQ_QUIT       = 1<<5,
//    PLRQ_PING       = 1<<6,
} player_request_t;
#define PLRQ_DEFAULT PLRQ_QUIT
*/

typedef enum {
    SRRQ_MAIN_MENU=0,
    SRRQ_MM_CHOICE,
    SRRQ_RECON,

    SRRQ_LOBBIES,
    SRRQ_LOBBY_STATE,
    SRRQ_LOBBY_START,

    SRRQ_TRADE_NOW,
    SRRQ_ON_TURN,
    SRRQ_GIMME_CARD,
    SRRQ_GAME_STATE,

    SRRQ_WRITE,
} server_request_t;

typedef enum {
    COMM_OK=0,      /**< communication done as it should have */
    COMM_QUIT,       /**< client wants to quit */
    COMM_BS ,         /**< Client sent bullshit */
    COMM_DIS,        /**< client disconnected (without QUIT) */
    COMM_TO         /**< timeout */
} comm_flag_t;

/**
 * @brief Communication interface with the player
 * 
 * containts callbacks to communicate with the player
 * 
 */
typedef struct {
    int cd;                             /**< connection descriptor */
    player_conn_state_t conn_state;     /**< conncection state of the player */
    comm_flag_t (*send_request)(int cd,server_request_t request, void *data); /**< Send a request to the client with some data and recieve the data*/
} player_comm_if_t;

#endif