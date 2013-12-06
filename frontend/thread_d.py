# -*- coding: utf-8 -*-

from PyQt4.QtCore import *
from net_sock import *

from time import sleep

import os

class Thread(QThread):
    msg_came = pyqtSignal('QString')
    error_came = pyqtSignal('QString', 'QString')

    def __init__(self, sock_name):
        # callback takes 1 argument: self (QThread) * to emit some actions *
        super(Thread, self).__init__()
        try:
            os.remove(sock_name)
        except OSError:
            pass
        self.__sock = Socket(sock_f_name = sock_name)
        self.__can_work = True
        self.__deleted = False

    def __del__(self):
        self.stop_thread()

    def stop_thread(self):
        if not self.__deleted:
            self.__can_work = False
            self.__sock.send_msg({
                "action": TERMINATE_ACT,
            });
            sleep(2)
            self.__sock.clear()
            self.terminate()
            self.wait()
            self.__deleted = True

    @property
    def working(self):
        return self.__can_work

    def stop_work(self):
        self.__can_work = False

    def send_message(self, msg):
        self.__sock.send_msg(msg)

    def do_work(self):
        if not self.__sock.connected:
            self.__sock.wait_connection()

        while (self.__can_work):
            data = self.__sock.recv_msg()
            if data:
                self.msg_came.emit(data)

    def run(self):
        try:
            self.do_work()
        except BrokenPipeException as e:
            print("Exception '{name}' came: {val}".format(name = type(e).__name__, val = e.args[0]))
            self.run()
        except SocketException as e:
            self.error_came.emit(type(e).__name__, e.args[0])
            self.run()
