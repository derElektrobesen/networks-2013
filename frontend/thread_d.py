# -*- coding: utf-8 -*-

from PyQt4.QtCore import *

class Thread(QThread):
    def __init__(self, callback):
        # callback takes 1 argument: self (QThread) * to emit some actions *
        super(Thread, self).__init__()
        self.callback = callback

    def __del__(self):
        self.wait()

    def run(self):
        callback(self)
