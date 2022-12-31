#include "shithead_protocol_comm_if.h"
#include "include/config.h"
#include "errno.h"
//player_request_t cli_rqs_field[] = {PLRQ_MM_CHOICE, /*PLRQ_LOBBY,*/ PLRQ_GAME_STATE, /*PLRQ_TOP_CARD,*/ PLRQ_RECON, /*PLRQ_PING,*/ PLRQ_GAME_START, PLRQ_QUIT};
//char *cli_rqs_str[] =              {"MM CHOICE", "LOBBY", "GAME STATE", "TOP CARD", "RECON", "PING", "GAME START", "QUIT"};
//proto_fn cli_rqs_handlers[];

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

bool cache_data[] = {
    [SRRQ_MAIN_MENU] = true,
    [SRRQ_GIMME_CARD] = true,
    // rest is false
    [SRRQ_WRITE] = false
};

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

comm_flag_t shit_req_send(int cd,server_request_t request, void *data, int dlen)
{
    char bfr[BFR_LEN+1];
    comm_flag_t flag;
    int TO_cnt = 0;
    char *dcpy = NULL;
    if(cache_data[request]) 
    {
        dcpy = alloca(dlen);
        memcpy(dcpy,data,dlen);
    }
    send:
    if(cache_data[request]) memcpy(data,dcpy,dlen);
    bzero(bfr, BFR_LEN+1);
    flag = bernie_sanders[request](cd,bfr, data);

    switch(flag)
    {
        case COMM_TO:
        case COMM_BS:
            if(TO_cnt++ > 3) { close(cd); break; }
            goto send;
        case COMM_DIS:
            close(cd);
            break;
        default:
            break;
    }
    return flag;
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
#define quit_handle(bfr) if(!strncmp(bfr, "QUIT\x0A", 5)) return COMM_QUIT
#define format_check(bfr, fmt) if(strncmp(bfr, fmt, strlen(fmt))) return COMM_BS; \
                               else bfr += strlen(fmt)
#define write_check(ret)     if(ret < 0) {if(errno == EAGAIN || errno == EWOULDBLOCK) return COMM_TO; return COMM_DIS; }
 

comm_flag_t ack_handle(int cd, char *bfr)
{
    // both ACKN and QUIT are 5 bytes long (including \x0A)
    bzero(bfr, BFR_LEN);
    int ret = read(cd, bfr, 5 );
    printD("ack_handle: bfr=%s, len=%d\n", bfr,ret);
    if(ret == 0) return COMM_DIS;
    if(ret != 5) return COMM_TO;
    quit_handle(bfr);
    if(strncmp(bfr, "ACKN\x0A",5)) return COMM_BS;
    bzero(bfr, BFR_LEN);
    return COMM_OK;
}

comm_flag_t send_MAIN_MENU(int cd, char *bfr, void *nick_bfr)
{
    int ret = *(int *)nick_bfr;
    sprintf(bfr, "MAIN MENU^%d^%d\x0A", ret, NIC_LEN);
    ret = write(cd, bfr, strlen(bfr));
    printD("send_MAIN_MENU write: bfr=%s,ret=%d\n", bfr, ret);
    write_check(ret);
    
    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, BFR_LEN); 
    printD("send_MAIN_MENU read: bfr=%s,ret=%d\n", bfr, ret);
    to_dis_handle(ret);
    quit_handle(bfr);
    format_check(bfr, "NICK^");

    ret = strnlen(bfr, NIC_LEN+1); // +1 for 0x0A
    if( ret <= NIC_MIN_LEN || ret > NIC_LEN+1 || bfr[ret-1] != 0x0A) return COMM_BS;
    strncpy((char *)nick_bfr, bfr, ret-1);
    return COMM_OK;
}

comm_flag_t send_LOBBY_START(int cd, char *bfr, void *data)
{
    sprintf(bfr, "LOBBY START\x0A");
    int ret = write(cd, bfr, strlen(bfr));
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, 4);
    to_dis_handle(ret);
    quit_handle(bfr);

    if(!strncmp(bfr, "YES\x0A", 4)) *(char*)data = 1;
    else if(!strncmp(bfr, "NO\x0A", 3)) *(char*)data = 0;
    else return COMM_BS;

    return COMM_OK;
}

comm_flag_t send_TRADE_NOW(int cd, char *bfr, void *data)
{
    sprintf(bfr, "TRADE NOW\x0A");
    int ret = write(cd, bfr, strlen(bfr));
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, BFR_LEN);
    to_dis_handle(ret);
    quit_handle(bfr);
    format_check(bfr, "TRADE^");
    if(bfr[3] != 0x0A) return COMM_BS;

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
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    return ack_handle(cd, bfr);
}

comm_flag_t send_WRITE(int cd, char *bfr, void *data)
{
    sprintf(bfr, "WRITE^%s\x0A", (char *)data);
    int ret = write(cd, bfr, strlen(bfr));
    printD("send_WRITE write: bfr=%s,ret=%d\n", bfr, ret);
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    return ack_handle(cd, bfr);
}

