#include "include/game.h"
#include "include/card.h"
#include "include/player.h"
#include "include/comm_if.h"
#include "include/player_quitter.h"

#include "shithead_protocol_comm_if.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <signal.h>
#include <stdio.h>

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

void sigsegv_handler(int signum)
{
    //close connections etc.
    // restart? xdddd
    // -> NEVER DIE ON SEGFAULT
    if(signum == SIGSEGV)
        exit(0);
}

player_t *players[MAX_PLAYERS*MAX_GAMES] = {0};
game_t *games[MAX_GAMES] = {0};
pthread_mutex_t pl_mutex = PTHREAD_MUTEX_INITIALIZER , gm_mutex = PTHREAD_MUTEX_INITIALIZER;

int serv_fd;

int get_lobby_games(lobby_info_t **lobbies)
{
    pthread_mutex_lock(&gm_mutex);

    int cnt = 0;
    for(int i=0; i<MAX_GAMES; i++)
    {
        if(games[i] && games[i]->state==GM_LOBBY && games[i]->players[0]) {
            cnt++;
        }
    }

    *lobbies = calloc(cnt+1,sizeof(lobby_info_t));
    if(!*lobbies) {pthread_mutex_unlock(&gm_mutex); return -1;}
    if(!cnt) {pthread_mutex_unlock(&gm_mutex); return 1;}
    printD("get_lobbies: found %d lobbies\n", cnt);
    cnt=0;

    for(int i=0; i<MAX_GAMES; i++)
    {
        if(!games[i] || games[i]->state!=GM_LOBBY || !games[i]->players[0]) continue;
        (*lobbies)[cnt].pl_cnt = game_player_count(games[i]);
        strncpy((*lobbies)[cnt].owner_nick, games[i]->players[0]->nick, NIC_LEN);
        (*lobbies)[cnt].gid = games[i]->id;
        cnt++;
    }

    pthread_mutex_unlock(&gm_mutex);

    return cnt+1;
}

char check_recon_cache(recon_cache_t *cache, int *i)
{
    /*to check: nick, pid, gid and player connection must be down*/
    pthread_mutex_lock(&pl_mutex);
    player_t *pl = NULL;
    for(*i=0; *i<MAX_PLAYERS*MAX_GAMES; (*i)++)
    {
        if(players[*i] && players[*i]->id==cache->id)
        {
            pl = players[*i];
            break;
        }
    }

    if( !pl || 
        pl->game_id != cache->gid || 
        pl->comm_if.conn_state == PL_CONN_UP ||
        strncmp(pl->nick, cache->nick,NIC_LEN))
    {
        pthread_mutex_unlock(&pl_mutex);
        *i = -1;
        return 0;
    }
    pthread_mutex_unlock(&pl_mutex);
    return cache->gid;
}


