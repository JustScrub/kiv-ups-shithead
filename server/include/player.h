#ifndef __SHITHEAD_PLAYER_H__
#define __SHITHEAD_PLAYER_H__

#include "card.h"
#include "comm_if.h"
#include <stdbool.h>

#define PL_PILE_HAND 1
#define PL_PILE_F_UP 2
#define PL_PILE_F_DWN 3
#define PL_PILE_NONE 0

/**
 * @brief States the player can be in
 * 
 */
typedef enum {
    PL_MAIN_MENU = 0,  /**< Player in main menu. (Can join/create a lobby) */
    PL_LOBBY,          /**< Player in lobby.*/
    PL_LOBBY_OWNER,          /**< Player in lobby. Can start game */
    PL_PLAYING_TRADING,
    PL_PLAYING_ON_TURN,        /**< Player is in play and it is their turn. */
    PL_PLAYING_WAITING,        /**< Player is in play, but not their turn. */
    PL_DONE            /**< Player won the game.*/
} player_state_t;

typedef struct {
    char nick[NIC_LEN+1];
    unsigned id;
    unsigned game_id;
    player_state_t state;
    int hand[13];          /**< number of cards of value i on index i */
    card_t face_up[3];
    card_t face_down[3];

    player_comm_if_t comm_if;
} player_t;

void player_create(player_t *out);
void player_clear(player_t *player);

int player_hand_card_cnt(player_t *player);
/**
 * @brief Checks if player has at least \c cnt of a specific card value
 * If plays from FACE DOWN, \c card is the card and \c cnt IS INDEX of the card (0-2)
 * 
 * @param player 
 * @param card_idx
 * @param cnt 
 * @return int 
 */
bool player_has_card(player_t *player, card_t card, int cnt);

/**
 * @brief Gets, whether player should play from hand, face-up cards or face-down cards.
 * 
 * 1 = hand
 * 2 = f-u
 * 3 = f-d
 * 0 = player won
 * 
 * @param player 
 * @return int 
 */
int player_plays_from(player_t *player);

/**
 * @brief Plays \c cnt cards of value \c card from whatever player plays from.
 * If plays from face down, \c cnt is INDEX of the card (0-2) (and \c card is irrelevant).
 * 
 * @param player 
 * @param card 
 * @param cnt 
 * @param play_deck 
 * @return true 
 * @return false 
 */
card_t player_play_cards(player_t *player, card_t card, int cnt, card_stack_t *play_deck);
bool player_draw_cards(player_t *player, int cnt, card_stack_t *draw_deck);
char player_secret_face_down(player_t *player);
void player_put_to_hand(player_t *player, int f_down_idx);

#endif