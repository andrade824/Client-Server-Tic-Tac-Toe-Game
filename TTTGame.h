//
// Created by devon on 5/6/15.
//

#ifndef MIDTERM_TICTACTOE_TTTGAME_H
#define MIDTERM_TICTACTOE_TTTGAME_H
#include <string>
#include "Player.h"

using std::string;

class TTTGame {
public:
    // Constructor that specifies the socket of the first player
    TTTGame(string name, Player player1);

    // Copy Constructor
    TTTGame(const TTTGame & copy);

    // Assignment op overload
    TTTGame * operator=(const TTTGame & rhs);

    // Returns true if this game has that player
    bool HasPlayer(const Player & player) const;

    // Gets the other player's id
    Player GetOtherPlayer(const Player & player) const;

    // Getters and setters
    Player GetPlayer1() const;
    void SetPlayer1(Player m_player1);
    Player GetPlayer2() const;
    void SetPlayer2(Player m_player2);
    string GetName() const;

    // Equality operator overload
    bool operator==(const TTTGame & other) const;
    bool operator==(const string & other) const;

private:
    Player m_player1;
    Player m_player2;
    string m_name;
};


#endif //MIDTERM_TICTACTOE_TTTGAME_H