// TODO: make code better
// lobbies full?
void *mm_player_thread(void *arg)
{
    pthread_detach(pthread_self());
    int i; int64_t choice; int idx = -1;
    lobby_info_t *lobbies = malloc(sizeof(lobby_info_t)); // just to free it in the first mm_win pass
    player_t *pl = (player_t *)arg;
    comm_flag_t ret; pthread_t tid;

    if(!pl) pthread_exit(NULL);
    pl->state = PL_MAIN_MENU;
    if(*pl->nick) {
        printD("Return to MM: %s\n",pl->nick);
        for(idx=0; idx<MAX_PLAYERS*MAX_GAMES; idx++)
            if(players[idx] && players[idx]->id == pl->id) break;
        if(idx == MAX_PLAYERS*MAX_GAMES) goto player_exit;
        goto mm_win;
    }

    pthread_mutex_lock(&pl_mutex);
    for(idx=0;players[idx];idx++);
    players[idx] = pl;
    pthread_mutex_unlock(&pl_mutex);

    *((int *)pl->nick) = pl->id;
    ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_MAIN_MENU, &pl->nick, NIC_LEN);
    if(ret != COMM_OK) goto player_exit;
    printD("New player: %s\n", pl->nick);

    mm_win:
    free(lobbies);
    if((i = get_lobby_games(&lobbies)) < 0) goto mm_win;

    ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_LOBBIES, lobbies, (i)*sizeof(lobby_info_t));
    if(ret != COMM_OK) goto player_exit;
    ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_MM_CHOICE, &choice, sizeof(int64_t));
    if(ret != COMM_IGN && ret != COMM_OK) goto player_exit;
    // Ignore -> choice = -1
    if(/*choice<0 ||*/ ret == COMM_IGN) goto mm_win;
    if((uint64_t)choice>MAX_GAMES) goto recon_handle;
    if(choice>i) goto mm_win; // lobby_cnt < choice <= MAX_GAMES
    // 0 <= choice <= lobby_cnt <= MAX_GAMES

    pthread_mutex_lock(&gm_mutex);
    if(choice){
        choice--;
        for(i=0;i<MAX_GAMES;i++){
            if(games[i] && games[i]->id == lobbies[choice].gid){
                choice = i;
                break;
            }
        }
        if(i==MAX_GAMES) {
            ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_WRITE, "Lobby no longer available. Choose again.", sizeof("Lobby no longer available. Choose again."));
            pthread_mutex_unlock(&gm_mutex);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win;
        }

        // check the game is still in lobby mode - might have changed where gm_mutex was unlocked
        if(games[choice]->state != GM_LOBBY || 
           game_player_count(games[choice]) == MAX_PLAYERS ||
           !game_add_player(games[choice], pl)){
            ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_WRITE, "Sorry, game started or lobby full. Choose again.", sizeof("Sorry, game started or lobby full. Choose again."));
            pthread_mutex_unlock(&gm_mutex);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win;
        }

        free(lobbies);
        pthread_mutex_unlock(&gm_mutex);
        pthread_exit(NULL);
    }

    for(choice=0;choice < MAX_GAMES && games[choice];choice++) ;
    if(choice == MAX_GAMES){
        ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_WRITE, "Sorry, no more games available. Choose again.", sizeof("Sorry, no more games available. Choose again."));
        pthread_mutex_unlock(&gm_mutex);
        if(ret != COMM_OK) goto player_exit;
        goto mm_win;
    }
    games[choice] = calloc(1,sizeof(game_t));
    game_create(pl, games[choice]);
    pthread_create(&tid,NULL,game_thread,games[choice]);

    free(lobbies);
    pthread_mutex_unlock(&gm_mutex);
    pthread_exit(NULL);

    recon_handle: ;
        int reconidx = -1;
        if(!(i = check_recon_cache((recon_cache_t *)choice, &reconidx))){
            ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_RECON, "I", 1);
            free((recon_cache_t *)choice);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win;
        }
        free((recon_cache_t *)choice);

        pthread_mutex_lock(&gm_mutex);
        for(choice = 0; choice<MAX_GAMES; choice++){
            if(games[choice]->id == (unsigned)i) break;
        }
        if(choice == MAX_GAMES || games[choice]->state != GM_PLAYING) 
        { 
            ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_RECON, "F", 1);
            pthread_mutex_unlock(&gm_mutex);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win; // game finished
        }
        
        /* Now, we must decide whether to use old player struct or new one
           can go to game -> old one (with cards etc) 
           cannot be added -> new one (not initialized yet)
        */
        pthread_mutex_lock(&pl_mutex);
        if(!players[reconidx]) // might have gotten deletd
        {
            ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_RECON, "I", 1);
            pthread_mutex_unlock(&pl_mutex);
            pthread_mutex_unlock(&gm_mutex);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win;
        }
        players[reconidx]->comm_if.cd = pl->comm_if.cd;
        players[reconidx]->id = pl->id;
        //players[reconidx]->comm_if.conn_state = PL_CONN_UP;
        if(!game_add_player(games[choice],players[reconidx]))
        {
            ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_RECON, "I", 1);
            free(players[reconidx]); players[reconidx] = NULL;
            pthread_mutex_unlock(&pl_mutex);
            pthread_mutex_unlock(&gm_mutex);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win; // game full
        }
        free(players[idx]); players[idx] = NULL;
        pthread_mutex_unlock(&pl_mutex);
        pthread_mutex_unlock(&gm_mutex);
        free(lobbies);
        pthread_exit(NULL);

    player_exit:
        //free(players[idx]); players[idx] = NULL;
        if(idx >= 0 && idx < MAX_PLAYERS*MAX_GAMES){
            pthread_mutex_lock(&pl_mutex);
            players[idx] = NULL;
            pthread_mutex_unlock(&pl_mutex);
        }
        printD("mm player_exit: nick=%s,idx=%d,ret=%d\n", pl->nick,idx, ret);
        if(pl->comm_if.conn_state != PL_CONN_DOWN) close(pl->comm_if.cd);
        free(pl);
        free(lobbies);
        pthread_exit(NULL);
}

