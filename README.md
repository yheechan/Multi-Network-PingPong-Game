# Multi-Network-PingPong-Game

<img src=docs/pingpong-1.gif width=500px>

## This is a multiplayer ping pong game over multi-network built with basic networking API using [Boost.Asio](https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio.html).
* Server manages all existing objects' (player 1, player 2, balls) position throughout the game.
* On client's request (keyboard press) server acts and updates the objects position.
* Every 0.1 seconds, the position of the ball is updated.
* Updated positions are broadcasted to each player for the client program to display the graphic interface.

## Game Play
* There are 3 levels: each level indicating the number of balls
	* level 1: RALLY < 5
	* level 2: 5 < RALLY < 15
	* level 3: 15 < RALLY 
* When the ball hits the middle of the player's bar, the power of the ball become 1.
* When ball hits the edge of the player's bar, the power of the ball becomes 2.

## Directory Structure
* ```./src/``` contains all the source code file for both client and server.

## Dependency
* [Boost.Asio](https://www.boost.org/doc/libs/1_82_0/doc/html/boost_asio.html)
	* Installation command: ```$ sudo apt-get install libboost-all-dev```
* [Ncurses](https://invisible-island.net/ncurses/)
	* Installation command: ```$ sudo apt-get install libncurses5-dev libncursesw5-dev```

## Command Sequence: Steps to build and run the program.
### Server Side
1. ```$ make```
2. ```$ cd ./bin/```
3. ```$ ./server <port>```

### Client 1 Side
1. ```$ make```
2. ```$ cd ./bin/```
3. ```$ ./client <ip address> <port>```

### Client 2 Side
1. ```$ make```
2. ```$ cd ./bin/```
3. ```$ ./client <ip address> <port>```
