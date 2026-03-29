TARGET = Dir
TEMPLATE = lib
CONFIG += shared

include(../../../BydaoScript.pri)

INCLUDEPATH += ../../../include

LIBS += -L../../../bin -lBydaoCore

HEADERS += \
    ../../../include/BydaoScript/BydaoModule.h \
    ../../../include/BydaoScript/BydaoObject.h \
    ../../../include/BydaoScript/BydaoValue.h \
    BydaoDir.h \
    BydaoDir.h \
    BydaoDir_global.h

SOURCES += \
    BydaoDir.cpp

DEFINES += BYDAODIR_LIBRARY

DESTDIR = ../../../bin/modules
