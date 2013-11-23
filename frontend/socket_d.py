# -*- coding: utf-8 -*-

import os
import socket
import json

class SocketException(Exception):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()

class NoParamsException(SocketException):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()

class MessageLenException(SocketException):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()

class BrokenPipeException(SocketException):
    def __init__(self):
        super(eval(self.__class__.__name__), self).__init__()

class Socket:
    def __init__(self, sock = None, sock_f_name = None):
        if not sock and not sock_f_name:
            raise NoParamsException('One parametr is required')

        if not sock:
            self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            self.sock.bind(sock_f_name)
            sock.listen(1)
        else:
            self.sock = sock

    def wait_connections(self):
        self.client_sock, self.addr = self.sock.accept()
        return self.client_sock

    def send_msg(self, data):
        msg = json.dumps(data)
        if len(msg) > MSG_MAX_LEN:
            raise MessageLenException(msg)

        # TODO
        if self.client_sock.send("%*d" % (LEN_MSG_LEN, len(msg))) == 0:
            raise BrokenPipeException()
        if self.client_sock.send(msg) == 0:
            raise BrokenPipeException()

    def recv_msg(self):
        pass
