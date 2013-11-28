# -*- coding: utf-8 -*-

import os
import socket
import json

class SocketException(Exception):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()

__ex_classes = "NoParamsExceptions MessageLenException BrokenPipeException \
    NoConnectionAcceptedException".rstrip().split()

for c in __ex_classes:
    exec("""
class {class_name}(SocketException):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()
    """.format(class_name = c))

class Socket:
    def __init__(self, sock = None, sock_f_name = None):
        if not sock and not sock_f_name:
            raise NoParamsException('One parametr is required')

        if not sock:
            self.__sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.__sock.bind(sock_f_name)
            self.__sock.listen(1)
        else:
            self.__sock = sock

        self.__client_sock = None
        self.__sock_file = sock_f_name

    def clear(self):
        self.close()
        if self.__sock:
            self.__sock.close()
            self.__sock = None
        if (self.__sock_file):
            os.remove(self.__sock_file)

    def close(self):
        if self.__client_sock:
            self.__client_sock.close()
            self.__client_sock = None

    def wait_connection(self):
        self.__client_sock, self.addr = self.__sock.accept()
        return self.__client_sock

    @property
    def connected(self):
        return bool(self.__client_sock)

    def send_msg(self, data, sock = None):
        msg = json.dumps(data)
        if not sock:
            sock = self.__client_sock

        if not sock:
            raise NoConnectionAcceptedException

        if len(msg) > MSG_MAX_LEN:
            raise MessageLenException(msg)

        # TODO Профиксить отсправку сообщения
        if sock.send("%*d" % (LEN_MSG_LEN, len(msg))) == 0:
            raise BrokenPipeException()
        if sock.send(msg) == 0:
            raise BrokenPipeException()

    def recv_msg(self, sock = None):
        if not sock:
            sock = self.__client_sock

        if not sock:
            raise NoConnectionAcceptedException

        return ''
