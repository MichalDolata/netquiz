CC=g++
CFLAGS=-g -Wall -std=c++14 -lprotobuf -pthread -I ../protobuf
DEPS = hellomake.h
OBJ = hellomake.o hellofunc.o 

netquiz_server: main.o client.o message.o question.o
	$(CC) -o $@ main.o client.o message.o question.o $(CFLAGS)

message.o: ../protobuf/message.pb.cc ../protobuf/message.pb.h
	$(CC) -c -o $@ ../protobuf/message.pb.cc $(CFLAGS)

client.o: client.cpp client.h
	$(CC) -c -o $@ client.cpp $(CFLAGS)

question.o: question.cpp question.h
	$(CC) -c -o $@ question.cpp $(CFLAGS)

main.o: main.cpp
	$(CC) -c -o $@ main.cpp $(CFLAGS)

clean:
	rm main.o client.o message.o question.o netquiz_server