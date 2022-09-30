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
    out->state = LOBBY;
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
        game->players[p] && (game->players[p]->state=PLAYING);
        // set "playing" status to all the players
        // lazy eval -> if the addr is NULL, skips the assignment
    }

    game->state = PLAYING;
}

card_t game_get_top_card(game_t *game)
{
    int i;
    for(i=0; card_game_value(card_stack_peek(game->play_deck,i))==3; i++)
        ;
    return card_stack_peek(game->play_deck,i);
}

void game_loop(game_t *game)
{
    int curr_player = 0;
    card_t card;

    // first player: left-to-right, who has 3 starts first. if no 3, then 4 etc...
    
    curr_player_chosen: 

    for(;;)
    {
        // show top card
        game->players[curr_player]->comm_if->tell_top(game_get_top_card(game));

        // check if player can play
        if(!game_check_can_play(game, curr_player))
        {
            game->players[curr_player]->comm_if->write("You cannot play at the moment.");
            goto cannot_play;
        }

        // wait till player plays valid card
        game->players[curr_player]->comm_if->rq_card();
        card = game->players[curr_player]->comm_if->read_card();
        while(!game_check_legal(game, card))
        {
            game->players[curr_player]->comm_if->write("Illegal card. Choose again.");
            game->players[curr_player]->comm_if->rq_card();
            card = game->players[curr_player]->comm_if->read_card();
        }

        // put the card on top of the deck
        card_stack_push(game->play_deck,card);

        // check for 8 or pile burn
        if(card_game_value(card) == 8)
        {
            game->active_8 = true;
        }
        if(game_check_burn_pile(game))
        {
            curr_player--; //so that the same player plays again
            card_stack_clear(game->play_deck);
        }

        // draw cards
        if(card_stack_peek(game->draw_deck, 0) != INVALID_CARD)
        {

        }

        cannot_play: 
        curr_player++;
        curr_player %= MAX_PLAYERS;
    }
}


