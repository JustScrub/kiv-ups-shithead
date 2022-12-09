#include "include/game.h"
#include "include/card.h"
#include "include/player.h"
#include "include/comm_if.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

static int gm_id = 0;

short allowed_requests_matrix[6];

void games_init()
{
    allowed_requests_matrix[PL_MAIN_MENU] = PLRQ_DEFAULT|PLRQ_RECON|PLRQ_LOBBY|PLRQ_MM_CHOICE;
    allowed_requests_matrix[PL_LOBBY] = PLRQ_DEFAULT;
    allowed_requests_matrix[PL_LOBBY_OWNER] = PLRQ_DEFAULT|PLRQ_GAME_START;
    allowed_requests_matrix[PL_PLAYING_WAITING] = PLRQ_DEFAULT|PLRQ_GAME_STATE|PLRQ_TOP_CARD;
    allowed_requests_matrix[PL_PLAYING_ON_TURN] = PLRQ_DEFAULT|PLRQ_GAME_STATE|PLRQ_TOP_CARD;
    allowed_requests_matrix[PL_DONE] = PLRQ_DEFAULT;
}

short allowed_reqs(player_state_t state)
{
    return allowed_requests_matrix[state];
}

void game_create(player_t *owner, game_t *out)
{
    out->id = ++gm_id;
    out->state = GM_LOBBY;
    out->draw_deck = card_stack_create();
    out->play_deck = card_stack_create();
    out->players = calloc(MAX_PLAYERS,sizeof(player_t *));
    out->players[0] = owner;
    owner->game_id = out->id;
    owner->state = PL_LOBBY_OWNER;
}

bool game_add_player(game_t *game,player_t *pl)
{
    int i;
    for(i=1;i<MAX_PLAYERS, game->players[i];i++);

    if(i<=MAX_PLAYERS)
    {
        game->players[i] = pl;
        pl->game_id = game->id;
        pl->state = PL_LOBBY;
        return true;
    }
    return false;
}

int game_player_count(game_t *game)
{
    int cnt = 0;
    for(int i; i<MAX_PLAYERS;i++)
    {
        if(game->players[i] && game->players[i]->comm_if->conn_state)
        {
            cnt++;
        }
    }
    return cnt;
}

void game_delete(game_t *game)
{
    free(game->draw_deck);
    free(game->play_deck);
    for(int i=0; i<MAX_PLAYERS; i++)
    {
        if(game->players[i])
        {
            game->players[i]->game_id = -1;
        }
    }
    free(game->players);
    free(game);
}

void game_init(game_t *game)
{
    for (int p=0;p<MAX_PLAYERS; p++)
    {
        game->players[p] && (game->players[p]->state=PL_PLAYING_WAITING);
        // set "playing" status to all the players
        // lazy eval -> if the addr is NULL, skips the assignment
    }

    // fill cards + shuffle
    for(int i=0; i < DECK_NUM*52 ;i++)
    {
        card_stack_push(game->draw_deck, (i%13)+2);
    }
    card_stack_shuffle(game->draw_deck);

    int offsets[] = {
        offsetof(player_t, face_down),
        offsetof(player_t, face_up),
    };
    int sizes[] = {3,3};

    // draw face up/down cards
    for(int i=0; i<2; i++)
    {
        for(int draw_idx = 0; draw_idx < sizes[i]; draw_idx++)
        {
            for(int p=0; p <MAX_PLAYERS; p++)
            {
                if(game->players[p])
                {
                    ((card_t *)((char *)game->players[p] + offsets[i]))[draw_idx] = card_stack_pop(game->draw_deck);
                    //          ^player_t address | ^offset of 
                    //           (struct)         |  the card array in the player_t struct
                }
            }
        }
    }

    for(int p=0; p <MAX_PLAYERS; p++)
        {
            if(game->players[p])
            {
                player_draw_cards(game->players[p],3,game->draw_deck);
            }
        }

    game->active_8 = false;
}

card_t game_get_top_card(game_t *game)
{
    int i;
    for(i=0; card_stack_peek(game->play_deck,i)==3; i++)
        ;
    return card_stack_peek(game->play_deck,i);
}

int game_check_cannot_play(game_t *game, player_t *player)
{
    card_t card = game_get_top_card(game);
    if(!card_is_valid(card)) return 0;

    // if plays from face-down, can always play
    // whether the chosen card is legal is decided later on
    if(player_plays_from(player) == PL_PILE_F_DWN) return 0;

    //check for 8
    if(game->active_8 && !player_has_card(player,8,1))
        return 1;
    
    // check for 2,3,10
    if(player_has_card(player,2,1) ||
       player_has_card(player,3,1) ||
       player_has_card(player,10,1)
    )
        return 0;

    // case 7 is on top
    if(card == 7)
    {
        for(card= 2; card <= 7; card++)
        {
            if(player_has_card(player,card,1))
                return 0;
        }
    }
    // other cards
    else
    {
        for(; card <= A_VAL; card++)
        {
            if(player_has_card(player,card,1))
                return 0;
        }
    }

    return 2;
}

