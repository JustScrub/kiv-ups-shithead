#include "include/player.h"

static pl_id = 0;

void player_create(player_t *out)
{
    out->id = ++pl_id;
    out->game_id = -1;
    out->state = PL_MAIN_MENU;
}
