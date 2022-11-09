#include "include/game.h"
#include "include/card.h"
#include "include/player.h"

#include "std_io_comm_if.h"

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include <signal.h>

/***********************************
TODO:
modify communication IF
    - 2 functions: 
        - initiate request (which to send) - sends rq and handles reply
        - handle request (which to handle) - reads specific request and replies. Must also check legal request (bit field?)
    - create matrix of [PL_STATE][REQUEST] = LEGAL/ILLEGAL in player.c

It is deterministic, when each request arrives (there's no request that can appear out of thin air)
    - even QUIT, it is read on player's turn

after timeout, ping client
************************************/

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

void *game_thread(void *arg)
{
    pthread_detach(pthread_self());
    game_t *game = (game_t *)arg;
    char *s = malloc(1); *s=0;
    if(!game)
    {
        //log error
        pthread_exit(NULL);
    }
    game->state = GM_LOBBY;
    for(;;)
    {
        while(game_player_count(game) <2)
        {
            sleep(1);
        }

        // wait for owner to start the game
        //while(!(game->players[0]->comm_if->lobby_start(game->players[0]->comm_if->cd))) ;
        for(;!*s;game->players[0]->comm_if->handle_request(game->players[0]->comm_if->cd, allowed_reqs(PL_LOBBY_OWNER),s));

        if(game_player_count(game) >=2) break;
    }
    free(s);
    game->state = GM_PREPARE;
    game_init(game);
    game_loop(game);
}

int get_lobby_games(game_t **lobbies)
{
    pthread_mutex_lock(&gm_mutex);

    int cnt = 0;
    for(int i=0; i<MAX_GAMES; i++)
    {
        if(games[i] && games[i]->state==GM_LOBBY) cnt++;
    }

    lobbies = calloc(cnt,sizeof(game_t *));
    cnt=0;

    for(int i=0; i<MAX_GAMES; i++)
    {
        if(games[i] && games[i]->state==GM_LOBBY) lobbies[cnt++] = games[i];
    }

    pthread_mutex_unlock(&gm_mutex);

    return cnt;
}

// TODO: make code better
// mm_players / lobbies full?
void *mm_player_thread(void *arg)
{
    pthread_detach(pthread_self());
    player_t *pl = calloc(1, sizeof(player_t)); 
    int i; unsigned choice;
    game_t **lobbies = NULL;

    player_create(&pl);
    pl->comm_if = NULL;
    pl->comm_if->cd = (int)arg;

    pthread_mutex_lock(&pl_mutex);
    for(i=0;players[i];i++);
    players[i] = pl;
    pthread_mutex_unlock(&pl_mutex);

    pl->comm_if->send_request(pl->comm_if->cd, SRRQ_MAIN_MENU, NULL);
    mm_win:
    choice = get_lobby_games(lobbies);
    /*pl->comm_if->tell_lobbies((int)arg, lobbies, choice);
    choice = pl->comm_if->mm_choice((int)arg); // blocks*/
    pl->comm_if->handle_request(pl->comm_if->cd, allowed_reqs(PL_MAIN_MENU),&choice);
    if(choice<0) goto mm_win;


    pthread_mutex_lock(&gm_mutex);
    if(choice)
    {
        choice--;
        // check the game is still in lobby mode - might have changed where gm_mutex was unlocked
        if(lobbies[choice]->state != GM_LOBBY || game_player_count(lobbies[choice]) == MAX_PLAYERS)
        {
            //pl->comm_if->write(pl->comm_if->cd, "Sorry, game started or lobby full. Choose again.");
            pl->comm_if->send_request(pl->comm_if->cd, SRRQ_WRITE, "Sorry, game started or lobby full. Choose again.");


            free(lobbies);
            pthread_mutex_unlock(&gm_mutex);
            goto mm_win;
        }

        game_add_player(lobbies[choice],&pl);

        free(lobbies);
        pthread_mutex_unlock(&gm_mutex);
        pthread_exit(NULL);
    }

    for(choice=0;games[choice];choice++) ;
    game_create(&pl, games[choice]);
    pthread_create(NULL,NULL,game_thread,lobbies[choice]);

    free(lobbies);
    pthread_mutex_unlock(&gm_mutex);
    pthread_exit(NULL);
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
    pthread_create(NULL, NULL, mm_player_thread, (void *)connfd);
}

int main()
{
    signal(SIGSEGV, sigsegv_handler);
    games_init();
    start_serv();
    for(;;) serv_accept();

    return 0;
}