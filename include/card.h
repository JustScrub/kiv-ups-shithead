#ifndef __SHITHEAD_CARD_H__
#define __SHITHEAD_CARD_H__

#include "config.h"

typedef unsigned char card_t;

#define J_VAL 11
#define Q_VAL 12
#define K_VAL 13
#define A_VAL 14
#define INVALID_CARD ((card_t)0)
#define card_is_valid(c)  (  ((c) >= 2) && ((c) <= 14)  )

#define ANSI_COLORS 1

#if ANSI_COLORS
#define ANSI_C_RED      "\033[31m"
#define ANSI_RST        "\033[0m"
#define ANSI_BOLD       "\033[1m"

#define reset_ansi_colors() printf("%s", ANSI_RST)
#endif


/*  CARDS: number 2-14, 11-14 being JQKA
 */

/**
 * @brief Prints the card in format <number><color_sign> to stdin
 * <number> = 2..9 | JKQA
 * 
 * ifdef ANSI_COLORS:
 * The output uses ansi colors and bold formatting.
 * The user must reset the formatting themselves
 */
/*
void print_card(card_t);
*/

typedef struct {
    card_t deck[DECK_NUM * 52];
    int head;
} card_stack_t;


card_stack_t *card_stack_create();
card_t card_stack_pop(card_stack_t *stack);
card_t card_stack_peek(card_stack_t *stack,int i);
void card_stack_push(card_stack_t *stack, card_t card);
int card_stack_height(card_stack_t *stack);
void card_stack_clear(card_stack_t *stack);
void card_stack_shuffle(card_stack_t *stack);

#endif