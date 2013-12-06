# -*- coding: utf-8 -*-

import os
import socket
import json
import struct

class SocketException(Exception):
    def __init__(self, arg):
        super(SocketException, self).__init__(arg)

__ex_classes = "NoParamsExceptions MessageLenException BrokenPipeException \
    NoConnectionAcceptedException ReceiveMessageFailureException".rstrip().split()

for c in __ex_classes:
    exec("""
class {class_name}(SocketException):
    def __init__(self, arg = ""):
        super({class_name}, self).__init__(arg)
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
        print("sent:     " + msg)                       # TODO: REMOVE ME
        if not sock:
            sock = self.__client_sock

        if not sock:
            raise NoConnectionAcceptedException()

        if len(msg) > MSG_MAX_LEN:
            raise MessageLenException(msg)

        if sock.send(struct.pack('L', len(msg))) == 0:
            raise BrokenPipeException("send failure")
        if sock.send(msg.encode()) == 0:
            raise BrokenPipeException("send failure")

    def recv_msg(self, sock = None):
        if not sock:
            sock = self.__client_sock

        if not sock:
            raise NoConnectionAcceptedException()

        msglen = sock.recv(LEN_MSG_LEN)

        if not msglen:
            raise BrokenPipeException("recv failure")

        msglen = struct.unpack('L', msglen)[0]
        if not msglen:
            raise ReceiveMessageFailureException()

        msg = sock.recv(msglen)
        print("received: " + msg.decode("utf-8"))                       # TODO: REMOVE ME
        if not msg:
            raise RecieveMessageFailureException()

        return msg.decode()
