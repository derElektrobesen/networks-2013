CC = gcc

CFLAGS = -Wall
FLAGS = -pthread -lpthread -D_MULTI_THREADED -lssl -lcrypto -MMD

SRV = SRV
CLI = CLI

SRV_TAR = srv
CLI_TAR = cli
O_DIR = obj

DEFINES =   DEBUG \
			PORT=7777 \
			SHORT_TIMEOUT=2 \
			LONG_TIMEOUT=10 \
			ALARM_DELAY=1 \
			FILE_TIMEOUT=3 \
			BUF_MAX_LEN=16384 \
			BUF_MAX_LEN_TSIZE=4 \
			CONTROL_INFO_LEN=32 \
			FILE_NAME_MAX_LEN=4096 \
			MAX_PIECES_COUNT=10000 \
			MAX_PACK_NUM=1000000 \
			MAX_PACK_NUM_LEN=7 \
			MAX_CONNECTIONS=128 \
			MAX_TRANSMISSIONS=128 \
			DONT_DO_SRAND \
			USE_LOOPBACK

# Unused macro defs
# PRINT_LINES

DEFS = $(DEFINES:%=-D%)

GLOBAL_SRCS = network.c proto.c
GLOBAL_OBJS = $(GLOBAL_SRCS:%.c=$(O_DIR)/%.o)

SRV_SRCS = srv.c
SRV_OBJS = $(SRV_SRCS:%.c=$(O_DIR)/%.o)

CLI_SRCS = cli.c
CLI_OBJS = $(CLI_SRCS:%.c=$(O_DIR)/%.o)

PARAMS = $(FLAGS) $(CFLAGS) $(DEFS)

.PHONY: all clean

all: srv cli

$(O_DIR):
	@mkdir -p $(O_DIR)

$(O_DIR)/%.o: %.c | $(O_DIR)
	$(CC) $(PARAMS) -c $< -o $@

srv: $(GLOBAL_OBJS) $(SRV_OBJS)
	$(CC) $(PARAMS) -D$(SRV) -c main.c -o $(O_DIR)/server.o
	$(CC) $(PARAMS) -D$(SRV) $(O_DIR)/server.o $(GLOBAL_OBJS) $(SRV_OBJS) -o $(SRV_TAR)
cli: $(GLOBAL_OBJS) $(CLI_OBJS)
	$(CC) $(PARAMS) -D$(CLI) -c main.c -o $(O_DIR)/client.o
	$(CC) $(PARAMS) -D$(CLI) $(O_DIR)/client.o $(GLOBAL_OBJS) $(CLI_OBJS) -o $(CLI_TAR)

clean:
	rm -f $(O_DIR)/* $(SRV_TAR) $(CLI_TAR)

-include $(O_DIR)/*.d
