# -*- coding: utf-8 -*-

from PyQt4.QtGui import *
from PyQt4.QtCore import *

class StatusWidget(QWidget):
    __elems = {}
    __cur_elem = ''
    def __init__(self, parent):
        QWidget.__init__(self, parent)

    def paintEvent(self, e):
        painter = QPainter(self)
        painter.setBrush(Qt.white)
        painter.setPen(QColor(0xa8, 0xa8, 0xa8))
        painter.drawRect(0, 0, self.width() - 1, self.height() - 1)
        if (self.__cur_elem):
            e = self.__elems[self.__cur_elem]
            count = e['count']
            piece = self.width() / count
            for line in e['lines']:
                self.__draw_line(painter, piece, line['color'], line['index'])
            for rect in e['rects']:
                self.__draw_line(painter, piece, rect['color'], rect['start'], rect['finish'])
        painter.setBrush(Qt.NoBrush)
        painter.setPen(QColor(0xa8, 0xa8, 0xa8))
        painter.drawRect(0, 0, self.width() - 1, self.height() - 1)

    def __draw_line(self, painter, piece_len, color, sindex, findex = -1):
        painter.setPen(color)
        painter.setBrush(color)
        painter.brush().setStyle(Qt.SolidPattern)
        if findex < 0:
            findex = sindex + 1
        r = QRectF(sindex * piece_len, 0.0, (findex - sindex) * piece_len, self.height())
        painter.drawRect(r)

    def init_elem(self, key, count):
        e = { 'count': count, 'lines': [], 'rects': [] }
        self.__elems[key] = e

    def add_line(self, key, index, color):
        e = { 'color': color, 'index': int(index) }
        self.__elems[key]['lines'].append(e)
        self.__combine_lines(key)

    def add_rect(self, key, start, finish, color):
        e = { 'color': color, 'start': int(start), 'finish': int(finish) }
        self.__elems[key]['rects'].append(e)

    def __combine_lines(self, key):
        #TODO
        pass

    def remove_lines(self, key):
        return self.init_elem(key, self.__elems[key]['count'])

    @property
    def current_elem(self):
        return self.__cur_elem

    @current_elem.setter
    def current_elem(self, key):
        if key not in self.__elems:
            raise KeyError(key + " not found in status widget elements. Use init_elem() to init it")
        self.__cur_elem = key
        self.repaint()
