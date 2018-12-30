CC=g++
CFLAGS=-Wall -std=c++14 
DEPS = hellomake.h
OBJ = hellomake.o hellofunc.o 

netquiz_server: main.o
	$(CC) -o $@ main.o $(CFLAGS)

main.o: main.cpp
	$(CC) -c -o $@ main.cpp $(CFLAGS)
