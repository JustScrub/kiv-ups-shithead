#include "shithead_protocol_comm_if.h"

char *cli_rqs_str[] =              {"MM CHOICE", "LOBBY", "GAME STATE", "TOP CARD", "RECON", "PING", "GAME START", "QUIT"};
player_request_t cli_rqs_field[] = {PLRQ_MM_CHOICE, PLRQ_LOBBY, PLRQ_GAME_STATE, PLRQ_TOP_CARD, PLRQ_RECON, PLRQ_PING, PLRQ_GAME_START, PLRQ_QUIT};
proto_fn cli_rqs_handlers[];

char *ser_rqs_str[] = {
    [SRRQ_MAIN_MENU] = "MAIN MENU",
    [SRRQ_GIMME_CARD] = "GIMME CARD",
    [SRRQ_ON_TURN] = "ON TURN",
    [SRRQ_YOUR_TURN] = "YOUR TURN",
    [SRRQ_YOUR_CARDS] = "YOUR CARDS",
    [SRRQ_TRADE_NOW] = "TRADE NOW",
    [SRRQ_PING] = "PING",
    [SRRQ_WRITE] = "WRITE",
    [SRRQ_TOP_CARD] = "TOP CARD"
};

proto_fn bernie_sanders[];

void consume_proto_part(char **rest, char *next)
{
     while(**rest&&**rest-DELIM)
    *(next++) = *((*rest)++);
}

comm_flag_t shit_req_send(int cd,server_request_t request, void *data)
{
    char bfr[256] = {0};
    comm_flag_t flag;
    flag = bernie_sanders[request](cd,bfr, data);
    if(flag == COMM_OK) return COMM_OK;
    // other stuff - pinging, quitting etc
}

comm_flag_t shit_req_handle(int cd,short rq_bfield, void *data)
{
    char bfr[256] = {0}, *rest = bfr, next[64] = {0};
    int i; comm_flag_t flag;
    recv(cd, bfr, 256, 0);
    consume_proto_part(&rest, next); // next contains request

    for(i=0;i<8;i++)
    {
        if(!strcmp(next, cli_rqs_str[i])) break;
    }
    if(i>7) return COMM_BS;
    if(!(rq_bfield&cli_rqs_field[i])) return COMM_ILL;

    if(rq_bfield&PLRQ_QUIT) ;//quit
    if(rq_bfield&PLRQ_PING) ;//reply to ping

    flag =  cli_rqs_handlers[i](cd,rest, data);
    if(flag == COMM_OK) return COMM_OK;

}





















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
    write(cd, &c, sizeof(c));
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
        write(cd, &globs[i].id, sizeof(int));
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

