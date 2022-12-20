#include "shithead_protocol_comm_if.h"
#include "include/config.h"
//player_request_t cli_rqs_field[] = {PLRQ_MM_CHOICE, /*PLRQ_LOBBY,*/ PLRQ_GAME_STATE, /*PLRQ_TOP_CARD,*/ PLRQ_RECON, /*PLRQ_PING,*/ PLRQ_GAME_START, PLRQ_QUIT};
//char *cli_rqs_str[] =              {"MM CHOICE", "LOBBY", "GAME STATE", "TOP CARD", "RECON", "PING", "GAME START", "QUIT"};
//proto_fn cli_rqs_handlers[];

#define ACK_STRLEN 3
#define BFR_LEN 256

#define RQ2STR(rq) [SRRQ_##rq] = #rq
char *ser_rqs_str[] = {
    RQ2STR(MAIN_MENU),
    RQ2STR(MM_CHOICE),
    RQ2STR(RECON),
    RQ2STR(LOBBIES),
    RQ2STR(LOBBY_STATE),
    RQ2STR(LOBBY_START),
    RQ2STR(TRADE_NOW),
    RQ2STR(ON_TURN),
    RQ2STR(GIMME_CARD),
    RQ2STR(GAME_STATE),
    RQ2STR(WRITE),
};
#undef RQ2STR

#define RQ2FN(rq) [SRRQ_##rq] = send_##rq
proto_fn bernie_sanders[] = {
    RQ2FN(MAIN_MENU),
    RQ2FN(MM_CHOICE),
    RQ2FN(RECON),
    RQ2FN(LOBBIES),
    RQ2FN(LOBBY_STATE),
    RQ2FN(LOBBY_START),
    RQ2FN(TRADE_NOW),
    RQ2FN(ON_TURN),
    RQ2FN(GIMME_CARD),
    RQ2FN(GAME_STATE),
    RQ2FN(WRITE),
};
#undef RQ2FN

/**
 * @brief Increments \c rest up until \c DELIM is found. Meanwhile, copies the characters to \c next
 * 
 * the delimiter is not copied and stays in \c rest .
 * if \c rest is empty, nothing happens.
 * if \c rest points to \c DELIM, nothing happens.
 * 
 * \c next is not cleared before copying nor is it null-terminated.
 * 
 * @param rest 
 * @param next 
 */
void consume_proto_part(char **rest, char *next)
{
     while(**rest&&**rest-DELIM)
    *(next++) = *((*rest)++);
}

// TODO: close socket on disconnect?
comm_flag_t shit_req_send(int cd,server_request_t request, void *data)
{
    char bfr[BFR_LEN];
    comm_flag_t flag;
    int TO_cnt = 0;
    send:
    bzero(bfr, BFR_LEN);
    flag = bernie_sanders[request](cd,bfr, data);

    switch(flag)
    {
        case COMM_TO:
        case COMM_BS:
            if(TO_cnt++ > 3) return flag;
            goto send;
        default:
            return flag;
    }
}

/*
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
    //if(rq_bfield&PLRQ_PING) ;//reply to ping

    flag =  cli_rqs_handlers[i](cd,rest, data);
    if(flag == COMM_OK) return COMM_OK;
}
*/

#define to_dis_handle(readlen) if(readlen == 0) return COMM_DIS; \
                               if(readlen < 0) return COMM_TO
#define quit_handle(bfr) if(!strncmp(bfr, "QUIT", 4)) return COMM_QUIT
#define format_check(bfr, fmt) if(strncmp(bfr, fmt, strlen(fmt))) return COMM_BS; \
                               else bfr += strlen(fmt)

inline comm_flag_t ack_handle(int cd, char *bfr)
{
    int ret = read(cd, bfr, ACK_STRLEN);
    if(ret < 0) return COMM_TO; //error
    if(ret == 0) return COMM_DIS;
    if(ret < ACK_STRLEN) return COMM_TO;
    if(strncmp(bfr, "ACK",ACK_STRLEN)) return COMM_BS;
    bzero(bfr, BFR_LEN);
    return COMM_OK;
}

