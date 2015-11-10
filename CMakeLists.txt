cmake_minimum_required(VERSION 3.1)
project(Midterm_TicTacToe)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SERVER_FILES server.cpp)
add_executable(TicTacToe_Server ${SERVER_FILES} TTTGame.cpp TTTGame.h Player.cpp Player.h status_codes.h status_codes.cpp sockets.h sockets.cpp)
target_link_libraries(TicTacToe_Server pthread)

set(CLIENT_FILES client.cpp)
add_executable(TicTacToe_Client ${CLIENT_FILES} status_codes.h status_codes.cpp sockets.h sockets.cpp TTTBoard.cpp TTTBoard.h)
target_link_libraries(TicTacToe_Client pthread)