# KIV-UPS SHITHEAD 

## communication
two way: both server and client can request stuff (eg. server: card, client: lobbies)
approaches:
  - two threads: request initiator, request commitor
    - initiator sends request and waits for reply
    - commitor waits for request and sends reply
  - two threads: reader, writer
    - writer sends requests and replies in conforming formats
    - reader reads requests and replies

In game, server can send stuff without initial request (eg. tell top card)

format of communication (i = initiator, c = commitor):
  1. request (i)
  2. N/ACK (c)
  3. reply (c)
  4. N/ACK (i)

format of messages - both requests and replies:
  1. COMMAND (capital chars, can contain spaces)
  2. data (ideally, each COMMAND has its data len)
  3. newline char (0x0A)

messages: 
 - server:
   - requests:
     - MAIN MENU - request the player to choose what to do in main menu (new lobby, join lobby)
     - GIMME CARD - request the player to play a card
     - TRADE NOW - request the player to trade cards before game starts and tell the result
     - YOUR TURN - tell the player he's on turn
     - TOP CARD - tell the player the top card (after YOUR TURN request)
        - data: the card (1 byte)
     - YOUR CARDS - tell the player his cards
        - data: "HAND"card","amount";" for all cards (13)"FACE UP"3 cards"FACE DOWN"mask
     - WRITE - the server writes a simple message to the client
        - data: the message
     - PING
   - replies:
    - MM CHOICE -> THANKS
    - LOBBY -> LOBBIES, data: "LOBBYi" for all created lobbies i
    - GAME STATE -> GAME STATE, data: "YOUR CARDS""HAND"card","amount";" for all cards (13)"FACE UP"3 cards"FACE DOWN"mask"TOP CARD"card"FACE UPS"("FU"cards for all other players, starting with next)"DRAW SIZE"byte
    - TOP CARD -> TOP CARD, data: card
    - PING -> THANKS

 - client:
   - requests:
     - MM CHOICE - request the server to process main menu choice
        - data: lobby ID (join) or 0 (create new)
     - LOBBY - request list of current lobbies
     - GAME STATE - game state to cache - player's cards, top card, other players' face up cards, draw deck size
     - GAME START - request the server to start the in-lobby game (cli must be lobby owner)
     - TOP CARD - request the top card
     - PING
   - replies:
     - MAIN MENU ->  THANKS (+ request lobbies!)
     - GIMME CARD -> CARD, data: card (1 byte) the player has and can be played
     - TRADE NOW -> TRADE, data: 3 card the player has in hand, order = which face up to trade
     - YOUR TURN -> THANKS, data: none
     - TOP CARD -> THANKS, data: none
     - WRITE -> THANKS, data: none
     - PING -> THANKS



