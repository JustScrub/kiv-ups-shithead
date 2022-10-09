#include <stdio.h>
#include <string.h>
#include "include/card.h"

card_t std_read_card(int cd,int *cnt)
{
    int out;
    scanf(" %d %d", cnt, &out);
    printf("\n");
    return (card_t)out;
}

void std_rq_card(int cd)
{
    printf("Gimme card\n");
}

void std_rq_trade(int cd)
{
    printf("Time to trade\n");
}

void std_read_trade(int cd,int *bfr)
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

void std_write(int cd,char *str)
{
    printf("%s\n", str);
}

void std_tell_top(int cd,card_t c)
{
    if(c == INVALID_CARD) 
    {
        printf("Top card: table is clear\n");
        return;
    }
    printf("Top card: %d", c);
    printf("\n");
}

void std_tell_cards(int cd,int *h, card_t *f, char d_mask)
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