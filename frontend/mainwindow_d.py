#!/usr/bin/python3

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from forms import FormMain
from thread import Thread

from log import Logger

from otherwindows import TorrentWindow, AboutWindow, LogsWindow, LoginWindow

import json
import hashlib
import ntpath
import pickle
import glob
import math
import random
import time

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

    downloaded_color = QColor(0xe2, 0xf5, 0xff)

    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        if not os.path.exists("DOWNLOADS_PATH"):
            os.makedirs("DOWNLOADS_PATH")
        if not os.path.exists("TORRENTS_PATH"):
            os.makedirs("TORRENTS_PATH")

        r = self.geometry()
        r.moveCenter(QApplication.desktop().availableGeometry().center())
        self.setGeometry(r)

        if ("DO_LOGIN"):
            self.show_login_form()

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

        self.timer = QTimer(self)
        self.connect(self.timer, SIGNAL("timeout()"), self.on_timer)
        self.timer.start(100)

        self.timer_iteration = 0

        self.transmissions = {}
        self.expected_response = 0

        Logger.log("Старт приложения")

        if ("DAEMONIZE"):
            self.run_daemons()

        self.load_torrents()

    def show_login_form(self):
        form = LoginWindow(self)
        form.show()

    def closeEvent(self, e):
        Logger.log("Завершение приложения")
        self.hide()
        self.cli_thread.stop_thread()
        self.srv_thread.stop_thread()

    def run_daemons(self):
        cli = subprocess.Popen("CLI", stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        Logger.log("Клиент запущен")
        srv = subprocess.Popen("SRV", stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, stdin=subprocess.PIPE)
        Logger.log("Сервер запущен")

    # Actions
    def on_package_sent_act(self, data):
        Logger.log("Часть #{n} файла '{fname}' была отправлена на адрес '{ip}'" \
                .format(ip = self.servers[data['id']], n = data['piece_id'], fname = data['file_name']))
        self.servers[data['id']]['sent'] += 1
        self.tableView_client.set_packs_sent(self.servers[data['id']]['sent'])

    def on_package_received_act(self, data):
        Logger.log("С адреса '{ip}' была получена часть #{n} файла '{fname}'" \
                .format(ip = self.servers[data['id']]['ip'], n = data['piece_id'],
                        fname = self.transmissions[data['hsum']]['filename']))
        self.transmissions[data['hsum']]['sent'] += 1
        self.tableView_main.set_packs_sent(packs = self.transmissions[data['hsum']]['sent'], key = data['hsum'])
        prc = self.transmissions[data['hsum']]['filesize']
        prc = self.transmissions[data['hsum']]['sent'] * PIECE_LEN / prc
        self.tableView_main.set_perc_sent(prc * 100 if prc < 1 else 100.0, data['hsum'])
        self.statusWidget.add_line(data['hsum'], data['piece_id'],
                self.servers[data['id']]['color'])

    def on_server_added_act(self, data):
        Logger.log("Подключен сервер '{ip}'".format(ip = data['ip']))
        self.servers[data['id']] = {
            'ip': data['ip'],
            'sent': 0,
            'color': QColor.fromHsv(random.randint(0, 359), 220, 190),
        }
        self.tableView_client.add_row(data['ip'])

    def on_client_added_act(self, data):
        Logger.log("Подключен клиент '{ip}'".format(ip = data['ip']))
        self.clients[data['id']] = data['ip']
        # TODO

    def on_server_removed_act(self, data):
        Logger.log("Сервер '{ip}' отключился".format(ip = self.servers[data['id']]['ip']))
        self.tableView_client.remove_row(self.servers[data['id']]['ip'])
        del self.servers[data['id']]

    def on_client_removed_act(self, data):
        Logger.log("Клиент '{ip}' отключился".format(ip = self.clients[data['id']]))
        del self.clients[data['id']]
        # TODO

    def on_answer_act(self, data):
        r = int(data['result'])
        text = ''
        if r != 0:
            text = ": возникла ошибка {e}".format(e = data['error'])
            if data['error'] == FILE_RECEIVING_FAILURE:
                self.on_actionStop_transmission_triggered(data['hsum'])
        Logger.log("Результат выполнения операции: {r}{text}".format(r = r, text = text))

        if 'act' in data and 'hsum' in data and data['hsum'] in self.transmissions:
            trm = self.transmissions[data['hsum']]
            if (data['act'] == STOP_TRM_ACT and r == 0) or (data['act'] == START_TRM_ACT and r != 0):
                trm['sent'] = trm['active'] = trm['finished'] = 0
            elif data['act'] == START_TRM_ACT:
                trm['trmid'] = data['trmid']
                trm['active'] = 1
            self.on_main_table_row_changed(data['hsum'])

    def on_file_received_act(self, data):
        s = data['hsum']
        struct = self.transmissions[s]
        Logger.log("Файл '{name}' был полностью получен".format(name = struct['filename']))
        self.create_torrent_file(struct['filename'], struct['filesize'],
            data['hsum'], "DOWNLOADS_PATH/" + struct['filename'])
        self.load_torrent(fname = "TORRENTS_PATH/" + s, add_row = False)
        struct['sent'] = -1
        struct['active'] = 0
        struct['finished'] = 1
        self.on_main_table_row_changed(s)

    def handle_srv_error(self, class_name, msg = None):
        Logger.log("Возникла ошибка в потоке сервера{msg}" \
                .format(msg = ": " + msg if msg else ""))
        return self.handle_error(class_name, msg, self.srv)

    def handle_cli_error(self, class_name, msg = None):
        Logger.log("Возникла ошибка в потоке клиента{msg}" \
                .format(msg = ": " + msg if msg else ""))
        return self.handle_error(class_name, msg, self.cli)

    def handle_srv_backend_message(self, msg):
        Logger.log("Возникла ошибка на стороне сервера{msg}" \
                .format(msg = ": " + msg if msg else ""))
        return self.handle_backend_message(msg, self.srv)

    def handle_cli_backend_message(self, msg):
        Logger.log("Возникла ошибка на стороне клиента{msg}" \
                .format(msg = ": " + msg if msg else ""))
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
                        filename = struct['filename'], filesize = struct['filesize'],
                        add_row = add_row)

        if not os.path.isfile("TORRENTS_PATH/" + hsum):
            self.create_torrent_file(filename, filesize, hsum)

        if type(filesize) != int:
            filesize = int(filesize)
        pieces_count = math.ceil(filesize / PIECE_LEN)

        Logger.log("Был загружен торрент файл для файла '{name}'{status}" \
                .format(name = filename, status = ", статус: загружен" if sent == -1 else ""))

        save = {
            'filename': filename,
            'filesize': filesize,
            'active': 0,
            'sent': sent,
            'last_index': sent,
        }
        if (sent == -1):
            save['finished'] = 1
        else:
            save['finished'] = 0
        self.transmissions[hsum] = save

        if add_row:
            self.tableView_main.add_row(hsum = hsum, name = filename, packs = pieces_count, sent = sent)
            self.statusWidget.init_elem(hsum, pieces_count)
            if sent == -1:
                self.statusWidget.add_rect(hsum, 0, pieces_count - 1, self.downloaded_color)

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
        Logger.log("Был создан торрент файл для файла '{name}'" \
                .format(name = filename))
        with open(fname, 'wb') as f:
            pickle.dump(struct, f, 0)

    def update_speed(self):
        for key, e in self.transmissions.items():
            delta = e['sent'] - e['last_index']
            speed = delta * PIECE_LEN / 1024.0
            self.tableView_main.set_speed(speed, key)
            e['last_index'] = e['sent']

    @pyqtSlot()
    def on_actionStart_transmission_triggered(self):
        key = self.tableView_main.current_row
        if not key:
            return
        s = self.transmissions[key]
        if not s['finished']:
            Logger.log("Отправлен запрос на получение файла '{name}'" \
                .format(name = s['filename']))
            self.cli_thread.send_message({'action': START_TRM_ACT,
                'hsum': key, 'filename': s['filename'], 'filesize': str(s['filesize'])})

    @pyqtSlot()
    def on_actionCreate_transmission_triggered(self):
        fname = QFileDialog.getOpenFileName(self, 'Open file to create a torrent', '~')
        if fname:
            hsum = hashlib.md5(open(fname, "rb").read()).hexdigest()
            if not os.path.exists("TORRENTS_PATH/" + hsum):
                self.create_torrent_file(ntpath.basename(fname), os.path.getsize(fname), hsum, fname)
                self.load_torrent(fname = "TORRENTS_PATH/" + hsum)
            else:
                QMessageBox.information(self, 'Ошибка',
                    'Торрент файл уже существует', QMessageBox.Ok)

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
        Logger.log("Торрен файл для файла '{name}' был удален" \
                .format(name = self.transmissions[key]['filename']))
        del self.transmissions[key]

    @pyqtSlot()
    def on_actionStop_transmission_triggered(self, key = None):
        if not key:
            key = self.tableView_main.current_row
        s = self.transmissions[key]
        if s['active']:
            Logger.log("Отправлен запрос на остановку получения файла '{name}'" \
                .format(name = s['filename']))
            self.cli_thread.send_message({'action': STOP_TRM_ACT, 'filename': s['filename'],
                'hsum': key, 'filesize': str(s['filesize']), 'trmid': s['trmid']})
            self.transmissions[key]['active'] = 0
            self.statusWidget.remove_lines(key)
            self.on_main_table_row_changed(key)
            s['active'] = s['finished'] = s['sent'] = 0

    @pyqtSlot('QString')
    def on_main_table_row_changed(self, key):
        if self.transmissions[key]['active']:
            self.actionStop_transmission.setEnabled(True)
            self.actionStart_transmission.setEnabled(False)
        else:
            self.actionStop_transmission.setEnabled(False)
            self.actionStart_transmission.setEnabled(self.transmissions[key]['sent'] != -1)
        self.actionRemove_transmission.setEnabled(True)
        self.statusWidget.current_elem = key

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

    @pyqtSlot()
    def on_action_showLogs_triggered(self):
        form = LogsWindow(self)
        form.set_logs(Logger.get_log())
        form.show()

    @pyqtSlot()
    def on_actionAbout_triggered(self):
        form = AboutWindow(self)
        form.show()

    @pyqtSlot()
    def on_timer(self):
        speed_time = 10
        status_w_time = 3
        table_w_time = 5
        if self.timer_iteration % speed_time == 0:
            self.update_speed()
        if self.timer_iteration % status_w_time == 0:
            self.statusWidget.repaint()
        if self.timer_iteration % table_w_time == 0:
            self.tableView_main.update_values()
            self.tableView_client.update_values()
            self.tableView_server.update_values()
        if self.timer_iteration > 100 * speed_time * status_w_time * table_w_time:
            self.timer_iteration = 0
        self.timer_iteration += 1

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
