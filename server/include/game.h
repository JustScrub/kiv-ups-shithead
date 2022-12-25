#ifndef __SHITHEAD_GAME_H__
#define __SHITHEAD_GAME_H__

#include <stdbool.h>
#include "card.h"
#include "comm_if.h"
#include "player.h"
#include "config.h"

/**
 * @brief States the game can be in
 * 
 */
typedef enum {
    GM_LOBBY = 0,          /**< game in lobby. Owner player can start game */
    GM_PREPARE,        /**< Deck shuffle, add cards etc.*/
    GM_PLAYING,        /**< game is in play. */
    GM_FINISHED        /**< Game is finished. Will be deleted soon. */
} game_state_t;

typedef struct {
    int id;
    game_state_t state;
    card_stack_t *draw_deck;
    card_stack_t *play_deck;
    player_t **players;
    int on_turn;

    bool active_8;
} game_t;

void game_create(player_t *owner, game_t *out);
bool game_add_player(game_t *game,player_t *pl);
/**
 * @brief Count of connected players
 * 
 * does not count disconnected players
 * 
 * @param game 
 * @return int 
 */
int game_player_count(game_t *game);
int game_playing_count(game_t *game);
void game_delete(game_t *game);
void game_init(game_t *game);
void game_loop(game_t *game);

/**
 * @brief Check if the played card is illegal.
 * 
 * Illegal:
 *  - Player does not have the card(s).
 *  - active 8, but player did not play 8
 *  - if player does not have 2, 3 or 10:
 *      - if top card is 7 and player played higher card
 *      - top card != 7 and player played lower card
 * 
 * return values:
 * 0=legal
 * 1=played from hand or face-up (player was idiot)
 * 2=played form face-down (player did not know)
 * 
 * @param game 
 * @param player
 * @param card 
 */
int game_check_illegal(game_t *game, player_t *player, card_t card, int cnt);

/**
 * @brief Checks if the player is able to make a move
 * 
 * Not able when:
 *  - the 8 on top is active and the player has no 8
 *  - if player does not have 2, 3 or 10:
 *      - if top card is 7 and player has only higher cards
 *      - top card != 7 and player only has lower cards
 * 
 * return:
 * 0=can play
 * 1=8 on top and player does not have 8
 * 2=other (must take deck in this case)
 * 
 * @param game 
 * @param player_idx 
 * @return reason
 */
int game_check_cannot_play(game_t *game, player_t *player);

/**
 * @brief Checks if the play pile is to be burned
 * 
 * To be burned if:
 *  - 10 on top
 *  - 4 consecutive card values
 * 
 * @param game 
 * @return true 
 * @return false 
 */
bool game_check_burn_pile(game_t *game);

/**
 * @brief Communication with player in-game
 * 
 * Handles Quitting and Disconnecting
 * 
 * @param game the game
 * @param pl_idx index of the player in the game
 * @param request request to send
 * @param data data to send or receive
 * @return comm_flag_t
 */
comm_flag_t game_comm(game_t *game, int pl_idx, server_request_t request, void *data, int dlen);

void game_send_all(game_t *game, server_request_t request, void *data);

void *game_thread(void *arg);

#endif