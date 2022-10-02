#ifndef __SHITHEAD_GAME_H__
#define __SHITHEAD_GAME_H__

#include <stdbool.h>
#include "card.h"
#include "player.h"

#define MAX_PLAYERS 4


/**
 * @brief States the game can be in
 * 
 */
typedef enum {
    GM_LOBBY = 0,          /**< game in lobby. Owner player can start game */
    GM_PREPARE,        /**< Players trading cards */
    GM_PLAYING,        /**< game is in play. */
    GM_FINISHED        /**< Game is finished. Will be deleted soon. */
} game_state_t;

typedef struct
{
    int id;
    game_state_t state;
    card_stack_t *draw_deck;
    card_stack_t *play_deck;
    player_t **players;

    bool active_8;
}game_t;


void game_create(player_t *owner, game_t *out);
void game_delete(game_t *game);
void game_init(game_t *game);
void game_loop(game_t *game);

/**
 * @brief Check if the played card is legal.
 * 
 * Illegal:
 *  - Player does not have the card(s).
 *  - active 8, but player did not play 8
 *  - if player does not have 2, 3 or 10:
 *      - if top card is 7 and player played higher card
 *      - top card != 7 and player played lower card
 * 
 * @param game 
 * @param card 
 * @return true 
 * @return false 
 */
bool game_check_legal(game_t *game, card_t card);

/**
 * @brief Checks if the player at the specified index is able to make a move
 * 
 * Not able when:
 *  - the 8 on top is active and the player has no 8
 *      - if this is the reason, inactivates the 8 automatically
 *  - if player does not have 2, 3 or 10:
 *      - if top card is 7 and player has only higher cards
 *      - top card != 7 and player only has lower cards
 * 
 * @param game 
 * @param player_idx 
 * @return int 
 */
bool game_check_can_play(game_t *game, int player_idx);

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

#endif