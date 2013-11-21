CC = gcc

CFLAGS = -Wall
FLAGS = -pthread -lpthread -D_MULTI_THREADED -MMD -lm
DEBUG_FLAGS = -ggdb3

SRV = SRV
CLI = CLI

SRV_TAR = srv
CLI_TAR = cli
O_DIR = obj

B_DIR = backend
F_DIR = frontend
FORMS_DIR = $(F_DIR)/forms
PATCHER = $(F_DIR)/patcher.pl

DEFINES =   DEBUG \
			PORT=7777 \
			SHORT_TIMEOUT=2 \
			LONG_TIMEOUT=3 \
			ALARM_U_DELAY=10000 \
			ALARM_S_DELAY=0 \
			FILE_TIMEOUT=10000u \
			BUF_MAX_LEN=8000l\
			BUF_MAX_LEN_TSIZE=4 \
			CONTROL_INFO_LEN=32 \
			FILE_NAME_MAX_LEN=255 \
			FULL_FILE_NAME_MAX_LEN=1024 \
			TMP_FILE_MAX_LEN=255 \
			MAX_PIECES_COUNT=10000u \
			MAX_PACK_NUM=1000000u \
			MAX_PACK_NUM_LEN=7 \
			MAX_CONNECTIONS=128 \
			MAX_TRANSMISSIONS=8u \
			CACHED_PIECES_COUNT=3u \
			HOME_DIR_PATH=\"/tmp/course_prj/downloads\" \
			APP_DIR_PATH=\"/tmp/course_prj\" \
			LOCK_FILE_PATH=\"/var/run/\" \
			FILE_PIECE_SIZE=\(10*BUF_MAX_LEN\) \
			MSG_LEN_T_SIZE=8 \
			DONT_DO_SRAND \
			USE_LOOPBACK

# Unused macro defs
# PRINT_LINES
# DAEMONIZE

DEFS = $(DEFINES:%=-D%)

MAIN_FILE=$(B_DIR)/main.c

GLOBAL_SRCS = network.c md5.c proto.c json.c
GLOBAL_OBJS = $(GLOBAL_SRCS:%.c=$(O_DIR)/%.o)

SRV_SRCS = srv.c
SRV_OBJS = $(SRV_SRCS:%.c=$(O_DIR)/%.o)

CLI_SRCS = cli.c
CLI_OBJS = $(CLI_SRCS:%.c=$(O_DIR)/%.o)

FORMS = main_form.ui about_form.ui
PY_FILES = main
UIGEN = pyuic4

PARAMS = $(FLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS)
UI_RULES = statusWidget:StatusWidget:statuswidget*

.PHONY: all clean

all: srv cli gui

$(O_DIR):
	@mkdir -p $(O_DIR)

$(O_DIR)/%.o: $(B_DIR)/%.c | $(O_DIR)
	$(CC) $(PARAMS) -c $< -o $@

$(FORMS_DIR)/%.py: $(FORMS_DIR)/%.ui
	$(UIGEN) $< -o $@
	$(PATCHER) -i $@ -m $(UI_RULES) -o $@.new -w $(shell pwd)/$(F_DIR)
	@rm -fv $@
	@mv -fv $@.new $@

$(F_DIR)/%: $(F_DIR)/%_default.py
	$(PATCHER) -i $@_default.py -o $@.py -w $(shell pwd)/$(F_DIR) -e forms

srv: $(GLOBAL_OBJS:%.c=$(B_DIR)/%.c) $(SRV_OBJS:%.c=$(B_DIR)/%.c) $(MAIN_FILE)
	$(CC) $(PARAMS) -D$(SRV) -c $(MAIN_FILE) -o $(O_DIR)/server.o
	$(CC) $(PARAMS) -D$(SRV) $(O_DIR)/server.o $(GLOBAL_OBJS) $(SRV_OBJS) -o $(SRV_TAR)

cli: $(GLOBAL_OBJS) $(CLI_OBJS) $(MAIN_FILE)
	$(CC) $(PARAMS) -D$(CLI) -c $(MAIN_FILE) -o $(O_DIR)/client.o
	$(CC) $(PARAMS) -D$(CLI) $(O_DIR)/client.o $(GLOBAL_OBJS) $(CLI_OBJS) -o $(CLI_TAR)

gui: $(PY_FILES:%=$(F_DIR)/%) $(FORMS:%.ui=$(FORMS_DIR)/%.py) $(PATCHER)

clean:
	@rm -f $(O_DIR)/* $(SRV_TAR) $(CLI_TAR) $(FORMS_DIR)/*.py -v

-include $(O_DIR)/*.d
