#ifndef __PLAYER_QUITTER_H__
#define __PLAYER_QUITTER_H__
#include "player.h"
#include "pthread.h"

typedef struct {
    void *data;
    int element_size;
    int size;
    int start;
    int cnt;
    pthread_mutex_t mut;
} queue_t;

typedef enum {
    Q_quiter = 0,
    Q_game_del
} game_queues_t;

void init_queues();

char queue_push(void *elem, game_queues_t q);
char queue_pop(void *elem, game_queues_t q);



#endif