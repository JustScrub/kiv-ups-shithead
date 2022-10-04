#ifndef __SHITHEAD_GAME_H__
#define __SHITHEAD_GAME_H__

#include <stdbool.h>
#include "card.h"
#include "player.h"
#include "config.h"

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
bool game_add_player(game_t *game,player_t *pl);
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
 * @param player
 * @param card 
 * @return true 
 * @return false 
 */
bool game_check_legal(game_t *game, player_t *player, card_t card, int cnt);

/**
 * @brief Checks if the player at the specified index is able to make a move
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
int game_check_cannot_play(game_t *game, int player_idx);

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