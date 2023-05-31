# Multi-Network-PingPong-Game

## This is a multiplayer ping pong game over multi-network built with basic networking API using [Boost.Asio](https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio.html).
* Server manages all existing objects' position throughout the game.
* On client's request (keyboard press) server acts and updates the objects position.
* Updated positions are broadcasted to each player for the client program to display the graphic interface.

## Directory Structure
* ```./src/``` contains all the source code file for both client and server.

## Command Sequence: Steps to build and run the program.
1. ```$ make```
2. ```$ cd bin\```
3. ```$ ./server <port>
4. ```$ ./client <ip address> <port>
