#ifndef __SHITHEAD_COMM_IF_H__
#define __SHITHEAD_COMM_IF_H__

#include "include/game.h"
#include "include/player.h"

typedef enum {
    PL_CONN_DOWN = 0,
    PL_CONN_UP
} player_conn_state_t;

typedef enum {
    PLRQ_LOBBY      = 1<<0,
    PLRQ_MM_CHOICE  = 1<<1,
    PLRQ_RECON      = 1<<2,
    PLRQ_GAME_STATE = 1<<3,
    PLRQ_GAME_START = 1<<4,
    PLRQ_TOP_CARD   = 1<<5,
    PLRQ_PING       = 1<<6,
    PLRQ_QUIT       = 1<<7
} player_request_t;

#define PLRQ_DEFAULT PLRQ_PING|PLRQ_QUIT

typedef enum {
    SRRQ_MAIN_MENU=0,
    SRRQ_GIMME_CARD,
    SRRQ_TRADE_NOW,
    SRRQ_YOUR_TURN,
    SRRQ_ON_TURN,
    SRRQ_TOP_CARD,
    SRRQ_YOUR_CARDS,
    SRRQ_WRITE,
    SRRQ_PING
} server_request_t;

typedef enum {
    COMM_OK=0,      /**< communication done as it should have */
    COMM_QUIT,       /**< client wants to quit */
    COMM_BS ,         /**< Client sent bullshit (unknown request) */
    COMM_ILL,        /**< illegal request from client */
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
    comm_flag_t (*handle_request)(int cd,short rq_bfield, void *data);  /**< Reply to a client request with data. 
                                                                      Client request must be from the rq_bfield.*/
} player_comm_if_t;

#endif