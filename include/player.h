#ifndef __SHITHEAD_PLAYER_H__
#define __SHITHEAD_PLAYER_H__

#include "card.h"
#include <stdbool.h>

/**
 * @brief States theplayer can be in
 * 
 */
typedef enum {
    PL_MAIN_MENU = 0,  /**< Player in main menu. (Can join/create a lobby) */
    PL_LOBBY,          /**< Player in lobby. Owner player can start game */
    PL_PLAYING         /**< Player is in play. */
} player_state_t;

/**
 * @brief Communication interface with the player
 * 
 * containts callbacks to communicate with the player
 * 
 */
typedef struct {
    card_t (*read_card)();      /**< Read which card was played */
    void   (*rq_card)();        /**< Request a card */
    void   (*write)(char *);    /**< Write a message to the player */
    void   (*tell_top)(card_t);  /**< Tell the player the top card */
} player_comm_if_t;

typedef struct {
    int id;
    int game_id;
    player_state_t state;
    bool hand[52];          /**< true if the player has card indicated by index */
    card_t face_up[3];
    card_t face_down[3];

    player_comm_if_t *comm_if;
} player_t;

void player_create(player_t *out);

#endif