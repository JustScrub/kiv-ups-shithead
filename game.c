#include "include/game.h"
#include <stdio.h>
#include <stdlib.h>

card_color_t card_get_color(card_t card)
{
    return card/13;
}

int card_get_value(card_t card){
    return card%13;
}
int card_game_value(card_t card)
{
    return card_get_value(card)+2; // 
}

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

    #ifdef ANSI_COLORS
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