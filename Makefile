CC=gcc

CFLAGS=-Wall
FLAGS=-pthread -lpthread -D_MULTI_THREADED

SRV=SRV
CLI=CLI

SRV_RES=srv
CLI_RES=cli

SRC=network.c

all: 
	$(CC) $(SRC) -D$(SRV) $(FLAGS) $(CFLAGS) -o $(SRV_RES)
	$(CC) $(SRC) -D$(CLI) $(FLAGS) $(CFLAGS) -o $(CLI_RES)
