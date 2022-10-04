#include "include/player.h"
#include "string.h"
static int pl_id = 0;

void player_create(player_t *out)
{
    out->id = ++pl_id;
    memset(out->hand, 0, 12*sizeof(int));
    memset(out->face_up, INVALID_CARD, 3*sizeof(card_t));
    memset(out->face_down, INVALID_CARD, 3*sizeof(card_t));
    out->game_id = -1;
    out->state = PL_MAIN_MENU;
}


int player_hand_card_cnt(player_t *player)
{
    int s = 0;
    for(int i=0;i<12;i++)
    {
        s += player->hand[i];
    }
    return s;
}

bool player_has_card(player_t *player, card_t card, int cnt)
{
    int real_cnt = 0;
    int i;
    if(cnt <= 0) return false; 
    int from = player_plays_from(player);
    if(from == PL_PILE_NONE) return false;

    if(from == PL_PILE_HAND)
    {
        real_cnt = player->hand[card];
        goto end_check;
    }
    
    if(from == PL_PILE_F_UP)
    {
        for(i = 0; i < 3; i++)
        {
            if(player->face_up[i] == card ) real_cnt++;
        }
        goto end_check;
    }
    

    for(i = 0; i < 3; i++)
    {
        if(player->face_down[i] == card ) real_cnt++;
    }
    
    end_check:
    if(real_cnt >= cnt) return true;  // has at least the required amount of the cards
    return false;
}

int player_plays_from(player_t *player)
{
    for(card_t c=0; c<12;c++)
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
            player->hand[card]--;
        }
        if(from == PL_PILE_F_UP)
        {
            for(int i=0;i<3;i++)
            {
                if(player->face_up[i] == card) player->face_up[i] == INVALID_CARD;
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
    for(; cnt > 0, card_is_valid(card); cnt--)
    {
        player->hand[card_stack_pop(draw_deck)-2]++;
        card = card_stack_peek(draw_deck, 0);
    }
}

void player_trade_cards(player_t *player)
{
    int j; card_t card;
    player->comm_if->rq_trade();
    player->comm_if->read_trade(&j);
    for(int i=0; i<3; i++)
    {
        card = ((card_t *)&j)[i];
        if(!card_is_valid(card)) continue;
        if(!player->hand[card])
        {
            player->comm_if->write("You don't have that card");
            break; //tries to cheat -> cannot trade
        }
        // switching
        player->hand[player->face_up[i]]++;
        player->hand[card]--;
        player->face_up[i] = card;
    }
}
