#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include "local_sock.h"
#include "status_codes.h"
#include "sockets.h"
#include "TTTBoard.h"

using std::cout;
using std::endl;

// Function prototypes
int init_client(char * server_name);
void ServerDisconnected();
bool TakeTurn(TTTBoard & board);

int client_sock = 0;

int main(int argc, char ** argv) {

    char buffer[BUF_SIZE];
    bool game_over = false;
    bool ingame = false;
    char temp;
    int row, col;
    StatusCode status;
    TTTBoard board;

    // Check command line for hostname
    if(argc != 3)
    {
        fprintf(stderr, "Usage: %s <server name> <display name>\n", argv[0]);
        exit(1);
    }

    // Initialize the client and get the client's socket
    client_sock = init_client(argv[1]);

    // Send the user's display name
    snprintf(buffer, BUF_SIZE, "register %s\n", argv[2]);
    write(client_sock, buffer, strlen(buffer));

    // Send user messages to the server
    while(!game_over)
    {
        // Wait for a response from the server
        if(!ReceiveStatus(client_sock, &status))
            ServerDisconnected();

        // Respond to the status
        switch(status)
        {
            case REGISTERED:
                cout << "Player name registered with server" << endl;
                break;
            case CREATED:
                cout << "New game created. You are X's. Waiting for a player to connect..." << endl;
                ingame = true;
                board.SetType(EXXES);
                break;
            case JOINED:
                cout << "You joined the game. You are O's. Other player's turn." << endl;
                ingame = true;
                board.SetType(OHS);
                break;
            case OTHER_JOINED:
                cout << "Somebody joined your game!" << endl;
                TakeTurn(board);
                break;
            case MOVE:
                // Receive the row and column
                if(!ReceiveInt(client_sock, &row))
                    ServerDisconnected();

                if(!ReceiveInt(client_sock, &col))
                    ServerDisconnected();

                printf("Received a move: row %d, col %d\n", row, col);
                board.OtherMakeMove(row, col);

                // Check if receiving move is loss or tie
                if(board.IsLost())
                {
                    // You lost D:!!!
                    cout << "You lost! D:" << endl;
                    game_over = true;
                }
                else if (board.IsDraw())
                {
                    cout << "You tied" << endl;
                    game_over = true;
                }
                else
                {
                    // Have the player take their turn
                    game_over = TakeTurn(board);
                }

                break;
            case LIST:
                temp = 'D';

                // Print out the list of games from the server
                while(temp != '\0')
                {
                    if(read(client_sock, &temp, 1) > 0)
                        cout << temp;
                    else
                        ServerDisconnected();
                }

                break;
            case PLAYER_DISCONNECT:
                ServerDisconnected();
                break;
            case OTHER_DISCONNECT:
                cout << "Other player disconnected :(" << endl;
                cout << "You win by default, congrats!" << endl;
                game_over = true;
                break;
            case GAME_EXISTS:
                cout << "That game already has two players connected. Try again." << endl;
                break;
            case INVALID_CMD:
                cout << "Invalid Command" << endl;
                break;
            default:
                cout << "Unrecognized response from the server" << endl;
                exit(1);
        }

        // Send commands if not in game yet
        if(!ingame)
        {
            cout << "Enter a command: ";

            // Read a line of text from the user
            fgets(buffer, BUF_SIZE - 1, stdin);

            // Write it out to the server
            write(client_sock, buffer, strlen(buffer));
        }
    }

    cout << "Thanks for playing!" << endl;

    return 0;
}

/**
 * Create the client socket and connect to the server
 */
int init_client(char * server_name)
{
    int client_sock = 0;    // This client's connection to the server
    struct addrinfo * aip;  // Server's address information
    struct addrinfo hint;   // Used when determining the server address info
    int err = 0;            // Used for capturing error messages

    // Setup hint structure for getaddrinfo
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;   // Use IPv4
    hint.ai_socktype = SOCK_STREAM; // Use TCP

    // Get server address info
    if((err = getaddrinfo(server_name, PORT_NUM, &hint, &aip)) != 0)
    {
        fprintf(stderr, "Failed to get server address info: %s", gai_strerror(err));
        exit(1);
    }

    // Create socket connection to server
    if((client_sock = socket(aip->ai_family, aip->ai_socktype, 0)) < 0)
    {
        perror("Failed to create socket");
        exit(1);
    }

    // Connect to the server
    if(connect(client_sock, aip->ai_addr, aip->ai_addrlen) < 0)
    {
        perror("Failed to connect to server");
        exit(1);
    }

    printf("Connected to server successfully!\n");

    return client_sock;
}

// Have the player take their turn
bool TakeTurn(TTTBoard & board)
{
    bool game_over = false;
    bool input_good = false;
    int row = 0, col = 0;

    // Display the board
    board.DrawBoard();

    while(!input_good)
    {
        printf("Enter move (row col): ");

        // Make sure two integers were inputted
        if(scanf("%d %d", &row, &col) == 2)
        {
            if(row < 0 || row > 2)
                printf("Invalid row input. Try again.\n");
            else if (col < 0 || col > 2)
                printf("Invalid column input. Try again.\n");
            else if(!board.IsBlank(row, col))
                printf("That cell isn't blank. Try again.\n");
            else
                input_good = true;
        }
        else
            printf("Invalid move input. Try again.\n");

        // flush any data from the internal buffers
        int c;
        while((c = getchar()) != '\n' && c != EOF);
    }

    board.PlayerMakeMove(row, col);
    cout << "Sending move to server" << endl;

    // Send the move to the server
    if(!SendStatus(client_sock, MOVE))
        ServerDisconnected();

    if(!SendInt(client_sock, row))
        ServerDisconnected();

    if(!SendInt(client_sock, col))
        ServerDisconnected();

    // Check for win/draw
    if(board.IsWon())
    {
        cout << "YOU WIN!!!!" << endl;
        game_over = true;
        SendStatus(client_sock, WIN);
    }
    else if (board.IsDraw())
    {
        cout << "You tied ._." << endl;
        game_over = true;
        SendStatus(client_sock, DRAW);
    }

    return game_over;
}

// Tell the user they've been disconnected then quit
void ServerDisconnected()
{
    printf("You've been disconnected from the server D:\n");
    close(client_sock);
    exit(1);
}