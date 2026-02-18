TARGET = Sys
TEMPLATE = lib
CONFIG += shared

include(../../../BydaoScript.pri)

INCLUDEPATH += ../../../include

LIBS += -L../../../bin -lBydaoCore

HEADERS += \
    ../../../include/BydaoScript/modules/BydaoSys.h \
    BydaoSys.h \
    BydaoSys_global.h

SOURCES += \
    BydaoSys.cpp

DEFINES += BYDAOSYS_LIBRARY

DESTDIR = ../../../bin/modules
