# src/cli/cli.pro
TARGET = bydao
TEMPLATE = app

include(../../BydaoScript.pri)

CONFIG += c++11 console

INCLUDEPATH += ../../include

LIBS += -L../../bin -lBydaoCore

SOURCES += main.cpp

# Установка пути вывода
DESTDIR = ../../bin
