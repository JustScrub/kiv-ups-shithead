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

player_t *mm_players[MAX_PLAYERS*12] = {0};
game_t *lobbies[12] = {0};
int serv_fd;


void std_test()
{
    player_t players[3];
    player_comm_if_t std_cif = 
    {
        .read_card = std_read_card,
        .rq_card = std_rq_card,
        .rq_trade = std_rq_trade,
        .read_trade = std_read_trade,
        .write = std_write,
        .tell_top = std_tell_top,
        .tell_cards = std_tell_cards
    };

    for(int i =0;i<3;i++)
    {
        player_create(players+i);
        players[i].comm_if = &std_cif;
    }

    game_t *game;
    game_create(&players[0],game);
    game_add_player(game, players+1);
    //game_add_player(game, players+2);

    game_init(game);

    game->draw_deck->head = 0;
    memset(game->players[1]->hand,0,13*sizeof(int));

    game_loop(game);
}

// game * must be valid! (store in array)
void *lobby_game(void *arg)
{
    game_t *game = (game_t *)arg;

    // while game->pl_cnt < 2: waiting for more players

    while(!(game->players[0]->comm_if->lobby_start(game->players[0]->comm_if->cd))) ;

    // still players must be > 1!
    game_init(game);
    game_loop(game);
}

// TODO: make code better
// mm_players / lobbies full?
// lobby full?
// switch player context?
void *mm_player(void *arg)
{
    player_t pl = {0}; int i; unsigned choice;
    player_create(&pl);
    pl.comm_if = NULL;
    pl.comm_if->cd = (int)arg;

    for(i=0;mm_players[i];i++);
    mm_players[i] = &pl;

    mm_win:
    pl.comm_if->tell_lobbies((int)arg, lobbies, 12);
    choice = pl.comm_if->mm_choice((int)arg);

    if(choice)
    {
        choice--;
        //if(lobbies[choice]->) check if not game full

        game_add_player(lobbies[choice],&pl);
        mm_players[i] = NULL;
        pthread_exit(NULL);
    }

    for(choice=0;lobbies[choice];choice++) ;
    game_create(&pl, lobbies[choice]);
    mm_players[i] = NULL;
    pthread_create(NULL,NULL,lobby_game,lobbies+choice);

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
    int len = sizeof(cli);
    int connfd = accept(serv_fd, (struct sockaddr*)&cli, &len);
    if (connfd < 0) {
        printf("server accept failed...\n");
        exit(0);
    }
    else
        printf("server accept the client...\n");

    pthread_create(NULL, NULL, mm_player, (void *)connfd);
}

int main()
{

    start_serv();
    for(;;) serv_accept();

    return 0;
}