CC = g++
CFLAGS = -std=c++11
CFLAGS += -g
LIBS = -pthread

POOL_PATH = ./ThreadPool
POOL_SRC = $(POOL_PATH)/DWThreadPool.cpp 
SERVER_SRC = DWSocketServer.cpp
CLIENT_SRC = client.cpp

all:
	make client
	make server

pool.o: $(POOL_SRC)
	$(CC) $(CFLAGS) -c $(POOL_SRC) -o pool.o $(LIBS)

server.o: $(SERVER_SRC)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o server.o $(LIBS)

server:
	make pool.o
	make server.o
	$(CC) $(CFLAGS) pool.o server.o test.cpp -o server.out $(LIBS)

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o client.out $(LIBS)


clean:
	-rm *.o *.out
