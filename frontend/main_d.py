#!/usr/bin/python3

from PyQt4.QtGui import QApplication

import os
os.chdir("HOME_PATH")

from mainwindow import MainWindow

def main(argv):
    app = QApplication(argv, True)

    wnd = MainWindow()
    wnd.show()

    sys.exit(app.exec_())

if __name__ == "__main__":
    main(sys.argv)
