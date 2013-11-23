#!/usr/bin/python3

from PyQt4.QtCore import *
from PyQt4.QtGui import *

from forms import *

class MainWindow(QMainWindow, FormMain):
    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

def main(argv):
    app = QApplication(argv, True)

    wnd = MainWindow()
    wnd.show()

    sys.exit(app.exec_())

if __name__ == "__main__":
    main(sys.argv)
