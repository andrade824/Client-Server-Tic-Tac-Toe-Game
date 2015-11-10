//
// Created by devon on 5/6/15.
//

#ifndef MIDTERM_TICTACTOE_PLAYER_H
#define MIDTERM_TICTACTOE_PLAYER_H
#include <string>
using std::string;

// Mode used to determine whether player is ingame or not
enum player_modes { COMMAND, INGAME };

class Player {
public:
    // Constructor
    Player(int socket);

    // Copy constructor
    Player(const Player & copy);

    // Assignment Op Overload
    Player * operator=(const Player & rhs);

    // Getters and Setters
    void SetName(string name);
    string GetName() const;
    int GetSocket() const;
    player_modes GetMode() const;
    void SetMode(player_modes mode);

    // Equality operator overload
    bool operator==(const Player & other) const;

private:
    int m_socket;
    string m_name;
    player_modes m_mode;
};


#endif //MIDTERM_TICTACTOE_PLAYER_H
