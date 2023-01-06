#include "include/player_quitter.h"
#include "include/config.h"
#include "include/player.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

queue_t glob_queues[] = {
    [Q_quiter] = {
        .data = NULL,
        .element_size = sizeof(unsigned),
        .size = MAX_PLAYERS*MAX_GAMES,
        .start = 0,
        .cnt = 0
    },
    [Q_game_del] = {
        .data = NULL,
        .element_size = sizeof(unsigned),
        .size = MAX_GAMES,
        .start = 0,
        .cnt = 0
    }
};

void init_queues()
{
    for(int i=0; i<sizeof(glob_queues)/sizeof(queue_t); i++)
    {
        glob_queues[i].data = calloc(glob_queues[i].size,glob_queues[i].element_size);
        pthread_mutex_init(&glob_queues[i].mut,NULL);
    }
}

char queue_push(void *elem, game_queues_t q)
{
    pthread_mutex_lock(&glob_queues[q].mut);
    if(glob_queues[q].cnt==glob_queues[q].size) {
        pthread_mutex_unlock(&glob_queues[q].mut);
        return 0;
    }
    int pos = (glob_queues[q].start+glob_queues[q].cnt) % glob_queues[q].size;
    pos *= glob_queues[q].element_size;
    printD("queue_push: Q=%c pos=%d\n",'Q' - q*13, pos);
    memcpy(((char *)(glob_queues[q].data))+pos,elem,glob_queues[q].element_size);
    glob_queues[q].cnt++;
    pthread_mutex_unlock(&glob_queues[q].mut);
    return 1;
}

char queue_pop(void *elem, game_queues_t q)
{
    pthread_mutex_lock(&glob_queues[q].mut);
    if(glob_queues[q].cnt==0) {
        pthread_mutex_unlock(&glob_queues[q].mut);
        return 0;
    }
    int pos = glob_queues[q].start * glob_queues[q].element_size;
    printD("queue_pop: Q=%c pos=%d\n", 'Q' - q*13,pos);
    void *pl = ((char *)(glob_queues[q].data))+pos;
    memcpy(elem,pl,glob_queues[q].element_size);
    glob_queues[q].start = (glob_queues[q].start+1) % glob_queues[q].size;
    glob_queues[q].cnt--;
    pthread_mutex_unlock(&glob_queues[q].mut);
    return 1;
}


/*
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
*/