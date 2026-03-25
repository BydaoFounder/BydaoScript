# src/cli/cli.pro
TARGET = bydao
TEMPLATE = app

QT -= gui
QT += core

CONFIG += c++17
CONFIG += debug_and_release
CONFIG += strict_c++
CONFIG += console

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# Общие пути
INCLUDEPATH += $$PWD/include

# Оптимизация
QMAKE_CXXFLAGS_RELEASE += -O2
QMAKE_CXXFLAGS_DEBUG += -O0 -g

INCLUDEPATH += ../../include

LIBS += -L../../bin -lBydaoCore

SOURCES += main.cpp

# Установка пути вывода
DESTDIR = ../../bin
