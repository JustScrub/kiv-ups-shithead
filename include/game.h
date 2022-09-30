#ifndef __SHITHEAD_GAME_H__
#define __SHITHEAD_GAME_H__

typedef unsigned char card_t;

#define J_IDX 9
#define Q_IDX 10
#define K_IDX 11
#define A_IDX 12

#ifdef ANSI_COLORS
#define ANSI_C_RED      "\u001b[31m"
#define ANSI_RST        "\u001b[0m"
#define ANSI_BOLD       "\u001b[1m"
#endif

/*  CARDS: number 0 - 12, times its color: 0 - 3
 * 2 - 10 => card = value - 2 (Clubs 2 = 0, Clubs 4 = 2...)
 * J, Q, K => 9,10,11
 * Ace => 12
 * 
 * value of card = card + 2 (ACE=14, Q=12, 2=2 etc...)
 */

typedef enum {
    CLUBS = 0,
    DIAMONDS,
    SPADES,
    HEARTS
} card_color_t;


card_color_t card_get_color(card_t card);
int card_get_value(card_t card);
int card_game_value(card_t card);

/**
 * @brief Prints the card in format <number><color_sign> to stdin
 * <number> = 2..9 | JKQA
 * 
 * ifdef ANSI_COLORS:
 * The output uses ansi colors and bold formatting.
 * The user must reset the formatting themselves
 */
void print_card();



typedef struct {
    card_t draw_deck[52];
    int head;
} card_stack_t;

typedef struct {
    int id;
    card_t hand[52];
    card_t face_up[3];
    card_t face_down[3];
} player_t;

typedef struct
{
    int id;
    card_stack_t *draw_deck;
    card_stack_t *play_deck;
    player_t *players;
}game_t;


#endif