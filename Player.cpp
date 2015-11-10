#include "Player.h"

// Constructor
Player::Player(int socket)
    : m_socket(socket), m_name("No Name"), m_mode(COMMAND)
{ }

// Copy constructor
Player::Player(const Player &copy)
    : m_socket(-1), m_name("No Name"), m_mode(COMMAND)
{
    *this = copy;
}

// Assignment op overload
Player * Player::operator=(const Player & rhs)
{
    if(&rhs != this)
    {
        m_socket = rhs.m_socket;
        m_name = rhs.m_name;
        m_mode = rhs.m_mode;
    }

    return this;
}


// Getters and Setters
void Player::SetName(string name)
{
    m_name = name;
}
string Player::GetName() const
{
    return m_name;
}

int Player::GetSocket() const
{
    return m_socket;
}

player_modes Player::GetMode() const
{
    return m_mode;
}

void Player::SetMode(player_modes mode)
{
    m_mode = mode;
}

// Equality operator overload
bool Player::operator==(const Player & other) const
{
    return m_name == other.m_name;
}