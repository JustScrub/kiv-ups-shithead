#include "include/card.h"
#include "include/game.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Pro tip: Imagine the server yells the strings in Arnold Schwarzenegger's voice

card_t shit_read_card(int cd, int *cnt)
{
    card_t c;
    recv(cd, &c, sizeof(c), 0);
    recv(cd, cnt, sizeof(int), 0);
}

void shit_rq_card(int cd)
{
    write(cd, "GIMME CARD", strlen("GIMME CARD"));
}

void shit_rq_trade(int cd)
{
    write(cd, "TRADE NOW", strlen("TRADE NOW"));
}

void shit_read_trade(int cd, int *bfr)
{
    // cli sends 3 bytes - the cards the player wishes to trade
    recv(cd, bfr+0,1,0);
    recv(cd, bfr+1,1,0);
    recv(cd, bfr+2,1,0);
    ((char *)bfr)[3] = INVALID_CARD;
}

void shit_write(int cd, char *str)
{
    write(cd, str, strlen(str));
}

void shit_tell_top(int cd, card_t c)
{
    write(cd, "TOP CARD", strlen("TOP CARD"));
    write(cd, c, sizeof(c));
}

void shit_tell_cards(int cd, int *h, card_t *f, char d_mask)
{
    write(cd, "YOUR CARDS", strlen("YOUR CARDS"));
    write(cd, "HAND", strlen("HAND"));
    for(int i=0;i<13;i++)
    {
        write(cd, &i, sizeof(int));
        write(cd, h+i, sizeof(int));
    }
    write(cd, "FACE UP", strlen("FACE UP"));
    for(int i=0;i<3;i++)
    {
        write(cd, f+i, sizeof(card_t));
    }
    write(cd, "FACE DOWN", strlen("FACE DOWN"));
    write(cd, &d_mask, sizeof(char));
}

void shit_tell_lobbies(int cd, void *lobbies, int n)
{
    game_t *globs = (game_t *)lobbies;
    write(cd, "LOBBIES", strlen("LOBBIES"));
    write(cd,&n,sizeof(n));
    for(int i=0; i<n;i++)
    {
        if(!(lobbies+i)) continue; // ??
        write(cd, "LOBBY", strlen("LOBBY"));
        write(cd, globs[i].id, sizeof(int));
        //write player count out of max
    }
}

unsigned shit_mm_choice(int cd)
{
    unsigned choice;
    recv(cd, &choice, sizeof(choice), 0);
    return choice;
}

bool shit_lobby_start(int cd)
{
    bool s;
    recv(cd,&s,sizeof(s),0);
    return s;
}

