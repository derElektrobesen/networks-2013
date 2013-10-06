CC = gcc

CFLAGS = -Wall
FLAGS = -pthread -lpthread -D_MULTI_THREADED

SRV = SRV
CLI = CLI

SRV_TAR = srv
CLI_TAR = cli

DEFINES =   DEBUG \
            PORT=7777 \
            SHORT_TIMEOUT=2 \
            LONG_TIMEOUT=10 \
            USE_LOOPBACK

# PRINT_LINES

DEFS = $(DEFINES:%=-D%)

GLOBAL_SRCS = network.c proto.c
GLOBAL_OBJS = $(GLOBAL_SRCS:%.c=%.o)

SRV_SRCS = srv.c
SRV_OBJS = $(SRV_SRCS:%.s=%.o)

CLI_SRCS = cli.c
CLI_OBJS = $(CLI_SRCS:%.c=%.o)

PARAMS = $(FLAGS) $(CFLAGS) $(DEFS)

%.o: %.c
	$(CC) $(PARAMS) -c $^

all: srv cli

srv: $(GLOBAL_OBJS) $(SRV_OBJS)
	$(CC) $(PARAMS) -D$(SRV) -c main.c -o $(SRV_TAR).o
	$(CC) $(PARAMS) -D$(SRV) $(SRV_TAR).o $(GLOBAL_OBJS) $(SRV_OBJS) -o $(SRV_TAR)
cli: $(GLOBAL_OBJS) $(CLI_OBJS)
	$(CC) $(PARAMS) -D$(CLI) -c main.c -o $(CLI_TAR).o
	$(CC) $(PARAMS) -D$(CLI) $(CLI_TAR).o $(GLOBAL_OBJS) $(CLI_OBJS) -o $(CLI_TAR)

clean:
	rm -f $(GLOBAL_OBJS) $(SRV_OBJS) $(CLI_OBJS) $(SRV_TAR).o $(CLI_TAR).o $(SRV_TAR) $(CLI_TAR)
