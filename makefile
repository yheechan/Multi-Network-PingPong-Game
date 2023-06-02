CXX = g++
CXXFLAGS = -std=c++11 -Wextra -Wpedantic
CLIENT_SOURCES = ./src/pingpong_client.cpp
SERVER_SOURCES = ./src/pingpong_server.cpp
CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)

all: client server

client: $(CLIENT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o client $(CLIENT_OBJECTS) -lpthread -lncurses -lboost_system
	mkdir bin/
	mv client ./bin/

server: $(SERVER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o server $(SERVER_OBJECTS) -lpthread -lncurses -lboost_system
	mv server ./bin/

clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS)
	rm -rf ./bin/

.PHONY: all clean
