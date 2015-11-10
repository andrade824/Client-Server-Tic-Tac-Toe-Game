#include <iostream>
#include <algorithm>
#include <list>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include <bits/local_lim.h>
#include "local_sock.h"
#include "Player.h"
#include "TTTGame.h"
#include "status_codes.h"
#include "sockets.h"

using std::cout;
using std::endl;
using std::list;
using std::find;
using std::remove_if;
using std::find_if;

/** Function prototypes **/
char * allocate_hostname();
int init_server();
void * handle_client(void * arg);
bool ProcessCommand(char buffer[], Player & player, bool & client_connected);
void sig_handler(int signum);
void DisconnectPlayer(const Player & player);
void ListGames(const Player & player);
void CreateJoinGame(Player & player, string game_name);

/** Global Variables **/
int server_sock = 0;    // Server socket

list<TTTGame *> game_list;    // List of open games

// Mutex to make sure operations on games list are atomic
pthread_mutex_t games_lock = PTHREAD_MUTEX_INITIALIZER;

int main() {
    // Signals used to kill the server gracefully
    if(signal(SIGINT, sig_handler) == SIG_ERR)
    {
        perror("Can't catch SIGINT");
        exit(1);
    }

    if(signal(SIGTERM, sig_handler) == SIG_ERR)
    {
        perror("Can't catch SIGTERM");
        exit(1);
    }

    // Initialize the server and get the server's socket
    server_sock = init_server();

    int new_socket = 0;

    // Infinitely accept clients and spawning threads
    while(true)
    {
        // Wait for a client, and then accept
        if((new_socket = accept(server_sock, NULL, NULL)) < 0)
        {
            perror("Failed to accept client");
            close(server_sock);
            exit(1);
        }

        cout << "New client connected" << endl;

        // Spawn thread to handle the client
        pthread_t threadid;
        pthread_create(&threadid, NULL, handle_client, (void *)&new_socket);
    }

    return 0;
}

/**
 * Allocates space for a hostname and fills it with the system's hostname
 *
 * @return The system hostname. Must be freed at a later point with "free"
 */
char * allocate_hostname() {
    char * host = 0;
    long maxsize = 0;

    // Get the maximum hostname size
    if((maxsize = sysconf(_SC_HOST_NAME_MAX)) < 0)
        maxsize = HOST_NAME_MAX;

    // Allocate memory for the hostname
    if((host = (char *)malloc((size_t)maxsize)) == NULL)
    {
        perror("Can't allocate hostname");
        exit(1);
    }

    // Actually get the hostname
    if(gethostname(host, (size_t)maxsize) < 0)
    {
        perror("Couldn't retrieve hostname");
        exit(1);
    }

    return host;
}

/**
 * Initialize the server: create the socket, bind, and start listening.
 *
 * @return Returns the server's socket
 */
