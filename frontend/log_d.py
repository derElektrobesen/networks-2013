# -*- coding: utf-8 -*-

from datetime import datetime

class Logger:
    __messages = '';

    @staticmethod
    def log(message):
        date = datetime.now()
        Logger.__messages += "[{day}.{month}.{year}, {hh}:{mm}:{ss}] " \
            .format(day = date.day, month = date.month, year = date.year,
                    hh = date.hour, mm = date.minute, ss = date.second) + message + "\n"

    @staticmethod
    def get_log():
        return Logger.__messages
