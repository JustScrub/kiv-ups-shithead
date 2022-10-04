#include "include/card.h"
#include <stdio.h>
#include <stdlib.h>

/*
void print_card(card_t card)
{
    //char chars[] = "♣♦♠♥JQKA";
    char chars[] = "cdshJQKA";
    char card_disp[3] = {0};
    if(card_get_value(card) < 9 )
    {
        sprintf(card_disp, "%i", card_game_value(card));
    } 
    else 
    {
        card_disp[0] = chars[card_get_value(card)-5]; // -5 = -9 + 4, 9=J, 4 is offset to chars
        card_disp[1] = 0;
    }

    #if ANSI_COLORS
    char *col_code;   
    switch (card_get_color(card))
    {
    case HEARTS:
    case DIAMONDS:
        col_code = ANSI_C_RED ANSI_BOLD;
        break;
    default:
        col_code = ANSI_RST ANSI_BOLD;
        break;
    }
    printf("%s", col_code);
    #endif

    printf("%s%c", card_disp, chars[card_get_color(card)]);
}
*/

card_stack_t *card_stack_create()
{
    return (card_stack_t *)calloc(1, sizeof(card_stack_t));
}
card_t card_stack_pop(card_stack_t *stack)
{
    if(!stack->head) return INVALID_CARD;
    return stack->deck[--(stack->head)];
}
card_t card_stack_peek(card_stack_t *stack, int i)
{
    if(1+i > stack->head) return INVALID_CARD;
    return stack->deck[stack->head - (1+i)]; // -1 = top, -i = i-th from top
}
void card_stack_push(card_stack_t *stack, card_t card)
{
    stack->deck[(stack->head)++] = card;
}
int card_stack_height(card_stack_t *stack)
{
    return stack->head;
}

void card_stack_shuffle(card_stack_t *stack)
{
    return;
}

void card_stack_clear(card_stack_t *stack)
{
    stack->head = 0;
}