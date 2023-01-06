#include "include/game.h"
#include "include/card.h"
#include "include/player.h"
#include "include/comm_if.h"
#include "include/player_quitter.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define RIG_BURN_END 0
#define RIG_SAME_CARDS 0 // change to value of the card - so 2 to 14 (0 is no rig)
#define RIG_HALF_DECK 1
#define RIG_NO_TRADING 1
#define RIG_NO_SHUFFLE 0
#define RIG_ALL_LEGAL 0
#define RIG_FDOWN_ONLY 0
#define RIG_NO_DRAW 0

static unsigned int gm_id = 0;

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
    // state mutex?
}

/**
 * @brief Allowed adding of players to the game
 * [GM_LOBBY][PL_MAIN_MENU] = classic add
 * other true values are for "reconnecting" players
 */
bool add_allowed[][7] = {
    [GM_LOBBY] = {[PL_MAIN_MENU] = true, [PL_LOBBY] = false }, // cannot reconnect from lobby.
    [GM_PREPARE] = {0},
    [GM_PLAYING] = {[PL_PLAYING_WAITING] = true, [PL_PLAYING_ON_TURN] = true, [PL_PLAYING_TRADING] = true , [PL_DONE] = false},
    [GM_FINISHED] = {0}
};

bool game_add_player(game_t *game,player_t *pl)
{
    int i = game->state == GM_LOBBY;
    if(!game || !pl) return false;
    if(!add_allowed[game->state][pl->state]) return false;

    for(;i<MAX_PLAYERS && game->players[i];i++);
    //    if(game->players[i]->id == pl->id) break;

    if(i<=MAX_PLAYERS)
    {
        pl->game_id = game->id;
        game->players[i] = pl;
        if(pl->state != PL_MAIN_MENU){
            game_comm(game, i, SRRQ_RECON, "R", 1);
            pl->comm_if.conn_state = PL_CONN_UP;
        }
        pl->state = game->state == GM_LOBBY? PL_LOBBY : PL_PLAYING_WAITING;
        return true;
    }
    return false;
}

int game_player_count(game_t *game)
{
    int cnt = 0;
    if(!game) return 0;
    for(int i=0; i<MAX_PLAYERS;i++)
    {
        if(game->players[i] && game->players[i]->comm_if.conn_state)
        {
            cnt++;
        }
    }
    return cnt;
}

int game_playing_count(game_t *game)
{
    int cnt = 0;
    if(game->state != GM_PLAYING) return 0;
    for(int i = 0; i<MAX_PLAYERS;i++)
    {
        if(game->players[i] && 
           game->players[i]->comm_if.conn_state && 
           game->players[i]->state != PL_DONE)
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
        #if RIG_SAME_CARDS
            card_stack_push(game->draw_deck, RIG_SAME_CARDS );
        #else
            card_stack_push(game->draw_deck, (i%13)+2);
        #endif

        #if RIG_HALF_DECK
            if(i == 26) break;
        #endif
    }
    #if !RIG_NO_SHUFFLE
        card_stack_shuffle(game->draw_deck);
    #endif

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
                    //                 ^player_t address | ^offset of 
                    //                  (struct)         |  the card array in the player_t struct
                }
            }
        }
    }

    #if !RIG_FDOWN_ONLY
    for(int p=0; p <MAX_PLAYERS; p++)
        {
            if(game->players[p])
            {
                player_draw_cards(game->players[p],3,game->draw_deck);
            }
        }
    #else
    for(int p=0; p <MAX_PLAYERS; p++)
        {
            if(game->players[p])
            {
                memset(game->players[p]->face_up, INVALID_CARD, 3*sizeof(card_t));
            }
        }
    #endif
    game->active_8 = false;
}

