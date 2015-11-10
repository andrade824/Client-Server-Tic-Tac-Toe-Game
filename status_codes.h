//
// Created by devon on 5/7/15.
//

#ifndef MIDTERM_TICTACTOE_STATUS_CODES_H
#define MIDTERM_TICTACTOE_STATUS_CODES_H

enum StatusCode
{
    INVALID_CMD,    // Invalid command
    JOINED,     // Joined a game
    OTHER_JOINED, // Other player joined your game
    CREATED,    // Created a game
    GAME_EXISTS,// The game already exists and has two players
    REGISTERED, // Registered a player name
    LIST,       // Listing all of the games
    PLAYER_DISCONNECT, // The player disconnected
    OTHER_DISCONNECT,   // The other player disconnected
    MOVE,       // A tic tac toe game move
    WIN,        // Somebody won
    DRAW       // It was a draw
};

// Send a status code to the specified socket
bool SendStatus(int socket, StatusCode status);

// Blocks until it receives a status
bool ReceiveStatus(int socket, StatusCode * status);

#endif //MIDTERM_TICTACTOE_STATUS_CODES_H
