# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'frontend/forms/login_form.ui'
#
# Created: Mon Dec 16 23:10:19 2013
#      by: PyQt4 UI code generator 4.10.3
#
# WARNING! All changes made in this file will be lost!

import sys

sys.path.append('/home/pavel/projects/Networks/networks-2013/frontend')

from statuswidget import StatusWidget
from tablewidget import ServerTable
from tablewidget import ClientTable
from tablewidget import MainTable

from PyQt4 import QtCore, QtGui

try:
    _fromUtf8 = QtCore.QString.fromUtf8
except AttributeError:
    def _fromUtf8(s):
        return s

try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except AttributeError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)

class Ui_LoginForm(object):
    def setupUi(self, LoginForm):
        LoginForm.setObjectName(_fromUtf8("LoginForm"))
        LoginForm.setWindowModality(QtCore.Qt.ApplicationModal)
        LoginForm.resize(350, 119)
        LoginForm.setMinimumSize(QtCore.QSize(350, 119))
        LoginForm.setMaximumSize(QtCore.QSize(350, 119))
        self.centralwidget = QtGui.QWidget(LoginForm)
        self.centralwidget.setObjectName(_fromUtf8("centralwidget"))
        self.verticalLayout = QtGui.QVBoxLayout(self.centralwidget)
        self.verticalLayout.setMargin(3)
        self.verticalLayout.setObjectName(_fromUtf8("verticalLayout"))
        self.gridWidget = QtGui.QWidget(self.centralwidget)
        self.gridWidget.setObjectName(_fromUtf8("gridWidget"))
        self.gridLayout = QtGui.QGridLayout(self.gridWidget)
        self.gridLayout.setMargin(0)
        self.gridLayout.setObjectName(_fromUtf8("gridLayout"))
        self.login_edt = QtGui.QLineEdit(self.gridWidget)
        self.login_edt.setObjectName(_fromUtf8("login_edt"))
        self.gridLayout.addWidget(self.login_edt, 0, 1, 1, 1)
        self.passw_edt = QtGui.QLineEdit(self.gridWidget)
        self.passw_edt.setEchoMode(QtGui.QLineEdit.Password)
        self.passw_edt.setObjectName(_fromUtf8("passw_edt"))
        self.gridLayout.addWidget(self.passw_edt, 1, 1, 1, 1)
        self.label = QtGui.QLabel(self.gridWidget)
        self.label.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.label.setObjectName(_fromUtf8("label"))
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.label_2 = QtGui.QLabel(self.gridWidget)
        self.label_2.setAlignment(QtCore.Qt.AlignRight|QtCore.Qt.AlignTrailing|QtCore.Qt.AlignVCenter)
        self.label_2.setObjectName(_fromUtf8("label_2"))
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 1)
        self.verticalLayout.addWidget(self.gridWidget)
        self.horizontalWidget = QtGui.QWidget(self.centralwidget)
        self.horizontalWidget.setObjectName(_fromUtf8("horizontalWidget"))
        self.horizontalLayout = QtGui.QHBoxLayout(self.horizontalWidget)
        self.horizontalLayout.setMargin(0)
        self.horizontalLayout.setObjectName(_fromUtf8("horizontalLayout"))
        spacerItem = QtGui.QSpacerItem(40, 20, QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.login_btn = QtGui.QPushButton(self.horizontalWidget)
        self.login_btn.setObjectName(_fromUtf8("login_btn"))
        self.horizontalLayout.addWidget(self.login_btn)
        self.close_btn = QtGui.QPushButton(self.horizontalWidget)
        self.close_btn.setObjectName(_fromUtf8("close_btn"))
        self.horizontalLayout.addWidget(self.close_btn)
        self.verticalLayout.addWidget(self.horizontalWidget)
        LoginForm.setCentralWidget(self.centralwidget)

        self.retranslateUi(LoginForm)
        QtCore.QMetaObject.connectSlotsByName(LoginForm)
        LoginForm.setTabOrder(self.login_edt, self.passw_edt)
        LoginForm.setTabOrder(self.passw_edt, self.login_btn)
        LoginForm.setTabOrder(self.login_btn, self.close_btn)

    def retranslateUi(self, LoginForm):
        LoginForm.setWindowTitle(_translate("LoginForm", "Login", None))
        self.login_edt.setPlaceholderText(_translate("LoginForm", "Логин для входа в систему", None))
        self.passw_edt.setPlaceholderText(_translate("LoginForm", "Пароль для входа в систему", None))
        self.label.setText(_translate("LoginForm", "Логин", None))
        self.label_2.setText(_translate("LoginForm", "Пароль", None))
        self.login_btn.setToolTip(_translate("LoginForm", "Войти в приложение", None))
        self.login_btn.setText(_translate("LoginForm", "Подтвердить", None))
        self.close_btn.setToolTip(_translate("LoginForm", "Отменить вход и закрыть приложение", None))
        self.close_btn.setText(_translate("LoginForm", "Отмена", None))

