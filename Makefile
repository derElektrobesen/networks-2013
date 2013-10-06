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
            TIMEOUT_DELAY=1 \
            BUF_MAX_LEN=1024 \
            MAX_PIECES_COUNT=10000 \
            USE_LOOPBACK \
            DONT_DO_SRAND

# Unused macro defs
# PRINT_LINES

DEFS = $(DEFINES:%=-D%)

GLOBAL_SRCS = network.c proto.c
GLOBAL_OBJS = $(GLOBAL_SRCS:%.c=%.o)

SRV_SRCS = srv.c
SRV_OBJS = $(SRV_SRCS:%.c=%.o)

CLI_SRCS = cli.c
CLI_OBJS = $(CLI_SRCS:%.c=%.o)

PARAMS = $(FLAGS) $(CFLAGS) $(DEFS)

%.o: %.c
	$(CC) $(PARAMS) -c $^

all: srv cli

srv: $(GLOBAL_OBJS) $(SRV_OBJS)
	$(CC) $(PARAMS) -D$(SRV) -c main.c -o server.o
	$(CC) $(PARAMS) -D$(SRV) server.o $(GLOBAL_OBJS) $(SRV_OBJS) -o $(SRV_TAR)
cli: $(GLOBAL_OBJS) $(CLI_OBJS)
	$(CC) $(PARAMS) -D$(CLI) -c main.c -o client.o
	$(CC) $(PARAMS) -D$(CLI) client.o $(GLOBAL_OBJS) $(CLI_OBJS) -o $(CLI_TAR)

clean:
	rm -f $(GLOBAL_OBJS) $(SRV_OBJS) $(CLI_OBJS) client.o server.o $(SRV_TAR) $(CLI_TAR)
