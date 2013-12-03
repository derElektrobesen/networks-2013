#!/usr/bin/python3

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from forms import FormMain
from thread import Thread

from otherwindows import TorrentWindow, AboutWindow

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

        self.tableView_main.rowSelected.connect(self.on_main_table_row_changed)
        self.tableView_main.rowDoubleClicked.connect(self.on_row_double_clicked)

        self.transmissions = {}
        self.ignore_row_change = 0

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

    def load_torrent(self, fname = None, sent = -1, hsum = None, filename = None, filesize = None):
        if fname:
            with open(fname, "rb") as f:
                struct = pickle.load(f)
                if 'default_path' in struct:
                    sent = -1
                else:
                    sent = 0
                return self.load_torrent(sent = sent, hsum = struct['hsum'],
                        filename = struct['filename'], filesize = struct['filesize'])

        if not os.path.isfile("TORRENTS_PATH/" + hsum):
            self.create_torrent_file(filename, filesize, hsum)

        save = {}
        if type(filesize) != int:
            filesize = int(filesize)
        self.tableView_main.add_row(
            hsum = hsum,
            name = filename,
            packs = math.ceil(filesize / PIECE_LEN),
            sent = sent)
        save['filename'] = filename
        save['filesize'] = filesize
        save['active'] = 0
        if (sent == -1):
            save['finished'] = 1
        else:
            save['finished'] = 0
            save['sent'] = sent
        self.transmissions[hsum] = save

    def load_torrents(self):
        for fname in glob.glob("TORRENTS_PATH/*"):
            self.load_torrent(fname = fname)

    def create_torrent_file(self, filename, filesize, hsum, fname = None):
        if type(filesize) != str:
            filesize = str(filesize)
        struct = {'filesize': filesize, 'hsum': hsum, 'filename': filename}
        if fname:
            struct['default_path'] = fname
        else:
            fname = "TORRENTS_PATH/" + hsum
        with open(fname, 'wb') as f:
            pickle.dump(struct, f)

    @pyqtSlot()
    def on_actionStart_transmission_triggered(self):
        key = self.tableView_main.current_row
        s = self.transmissions[key]
        if not s['finished']:
            s['active'] = 1
            self.cli_thread.send_message({'action': START_TRM_ACT, 'trmid': s['trmid']})

    @pyqtSlot()
    def on_actionCreate_transmission_triggered(self):
        fname = QFileDialog.getOpenFileName(self, 'Open file to create a torrent', '~')
        self.create_torrent_file(ntpath.basename(fname), os.path.getsize(fname),
                hashlib.md5(open(fname).read().encode()).hexdigest(), fname)
        self.load_torrent(fname = "TORRENTS_PATH/" + hsum)

    @pyqtSlot()
    def on_actionRemove_transmission_triggered(self):
        key = self.tableView_main.current_row
        if not key:
            return
        if self.transmissions[key]['active']:
            self.on_actionStop_transmission_triggered()
        self.tableView_main.remove_row(key)
        try:
            os.remove("TORRENTS_PATH/" + key)
        except OSError:
            pass
        del self.transmissions[key]

    @pyqtSlot()
    def on_actionStop_transmission_triggered(self):
        key = self.tableView_main.current_row
        s = self.transmissions[key]
        self.cli_thread.send_message({'action': STOP_TRM_ACT,
            'filename': s['filename'], 'hsum': key, 'filesize': str(s['filesize'])})

    @pyqtSlot('QString')
    def on_main_table_row_changed(self, key):
        if not self.ignore_row_change:
            self.ignore_row_change = 1
            return
        if self.transmissions[key]['active']:
            self.actionStop_transmission.setEnabled(True)
            self.actionStart_transmission.setEnabled(False)
        else:
            self.actionStop_transmission.setEnabled(False)
            self.actionStart_transmission.setEnabled(True)
        self.actionRemove_transmission.setEnabled(True)

    @pyqtSlot('QString')
    def on_row_double_clicked(self, key):
        form = TorrentWindow(self)
        s = self.transmissions[key]
        form.size_edit.setText(str(s['filesize']))
        form.sum_edit.setText(key)
        form.name_edit.setText(s['filename'])
        form.save_btn.setVisible(False)
        form.show()

    @pyqtSlot()
    def on_actionAdd_torrent_triggered(self):
        form = TorrentWindow(self)
        form.size_edit.setReadOnly(False)
        form.sum_edit.setReadOnly(False)
        form.name_edit.setReadOnly(False)
        form.save_btn.clicked.connect(self.add_remote_torrent)
        self.current_torrent_form = form
        form.show()

    @pyqtSlot(QEvent)
    def add_remote_torrent(self):
        form = self.current_torrent_form
        if (len(form.sum_edit.text()) != 32):
            QMessageBox.information(self, 'Ошибка',
                'Необходимо указать корректную MD5 сумму файла', QMessageBox.Ok)
        else:
            form.close()
            self.load_torrent(filename = form.name_edit.text(),
                    filesize = form.size_edit.text(), sent = 0,
                    hsum = form.sum_edit.text())
