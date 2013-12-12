#!/usr/bin/python3

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from forms import FormMain
from thread import Thread

from log import Logger

from otherwindows import TorrentWindow, AboutWindow

import json
import hashlib
import ntpath
import pickle
import glob
import math

import os
import subprocess

class MainException(Exception):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()

__ex_classes = "WrongActionException MainError".rstrip().split()

for c in __ex_classes:
    exec("""
class {class_name}(MainException):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()
    """.format(class_name = c))

class MainWindow(QMainWindow, FormMain):
    srv = 'server'
    cli = 'client'

    clients = {}
    servers = {}

    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        if not os.path.exists("DOWNLOADS_PATH"):
            os.makedirs("DOWNLOADS_PATH")
        if not os.path.exists("TORRENTS_PATH"):
            os.makedirs("TORRENTS_PATH")

        self.actions = {
            PACKAGE_SENT_ACT: self.on_package_sent_act,
            PACKAGE_RECEIVED_ACT: self.on_package_received_act,
            SERVER_ADDED_ACT: self.on_server_added_act,
            CLIENT_ADDED_ACT: self.on_client_added_act,
            SERVER_REMOVED_ACT: self.on_server_removed_act,
            CLIENT_REMOVED_ACT: self.on_client_removed_act,
            ANSWER_ACT: self.on_answer_act,
            FILE_RECEIVED_ACT: self.on_file_received_act,
        }

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
        self.expected_response = 0

        if ("DAEMONIZE"):
            self.run_daemons()

        self.load_torrents()

    def closeEvent(self, e):
        self.hide()
        self.cli_thread.stop_thread()
        self.srv_thread.stop_thread()

    def run_daemons(self):
        cli = subprocess.Popen("CLI", stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        srv = subprocess.Popen("SRV", stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, stdin=subprocess.PIPE)

    # Actions
    def on_package_sent_act(self, data):
        Logger.log("Часть #{n} файла '{fname}' была отправлена на адрес '{ip}'" \
                .format(ip = self.servers[data['id']], n = data['piece_id'], fname = data['file_name']))
        self.servers[data['id']]['sent'] += 1
        self.tableView_client.set_packs_sent(self.servers[data['id']]['sent'])

    def on_package_received_act(self, data):
        Logger.log("С адреса '{ip}' была получена часть #{n} файла '{fname}'" \
                .format(ip = self.servers[data['id']], n = data['piece_id'],
                        fname = self.transmissions[data['hsum']]['filename']))
        self.transmissions[data['hsum']]['sent'] += 1
        self.tableView_main.set_packs_sent(packs = self.transmissions[data['hsum']]['sent'], key = data['hsum'])
        prc = self.transmissions[data['hsum']]['filesize']
        prc = self.transmissions[data['hsum']]['sent'] * PIECE_LEN / prc
        self.tableView_main.set_perc_sent(prc * 100 if prc < 1 else 100.0, data['hsum'])

    def on_server_added_act(self, data):
        Logger.log("Подключен сервер '{ip}'".format(ip = data['ip']))
        self.servers[data['id']] = {
            'ip': data['ip'],
            'sent': 0,
        }
        self.tableView_client.add_row(data['ip'])

    def on_client_added_act(self, data):
        Logger.log("Подключен клиент '{ip}'".format(ip = data['ip']))
        self.clients[data['id']] = data['ip']
        # TODO

    def on_server_removed_act(self, data):
        Logger.log("Сервер '{ip}' отключился".format(ip = self.servers[data['id']]))
        self.tableView_client.remove_row(self.servers[data['id']]['ip'])
        del self.servers[data['id']]

    def on_client_removed_act(self, data):
        Logger.log("Клиент '{ip}' отключился".format(ip = self.clients[data['id']]))
        del self.clients[data['id']]
        # TODO

    def on_answer_act(self, data):
        r = data['result']
        text = ''
        if r != '0':
            text = ": возникла ошибка {e}".format(e = data['error'])
            if data['error'] == FILE_RECEIVING_FAILURE:
                self.on_actionStop_transmission_triggered(data['hsum'])
        Logger.log("Результат выполнения операции: {r}{text}".format(r = r, text = text))

    def on_file_received_act(self, data):
        s = data['hsum']
        struct = self.transmissions[s]
        self.create_torrent_file(struct['filename'], struct['filesize'],
            data['hsum'], "DOWNLOADS_PATH/" + struct['filename'], add_row = False)
        self.load_torrent(fname = "TORRENTS_PATH/" + s)

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
                thread = (sender == self.srv and 'Сервер') or \
                         (sender == self.cli and 'Клиент') or \
                         'Неизвестно', \
                msg = msg), QMessageBox.Ok)

    def handle_backend_message(self, msg, sender = None):
        try:
            self.__handle_backend_message(msg, sender)
        except MainError as e:
            self.handle_error(type(e).__name__, e.args[0])

    def __handle_backend_message(self, msg, sender = None):
        data = json.loads(msg, encoding='utf-8')
        return self.actions[data['action']](data)

    def load_torrent(self, fname = None, sent = -1, hsum = None, filename = None,
            filesize = None, add_row = True):
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
        if add_row:
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
        fname = "TORRENTS_PATH/" + hsum
        with open(fname, 'wb') as f:
            pickle.dump(struct, f, 0)

    @pyqtSlot()
    def on_actionStart_transmission_triggered(self):
        key = self.tableView_main.current_row
        if not key:
            return
        s = self.transmissions[key]
        if not s['finished']:
            s['active'] = 1
            self.cli_thread.send_message({'action': START_TRM_ACT,
                'hsum': key, 'filename': s['filename'], 'filesize': str(s['filesize'])})
            self.on_main_table_row_changed(key)

    @pyqtSlot()
    def on_actionCreate_transmission_triggered(self):
        fname = QFileDialog.getOpenFileName(self, 'Open file to create a torrent', '~')
        if fname:
            hsum = hashlib.md5(open(fname, "rb").read()).hexdigest()
            self.create_torrent_file(ntpath.basename(fname), os.path.getsize(fname), hsum, fname)
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
    def on_actionStop_transmission_triggered(self, key = None):
        if not key:
            key = self.tableView_main.current_row
        s = self.transmissions[key]
        self.cli_thread.send_message({'action': STOP_TRM_ACT,
            'filename': s['filename'], 'hsum': key, 'filesize': str(s['filesize'])})

    @pyqtSlot('QString')
    def on_main_table_row_changed(self, key):
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
