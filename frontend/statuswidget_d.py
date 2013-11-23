# -*- coding: utf-8 -*-

import sys

sys.path.append('/home/pavel/projects/Networks/networks-2013/frontend')


from PyQt4.QtGui import *
from PyQt4.QtCore import *

class StatusWidget(QWidget):
    def __init__(self, parent):
        QWidget.__init__(self, parent)

    def paintEvent(self, e):
        painter = QPainter(self)
        painter.setBrush(Qt.white)
        painter.setPen(Qt.red)
        painter.drawRect(0, 0, self.width() - 1, self.height() - 1)
