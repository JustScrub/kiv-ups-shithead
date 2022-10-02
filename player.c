#include "include/player.h"

static pl_id = 0;

void player_create(player_t *out)
{
    out->id = ++pl_id;
    out->game_id = -1;
    out->state = PL_MAIN_MENU;
}


int player_hand_card_cnt(player_t *player)
{
    int s = 0;
    for(int i=0;i<52;i++)
    {
        s += player->hand[i];
    }
    return s;
}

/**
 * @brief Checks if player has card and returns 0 iff not, else a positive int
 * 
 * return values: 0=does not have, 1=hand, 2=f-up, 3=f-down
 * 
 * @param player 
 * @param card_idx = value returned by \c card_get_value(card)
 * @return int 
 */
int player_has_card(player_t *player, char card_idx)
{
    // too lazy to make loops. Sry ¯\_(ツ)_/¯
    if(
        player->hand[card_idx   ] ||
        player->hand[card_idx+13] ||
        player->hand[card_idx+26] ||
        player->hand[card_idx+39]
    ) return 1;

    if(
        card_get_value( player->face_up[0] ) == card_idx ||
        card_get_value( player->face_up[1] ) == card_idx ||
        card_get_value( player->face_up[2] ) == card_idx
    ) return 2;

    if(
        card_get_value( player->face_down[0] ) == card_idx ||
        card_get_value( player->face_down[1] ) == card_idx ||
        card_get_value( player->face_down[2] ) == card_idx
    ) return 3;

    return 0;
}

int player_plays_from(player_t *player)
{
    for(card_t c=0; c<52;c++)
    {
        if(player->hand[c]) return 1;
    }

    for(int i=0;i<3;i++) if(player->face_up[i] != INVALID_CARD) return 2;
    for(int i=0;i<3;i++) if(player->face_down[i] != INVALID_CARD) return 3;

    player->state = PL_DONE;
    return 0;

}

void player_trade_cards(player_t *player)
{
    int j; card_t card;
    player->comm_if->rq_trade();
    player->comm_if->read_trade(&j);
    for(int i=0; i<3; i++)
    {
        card = ((card_t *)j)[i];
        if(card == INVALID_CARD) continue;
        if(player_has_card(player, card_get_value(card)) != 1)
        {
            player->comm_if->write("You don't have that card");
            break; //tries to cheat -> cannot trade
        }
        // switching
        player->hand[player->face_up[i]] = true;
        player->hand[card] = false;
        player->face_up[i] = card;
    }
}
