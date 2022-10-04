#include "include/game.h"
#include "include/card.h"
#include "include/player.h"

#include <stdio.h>
#include <string.h>

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

void tell_cards(int *h, card_t *f, char d_mask)
{
    printf("Hand: \n");
    for(int i=0;i<13;i++)
    {
        if(h[i]) printf("%dx%d ",h[i], i+2);
    }

    printf("\nFace-Up: \n");
    for(int i=0;i<3;i++)
    {
        printf("%d ",f[i]);
    }

    printf("\nFace Down: \n");
    for(int i=0;i<3;i++)
    {
        printf("%c",d_mask&(1<<i) ? '?': ' ');
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
    //game_add_player(game, players+2);

    game_init(game);

    game->draw_deck->head = 0;
    memset(game->players[1]->hand,0,13*sizeof(int));

    game_loop(game);

    return 0;
}