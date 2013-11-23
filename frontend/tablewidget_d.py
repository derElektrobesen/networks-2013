# -*- coding: utf-8 -*-

from PyQt4.QtGui import *
from PyQt4.QtCore import *

class TableWidget(QTableView):
    def __init__(self, parent = None, cols = []):
        super(TableWidget, self).__init__(parent)
        self.__setup_cols(cols)

    def __setup_cols(self, columns):
        self.model = QStandardItemModel(10, len(columns), self)
        for index, column in enumerate(columns):
            self.model.setHorizontalHeaderItem(index, QStandardItem(column))
        self.setModel(self.model)
        self.horizontalHeader().setResizeMode(QHeaderView.Stretch)

class MainTable(TableWidget):
    def __init__(self, parent = None):
        cols = ['Передача', 'Скорость (кб/с)', 'Пакетов всего', 'Передано пакетов', 'Передано %']
        super(MainTable, self).__init__(parent, cols)

class ClientTable(TableWidget):
    def __init__(self, parent = None):
        cols = ['Адрессат', 'Скорость (кб/с)', 'Пакетов получено']
        super(ClientTable, self).__init__(parent, cols)

class ServerTable(TableWidget):
    def __init__(self, parent = None):
        cols = ['Передача', 'Скорость (кб/с)', 'Пакетов всего', 'Передано пакетов', 'Передано %']
        super(ServerTable, self).__init__(parent, cols)
