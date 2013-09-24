CC=gcc

CFLAGS=-Wall

SRV=SRV
CLI=CLI

SRV_RES=srv
CLI_RES=cli

SRC=network.c

all: 
	$(CC) $(SRC) -D$(SRV) $(CFLAGS) -o $(SRV_RES)
	$(CC) $(SRC) -D$(CLI) $(CFLAGS) -o $(CLI_RES)