/* pushes players from lobby or game to main menu */
void *player_quitter(void *arg) {
    pthread_detach(pthread_self());
    player_t *pl = NULL; pthread_t tid;
    unsigned plid = 0;
    while(1) {
        if(queue_pop(&plid,Q_quiter)) {
            pl = NULL;
            pthread_mutex_lock(&pl_mutex);

            for(int i=0; i<MAX_PLAYERS*MAX_GAMES; i++)
                if(players[i] && players[i]->id == plid) {
                    plid = i;
                    pl = players[i]; break;
                }
            if(!pl) { pthread_mutex_unlock(&pl_mutex); continue; }

            printD("player_quitter: quit=%s\n", pl->nick);
            if(pl->state == PL_MAIN_MENU)  // already quit
                {pthread_mutex_unlock(&pl_mutex);continue;}
            if(pl->comm_if.conn_state == PL_CONN_DOWN) {  // free disconnected players in lobby
                if(pl->state != PL_LOBBY && pl->state != PL_LOBBY_OWNER) 
                    {pthread_mutex_unlock(&pl_mutex);continue;}

                printD("player_quitter: free disconnected lobby player=%s\n", pl->nick);
                free(players[plid]); players[plid] = NULL;
                pthread_mutex_unlock(&pl_mutex);
                continue;
            }
            player_clear(pl);
            pthread_create(&tid,NULL,mm_player_thread,pl);
            pthread_mutex_unlock(&pl_mutex);
            printD("player_quitter: quit=%s done\n", pl->nick);
        }
        else sleep(1);
    }
    return arg;
}

void *game_deleter(void *arg)
{
    pthread_detach(pthread_self());
    unsigned gm;
    while(1) {
        //sleep(1);
        //continue;
        if(queue_pop(&gm,Q_game_del)) {
            printD("game_deleter: delete=%d\n", gm);
            pthread_mutex_lock(&gm_mutex);
            for(int i=0; i<MAX_GAMES; i++)
                if(games[i] && games[i]->id == gm)
                {
                    for(int j=0; j<MAX_PLAYERS; j++)
                        if(games[i]->players[j] && games[i]->players[j]->comm_if.conn_state == PL_CONN_UP)
                            queue_push(&games[i]->players[j]->id, Q_quiter);

                    game_delete(games[i]);
                    games[i] = NULL;
                    break;
                }
            pthread_mutex_unlock(&gm_mutex);

            pthread_mutex_lock(&pl_mutex);
            for(int i=0; i<MAX_PLAYERS*MAX_GAMES; i++)
                if(players[i] && players[i]->game_id == gm)
                {
                    if(players[i]->comm_if.conn_state == PL_CONN_DOWN)
                        { free(players[i]); players[i] = NULL; }
                }
            pthread_mutex_unlock(&pl_mutex);

            printD("game_deleter: delete=%d done\n", gm);
        }
        else sleep(1);
    }
    return arg;
}

/**
 * @TODO
 * PARAMETRIZE IP, PORT
 * 
 */

void start_serv(char *ip_s, char *port_s)
{
    int sockfd;
    struct sockaddr_in servaddr;
   
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
   
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    if(!inet_aton(ip_s, &servaddr.sin_addr)) {
        printf("Invalid IP address\n");
        exit(0);
    }
    int port = atoi(port_s);
    servaddr.sin_port = htons((uint16_t)port);
   
    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");
   
    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");

    serv_fd = sockfd;
}

#define TO_SEC 24
struct timeval timeout = {
    .tv_sec=TO_SEC,
};


player_comm_if_t shit_if = {
    .send_request = shit_req_send,
    .conn_state = PL_CONN_UP
};

void serv_accept(pthread_t *tid)
{
    struct sockaddr_in cli;
    unsigned len = sizeof(cli);
    int connfd = accept(serv_fd, (struct sockaddr*)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");

    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    player_t *pl = calloc(1, sizeof(player_t)); 
    player_create(pl);
    pl->comm_if = shit_if;
    pl->comm_if.cd = connfd;
    printD("serv_acc: id=%d\n", pl->id);

    pthread_create(tid, NULL, mm_player_thread, (void *)pl);
    //close(connfd);
    //free(pl);
}

int main(int argc, char **argv)
{
    if(argc != 3) {
        printf("Usage: %s <ip> <port>\n", argv[0]);
        printf("IP must be in IPv4 format as specified by inet_aton(3).\n");
        exit(0);
    }
    //signal(SIGSEGV, sigsegv_handler);
    signal(SIGPIPE, SIG_IGN);

    pthread_t tid;

    init_queues();
    pthread_create(&tid,NULL,player_quitter,NULL);
    pthread_create(&tid,NULL,game_deleter,NULL);

    start_serv(argv[1], argv[2]);
    for(;;) serv_accept(&tid);

    return 0;
}