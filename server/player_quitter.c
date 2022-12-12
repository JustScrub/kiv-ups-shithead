#include "include/player_quitter.h"
#include "include/config.h"
#include "include/player.h"
#include "pthread.h"

player_t *quit_queue[MAX_PLAYERS*MAX_GAMES] = {0};
int quit_queue_cnt = 0;
int queue_start = 0;

pthread_mutex_t qq_mutex = PTHREAD_MUTEX_INITIALIZER;

player_t *quitter_pop()
{
    pthread_mutex_lock(&qq_mutex);
    if(quit_queue_cnt==0) {
        pthread_mutex_unlock(&qq_mutex);
        return NULL;
    }
    player_t *pl = quit_queue[queue_start];
    quit_queue[queue_start] = NULL;
    queue_start = (queue_start+1) % (MAX_PLAYERS*MAX_GAMES);
    quit_queue_cnt--;
    pthread_mutex_unlock(&qq_mutex);
    return pl;
}

void quitter_push(player_t *pl)
{
    pthread_mutex_lock(&qq_mutex);
    if(quit_queue_cnt==MAX_PLAYERS*MAX_GAMES) {
        pthread_mutex_unlock(&qq_mutex);
        return;
    }
    quit_queue[(queue_start+quit_queue_cnt) % (MAX_PLAYERS*MAX_GAMES)] = pl;
    quit_queue_cnt++;
    pthread_mutex_unlock(&qq_mutex);
}