comm_flag_t send_MAIN_MENU(int cd, char *bfr, void *nick_bfr)
{
    int ret = *(int *)nick_bfr;
    sprintf(bfr, "MAIN MENU^%d^%d\x0A", ret, NIC_LEN);
    ret = write(cd, bfr, strlen(bfr));
    if(ret < 0) return COMM_TO;
    
    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, BFR_LEN); 
    to_dis_handle(ret);
    quit_handle(bfr);
    format_check(bfr, "NICK^");

    char nlen[NIC_LEN+3] = {0}; // +3 for "XX^" where XX is the length of the nick
    consume_proto_part(&bfr, nlen); // nlen contains the length of the nick
    bfr++; // past the delimiter
    ret = strtol(nlen, NULL, 10);
    if(strlen(bfr) != ret) return COMM_BS;

    strncpy((char *)nick_bfr, bfr, ret);
    return COMM_OK;
}

comm_flag_t send_LOBBY_START(int cd, char *bfr, void *data)
{
    sprintf(bfr, "LOBBY START\x0A");
    int ret = write(cd, bfr, strlen(bfr));
    if(ret < 0) return COMM_TO;
    if(ret < strlen(bfr)) return COMM_TO;

    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, 3);
    to_dis_handle(ret);
    quit_handle(bfr);

    if(!strncmp(bfr, "YES", 3)) *(char*)data = 1;
    else if(!strncmp(bfr, "NO", 2)) *(char*)data = 0;
    else return COMM_BS;

    return COMM_OK;
}

comm_flag_t send_TRADE_NOW(int cd, char *bfr, void *data)
{
    sprintf(bfr, "TRADE NOW\x0A");
    int ret = write(cd, bfr, strlen(bfr));
    if(ret < 0) return COMM_TO;
    if(ret < strlen(bfr)) return COMM_TO;

    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, BFR_LEN);
    to_dis_handle(ret);
    quit_handle(bfr);
    format_check(bfr, "TRADE^");

    for(ret = 0; ret < 3; ret++)
    {
        bfr[ret] -= 0x30;
        if(bfr[ret] == 0) continue;
        if(bfr[ret]  < 2) return COMM_BS;
        if(bfr[ret]  > 14) return COMM_BS;
    }
    memcpy(data, bfr, 3);
    return COMM_OK;
}

comm_flag_t send_ON_TURN(int cd, char *bfr, void *data)
{
    sprintf(bfr, "ON TURN^%s\x0A", (char *)data);
    int ret = write(cd, bfr, strlen(bfr));
    if(ret < 0) return COMM_TO;
    if(ret < strlen(bfr)) return COMM_TO;

    return ack_handle(cd, bfr);
}

comm_flag_t send_WRITE(int cd, char *bfr, void *data)
{
    sprintf(bfr, "WRITE^%s\x0A", (char *)data);
    int ret = write(cd, bfr, strlen(bfr));
    if(ret < 0) return COMM_TO;
    if(ret < strlen(bfr)) return COMM_TO;

    return ack_handle(cd, bfr);
}

comm_flag_t send_MM_CHOICE(int cd, char *bfr, void *data)
{
    sprintf(bfr, "MM CHOICE\x0A");
    int ret = write(cd, bfr, strlen(bfr));
    if(ret < 0) return COMM_TO;
    if(ret < strlen(bfr)) return COMM_TO;

    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, BFR_LEN);
    if(ret < 0) // Timeout OK with this request
    {
        *(long*)data = -1;
        return COMM_OK;
    }
    if(ret == 0) return COMM_DIS;
    quit_handle(bfr);

    if(!strncmp(bfr, "LB", 2))
    {
        bfr += 3; // "LB^"
        ret = strtol(bfr, NULL, 10);
        if(ret < 0) return COMM_BS;
        if(ret > MAX_GAMES) return COMM_BS;
        *(long*)data = ret;
        return COMM_OK;
    }
    else if(!strncmp(bfr, "RECON", 5))
    {
        //in data: "RECON^nick^player_id^game_id"
        //out data: recon_cache_t*
        bfr += 6; // "RECON^"
        recon_cache_t *rc = calloc(1,sizeof(recon_cache_t));
        char *tmp = alloca(NIC_LEN);
        bzero(tmp, NIC_LEN);

        consume_proto_part(&bfr, tmp); // tmp=nick
        strncpy(rc->nick, tmp, NIC_LEN);
        bfr++; // past the delimiter
        bzero(tmp, NIC_LEN);

        consume_proto_part(&bfr, tmp); // tmp=player_id
        rc->id = strtol(tmp, NULL, 10);
        bfr++; // past the delimiter
        bzero(tmp, NIC_LEN);

        consume_proto_part(&bfr, tmp); // tmp=game_id; *bfr should be \0
        rc->gid = strtol(tmp, NULL, 10);

        *(recon_cache_t**)data = rc;
        return COMM_OK;
    }
    else return COMM_BS;

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

