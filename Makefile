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
	$(CC) $(FLAGS) $(CFLAGS) $(DEFS) -c $^

srv: $(OBJS)
	$(CC) $(PARAMS) -D$(SRV) -c main.c
	$(CC) $(PARAMS) -D$(SRV) main.o $(OBJS) -o $(SRV_TAR)
cli: $(OBJS)
	$(CC) $(PARAMS) -D$(CLI) -c main.c
	$(CC) $(PARAMS) -D$(CLI) main.o $(OBJS) -o $(CLI_TAR)

all: srv cli
clean:
	rm -f $(OBJS) $(SRV_TAR) $(CLI_TAR)
