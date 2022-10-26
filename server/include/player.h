#ifndef __SHITHEAD_PLAYER_H__
#define __SHITHEAD_PLAYER_H__

#include "card.h"
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
    PL_PLAYING_ON_TURN,        /**< Player is in play and it is their turn. */
    PL_PLAYING_WAITING,        /**< Player is in play, but not their turn. */
    PL_DONE            /**< Player won the game.*/
} player_state_t;

typedef enum {
    PL_CONN_DOWN = 0,
    PL_CONN_UP
} player_conn_state_t;

/**
 * @brief Communication interface with the player
 * 
 * containts callbacks to communicate with the player
 * 
 */
typedef struct {
    int cd;                             /**< connection descriptor */
    card_t (*read_card)(int cd,int *cnt);      /**< Read which card was played (and how many of them). 
                                             If the player plays from face-down cards, then the
                                             return value is the index of the face-down card (i.e. 0 for first face-down etc.) */
    void   (*rq_card)(int cd);                /**< Request a card */
    void   (*rq_trade)(int cd);               /**< Start trading cards */
    void   (*read_trade)(int cd,int *bfr);     /**< Read what cards the player switched 
                                            (byte order represents face-up card order, endianness does not matter, 
                                            unchanged cards are represented by 0xFF - last byte always 0xFF) */
    void   (*write)(int cd, char *);            /**< Write a message to the player */
    void   (*tell_top)(int cd, card_t);         /**< Tell the player the top card */
    void   (*tell_cards)(int cd, int *h, card_t *f_u, char f_d); /**< Tell player their cards */


    void   (*tell_lobbies)(int cd, void *lobbies, int n);
    unsigned    (*mm_choice)(int cd);  /**< MainMenu choice: 0=create_lobby, other=join_lobby*/

    bool (*lobby_start)(int cd);
} player_comm_if_t;

typedef struct {
    int id;
    int game_id;
    player_state_t state;
    player_conn_state_t conn_state;
    int hand[13];          /**< number of cards of value i on index i */
    card_t face_up[3];
    card_t face_down[3];

    player_comm_if_t *comm_if;
} player_t;

void player_create(player_t *out);

int player_hand_card_cnt(player_t *player);
/**
 * @brief Checks if player has at least \c cnt of a specific card value
 * 
 * @param player 
 * @param card_idx
 * @param cnt 
 * @return int 
 */
bool player_has_card(player_t *player, card_t card, int cnt);
void player_trade_cards(player_t *player);

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

bool player_play_cards(player_t *player, card_t card, int cnt, card_stack_t *play_deck);
bool player_draw_cards(player_t *player, int cnt, card_stack_t *draw_deck);
char player_secret_face_down(player_t *player);
void player_put_to_hand(player_t *player, int f_down_idx);

#endif