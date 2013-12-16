#!/usr/bin/python3

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from log import Logger

from forms import FormTorrent, FormAbout, FormLogs

class TorrentWindow(QDialog, FormTorrent):
    def __init__(self, parent = None):
        QDialog.__init__(self, parent)
        self.setupUi(self)
        self.size_edit.setValidator(QRegExpValidator(QRegExp('^[123456789][\d]*')))
        self.name_edit.setValidator(QRegExpValidator(QRegExp('^[^\s].*')))
        self.sum_edit.setValidator(QRegExpValidator(QRegExp('[\dabcdef]*')))

class AboutWindow(QDialog, FormAbout):
    def __init__(self, parent = None):
        QDialog.__init__(self, parent)
        self.setupUi(self)

class LogsWindow(QMainWindow, FormLogs):
    def __init__(self, parent = None):
        QDialog.__init__(self, parent)
        self.setupUi(self)

    def set_logs(self, messages):
        self.log_edit.setPlainText(messages)

    @pyqtSlot()
    def on_save_report_act_triggered(self):
        filename = QFileDialog.getSaveFileName(self, "Сохранить отчет", "HOME_PATH")
        if not filename:
            return
        try:
            with open(filename, 'w') as f:
                Logger.log("Отчет был сохранен в файл '{name}'".format(name = filename))
                f.write(Logger.get_log())
        except IOError as e:
            msg = "При сохранении отчета возникла ошибка: {text}".format(text = str(e.args[1]))
            Logger.log(msg)
            QMessageBox.critical(self, "Ошибка", msg)
