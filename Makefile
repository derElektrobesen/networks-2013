CC = gcc

CFLAGS = -Wall
FLAGS = -pthread -lpthread -D_MULTI_THREADED

SRV = SRV
CLI = CLI

SRV_TAR = srv
CLI_TAR = cli

DEFINES = DEBUG, \
		  PORT=7777, \
		  RETRY_TIMEOUT=3, 

DEFS = $(DEFINES:%,=-D%)

SRCS = network.c proto.c
OBJS = $(SRCS:%.c=%.o)

PARAMS = $(FLAGS) $(CFLAGS) $(DEFS)

%.o: %.c
	$(CC) $(PARAMS) -c $^

srv: $(OBJS)
	$(CC) $(PARAMS) -D$(SRV) -c main.c -o $(SRV_TAR).o
	$(CC) $(PARAMS) -D$(SRV) $(SRV_TAR).o $(OBJS) -o $(SRV_TAR)
cli: $(OBJS)
	$(CC) $(PARAMS) -D$(CLI) -c main.c -o $(CLI_TAR).o
	$(CC) $(PARAMS) -D$(CLI) $(CLI_TAR).o $(OBJS) -o $(CLI_TAR)

all: srv cli
clean:
	rm -f $(OBJS) $(SRV_TAR).o $(CLI_TAR).o $(SRV_TAR) $(CLI_TAR)
