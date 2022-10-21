#include "include/player.h"
#include "string.h"
static int pl_id = 0;

void player_create(player_t *out)
{
    out->id = ++pl_id;
    memset(out->hand, 0, 13*sizeof(int));
    memset(out->face_up, INVALID_CARD, 3*sizeof(card_t));
    memset(out->face_down, INVALID_CARD, 3*sizeof(card_t));
    out->game_id = -1;
    out->state = PL_MAIN_MENU;
    out->conn_state = PL_CONN_UP;
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
    if(!player_has_card(player, card, cnt)) return false;

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
                if(player->face_up[i] == card) player->face_down[i] == INVALID_CARD;
            }
        }
        
        card_stack_push(play_deck, card);
    }
    return 0;
}

bool player_draw_cards(player_t *player, int cnt, card_stack_t *draw_deck)
{
    card_t card = card_stack_peek(draw_deck, 0);
    for(; cnt > 0; cnt--)
    {
        if(!card_is_valid(card)) break;
        player->hand[card_stack_pop(draw_deck)-2]++;
        card = card_stack_peek(draw_deck, 0);
    }
    return true;
}

void player_trade_cards(player_t *player)
{
    int j; card_t card;
    player->comm_if->rq_trade(player->comm_if->cd);
    player->comm_if->read_trade(player->comm_if->cd,&j);
    for(int i=0; i<3; i++)
    {
        card = ((card_t *)&j)[i];
        if(!card_is_valid(card)) continue;
        if(!player->hand[card-2])
        {
            player->comm_if->write(player->comm_if->cd,"You don't have that card");
            break; //tries to cheat -> cannot trade
        }
        // switching
        player->hand[player->face_up[i] -2]++;
        player->hand[card-2]--;
        player->face_up[i] = card;
    }
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
