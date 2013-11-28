#!/usr/bin/python3

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from forms import *
from thread import Thread

class MainWindow(QMainWindow, FormMain):
    srv = 'server'
    cli = 'client'

    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        self.cli_thread = Thread(CLI_SOCK_PATH)
        self.srv_thread = Thread(SRV_SOCK_PATH)

        self.cli_thread.msg_came.connect(self.handle_cli_backend_message)
        self.srv_thread.msg_came.connect(self.handle_srv_backend_message)
        
        self.cli_thread.error_came.connect(self.handle_cli_error)
        self.srv_thread.error_came.connect(self.handle_srv_error)

        self.cli_thread.start()
        self.srv_thread.start()

    def closeEvent(self, e):
        self.hide()
        self.cli_thread.stop_thread()
        self.srv_thread.stop_thread()

    def handle_srv_error(self, class_name, msg = None):
        return self.handle_error(class_name, msg, self.srv)

    def handle_cli_error(self, class_name, msg = None):
        return self.handle_error(class_name, msg, self.cli)

    def handle_srv_backend_message(self, msg):
        return self.handle_backend_message(msg, self.srv)

    def handle_cli_backend_message(self, msg):
        return self.handle_backend_message(msg, self.cli)

    def handle_error(self, class_name, msg = None, sender = None):
        QMessageBox.information(self, 'Ошибка', \
                'Возникла ошибка в потоке "{thread}": {msg}'.format( \
                (sender == self.srv and 'Сервер') or \
                (sender == self.cli and 'Клиент') or \
                'Неизвестно', \
                msg), QMessageBox.Ok)

    def handle_backend_message(self, msg, sender = None):
        pass

def main(argv):
    app = QApplication(argv, True)

    wnd = MainWindow()
    wnd.show()

    sys.exit(app.exec_())

if __name__ == "__main__":
    main(sys.argv)