void player_trade_cards(game_t *game, int pl_idx)
{
    int j; card_t card;
    comm_flag_t ret;
    player_t *player = game->players[pl_idx];
    ret = game_comm(game, pl_idx, SRRQ_TRADE_NOW, &j, sizeof(j));
    printD("TRADE: pl_idx=%d, comm=%d\n",pl_idx, ret);
    if(ret != COMM_OK) return;
    for(int i=0; i<3; i++)
    {
        card = ((card_t *)&j)[i];
        if(!card_is_valid(card)) continue;
        if(player->hand[card-2])
        {
            // switching
            player->hand[player->face_up[i] -2]++;
            player->hand[card-2]--;
            player->face_up[i] = card;
            continue;
        }
        
        for(ret = 0; ret<3; ret++)
            if(player->face_up[ret] == card)
                break;
        if(ret == 3) continue; // does not have the card
        //has card in
        // switch face_up[i] and face_up[ret]
        card = player->face_up[i];
        player->face_up[i] = player->face_up[ret];
        player->face_up[ret] = card;
        continue;
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
    #if RIG_ALL_LEGAL
        return 2;
    #else
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
    #endif
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
    #if RIG_ALL_LEGAL
        return 0;
    #else
    if(!card_is_valid(card)) return 1;
    if(!player_has_card(player, card, cnt)) return 1;
    int top = game_get_top_card(game);
    if(game->active_8)
    {
        if(card==8) return 0;
    }
    else if(card == 2 || card == 3 || card == 10)
    {
        return 0;
    }
    else if(top == 7)
    {
        if(card <= 7) return 0; 
    }
    else if(card >= top)
    {
        return 0;
    }
    return (player_plays_from(player) == PL_PILE_F_DWN)+1;
    #endif
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
    //comm_flag_t ret;

    game_send_all(game, SRRQ_GAME_STATE, game, sizeof(game_t *));

    #if !RIG_NO_TRADING
    // Players trading
    for(curr_player = 0; curr_player < MAX_PLAYERS; curr_player++)
    {
        if(!game->players[curr_player]) continue;
        
        //if(game_comm(game, curr_player, SRRQ_GAME_STATE, game, sizeof(game_t *)) != COMM_OK) continue;
        game->players[curr_player]->state = PL_PLAYING_TRADING;
        player_trade_cards(game,curr_player);
        if(game->players[curr_player])
            game->players[curr_player]->state = PL_PLAYING_WAITING;
        game_send_all(game, SRRQ_GAME_STATE, game, sizeof(game_t *));
    }
    #endif

    // first player: left-to-right, who has 3 in hand starts first. if no 3, then 4 etc...
    j=3;
    for (curr_player = 0; j <= A_VAL; curr_player = (curr_player+1)%MAX_PLAYERS, j += !curr_player)
    {
        if(!game->players[curr_player]) continue;
        if(player_has_card(game->players[curr_player],j,1)) break;
    }
    printD("First player: %s\n", game->players[curr_player]?game->players[curr_player]->nick: "NULL");
    // game start
    game->state = GM_PLAYING;

    printD("playing: %d", game_playing_count(game));
    for(; game_playing_count(game) > 1 ; curr_player++, curr_player %= MAX_PLAYERS)
    {
        printD("game loop %d: new round\n",game->id);
        if(!game->players[curr_player]) continue;
        if(game->players[curr_player]->comm_if.conn_state == PL_CONN_DOWN) continue; // reconnecting
        if(game->players[curr_player]->state == PL_DONE) continue; // player has won, spectating 

        player = game->players[curr_player];
        player->state = PL_PLAYING_ON_TURN;

        game_send_all(game, SRRQ_GAME_STATE, game, sizeof(game_t *));
        game_send_all(game, SRRQ_ON_TURN, player->nick, NIC_LEN);
        if(!game->players[curr_player]) goto gloop_end;   // player might have disconnected

        // check if player cannot play
        if((reason=game_check_cannot_play(game, player)))
        {
            if(game_comm(game, curr_player, SRRQ_WRITE, "You cannot play at the moment.", sizeof("You cannot play at the moment.")) != COMM_OK) 
                goto gloop_end;   // player might have disconnected

            if(reason==1) game->active_8 = false;
            else{
                player_draw_cards( player, card_stack_height(game->play_deck), game->play_deck);
            }
            goto cannot_play;
        }

        // wait till player plays valid card
        legal_check: 
        j = reason = (player_plays_from(player)==PL_PILE_F_DWN); // load to reason for later cache
        game_send_all(game, SRRQ_GAME_STATE,game, sizeof(game_t *));
        if(!game->players[curr_player]) goto gloop_end;   // player might have disconnected
        game_send_all(game, SRRQ_WRITE, "Waiting for player to play", sizeof("Waiting for player to play"));
        if(game_comm(game, curr_player, SRRQ_GIMME_CARD, &j, sizeof(int)) != COMM_OK) goto gloop_end;   // player might have disconnected
        //j: LSB: card, other bytes: count
        card = (char)j&0xFF;
        j >>= 8;

        if(reason) // pl sent idx of card in face-down pile
        {
            card = player->face_down[(j=card)]; // card is now the card, j is the idx
            printD("Player %s played face-down card %d on idx %d\n", player->nick, card,j);
        }
        if((reason=game_check_illegal(game, player, card,j)))
        {
            if(reason==1) // player chose bad card
            {
                if(game_comm(game, curr_player, SRRQ_WRITE, "Illegal card(s). Choose again.", sizeof( "Illegal card(s). Choose again."))!=COMM_OK)
                    goto gloop_end;   // player might have disconnected
                goto legal_check;
            }
            else // played from face-down -> could not know result
            {
                player_put_to_hand(player, j); // j is idx of card in face-down pile

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
            if(!player_plays_from(player))
            {
                game_comm(game, curr_player, SRRQ_WRITE, "Congrats, you won!", sizeof("Congrats, you won!"));
                game->players[curr_player]->state = PL_DONE;
                goto gloop_end;
            }
            goto legal_check; // plays again
        }

        #if !RIG_NO_DRAW && !RIG_FDOWN_ONLY
        // draw cards
        j = player_hand_card_cnt(player);
        player_draw_cards(player, 3-j, game->draw_deck);
        #endif

        if(!player_plays_from(player))
        {
            game_comm(game, curr_player, SRRQ_WRITE, "Congrats, you won!", sizeof("Congrats, you won!"));
            game->players[curr_player]->state = PL_DONE;
            goto gloop_end;
        }

        cannot_play: 
        player->state = PL_PLAYING_WAITING;
        gloop_end: ;
        #if RIG_BURN_END
            card_stack_clear(game->play_deck);
            game->active_8 = false;
        #endif
    }

    char outstr[25] = {0};
    for(j=0; j<MAX_PLAYERS; j++)
    {
        if(game->players[j] && game->players[j]->state != PL_DONE)
        {
            snprintf(outstr, 25, "SHITHEAD: %s", game->players[j]->nick);
            game->players[j]->state = PL_DONE;
            game_send_all(game, SRRQ_WRITE, outstr, 25);
            sleep(5);
            break;
        }
    }
}

comm_flag_t game_comm(game_t *game, int pl_idx, server_request_t request, void *data, int dlen)
{
    comm_flag_t ret;
    if(!game->players[pl_idx]) return COMM_DIS;
    ret = game->players[pl_idx]->comm_if.send_request(game->players[pl_idx]->comm_if.cd, request, data, dlen);
    
    switch (ret)
    {
    case COMM_OK:
    case COMM_IGN:
        break;

    case COMM_QUIT:
        queue_push(&game->players[pl_idx]->id, Q_quiter);
        game->players[pl_idx] = NULL;
        break;

    case COMM_DIS:
    case COMM_BS:
    case COMM_TO:
        game->players[pl_idx]->comm_if.conn_state = PL_CONN_DOWN;
        queue_push(&game->players[pl_idx]->id, Q_quiter);
        game->players[pl_idx] = NULL;
        break;

    default:
        break;
    }
    return ret;
}

#include <pthread.h>

void *game_thread(void *arg)
{
    pthread_detach(pthread_self());
    game_t *game = (game_t *)arg;
    char *s = malloc(1); *s=0;
    comm_flag_t ret;
    if(!game)
    {
        //log error
        free(s);
        pthread_exit(NULL);
    }
    game->state = GM_LOBBY;
    for(int lobby_idle = 0;; lobby_idle++)
    {
        if(lobby_idle > 30)
        {
            //log error
            goto lobby_owner_quit;
        }

        *s=0;
        // tell owner the state of the lobby
        ret = game_comm(game, 0, SRRQ_LOBBY_STATE, game, sizeof(game_t *));
        if(ret != COMM_OK) goto lobby_owner_quit;

        // tell rest of the players the state of the lobby
        for(int i=1; i<MAX_PLAYERS; i++)
        {
            if(!game->players[i]) continue;
            ret = game_comm(game, i, SRRQ_LOBBY_STATE, game, sizeof(game_t *));
        }

        if(game_player_count(game) < 2) continue;
        ret = game_comm(game, 0, SRRQ_LOBBY_STATE, game, sizeof(game_t *));
        if(ret != COMM_OK) goto lobby_owner_quit;

        // wait for owner to start the game
        game_comm(game, 0, SRRQ_LOBBY_START, s, 1);
        if(ret != COMM_OK && ret != COMM_IGN) goto lobby_owner_quit;
        if(ret == COMM_IGN) *s=0;
        printD("lobby loop: start=%d\n", *s);

        if(!*s) continue;
        if(*s && game_player_count(game) < 2){
            game_comm(game, 0, SRRQ_WRITE, "Not enough players to start the game.", sizeof("Not enough players to start the game."));
            if(ret != COMM_OK) goto lobby_owner_quit;
            continue;
        }
        else break;
    }

    free(s);
    game->state = GM_PREPARE;
    game_send_all(game, SRRQ_WRITE, "GAME STARTS", sizeof("GAME STRATS"));
    game_init(game);
    game_loop(game);
    game_send_all(game, SRRQ_WRITE, "GAME ENDS", sizeof("GAME ENDS"));
    game->state = GM_FINISHED;
    printD("game ended: %d\n", game->id);

    for(int i=0; i<MAX_PLAYERS; i++) // previously 1?
    {
        if(!game->players[i]) continue;
        queue_push(&game->players[i]->id, Q_quiter);
        game->players[i] = NULL;
    }

    queue_push(&game->id,Q_game_del);
    pthread_exit(NULL);

    lobby_owner_quit:
        for(int i=0; i<MAX_PLAYERS; i++)
        {
            if(!game->players[i]) continue;
            game_comm(game,i,SRRQ_WRITE, "The lobby owner quit or idle for too long.", sizeof("The lobby owner quit or idle for too long."));
            if(game->players[i]){
                queue_push(&game->players[i]->id, Q_quiter);
                game->players[i] = NULL;
            }
        }
        free(s);
        queue_push(&game->id,Q_game_del);
        pthread_exit(NULL);
}


