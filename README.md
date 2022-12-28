# KIV-UPS SHITHEAD 

## communication
Server always initiates the communication and either requests data from the client or informs them of something. Client then replies with requested data or simply thanks the server.

format of communication (s = server, c = client):
  1. request/inform (s)
  2. N/ACK (c)
  3. [reply (c)]

format of messages - both requests and replies:
  1. COMMAND (capital chars, underscores in stead of spaces)
  2. data, multiple entries delimitied by '^', also delimited from the command 
  3. newline char (0x0A)

Some requests may contain data or may be "informational" - they do not request any data (reply is then "THANKS\n").
Cards sent in this protocol are represented by the ASCII character 0x30 + card value (values of J,Q,K,A are 11,12,13 and 14 respectively) - i.e. numbers '2'-'9' and charactes ':', ';', '<', '=' and '>'. The character '0' represents "no card".

## messages: 
### server:
  - MAIN_MENU - request the player to choose nick
     - data: player ID^max nick len
  - MM_CHOICE - request the player to choose what to do in Main menu (join/create lobby or reconnect)
  - RECONN - if the player chose to reconnect, let them know the result of the recon
    - data: I(NVALID)|L(OBBY)|R(UNNING)|F(INISHED) (one of these)
      - INVALID|FINISHED: player sent invalid cache or game finished -> must choose a lobby
      - LOBBY|RUNNING: reconn succesful + tell the phase of the game.

  - LOBBIES - tell the client the lobbies
     - data: "i:N^i+1:N...", where:
       - i is ID of lobby, 
       - N is number of connected people, 
  - LOBBY_STATE - tell the player state of the lobby
     - data: nicks of players in the lobby, first one being the owner
  - LOBBY_START - request the lobby owner to start the game

  - TRADE_NOW - request the player to trade cards before game starts and tell the result
  - ON_TURN - tell the players who's on turn
     - data: player nick
  - GIMME_CARD - request the player to play a card
  - GAME_STATE 
      - data: "nick1:2s,3s,...,Ks,As:fmask:dmask^nick2:hand_cnt:fmask:dmask^...^T:card^D:draw_num", where: 
        - nick1, nick2, ... are nicks of players in the game
         - 2s, 3s, ..., Ks, As are numbers of cards in hand of the player, only after recieving player's nick
         - fmask is a mask of face ups, 3 bytes, each the faceup, '0' if no card
         - dmask is a mask of face downs, 3 bytes, either '0' or '1' for there or not there
         - hand_cnt is the number of cards in hand of the player who's not the reciever of the message
         - T:card is the top card
         - D:draw_num is the number of cards in draw pile

  - WRITE - the server writes a simple message to the client
     - data: the message (terminated by the newline char of the message format)

### client:
 - MAIN MENU ->  NICK, data: nick (len up to NL char)
 - MM CHOICE -> LB, data: (0 (create lobby) | number (lobby ID)) | "RECONN^cache" to reconnect
    - cache: the player cache to be verified by the server: "nick^id^game_id"
 - RECONN -> THANKS

 - LOBBIES -> THANKS
 - LOBBY STATE -> THANKS
 - LOBBY START -> YES|NO (or nothing and let it timeout)

 - TRADE NOW -> TRADE, data: 3 cards the player has in hand, order = which face up to trade, 0 for no trade
 - ON TURN -> THANKS, data: none
 - GIMME CARD -> CARD, data: "card^n", n=number of them or "idx^1" if plays from face down
 - GAME STATE -> THANKS, data: none

 - WRITE -> THANKS, data: none
 - anything -> QUIT