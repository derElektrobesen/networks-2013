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
FORMS_DIR_NAME = forms
FORMS_DIR = $(F_DIR)/$(FORMS_DIR_NAME)
PATCHER = $(F_DIR)/patcher.pl

# gui actions
ACTS = \
	START_TRM_ACT=\"start_trm\"	\
	STOP_TRM_ACT=\"stop_trm\" \
	PACKAGE_SENT_ACT=\"package_sent\" \
	PACKAGE_RECEIVED_ACT=\"package_received\" \
	SERVER_ADDED_ACT=\"server_added\" \
	CLIENT_ADDED_ACT=\"client_added\" \
	SERVER_REMOVED_ACT=\"server_removed\" \
	CLIENT_REMOVED_ACT=\"client_removed\" \
	TERMINATE_ACT=\"terminate\" \
	ANSWER_ACT=\"answer\" \
	FILE_RECEIVED_ACT=\"file_received\" \
	FILE_RECEIVING_FAILURE=\"receiving_failure\"

HOME = $(shell echo ~)/course_prj
TORRENTS_DIR = $(HOME)/torrents
DOWNLOADS_DIR = $(HOME)/downloads
INTERFACE_CLI_SOCKET_PATH = $(HOME)/i_cli.sock
INTERFACE_SRV_SOCKET_PATH = $(HOME)/i_srv.sock
MSG_LEN_T_SIZE = 4
BUF_MAX_LEN=8000
FRONTEND_HOME = $(shell pwd)/$(F_DIR)
EXEC_DIR = $(shell pwd)
FILE_PATH_FLAG = default_path

DAEMONIZE =
DO_LOGIN =

PROTO_STRUCT_SIZE = 296

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
			MAX_PIECES_COUNT=100000u \
			MAX_PACK_NUM=1000000u \
			MAX_PACK_NUM_LEN=7 \
			$(ACTS) \
			JSON_MAX_OPTS=10 \
			MAX_CONNECTIONS=128 \
			MAX_TRANSMISSIONS=8u \
			CACHED_PIECES_COUNT=3u \
			CACHED_QUEUE_LEN=20u \
			HOME_DIR_PATH=\"$(DOWNLOADS_DIR)\" \
			APP_DIR_PATH=\"$(HOME)\" \
			LOCK_FILE_PATH=\"$(HOME)\" \
			INTERFACE_CLI_SOCKET_PATH=\"$(INTERFACE_CLI_SOCKET_PATH)\" \
			INTERFACE_SRV_SOCKET_PATH=\"$(INTERFACE_SRV_SOCKET_PATH)\" \
			FILE_PIECE_SIZE=\(10*BUF_MAX_LEN\) \
			MSG_LEN_T_SIZE=$(MSG_LEN_T_SIZE) \
			PROTO_STRUCT_SIZE=$(PROTO_STRUCT_SIZE) \
			TORRENTS_PATH=\"$(TORRENTS_DIR)\" \
			FILE_PATH_FLAG=\"$(FILE_PATH_FLAG)\" \
			DONT_DO_SRAND \
			IDENT=\"course_prj__\" \
			$(DAEMONIZE)

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

FORMS = main_form.ui about_form.ui torrent_form.ui logs_form.ui login_form.ui
F_MAIN_FILE = $(F_DIR)/main.py
PY_FILES = main statuswidget tablewidget net_sock thread mainwindow otherwindows log
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
			$(subst =,:,$(ACTS)) \
			HOME_PATH:$(FRONTEND_HOME) \
			TORRENTS_PATH:$(TORRENTS_DIR) \
			FILE_PATH_FLAG:$(FILE_PATH_FLAG) \
			DOWNLOADS_PATH:$(DOWNLOADS_DIR) \
			PIECE_LEN:$(shell echo "$(BUF_MAX_LEN) - $(PROTO_STRUCT_SIZE)" | bc) \
			CLI:$(EXEC_DIR)/$(CLI_TAR) \
			SRV:$(EXEC_DIR)/$(SRV_TAR) \
			DAEMONIZE:$(DAEMONIZE) \
			DO_LOGIN:$(DO_LOGIN)

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
