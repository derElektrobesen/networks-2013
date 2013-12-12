# -*- coding: utf-8 -*-

from PyQt4.QtGui import *
from PyQt4.QtCore import *

class TableWidget(QTableView):
    rowSelected = pyqtSignal('QString')
    rowDoubleClicked = pyqtSignal('QString')
    def __init__(self, parent = None, cols = []):
        super(TableWidget, self).__init__(parent)
        self.__setup_cols(cols)
        self.__keys = {}
        self.__cur_row = ''
        
        selection_model = self.selectionModel()
        QObject.connect(selection_model, SIGNAL('currentRowChanged(QModelIndex, QModelIndex)'),
                self.__on_row_changed)

        QObject.connect(self, SIGNAL('doubleClicked(QModelIndex)'), self.on_double_click)

    def __setup_cols(self, columns):
        self.__model = QStandardItemModel(0, len(columns), self)
        for index, column in enumerate(columns):
            self.__model.setHorizontalHeaderItem(index, QStandardItem(column))
        self.setModel(self.__model)
        self.horizontalHeader().setResizeMode(QHeaderView.Stretch)

    def add_row(self, row, key):
        rrow = []
        for cell in row:
            rrow.append(QStandardItem(str(cell)))
        self.__keys[key] = rrow[0]
        self.__model.appendRow(rrow)
        self.__model.sort(self.__model.sortRole())
        if not self.__cur_row:
            self.select_row(0)

    def select_row(self, row_index):
        self.selectionModel().select(QItemSelection(
                self.__model.item(row_index, 0).index(),
                self.__model.item(row_index, self.__model.columnCount() - 1).index()),
                QItemSelectionModel.Select)

    def remove_row(self, key):
        self.__model.takeRow(self.__keys[key].index().row())

    def set_sort_role(self, index):
        self.__model.setSortRole(index)

    def get_key_by_index(self, index):
        item = self.__model.item(index.row(), 0)
        k = ''
        for key in self.__keys:
            if self.__keys[key] == item:
                k = key
                break
        return k

    def change_row(self, col_index, row_index = -1, row_key = '', data = ''):
        if row_index == -1 and row_key == '':
            raise ValueError("Incorrect params given: row_index or row_key are expected")
        if row_key:
            row_index = self.__keys[row_key].row()
        self.__model.item(row_index, col_index).setText(data)

    @property
    def current_row(self):
        return self.__cur_row

    @pyqtSlot(QModelIndex, QModelIndex)
    def __on_row_changed(self, current, previous):
        k = self.get_key_by_index(current)
        self.__cur_row = k
        self.rowSelected.emit(k)

    @pyqtSlot(QModelIndex)
    def on_double_click(self, index):
        k = self.get_key_by_index(index)
        self.rowDoubleClicked.emit(k)

class MainTable(TableWidget):
    def __init__(self, parent = None):
        cols = ['Передача', 'Скорость (кб/с)', 'Пакетов всего', 'Передано пакетов', 'Передано %']
        super(MainTable, self).__init__(parent, cols)
        self.set_sort_role(0)

    def add_row(self, hsum, name = '', packs = 0, sent = -1, speed = 0):
        sent = packs if sent < 0 else sent
        super(MainTable, self).add_row([name, speed, packs, sent,
            packs / sent * 100 if sent != 0 else 0], hsum)
        self.set_sort_role(0)

    def set_speed(self, speed, key):
        super(MainTable, self).change_row(1, row_key = key, data = "%.2f" % speed)

    def set_packs_sent(self, packs, key):
        super(MainTable, self).change_row(3, row_key = key, data = str(packs))

    def set_perc_sent(self, perc, key):
        super(MainTable, self).change_row(4, row_key = key, data = "%.1f" % perc)

class ClientTable(TableWidget):
    def __init__(self, parent = None):
        cols = ['Адресат', 'Скорость (кб/с)', 'Пакетов получено']
        super(ClientTable, self).__init__(parent, cols)
        self.set_sort_role(0)

    def add_row(self, ip):
        super(ClientTable, self).add_row([ip, 0, 0], ip)

    def set_packs_sent(self, packs, ip):
        super(ClientTable, self).change_row(2, row_key = ip, data = str(packs))

    def set_speed(self, speed, ip):
        super(ClientTable, self).change_row(1, row_key = ip, data = str(speed))

class ServerTable(TableWidget):
    def __init__(self, parent = None):
        cols = ['Передача', 'Скорость (кб/с)', 'Пакетов всего', 'Передано пакетов', 'Передано %']
        super(ServerTable, self).__init__(parent, cols)