bool game_check_burn_pile(game_t *game)
{
    card_t c = game_get_top_card(game);
    if(!card_is_valid(c)) return false;
    if ( c == 10) return true;
    for(int i=1;i<4;i++)
    {
        if( card_stack_peek(game->play_deck, i) != c) return false;
    }
    return true;
}

int game_check_illegal(game_t *game, player_t *player, card_t card, int cnt)
{
    int reason = (player_plays_from(player) == PL_PILE_F_DWN)+1;
    int top = game_get_top_card(game);
    if(game->active_8)
    {
        if(card==8) goto check_has;
    }
    else if(card == 2 || card == 3 || card == 10)
    {
        goto check_has;
    }
    else if(top == 7)
    {
        if(card <= 7) goto check_has; 
    }
    else if(card >= top)
    {
        goto check_has;
    }
    return reason;

    check_has:
        return player_has_card(player, card, cnt)? 0 : reason;
}

void game_loop(game_t *game)
{
    int curr_player = 0, j, reason;
    card_t card = INVALID_CARD;
    player_t *player; int cd;

    // Players trading
    for(curr_player = 0; curr_player < MAX_PLAYERS; curr_player++)
    {
        if(game->players[curr_player]) player_trade_cards(game->players[curr_player]);
    }

    // first player: left-to-right, who has 3 in hand starts first. if no 3, then 4 etc...
    j=3;
    for (curr_player = 0; curr_player < MAX_PLAYERS; j++, curr_player++)
    {
        if(!game->players[curr_player]) continue;
        if(player_has_card(game->players[curr_player],j,1)) break;
    }
    
    // game start
    game->state = GM_PLAYING;

    for(;; curr_player++, curr_player %= MAX_PLAYERS)
    {
        if(!game->players[curr_player]) continue;
        if(game->players[curr_player]->state == PL_DONE) 
        {
            //game->players[curr_player] = NULL;
            continue;
        }

        player = game->players[curr_player];
        cd = player->comm_if->cd;
        player->state = PL_PLAYING_ON_TURN;

        // show top card
        /*player->comm_if->tell_top(player->comm_if->cd, game_get_top_card(game));
        player->comm_if->tell_cards(
            player->comm_if->cd,
            player->hand, 
            player->face_up,
            player_secret_face_down(player)
            );*/

        player->comm_if->send_request(cd,SRRQ_TOP_CARD, (void *)game_get_top_card(game));
        player->comm_if->send_request(cd,SRRQ_YOUR_CARDS, player);

        // check if player cannot play
        if((reason=game_check_cannot_play(game, player)))
        {
            //player->comm_if->write(player->comm_if->cd,"You cannot play at the moment.");
            player->comm_if->send_request(cd, SRRQ_WRITE, "You cannot play at the moment.");
            if(reason==1) game->active_8 = false;
            else{
                player_draw_cards( player, card_stack_height(game->play_deck), game->play_deck);
            }
            goto cannot_play;
        }

        // wait till player plays valid card
        legal_check: 
        /*player->comm_if->rq_card(player->comm_if->cd);
        card = player->comm_if->read_card(player->comm_if->cd,&j);*/
        player->comm_if->send_request(cd, SRRQ_GIMME_CARD, &j);

        if(player_plays_from(player) == PL_PILE_F_DWN) card = player->face_down[card];
        while((reason=game_check_illegal(game, player, card,j)))
        {
            if(reason==1) // player chose bad card
            {
                //player->comm_if->write(player->comm_if->cd,"Illegal card(s). Choose again.");
                player->comm_if->send_request(cd, SRRQ_WRITE, "Illegal card(s). Choose again.");
                /*player->comm_if->rq_card(player->comm_if->cd);
                card = player->comm_if->read_card(player->comm_if->cd,&j);*/
                player->comm_if->send_request(cd, SRRQ_GIMME_CARD, &j);
            }
            else // played from face-down -> could not know result
            {
                player_put_to_hand(player, card);
                // if 8 is active and the random card was not an 8,
                // the 8 procs, passing to another person
                // but since the player saw their face-down card, it stays in their hand
                // if 8 is not active though, the player takes the whole playing deck to their hand
                // (along with the now revealed card)
                if(!game->active_8)
                {
                    player_draw_cards( player, card_stack_height(game->play_deck), game->play_deck);
                }
                else game->active_8 = false;

                goto cannot_play;
            }
            
        }

        // Play the player's cards
        player_play_cards(player, card, j, game->play_deck);
        // check for 8 or pile burn
        if(game_get_top_card(game) == 8)
        {
            game->active_8 = true;
        }
        if(game_check_burn_pile(game))
        {
            game->active_8 = false;
            card_stack_clear(game->play_deck);
            goto legal_check; // plays again
        }

        // draw cards
        j = player_hand_card_cnt(player);
        player_draw_cards(player, 3-j, game->draw_deck);

        if(!player_plays_from(player))
        {
            //player->comm_if->write(player->comm_if->cd,"Congrats, you won!");
            player->comm_if->send_request(cd, SRRQ_WRITE, "Congrats, you won!");
        }

        cannot_play: 
        player->state = PL_PLAYING_WAITING;
    }
}


