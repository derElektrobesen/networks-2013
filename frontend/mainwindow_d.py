#!/usr/bin/python3

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from forms import *
from thread import Thread

import json
import hashlib
import ntpath
import pickle
import glob
import math

import os

class MainException(Exception):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()

__ex_classes = "WrongActionException".rstrip().split()

for c in __ex_classes:
    exec("""
class {class_name}(MainException):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()
    """.format(class_name = c))

class MainWindow(QMainWindow, FormMain):
    srv = 'server'
    cli = 'client'

    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        self.cli_thread = Thread(CLI_SOCK_PATH)
        self.srv_thread = Thread(SRV_SOCK_PATH)

        self.cli_thread.msg_came.connect(self.handle_cli_backend_message)
        self.srv_thread.msg_came.connect(self.handle_srv_backend_message)
        
        self.cli_thread.error_came.connect(self.handle_cli_error)
        self.srv_thread.error_came.connect(self.handle_srv_error)

        self.cli_thread.start()
        self.srv_thread.start()

        self.load_torrents()

    def closeEvent(self, e):
        self.hide()
        self.cli_thread.stop_thread()
        self.srv_thread.stop_thread()

    def handle_srv_error(self, class_name, msg = None):
        return self.handle_error(class_name, msg, self.srv)

    def handle_cli_error(self, class_name, msg = None):
        return self.handle_error(class_name, msg, self.cli)

    def handle_srv_backend_message(self, msg):
        return self.handle_backend_message(msg, self.srv)

    def handle_cli_backend_message(self, msg):
        return self.handle_backend_message(msg, self.cli)

    def handle_error(self, class_name, msg = None, sender = None):
        QMessageBox.information(self, 'Ошибка', \
                'Возникла ошибка в потоке "{thread}": {msg}'.format( \
                (sender == self.srv and 'Сервер') or \
                (sender == self.cli and 'Клиент') or \
                'Неизвестно', \
                msg), QMessageBox.Ok)

    def handle_backend_message(self, msg, sender = None):
        try:
            self.__handle_backend_message(msg, sender)
        except MainError as e:
            self.handle_error(type(e).__name__, e.args[0])

    def __handle_backend_message(self, msg, sender = None):
        data = json.loads(msg, encoding='utf-8')
        print(data)
        # TODO

    def load_torrent(self, fname):
        with open(fname, "rb") as f:
            struct = pickle.load(f)
            self.tableView_main.add_row(
                name = struct['filename'],
                packs = math.ceil(int(struct['filesize']) / PIECE_LEN),
                sent = -1)                                          # all packs are sent

    def load_torrents(self):
        for fname in glob.glob("TORRENTS_PATH/*"):
            self.load_torrent(fname)

    @pyqtSlot()
    def on_actionStart_transmission_triggered(self):
        self.cli_thread.send_message({'action': START_TRM_ACT, 'filename': 'hell', \
                'hsum': "944218a842edf845390ccd72e27617d7", \
                'filesize': "59"})

    @pyqtSlot()
    def on_actionCreate_transmission_triggered(self):
        fname = QFileDialog.getOpenFileName(self, 'Open file to create a torrent', '~')
        struct = { \
            'default_path': fname, \
            'filesize': str(os.path.getsize(fname)), \
            'hsum': hashlib.md5(open(fname).read().encode()).hexdigest(), \
            'filename': ntpath.basename(fname), \
        }
        path = "TORRENTS_PATH/" + struct['hsum']
        with open(path, 'wb') as f:
            pickle.dump(struct, f)
        self.load_torrent(path)
