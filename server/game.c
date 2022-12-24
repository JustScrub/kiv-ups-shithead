#include "include/game.h"
#include "include/card.h"
#include "include/player.h"
#include "include/comm_if.h"
#include "include/player_quitter.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>

static int gm_id = 0;

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

/**
 * @brief Allowed adding of players to the game
 * [GM_LOBBY][PL_MAIN_MENU] = classic add
 * other true values are for "reconnecting" players
 */
bool add_allowed[][6] = {
    [GM_LOBBY] = {[PL_MAIN_MENU] = true, [PL_LOBBY] = true },
    [GM_PREPARE] = {0},
    [GM_PLAYING] = {[PL_PLAYING_WAITING] = true, [PL_PLAYING_ON_TURN] = true, [PL_DONE] = true},
    [GM_FINISHED] = {0}
};

bool game_add_player(game_t *game,player_t *pl)
{
    int i = game->state == GM_LOBBY;
    if(!game || !pl) return false;
    if(!add_allowed[game->state][pl->state]) return false;

    for(;i<MAX_PLAYERS, game->players[i];i++)
        if(game->players[i]->id == pl->id) break;

    if(i<=MAX_PLAYERS)
    {
        game->players[i] = pl;
        pl->game_id = game->id;
        if(pl->state != PL_MAIN_MENU)
            game_comm(game, i, SRRQ_RECON, game->state == GM_LOBBY ? "LOBBY" : "RUNNING", game->state == GM_LOBBY ? strlen("LOBBY") : strlen("RUNNING"));
        pl->state = game->state == GM_LOBBY? PL_LOBBY : PL_PLAYING_WAITING;
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

void player_trade_cards(game_t *game, int pl_idx)
{
    int j; card_t card;
    comm_flag_t ret;
    player_t *player = game->players[pl_idx];
    ret = game_comm(game, pl_idx, SRRQ_TRADE_NOW, &j, sizeof(j));
    if(ret == COMM_QUIT || ret == COMM_DIS) return;
    for(int i=0; i<3; i++)
    {
        card = ((card_t *)&j)[i];
        if(!card_is_valid(card)) continue;
        if(!player->hand[card-2])
        {
            ret = game_comm(game, pl_idx, SRRQ_WRITE, "You don't have that card.", strlen("You don't have that card."));
            if(ret == COMM_QUIT || ret == COMM_DIS) return;
            break; //tries to cheat -> cannot trade
        }
        // switching
        player->hand[player->face_up[i] -2]++;
        player->hand[card-2]--;
        player->face_up[i] = card;
    }
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

void game_send_all(game_t *game, server_request_t request, void *data, int dlen)
{
    for(int i=0; i<MAX_PLAYERS; i++)
    {
        if(game->players[i])
        {
            game_comm(game, i, request, data, dlen);
        }
    }
}

void game_loop(game_t *game)
{
    int curr_player = 0, j, reason;
    card_t card = INVALID_CARD;
    player_t *player;
    comm_flag_t ret;

    // Players trading
    for(curr_player = 0; curr_player < MAX_PLAYERS; curr_player++)
    {
        if(!game->players[curr_player]) continue;
        
        if(game_comm(game, curr_player, SRRQ_GAME_STATE, game, sizeof(game_t *)) != COMM_OK) continue;
        player_trade_cards(game,curr_player);
        game->players[curr_player]->state = PL_PLAYING_WAITING;
    }

    // first player: left-to-right, who has 3 in hand starts first. if no 3, then 4 etc...
    j=3;
    for (curr_player = 0; j <= A_VAL; curr_player = (curr_player+1)%MAX_PLAYERS, j += !curr_player)
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
            //player won, but still spectating
            continue;
        }

        player = game->players[curr_player];
        player->state = PL_PLAYING_ON_TURN;

        game_send_all(game, SRRQ_GAME_STATE, game, sizeof(game_t *));
        game_send_all(game, SRRQ_ON_TURN, player->nick, NIC_LEN);
        if(!game->players[curr_player]) continue;   // player might have disconnected

        // check if player cannot play
        if((reason=game_check_cannot_play(game, player)))
        {
            if(game_comm(game, curr_player, SRRQ_WRITE, "You cannot play at the moment.", strlen("You cannot play at the moment.")) != COMM_OK) 
                goto gloop_end;   // player might have disconnected

            if(reason==1) game->active_8 = false;
            else{
                player_draw_cards( player, card_stack_height(game->play_deck), game->play_deck);
            }
            goto cannot_play;
        }

        // wait till player plays valid card
        legal_check: 
        if(game_comm(game, curr_player, SRRQ_GIMME_CARD, &j, sizeof(int)) != COMM_OK) goto gloop_end;   // player might have disconnected
        //j: LSB: card, other bytes: count
        card = (char)j;
        j >>= 8;

        if(player_plays_from(player) == PL_PILE_F_DWN) card = player->face_down[card];
        if((reason=game_check_illegal(game, player, card,j)))
        {
            if(reason==1) // player chose bad card
            {
                //player->comm_if->send_request(cd, SRRQ_WRITE, "Illegal card(s). Choose again.");
                if(game_comm(game, curr_player, SRRQ_WRITE, "Illegal card(s). Choose again.", strlen( "Illegal card(s). Choose again."))!=COMM_OK)
                    goto gloop_end;   // player might have disconnected
                goto legal_check;
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
            game_comm(game, curr_player, SRRQ_WRITE, "Congrats, you won!", strlen("Congrats, you won!"));
            continue;
        }

        cannot_play: 
        player->state = PL_PLAYING_WAITING;
        gloop_end: ;
    }
}

comm_flag_t flag_map[] = {
    [COMM_OK] = COMM_OK,
    [COMM_QUIT] = COMM_QUIT,
    [COMM_DIS] = COMM_DIS,
    // handle timeout and bullshit as disconnect
    [COMM_TO] = COMM_DIS,
    [COMM_BS] = COMM_DIS,
};
comm_flag_t game_comm(game_t *game, int pl_idx, server_request_t request, void *data, int dlen)
{
    comm_flag_t ret;
    ret = game->players[pl_idx]->comm_if->send_request(game->players[pl_idx]->comm_if->cd, request, data, dlen);
    ret = flag_map[ret];
    /*
    if(send)
    else
        ret = game->players[pl_idx]->comm_if->handle_request(game->players[pl_idx]->comm_if->cd, state, data);
    */
    if(ret == COMM_QUIT)
    {
        quitter_push(game->players[pl_idx]);
        game->players[pl_idx] = NULL;
    }

    if(ret == COMM_DIS)
    {
        game->players[pl_idx]->comm_if.conn_state = PL_CONN_DOWN;
        //quitter_push(game->players[pl_idx]);
        game->players[pl_idx] = NULL;
    }

    return ret;
}

void *game_thread(void *arg)
{
    pthread_detach(pthread_self());
    game_t *game = (game_t *)arg;
    char *s = malloc(1); *s=0;
    comm_flag_t ret;
    if(!game)
    {
        //log error
        pthread_exit(NULL);
    }
    game->state = GM_LOBBY;
    for(;;)
    {
        // tell owner the state of the lobby
        ret = game_comm(game, 0, SRRQ_LOBBY_STATE, game, sizeof(game_t *));
        if(ret == COMM_QUIT || ret == COMM_DIS) goto lobby_owner_quit;

        // tell rest of the players the state of the lobby
        for(int i=1; i<MAX_PLAYERS; i++)
        {
            if(!game->players[i]) continue;
            ret = game_comm(game, i, SRRQ_LOBBY_STATE, game, sizeof(game_t *));
        }

        // wait for owner to start the game
        game_comm(game, 0, SRRQ_LOBBY_START, s, 1);
        if(ret == COMM_QUIT || ret == COMM_DIS) goto lobby_owner_quit;

        if(*s && game_player_count(game) >= 2) break;
    }

    free(s);
    game->state = GM_PREPARE;
    game_init(game);
    game_loop(game);
    game->state = GM_FINISHED;

    for(int i=1; i<MAX_PLAYERS; i++)
    {
        if(!game->players[i]) continue;
        quitter_push(game->players[i]);
    }

    queue_push(&game->id,Q_game_del);
    pthread_exit(NULL);

    lobby_owner_quit:
        for(int i=0; i<MAX_PLAYERS; i++)
        {
            if(!game->players[i]) continue;
            *s = game->players[i]->comm_if->send_request(game->players[i]->comm_if->cd, SRRQ_WRITE, "The lobby owner has quit.");
            if(*s != COMM_QUIT || *s != COMM_DIS) queue_push(game->players[i], Q_quiter);
        }
        free(s);
        queue_push(&game->id,Q_game_del);
        pthread_exit(NULL);
}


