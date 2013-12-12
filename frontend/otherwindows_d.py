#!/usr/bin/python3

from PyQt4.QtCore import *
from PyQt4.QtGui import *

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

class LogsWindow(QDialog, FormLogs):
    def __init__(self, parent = None):
        QDialog.__init__(self, parent)
        self.setupUi(self)

    def set_logs(self, messages):
        self.log_edit.setPlainText(messages)
