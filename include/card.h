#ifndef __SHITHEAD_CARD_H__
#define __SHITHEAD_CARD_H__


typedef unsigned char card_t;

#define J_IDX 9
#define Q_IDX 10
#define K_IDX 11
#define A_IDX 12
#define INVALID_CARD 0xFF
#define INVALID_CARD_VAL -1

#define ANSI_COLORS 1


#if ANSI_COLORS
#define ANSI_C_RED      "\033[31m"
#define ANSI_RST        "\033[0m"
#define ANSI_BOLD       "\033[1m"
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
void print_card(card_t);


typedef struct {
    card_t deck[52];
    int head;
} card_stack_t;


card_stack_t *card_stack_create();
card_t card_stack_pop(card_stack_t *stack);
card_t card_stack_peek(card_stack_t *stack,int i);
void card_stack_push(card_stack_t *stack, card_t card);
void card_stack_clear(card_stack_t *stack);
void card_stack_shuffle(card_stack_t *stack);

#endif