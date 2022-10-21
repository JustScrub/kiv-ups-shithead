#ifndef __SHITHEAD_CONFIG_H__
#define __SHITHEAD_CONFIG_H__

/**
 * @brief Number of decks included. Must be 1 or higher.
 * 
 * One deck includes cards 2-10,J,Q,K,A four times (i.e. 52 cards)
 * Two decks would then include 104 cards, each number 8 times, and so on
 */
#define DECK_NUM 1

/**
 * @brief Max players per one game.
 * 
 * The number of players must be less then or equal to number of cards divided by 9
 * The number of cards can be calculated from the DECK_NUM, defined above: number of cards = DECK_NUM * 52
 * 
 * (i.e. MAX_PLAYERS <= DECK_NUM * 52/9)
 * 
 * Division by 9: each player starts with 9 cards (3 in hand, 3 face-up, 3 face-down)
 */
#define MAX_PLAYERS 4

#define MAX_GAMES 12

#endif