TARGET = File
TEMPLATE = lib
CONFIG += shared

include(../../../BydaoScript.pri)

INCLUDEPATH += ../../../include

LIBS += -L../../../bin -lBydaoCore

HEADERS += \
    ../../../include/BydaoScript/BydaoModule.h \
    ../../../include/BydaoScript/BydaoNative.h \
    ../../../include/BydaoScript/BydaoValue.h \
    BydaoFile.h \
    BydaoFile.h \
    BydaoFile_global.h

SOURCES += \
    BydaoFile.cpp

DEFINES += BYDAOFILE_LIBRARY

DESTDIR = ../../../bin/modules