int init_server()
{
    int orig_sock = 0; // The server's socket
    char * hostname;    // The system hostname

    struct addrinfo * aip;   // Server's address info
    struct addrinfo hint;   // Used when determining the server info

    int err = 0;    // Generic error code
    int reuse = 1;

    // Setup hint structure for getaddrinfo
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;   // Use IPv4
    hint.ai_socktype = SOCK_STREAM; // Use TCP

    // Get the system's address info
    hostname = allocate_hostname();
    if((err = getaddrinfo(hostname, PORT_NUM, &hint, &aip)) != 0)
    {
        fprintf(stderr, "Failed to get address info: %s", gai_strerror(err));
        free(hostname);
        exit(1);
    }
    free(hostname);

    // Create the socket
    if((orig_sock = socket(aip->ai_addr->sa_family, aip->ai_socktype, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(1);
    }

    // Let us reuse an old address
    if(setsockopt(orig_sock, SOL_SOCKET, SO_REUSEADDR, &reuse,  sizeof(int)) < 0)
    {
        perror("Failed to setup socket for address reuse");
        exit(1);
    }

    // Bind the created socket to the address information
    if(bind(orig_sock, aip->ai_addr, aip->ai_addrlen) < 0)
    {
        perror("Failed to bind socket");
        close(orig_sock);
        exit(1);
    }

    // Free up the generated address info
    freeaddrinfo(aip);

    // Start listening on the socket
    if(listen(orig_sock, MAX_BACKLOG) < 0)
    {
        perror("Failed to start listening");
        close(orig_sock);
        exit(1);
    }

    return orig_sock;
}

/**
 * Handles a single client connection
 */
void * handle_client(void * arg)
{
    int client_sock = *(int *)arg;
    char buffer[BUF_SIZE];
    bool client_connected = true;
    char temp = '\0';
    int row = 0, col = 0;
    int i = 0;

    // Create the player
    Player player(client_sock);

    // Always handle the client
    while(client_connected)
    {
        // Process commands or pass game data
        if(player.GetMode() == COMMAND)
        {
            // Read a line of text or until the buffer is full
            for(i = 0; (i < (BUF_SIZE - 1)) && temp != '\n' && client_connected; ++i)
            {
                // Receive a single character and make sure the client is still connected
                if(read(client_sock, &temp, 1) == 0)
                    client_connected = false;
                else
                    buffer[i] = temp;
            }

            // Reset temp so we don't get an infinite loop
            temp = '\0';
            buffer[i] = '\0';
            buffer[i - 1] = '\0';
            cout << "Received command \"" << buffer << "\" from " << player.GetName() << endl;
            buffer[i - 1] = '\n';

            // If there's an invalid command, tell the client
            if(!ProcessCommand(buffer, player, client_connected))
                SendStatus(player.GetSocket(), INVALID_CMD);
        }
        else if (player.GetMode() == INGAME)
        {
            // Get the game the player is a part of
            pthread_mutex_lock(&games_lock);
            auto game = find_if(game_list.begin(), game_list.end(),
                      [player] (TTTGame * game) { return game->HasPlayer(player); });
            auto end = game_list.end();
            pthread_mutex_unlock(&games_lock);

            // Something horrible has gone wrong
            if(game == end)
                cout << "Somehow Player " << player.GetName() << " isn't a part of a game but is INGAME" << endl;
            else
            {
                StatusCode status;
                client_connected = ReceiveStatus(player.GetSocket(), &status);

                // If the player is still connected, then perform the move
                if(client_connected)
                {
                    switch(status)
                    {
                        case MOVE:
                            // Pass the row and column right along
                            ReceiveInt(player.GetSocket(), &row);
                            ReceiveInt(player.GetSocket(), &col);
                            cout << "Received moved from " << player.GetName()
                                 << ": row=" << row << ", col=" << col << endl;

                            SendStatus((*game)->GetOtherPlayer(player).GetSocket(), MOVE);
                            SendInt((*game)->GetOtherPlayer(player).GetSocket(), row);
                            client_connected = SendInt((*game)->GetOtherPlayer(player).GetSocket(), col);
                            cout << "Sent move to " << (*game)->GetOtherPlayer(player).GetName() << endl;

                            break;
                        case WIN:
                            cout << player.GetName() << " won a game against " << (*game)->GetOtherPlayer(player).GetName() << endl;
                            client_connected = false;
                            break;
                        case DRAW:
                            cout << player.GetName() << " tied against " << (*game)->GetOtherPlayer(player).GetName() << endl;
                            client_connected = false;
                            break;
                        default:
                            client_connected = SendStatus(player.GetSocket(), INVALID_CMD);
                    }
                }
            }
        }
    }

    // The client disconnected on us D:
    cout << "Player \"" << player.GetName() << "\" has disconnected" << endl;
    DisconnectPlayer(player);
    close(client_sock);

    return (void *)0;
}

/**
 * Processes a command coming from the client
 *
 * @param buffer The command sent from the user
 * @param player A reference to the player who sent the command
 * @param game A pointer to the game the player will be a part of
 *
 * @return True if the command was valid, false otherwise
 */
bool ProcessCommand(char buffer[], Player & player, bool & client_connected)
{
    char s_command[BUF_SIZE], s_arg[BUF_SIZE];
    string command, arg;
    bool valid_cmd = true;

    // Convert the input command into separate command and argument
    int num = sscanf(buffer, "%s %s", s_command, s_arg);
    command = s_command;
    arg = s_arg;

    // Scanf needs to have captured either one or two strings
    if(command == "join" && num == 2)
    {
        CreateJoinGame(player, arg);
    }
    else if (command == "register" && num == 2)
    {
        // Set the player's name
        player.SetName(arg);
        SendStatus(player.GetSocket(), REGISTERED);
        cout << "Registered player name \"" << arg << "\"" << endl;
    }
    else if (command == "list" && num == 1)
    {
        ListGames(player);
        cout << player.GetName() << " listed all open games" << endl;
    }
    else if (command == "leave" && num == 1)
    {
        SendStatus(player.GetSocket(), PLAYER_DISCONNECT);
        client_connected = false;
    }
    else
        valid_cmd = false;

    return valid_cmd;
}

// Create or join a player to a game
void CreateJoinGame(Player & player, string game_name)
{
    pthread_mutex_lock(&games_lock);

    auto iter = find_if(game_list.begin(), game_list.end(),
        [game_name] (TTTGame * game) { return game->GetName() == game_name; });

    // Check if the game already exists
    if(iter != game_list.end())
    {
        // Check if the game already has two players
        if((*iter)->GetPlayer2() == -1)
        {
            // If not, join the two players together and notify them
            (*iter)->SetPlayer2(player);
            player.SetMode(INGAME);
            SendStatus(player.GetSocket(), JOINED);
            SendStatus((*iter)->GetPlayer1().GetSocket(), OTHER_JOINED);
        }
        else    // Otherwise, tell the player that game already exists
            SendStatus(player.GetSocket(), GAME_EXISTS);
    }
    else
    {
        // Create a new game and add it to the list
        TTTGame * game = new TTTGame(game_name, player);
        game_list.push_back(game);
        SendStatus(player.GetSocket(), CREATED);
        player.SetMode(INGAME);
    }

    pthread_mutex_unlock(&games_lock);
}

// Send a list of all open games to the player
void ListGames(const Player & player)
{
    int num_open_games = 0;

    // Send a list of all of the games
    SendStatus(player.GetSocket(), LIST);
    write(player.GetSocket(), "---All Open Games---\n", 21);

    pthread_mutex_lock(&games_lock);
    for(auto iter = game_list.begin(); iter != game_list.end(); ++iter)
    {
        // Only print out the games that don't have a player 2
        if((*iter)->GetPlayer2() == -1) {
            write(player.GetSocket(), (*iter)->GetName().c_str(), (*iter)->GetName().length());
            write(player.GetSocket(), "\n", 1);
            ++num_open_games;
        }
    }
    pthread_mutex_unlock(&games_lock);

    // If there are no open games, tell the client
    if(num_open_games == 0)
        write(player.GetSocket(), "No open games\n", 14);

    // Tell the client we're done sending
    write(player.GetSocket(), "\0", 1);
}

// Remove the player from any games they were a part of
void DisconnectPlayer(const Player & player)
{
    // Remove player from any games they were a part of and notify the other player
    pthread_mutex_lock(&games_lock);

    // Inform the other player that this player disconnected
    for(auto iter = game_list.begin(); iter != game_list.end(); ++iter)
    {
        if((*iter)->GetPlayer1() == player)
            SendStatus((*iter)->GetPlayer2().GetSocket(), OTHER_DISCONNECT);
        else if ((*iter)->GetPlayer2() == player)
            SendStatus((*iter)->GetPlayer1().GetSocket(), OTHER_DISCONNECT);
    }

    // Remove any games the player was a part of
    game_list.remove_if([player] (TTTGame * game) { return game->HasPlayer(player); });

    pthread_mutex_unlock(&games_lock);
}

/**
 * Gracefully cleanup the sockets upon catching a signal
 */
void sig_handler(int signum)
{
    switch(signum)
    {
        case SIGTERM:
        case SIGINT:
            close(server_sock);

            // Dynamically delete every game in the list and close every connected client
            for(auto iter = game_list.begin(); iter != game_list.end(); ++iter)
            {
                close((*iter)->GetPlayer1().GetSocket());
                close((*iter)->GetPlayer2().GetSocket());

                // Remove the game
                delete *iter;
            }

            game_list.clear();

            printf("Exiting server\n");
            exit(0);
            break;
        default: printf("Unrecognized signal captured: %d", signum);
    }
}