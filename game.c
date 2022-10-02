#include "include/game.h"
#include "include/card.h"
#include "include/player.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>


static gm_id = 0;

void game_create(player_t *owner, game_t *out)
{
    out->id = ++gm_id;
    out->state = GM_LOBBY;
    out->draw_deck = card_stack_create();
    out->play_deck = card_stack_create();
    out->players = calloc(MAX_PLAYERS,sizeof(player_t *));
    out->players[0] = owner;
    owner->game_id = out->id;
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
    // fill cards + shuffle
    for(card_t card = 0; card < 52; card++)
    {
        card_stack_push(game->draw_deck, card);
    }
    card_stack_shuffle(game->draw_deck);

    int offsets[] = {
        offsetof(player_t, face_down),
        offsetof(player_t, face_up),
    };
    int sizes[] = {3,3};

    // draw face up/down cards
    for(int i=0; i <3 /*u*/; i++)
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

    for(int i=0;i<3;i++)
    {
        for(int p=0; p <MAX_PLAYERS; p++)
            {
                if(game->players[p])
                {
                    game->players[p]->hand[card_stack_pop(game->draw_deck)] = true;
                }
            }
    }

    for (int p=0;p<MAX_PLAYERS; p++)
    {
        game->players[p] && (game->players[p]->state=PL_PLAYING);
        // set "playing" status to all the players
        // lazy eval -> if the addr is NULL, skips the assignment
    }

    game->state = GM_PREPARE;
}

card_t game_get_top_card(game_t *game)
{
    int i;
    for(i=0; card_game_value(card_stack_peek(game->play_deck,i))==3; i++)
        ;
    return card_stack_peek(game->play_deck,i);
}

bool game_check_legal(game_t *game, player_t *player, card_t card)
{
    // best choice here is decision tree.
    // Well, I'm lazy for that.
    // Best I can do is if-else (-:

    int cval = card_game_value(card);
    int topval = card_game_value(game_get_top_card(game));
    if(game->active_8)
    {
        if(cval==8) goto check_has;
    }
    else if(cval == 2 || cval == 3 || cval == 10)
    {
        goto check_has;
    }
    else if(topval == 7)
    {
        if(cval <= 7) goto check_has; 
    }
    else if(cval >= topval)
    {
        goto check_has;
    }
    return false;

    check_has:
    return player_plays_from(player) == player_has_card(player, card_get_value(card));
}

void game_loop(game_t *game)
{
    int curr_player = 0, j;
    card_t card;

    // Players trading
    for(curr_player = 0; curr_player < MAX_PLAYERS; curr_player++)
    {
        if(game->players[curr_player]) player_trade_cards(game->players[curr_player]);
    }

    // first player: left-to-right, who has 3 in hand starts first. if no 3, then 4 etc...
    j=1; // 1 maps to game value 3
    for (curr_player = 0; curr_player < MAX_PLAYERS; j++, curr_player++)
    {
        if(player_has_card(game->players[curr_player],j) == 1) break;
    }
    
    // game start
    game->state = GM_PLAYING;

    for(;; curr_player++, curr_player %= MAX_PLAYERS)
    {
        if(!game->players[curr_player]) continue;
        if(game->players[curr_player]->state == PL_DONE) 
        {
            game->players[curr_player] = NULL;
            continue;
        }

        // show top card
        game->players[curr_player]->comm_if->tell_top(game_get_top_card(game));

        // check if player can play
        if(!game_check_can_play(game, curr_player))
        {
            game->players[curr_player]->comm_if->write("You cannot play at the moment.");
            goto cannot_play;
        }

        // wait till player plays valid card
        legal_check: 
        game->players[curr_player]->comm_if->rq_card();
        card = game->players[curr_player]->comm_if->read_card(&j);
        while(!game_check_legal(game, game->players[curr_player], card))
        {
            game->players[curr_player]->comm_if->write("Illegal card(s). Choose again.");
            game->players[curr_player]->comm_if->rq_card();
            card = game->players[curr_player]->comm_if->read_card(&j);
        }

        // put the card(s) on top of the deck
        for(;j>0;j--)
            card_stack_push(game->play_deck,card);

        // check for 8 or pile burn
        if(card_game_value(card) == 8)
        {
            game->active_8 = true;
        }
        if(game_check_burn_pile(game))
        {
            card_stack_clear(game->play_deck);
            goto legal_check; // plays again
        }

        // draw cards
        int cnt = player_hand_card_cnt(game->players[curr_player]);
        for(int i=3; i > 0, card_stack_peek(game->draw_deck, 0) != INVALID_CARD; i--, cnt++)
        {
            if (cnt >= 3) break; 
            game->players[curr_player]->hand[card_stack_pop(game->draw_deck)] = true;
        }

        if(!player_plays_from(game->players[curr_player]))
        {
            game->players[curr_player]->comm_if->write("Congrats, you won!");
        }

        cannot_play: 
    }
}


