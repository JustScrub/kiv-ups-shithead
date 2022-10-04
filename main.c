#include "include/game.h"
#include "include/card.h"
#include "include/player.h"

#include <stdio.h>

card_t read_card(int *cnt)
{
    int out;
    scanf(" %d %d", cnt, &out);
    printf("\n");
    return (card_t)out;
}

void rq_card()
{
    printf("Gimme card\n");
}

void rq_trade()
{
    printf("Time to trade\n");
}

void read_trade(int *bfr)
{
    card_t c;
    *bfr = 0xFFFFFFFF;
    for(int i = 0; i<3;i++)
    {
        scanf(" %c", &c);
        if(c=='q') break;

        printf("%i\n",c);

        ((char *)(bfr))[i] = c;
    }
}

void write(char *str)
{
    printf("%s\n", str);
}

void tell_top(card_t c)
{
    if(c == INVALID_CARD) 
    {
        printf("Top card: table is clear\n");
        return;
    }
    printf("Top card: %d", c);
    printf("\n");
}

void tell_cards(int *h, card_t *f)
{
    printf("Hand: \n");
    for(int i=0;i<12;i++)
    {
        if(h[i]) printf("%dx%d ",h[i], i);
    }

    printf("\nFace-Up: \n");
    for(int i=0;i<3;i++)
    {
        printf("%d ",f[i]);
    }

    printf("\n");
}

int main()
{
    
    player_t players[3];
    player_comm_if_t cif = 
    {
        .read_card = read_card,
        .rq_card = rq_card,
        .rq_trade = rq_trade,
        .read_trade = read_trade,
        .write = write,
        .tell_top = tell_top,
        .tell_cards = tell_cards
    };

    for(int i =0;i<3;i++)
    {
        player_create(players+i);
        players[i].comm_if = &cif;
    }

    game_t *game;
    game_create(&players[0],game);
    game_add_player(game, players+1);
    game_add_player(game, players+2);

    game_init(game);
    game_loop(game);

    return 0;
}