comm_flag_t send_MM_CHOICE(int cd, char *bfr, void *data)
{
    sprintf(bfr, "MM CHOICE\x0A");
    int ret = write(cd, bfr, strlen(bfr));
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, BFR_LEN);
    printD("send_MM_CHOICE read: bfr=%s,ret=%d\n", bfr, ret);
    if(ret < 0) // Timeout OK with this request
    {
        *(int64_t*)data = -1;
        return COMM_TO;
    }
    if(ret == 0) return COMM_DIS;
    quit_handle(bfr);

    if(!strncmp(bfr, "LOBBY^", 6))
    {
        bfr += 6; // "LOBBY^"
        ret = strtol(bfr, &bfr, 10);
        if(ret < 0) return COMM_BS;
        if(ret > MAX_GAMES) return COMM_BS;
        if(*bfr != 0x0A) return COMM_BS;
        *(int64_t*)data = ret;
        return COMM_OK;
    }
    else if(!strncmp(bfr, "RECON^", 6))
    {
        //in data: "RECON^nick^player_id^game_id\x0A"
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
        if(rc->id <= 0){
            free(rc);
            return COMM_BS;
        };
        bfr++; // past the delimiter
        bzero(tmp, NIC_LEN);

        consume_proto_part(&bfr, tmp); // tmp=game_id\x0A; *bfr should be 0
        rc->gid = strtol(tmp, &bfr, 10);
        if(rc->gid <= 0 || *bfr != 0x0A){
            free(rc);
            return COMM_BS;
        };

        *(recon_cache_t**)data = rc;
        return COMM_OK;
    }
    else return COMM_BS;

}

comm_flag_t send_GIMME_CARD(int cd, char *bfr, void *data)
{
    sprintf(bfr, "GIMME CARD\x0A");
    int ret = write(cd, bfr, strlen(bfr));
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    ret = ack_handle(cd, bfr);
    if(ret != COMM_OK) return ret;

    ret = read(cd, bfr, BFR_LEN); // "CARD^card^cnt\x0A"
    printD("send_GIMME_CARD read: bfr=%s,len=%d\n", bfr, ret);
    to_dis_handle(ret);
    quit_handle(bfr);
    format_check(bfr, "CARD^");

    ret = *bfr++ - 0x30;

    printD("send_GIMME_CARD: face_down=%c\n", (*(int*)data)? 'Y':'N');
    if(*(int*)data) // plays from face-down => read idx of card, not card itself
       { if(ret < 0 || ret > 2) return COMM_BS; }
    else
       { if(!card_is_valid(ret)) return COMM_BS; }

    if(*bfr != '^') return COMM_BS;
    *(int*)data = ret;
    bfr++; // past the delimiter

    ret = strtol(bfr, &bfr, 10);
    if(ret <= 0) return COMM_BS;
    if(ret > 4*DECK_NUM) return COMM_BS;
    if(*bfr != 0x0A) return COMM_BS;
    *(int*)data |= ret << 8;

    return COMM_OK;
}

comm_flag_t send_RECON(int cd, char *bfr, void *data)
{
    sprintf(bfr, "RECON^%c\x0A", *(char *)data);
    int ret = write(cd, bfr, strlen(bfr));
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    return ack_handle(cd, bfr);
}

comm_flag_t send_LOBBY_STATE(int cd, char *bfr, void *data)
{
    game_t *g = (game_t*)data;
    int len = sprintf(bfr, "LOBBY STATE");
    for(int i=0; i<MAX_PLAYERS; i++)
    {
        if(!g->players[i]) continue;
        len += snprintf(bfr+len, BFR_LEN-len, "^%s", g->players[i]->nick);
    }
    strncat(bfr, "\x0A", 2);
    int ret = write(cd, bfr, strlen(bfr));
    to_dis_handle(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    return ack_handle(cd, bfr);
}

comm_flag_t send_LOBBIES(int cd, char *bfr, void *data)
{
    int *g = (int *)data;
    //int cnt;
    int len = sprintf(bfr, "LOBBIES");
    for(;*g;g+=2)
    {
        len += snprintf(bfr+len,BFR_LEN-len, "^%d:%d",*g, *(g+1));
    }
    strncat(bfr, "\x0A", 2);
    int ret = write(cd, bfr, strlen(bfr));
    printD("send_LOBBIES: %s\n", bfr);
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    return ack_handle(cd, bfr);
}

int dmask(player_t *pl, char *bfr)
{
    for(int i=0; i<3; i++)
    {
        bfr[i] = card_is_valid(pl->face_down[i]) + 0x30;
    }
    return 3;
}
int fmask(player_t *pl, char *bfr)
{
    for(int i=0; i<3; i++)
    {
        bfr[i] = pl->face_up[i] + 0x30;
    }
    return 3;
}
int hmask(player_t *pl, char *bfr, int blen)
{
    int len = 0;
    for(int i=0; i<13; i++)
    {
        len += snprintf(bfr+len, blen-len, "%d,", pl->hand[i]);
    }
    bfr[len-1] = 0;
    return len-1;
}
comm_flag_t send_GAME_STATE(int cd, char *bfr, void *data)
{
    game_t *g = (game_t*)data;
    printD("send_GAME_STATE: %d\n", g->id);
    int len = sprintf(bfr, "GAME STATE");
    for(int i=0; i<MAX_PLAYERS; i++)
    {
        if(!g->players[i]) continue;
        len += snprintf(bfr+len, BFR_LEN-len, "^%s", g->players[i]->nick);
        if(cd != g->players[i]->comm_if.cd)
        {
            len += snprintf(bfr+len, BFR_LEN-len, ":%d", player_hand_card_cnt(g->players[i]));
        }
        else
        {
            (bfr+len++)[0] = ':';
            len += hmask(g->players[i], bfr+len, BFR_LEN-len);
        }
        (bfr+len++)[0] = ':';
        len += fmask(g->players[i], bfr+len);
        (bfr+len++)[0] = ':';
        len += dmask(g->players[i], bfr+len);
 
    }
    len += snprintf(bfr+len, BFR_LEN-len, "^%d:%d:%d\x0A", 
                            game_get_top_card(g), 
                            card_stack_height(g->play_deck), 
                            card_stack_height(g->draw_deck));
    int ret = write(cd, bfr, len);
    write_check(ret);
    if(ret < strlen(bfr)) return COMM_TO;

    return ack_handle(cd, bfr);
}

