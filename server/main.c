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
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

#include <signal.h>

void sigsegv_handler(int signum)
{
    //close connections etc.
    // restart? xdddd
    // -> NEVER DIE ON SEGFAULT
    exit(0);
}

struct timeval timeout = {
    .tv_sec=10
};

player_t *players[MAX_PLAYERS*MAX_GAMES] = {0};
game_t *games[MAX_GAMES] = {0};
pthread_mutex_t pl_mutex = PTHREAD_MUTEX_INITIALIZER , gm_mutex = PTHREAD_MUTEX_INITIALIZER;

int serv_fd;

// TODO: don't pass ptrs, but arr of arrs: {{id,player_cnt}, ...}
int get_lobby_games(game_t **lobbies)
{
    pthread_mutex_lock(&gm_mutex);

    int cnt = 0;
    for(int i=0; i<MAX_GAMES; i++)
    {
        if(games[i] && games[i]->state==GM_LOBBY) cnt++;
    }

    lobbies = calloc(cnt+1,sizeof(game_t *));
    cnt=0;

    for(int i=0; i<MAX_GAMES; i++)
    {
        if(games[i] && games[i]->state==GM_LOBBY) lobbies[cnt++] = games[i];
    }

    pthread_mutex_unlock(&gm_mutex);

    return cnt;
}

char check_recon_cache(recon_cache_t *cache, int *i)
{
    /*to check: nick, pid, gid and player connection must be down*/
    pthread_mutex_lock(&pl_mutex);
    player_t *pl = NULL;
    for(*i=0; *i<MAX_PLAYERS*MAX_GAMES; *i++)
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

player_comm_if_t shit_if = {
    .send_request = shit_req_send,
    .conn_state = PL_CONN_UP
};

// TODO: make code better
// lobbies full?
void *mm_player_thread(void *arg)
{
    pthread_detach(pthread_self());
    int i; int64_t choice; int idx;
    game_t **lobbies = malloc(sizeof(game_t *)); // just to free it in the first mm_win pass
    player_t *pl = (player_t *)arg;
    comm_flag_t ret;

    if(!pl) pthread_exit(NULL);
    if(*pl->nick) goto mm_win;

    pthread_mutex_lock(&pl_mutex);
    for(idx=0;players[idx];idx++);
    players[idx] = pl;
    pthread_mutex_unlock(&pl_mutex);

    *((int *)pl->nick) = pl->id;
    ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_MAIN_MENU, &pl->nick, NIC_LEN);
    if(ret != COMM_OK) goto player_exit;

    mm_win:
    free(lobbies);
    i = get_lobby_games(lobbies);
    ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_LOBBIES, lobbies, (i+1)*sizeof(game_t *));
    if(ret != COMM_OK) goto player_exit;
    ret = pl->comm_if.send_request(pl->comm_if.cd, SRRQ_MM_CHOICE, &choice, sizeof(long));
    if(ret != COMM_OK) goto player_exit;
    // Timeout -> choice = -1
    if(choice<0) goto mm_win;
    if((uint64_t)choice>MAX_GAMES) goto recon_handle;
    if(choice>i) goto mm_win; // lobby_cnt < choice < MAX_GAMES
    // 0 <= choice <= i <= MAX_GAMES


    pthread_mutex_lock(&gm_mutex);
    if(choice)
    {
        choice--;
        // check the game is still in lobby mode - might have changed where gm_mutex was unlocked
        if(lobbies[choice]->state != GM_LOBBY || 
           game_player_count(lobbies[choice]) == MAX_PLAYERS ||
           !game_add_player(lobbies[choice], pl))
        {
            ret = pl->comm_if->send_request(pl->comm_if->cd, SRRQ_WRITE, "Sorry, game started or lobby full. Choose again.", strlen("Sorry, game started or lobby full. Choose again."));
            pthread_mutex_unlock(&gm_mutex);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win;
        }

        free(lobbies);
        pthread_mutex_unlock(&gm_mutex);
        pthread_exit(NULL);
    }

    for(choice=0;choice < MAX_GAMES && games[choice];choice++) ;
    if(choice == MAX_GAMES)
    {
        ret = pl->comm_if->send_request(pl->comm_if->cd, SRRQ_WRITE, "Sorry, no more games available. Choose again.", strlen("Sorry, no more games available. Choose again."));
        pthread_mutex_unlock(&gm_mutex);
        if(ret != COMM_OK) goto player_exit;
        goto mm_win;
    }
    games[choice] = calloc(1,sizeof(game_t));
    game_create(pl, games[choice]);
    pthread_create(NULL,NULL,game_thread,lobbies[choice]);

    free(lobbies);
    pthread_mutex_unlock(&gm_mutex);
    pthread_exit(NULL);

    recon_handle:
        int reconidx = -1;
        if(!(i = check_recon_cache((recon_cache_t *)choice, &reconidx)))
        {
            ret = pl->comm_if.send_request(pl->comm_if->cd, SRRQ_RECON, "INVALID", strlen("INVALID"));
            free(choice);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win;
        }
        free(choice);

        pthread_mutex_lock(&gm_mutex);
        for(choice = 0; choice<MAX_GAMES; choice++)
        {
            if(games[choice]->id == i) break;
        }
        if(choice == MAX_GAMES || games[choice]->state == GM_FINISHED) 
        { 
            ret = pl->comm_if->send_request(pl->comm_if->cd, SRRQ_RECON, "FINISHED", strlen("FINISHED"));
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
            ret = pl->comm_if->send_request(pl->comm_if.cd, SRRQ_RECON, "INVALID", strlen("INVALID"));
            pthread_mutex_unlock(&pl_mutex);
            pthread_mutex_unlock(&gm_mutex);
            if(ret != COMM_OK) goto player_exit;
            goto mm_win;
        }
        players[reconidx]->comm_if.cd = pl->comm_if.cd;
        players[reconidx]->comm_if.conn_state = PL_CONN_UP;
        if(!game_add_player(games[choice],players[reconidx]))
        {
            ret = pl->comm_if->send_request(pl->comm_if.cd, SRRQ_RECON, "INVALID", strlen("INVALID"));
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
        free(players[idx]); players[idx] = NULL;
        free(lobbies);
        pthread_exit(NULL);
}

