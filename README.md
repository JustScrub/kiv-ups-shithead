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
    - data: INVALID|LOBBY|TRADE|RUNNING|FINISHED (one of these)
      - INVALID|FINISHED: player sent invalid cache or game finished -> must choose a lobby
      - LOBBY|TRADE|RUNNING: reconn succesful + tell the phase of the game.

  - LOBBIES - tell the client the lobbies
     - data: "LOBBYi^N^nick1^nick2...^nickN^LOBBYi+1...", where:
       - i is ID of lobby, 
       - N is number of connected people, 
       - nickI is the nick of I-th person.
  - LOBBY_STATE - tell the player state of the lobby
     - data: nicks of players in the lobby, first one being the owner
  - LOBBY_START - request the lobby owner to start the game

  - TRADE_NOW - request the player to trade cards before game starts and tell the result
  - ON_TURN - tell the players who's on turn
     - data: player nick
  - GIMME_CARD - request the player to play a card
  - GAME_STATE 
      - data: "YOUR CARDS^HAND^2s^3s...Ks^As^FACE UP^fmask^FACE DOWN^dmask^TOP CARD^card^FACE UPS^nick1^fmask1...^nickN^fmaskN^DRAW SIZE^dn", where: 
        - data between HAND and "FACE UP" is the number of 2s, 3s... and so on (up to As) the player has, 
        - fmask is 3 characters representing face-up cards, 
        - dmask is 3 characters - 1s or 0s - 1s in positions of face down cards still on board, 
        - the part after "FACE UPS" specifies the face up cards of other players represented in "fmask format" and 
        - dn is the number of cards in the draw deck.

  - WRITE - the server writes a simple message to the client
     - data: the message (terminated by the newline char of the message format)

### client:
 - MAIN MENU ->  NICK, data: nick len^nick
 - MM CHOICE -> MMC, data: (0 (create lobby) | number (lobby ID)) | "RECONN^cache" to reconnect
    - cache: the player cache to be verified by the server: "nick^id^game_id"
 - RECONN -> THANKS

 - LOBBIES -> THANKS
 - LOBBY STATE -> THANKS
 - LOBBY START -> YES|NO (or nothing and let it timeout)

 - TRADE NOW -> TRADE, data: 3 cards the player has in hand, order = which face up to trade, 0 for no trade
 - ON TURN -> THANKS, data: none
 - GIMME CARD -> CARD, data: "card^n", n=number of them
 - GAME STATE -> THANKS, data: none

 - WRITE -> THANKS, data: none
 - anything -> QUIT