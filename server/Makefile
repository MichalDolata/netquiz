CC=g++
CFLAGS=-g -Wall -std=c++14 -lprotobuf -pthread
DEPS = hellomake.h
OBJ = hellomake.o hellofunc.o 

netquiz_server: main.o client.o message.o
	$(CC) -o $@ main.o client.o message.o $(CFLAGS)

message.o: message.pb.cc message.pb.h
	$(CC) -c -o $@ message.pb.cc $(CFLAGS)

client.o: client.cpp client.h
	$(CC) -c -o $@ client.cpp $(CFLAGS)

main.o: main.cpp
	$(CC) -c -o $@ main.cpp $(CFLAGS)

clean:
	rm main.o client.o message.o netquiz_server