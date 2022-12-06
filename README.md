# KIV-UPS SHITHEAD 

## communication
Both client and server can send requests. Who sends which when is deterministic and depends on the state of the game or client.

format of communication (i = initiator, c = commitor):
  1. request (i)
  2. N/ACK (c)
  3. reply (c)
  4. N/ACK (i)

format of messages - both requests and replies:
  1. COMMAND (capital chars, can contain spaces)
  2. data, multiple entries delimitied by '^', also delimited from the command 
  3. newline char (0x0A)

Some requests may contain data or may be "informational" - they do not request any data (reply is then "THANKS\n").
Cards sent in this protocol are represented by the ASCII character 0x30 + card value (values of J,Q,K,A are 11,12,13 and 14 respectively) - i.e. numbers '2'-'9' and charactes ':', ';', '<', '=' and '>'. The character '0' represents "no card".

## messages: 
### server:
#### requests:
  - MAIN MENU - request the player to choose nick
  - GIMME CARD - request the player to play a card
  - TRADE NOW - request the player to trade cards before game starts and tell the result
  - YOUR TURN - tell the player he's on turn
  - ON TURN - tell the players who's on turn
     - data: player nick
  - TOP CARD - tell the player the top card (after YOUR TURN request)
     - data: the card
  - YOUR CARDS - tell the player their cards
     - data: "HAND^2s^3s...Ks^As^FACE UP^fmask^FACE DOWN^dmask", where: 
       - data between HAND and "FACE UP" is the number of 2s, 3s... and so on (up to As) the player has, 
       - fmask is 3 characters representing face-up cards and 
       - dmask is 3 characters - 1s or 0s - 1s in positions of face down cards still on board
  - WRITE - the server writes a simple message to the client
     - data: the message (terminated by the newline char of the message format)
  - PING
#### replies:
  - MM CHOICE -> THANKS
  - LOBBY -> LOBBIES, data: "LOBBYi^N^nick1^nick2...^nickN^LOBBYi+1...", where:
   - i is ID of lobby, 
   - N is number of connected people, 
   - nickI is the nick of I-th person.
  - TOP CARD -> TOP CARD, data: card
  - RECON -> INVALID|LOBBY|TRADE|RUNNING|FINISHED (one of these)
  - PING -> THANKS
  - GAME START -> THANKS|ILLEGAL (ILLEGAL if player not owner)
  - GAME STATE -> GAME STATE, data: "YOUR CARDS^HAND^2s^3s...Ks^As^FACE UP^fmask^FACE DOWN^dmask^TOP CARD^card^FACE UPS^nick1^fmask1...^nickN^fmaskN^DRAW SIZE^dn", where: 
    - data between HAND and "FACE UP" is the number of 2s, 3s... and so on (up to As) the player has, 
    - fmask is 3 characters representing face-up cards, 
    - dmask is 3 characters - 1s or 0s - 1s in positions of face down cards still on board, 
    - the part after "FACE UPS" specifies the face up cards of other players represented in "fmask format" and 
    - dn is the number of cards in the draw deck.

### client:
#### requests:
 - MM CHOICE - request the server to process main menu choice
    - data: lobby ID (join) or 0 (create new)
 - LOBBY - request list of current lobbies
 - RECON - inform the server about reconnectiong client, data: cached data for the server to validate
 - GAME STATE - game state to cache - player's cards, top card, other players' face up cards, draw deck size
 - GAME START - request the server to start the in-lobby game (cli must be lobby owner)
 - TOP CARD - request the top card
 - PING
#### replies:
 - MAIN MENU ->  NICK, data: nick (+ request lobbies!)
 - GIMME CARD -> CARD, data: "card^n", n=number of them
 - TRADE NOW -> TRADE, data: 3 cards the player has in hand, order = which face up to trade, 0 for no trade
 - YOUR TURN -> THANKS, data: none
 - ON TURN -> THANKS, data: none
 - YOUR CARDS -> THANKS, data: none
 - TOP CARD -> THANKS, data: none
 - WRITE -> THANKS, data: none
 - PING -> THANKS
 - anything -> QUIT


client request permissions based on their state:
  - all states:
    - PING, QUIT
  - MAIN MENU:
    - RECON, LOBBY, MM CHOICE, QUIT
  - LOBBY:
    - nothing special
  - LOBBY_OWNER:
    - GAME START
  - PLAYING_WAITING:
    - GAME STATE, TOP CARD
  - PLAYING_ON_TURN:
    - GAME STATE, TOP CARD
  - DONE:
    - nothing special
