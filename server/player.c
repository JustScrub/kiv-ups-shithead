#include "include/player.h"
#include "string.h"
#include <stdio.h>
static int pl_id = 0;

void player_create(player_t *out)
{
    out->id = ++pl_id;
    memset(out->nick,0,NIC_LEN+1);
    player_clear(out);
}

void player_clear(player_t *player)
{
    memset(player->hand, 0, 13*sizeof(int));
    memset(player->face_up, INVALID_CARD, 3*sizeof(card_t));
    memset(player->face_down, INVALID_CARD, 3*sizeof(card_t));
    player->game_id = -1;
    player->state = PL_MAIN_MENU;
}

int player_hand_card_cnt(player_t *player)
{
    int s = 0;
    for(int i=0;i<13;i++)
    {
        s += player->hand[i];
    }
    return s;
}

bool player_has_card(player_t *player, card_t card, int cnt)
{
    int real_cnt = 0;
    int i;
    if(cnt <= 0 || !card_is_valid(card)) return false; 
    int from = player_plays_from(player);
    if(from == PL_PILE_NONE) return false;

    if(from == PL_PILE_HAND)
        real_cnt = player->hand[card-2];
    else if(from == PL_PILE_F_UP)
        for(i = 0; i < 3; i++) real_cnt +=   player->face_up[i] == card;
    else 
        for(i = 0; i < 3; i++) real_cnt += player->face_down[i] == card;

    return real_cnt >= cnt;  // has at least the required amount of the cards
}

int player_plays_from(player_t *player)
{
    for(card_t c=0; c<13;c++)
    {
        if(player->hand[c]) return PL_PILE_HAND;
    }

    for(int i=0;i<3;i++) if(card_is_valid( player->face_up[i] )) return PL_PILE_F_UP;
    for(int i=0;i<3;i++) if(card_is_valid( player->face_down[i] )) return PL_PILE_F_DWN;

    player->state = PL_DONE;
    return PL_PILE_NONE;

}

bool player_play_cards(player_t *player, card_t card, int cnt, card_stack_t *play_deck)
{
    printD("play cards: %s %d %d\n", player->nick, card, cnt);
    if(!player_has_card(player, card, cnt)) {printD("play cards: does not have\n");return false;}
    // this if is outside the for loop =>
        // even if has 4 times the card, but 3 in hand and 1 on face up, they can play only 3 from hand
        // and wait for the next turn to play the last one from face up

    int from = player_plays_from(player);
    
    for(; cnt>0; cnt--)
    {
        if(from == PL_PILE_HAND)
        {
            player->hand[card-2]--;
        }
        if(from == PL_PILE_F_UP)
        {
            for(int i=0;i<3;i++)
            {
                if(player->face_up[i] == card) 
                {
                    player->face_up[i] = INVALID_CARD;
                    break;
                }
            }
        }
        if(from == PL_PILE_F_DWN)
        {
            for(int i=0;i<3;i++)
            {
                if(player->face_down[i] == card) player->face_down[i] = INVALID_CARD;
            }
        }
        
        card_stack_push(play_deck, card);
    }
    return 0;
}

bool player_draw_cards(player_t *player, int cnt, card_stack_t *draw_deck)
{
    if(!player) return false;
    card_t card = card_stack_peek(draw_deck, 0);
    for(; cnt > 0; cnt--)
    {
        if(!card_is_valid(card)) break;
        player->hand[card_stack_pop(draw_deck)-2]++;
        card = card_stack_peek(draw_deck, 0);
    }
    return true;
}

char player_secret_face_down(player_t *player)
{
    return      (card_is_valid(player->face_down[2]))<<2 | 
                (card_is_valid(player->face_down[1]))<<1 | 
                (card_is_valid(player->face_down[0]))<<0 ;
}

void player_put_to_hand(player_t *player, int f_down_idx)
{
    player->hand[player->face_down[f_down_idx] -2]++;
    player->face_down[f_down_idx] = INVALID_CARD;
}
