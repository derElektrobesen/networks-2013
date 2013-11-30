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

# gui actions
START_TRM = \"0\"
STOP_TRM = \"1\"
PACKAGE_SENT = \"2\"
PACKAGE_RECIEVED = \"3\"
SERVER_ADDED = \"4\"
CLIENT_ADDED = \"5\"
SERVER_REMOVED = \"6\"
CLIENT_REMOVED = \"7\"
TERMINATE = \"8\"

HOME = /tmp/course_prj
INTERFACE_CLI_SOCKET_PATH = $(HOME)/i_cli.sock
INTERFACE_SRV_SOCKET_PATH = $(HOME)/i_srv.sock
MSG_LEN_T_SIZE = 8
BUF_MAX_LEN=8000
FRONTEND_HOME = $(shell pwd)/$(F_DIR)

DEFINES =   DEBUG \
			PORT=7777 \
			SHORT_TIMEOUT=2 \
			LONG_TIMEOUT=3 \
			ALARM_U_DELAY=10000 \
			ALARM_S_DELAY=0 \
			FILE_TIMEOUT=10000u \
			BUF_MAX_LEN=$(BUF_MAX_LEN)l\
			BUF_MAX_LEN_TSIZE=4 \
			CONTROL_INFO_LEN=32 \
			FILE_NAME_MAX_LEN=255 \
			FULL_FILE_NAME_MAX_LEN=1024 \
			TMP_FILE_MAX_LEN=255 \
			MAX_PIECES_COUNT=10000u \
			MAX_PACK_NUM=1000000u \
			MAX_PACK_NUM_LEN=7 \
			START_TRM_ACT=$(START_TRM) \
			STOP_TRM_ACT=$(STOP_TRM) \
			PACKAGE_SENT_ACT=$(PACKAGE_SENT) \
			PACKAGE_RECIEVED_ACT=$(PACKAGE_RECIEVED) \
			SERVER_ADDED_ACT=$(SERVER_ADDED) \
			CLIENT_ADDED_ACT=$(CLIENT_ADDED) \
			SERVER_REMOVED_ACT=$(SERVER_REMOVED) \
			CLIENT_REMOVED_ACT=$(CLIENT_REMOVED) \
			TERMINATE_ACT=$(TERMINATE) \
			JSON_MAX_OPTS=10 \
			MAX_CONNECTIONS=128 \
			MAX_TRANSMISSIONS=8u \
			CACHED_PIECES_COUNT=3u \
			CACHED_QUEUE_LEN=20u \
			HOME_DIR_PATH=\"$(HOME)/downloads\" \
			APP_DIR_PATH=\"$(HOME)\" \
			LOCK_FILE_PATH=\"/var/run/\" \
			INTERFACE_CLI_SOCKET_PATH=\"$(INTERFACE_CLI_SOCKET_PATH)\" \
			INTERFACE_SRV_SOCKET_PATH=\"$(INTERFACE_SRV_SOCKET_PATH)\" \
			FILE_PIECE_SIZE=\(10*BUF_MAX_LEN\) \
			MSG_LEN_T_SIZE=$(MSG_LEN_T_SIZE) \
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
F_MAIN_FILE = $(F_DIR)/main.py
PY_FILES = main statuswidget tablewidget net_sock thread
UIGEN = pyuic4
UIGEN_EXISTS = $(shell $(UIGEN) --version 2>/dev/null)

DEFAULT_POSTFIX = _d

PARAMS = $(FLAGS) $(CFLAGS) $(DEBUG_FLAGS) $(DEFS)

UI_RULES_ = statusWidget:StatusWidget:statuswidget \
		   ../images:images \
		   tableView_main:MainTable:tablewidget \
		   tableView_client:ClientTable:tablewidget \
		   tableView_server:ServerTable:tablewidget
MAIN_RULES_ = \
			CLI_SOCK_PATH:\"$(INTERFACE_CLI_SOCKET_PATH)\" \
			SRV_SOCK_PATH:\"$(INTERFACE_SRV_SOCKET_PATH)\" \
			MSG_MAX_LEN:$(BUF_MAX_LEN) \
			LEN_MSG_LEN:$(MSG_LEN_T_SIZE) \
			START_TRM:$(START_TRM) \
			STOP_TRM:$(STOP_TRM) \
			PACKAGE_SENT:$(PACKAGE_SENT) \
			PACKAGE_RECIEVED:$(PACKAGE_RECIEVED) \
			SERVER_ADDED:$(SERVER_ADDED) \
			CLIENT_ADDED:$(CLIENT_ADDED) \
			SERVER_REMOVED:$(SERVER_REMOVED) \
			CLIENT_REMOVED:$(CLIENT_REMOVED) \
			TERMINATE:$(TERMINATE) \
			HOME_PATH:$(FRONTEND_HOME)

CREATE_RULE = $(shell echo '$1' | perl -e 'my $$r = ""; while (<>) { s/\s+/*/g; $$r .= $$_; } print "$$r"')
UI_RULES = $(call CREATE_RULE, $(UI_RULES_))
MAIN_RULES = $(call CREATE_RULE, $(MAIN_RULES_))
 
.PHONY: all clean

all: srv cli gui

$(O_DIR):
	@mkdir -p $(O_DIR)

$(O_DIR)/%.o: $(B_DIR)/%.c | $(O_DIR)
	$(CC) $(PARAMS) -c $< -o $@

$(FORMS_DIR)/%.py: $(FORMS_DIR)/%.ui
ifeq ($(UIGEN_EXISTS),)
	@echo "ERROR: pyiuc4 not found"
else
	$(UIGEN) $< -o $@
	$(PATCHER) -i $@ -m $(UI_RULES) -o $@.new -w $(FRONTEND_HOME)
	@rm -f $@
	@mv -f $@.new $@
endif

$(F_DIR)/%.py: $(F_DIR)/%$(DEFAULT_POSTFIX).py
	$(PATCHER) -i $^ -m $(MAIN_RULES) -o $(F_DIR)/$*.py -w $(FRONTEND_HOME)
	@chmod +x $(F_MAIN_FILE)

srv: $(GLOBAL_OBJS:%.c=$(B_DIR)/%.c) $(SRV_OBJS:%.c=$(B_DIR)/%.c) $(MAIN_FILE)
	$(CC) $(PARAMS) -D$(SRV) -c $(MAIN_FILE) -o $(O_DIR)/server.o
	$(CC) $(PARAMS) -D$(SRV) $(O_DIR)/server.o $(GLOBAL_OBJS) $(SRV_OBJS) -o $(SRV_TAR)

cli: $(GLOBAL_OBJS) $(CLI_OBJS) $(MAIN_FILE)
	$(CC) $(PARAMS) -D$(CLI) -c $(MAIN_FILE) -o $(O_DIR)/client.o
	$(CC) $(PARAMS) -D$(CLI) $(O_DIR)/client.o $(GLOBAL_OBJS) $(CLI_OBJS) -o $(CLI_TAR)

gui: $(PY_FILES:%=$(F_DIR)/%.py) $(FORMS:%.ui=$(FORMS_DIR)/%.py) $(PATCHER)

clean:
	@rm -f $(O_DIR)/* $(SRV_TAR) $(CLI_TAR) $(FORMS:%.ui=$(FORMS_DIR)/%.py) $(PY_FILES:%=$(F_DIR)/%.py)
	@echo "Directory has been successfully cleaned."

-include $(O_DIR)/*.d
