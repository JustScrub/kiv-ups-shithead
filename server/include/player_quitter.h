#ifndef __PLAYER_QUITTER_H__
#define __PLAYER_QUITTER_H__
#include "player.h"


void quitter_push(player_t *player);
player_t *quitter_pop();

#endif