void *player_quitter(void *arg) {
    pthread_detach(pthread_self());
    player_t *pl;
    while(1) {
        if(queue_pop(pl,Q_quiter)) {
            if(pl->comm_if.conn_state != PL_CONN_UP) break;
            player_clear(pl);
            pthread_create(NULL,NULL,mm_player_thread,pl);
        }
        sleep(1);
    }
}

void *game_deleter(void *arg)
{
    pthread_detach(pthread_self());
    int gm;
    while(1) {
        if(queue_pop(&gm,Q_game_del)) {

            pthread_mutex_lock(&pl_mutex);
            for(int i=0; i<MAX_PLAYERS*MAX_GAMES; i++)
                if(players[i] && players[i]->game_id == gm)
                    if(players[i]->comm_if.conn_state == PL_CONN_UP)
                        queue_push(players[i],Q_quiter); // quit any players still in the game (somehow)
                    else
                        { free(players[i]); players[i] = NULL; }
            pthread_mutex_unlock(&pl_mutex);

            pthread_mutex_lock(&gm_mutex);
            for(int i=0; i<MAX_GAMES; i++)
                if(games[i] && games[i]->id == gm)
                {
                    game_delete(games[i]);
                    games[i] = NULL;
                    break;
                }
            pthread_mutex_unlock(&gm_mutex);
        }
        sleep(1);
    }
}

void start_serv()
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(1234);
   
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

void serv_accept()
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

    pthread_create(NULL, NULL, mm_player_thread, (void *)pl);
}

int main()
{
    signal(SIGSEGV, sigsegv_handler);

    init_queues();
    pthread_create(NULL,NULL,player_quitter,NULL);
    pthread_create(NULL,NULL,game_deleter,NULL);

    start_serv();
    for(;;) serv_accept();

    return 0;
}