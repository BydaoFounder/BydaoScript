TARGET = Web
TEMPLATE = lib
CONFIG += shared
CONFIG += c++17
CONFIG += debug_and_release
CONFIG += strict_c++

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

INCLUDEPATH += ../../../include

LIBS += -L../../../bin -lBydaoCore

HEADERS += \
    BydaoWeb.h \
    BydaoWeb_global.h

SOURCES += \
    BydaoWeb.cpp

DEFINES += BYDAOWEB_LIBRARY

#QMAKE_LFLAGS += /VERBOSE

DESTDIR = ../../../bin/modules
