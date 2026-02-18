# src/core/core.pro
TARGET = BydaoCore
TEMPLATE = lib
CONFIG += shared

include(../../BydaoScript.pri)

HEADERS += \
    ../../include/BydaoScript/BydaoBool.h \
    ../../include/BydaoScript/BydaoDisassembler.h \
    ../../include/BydaoScript/BydaoInt.h \
    ../../include/BydaoScript/BydaoNull.h \
    ../../include/BydaoScript/BydaoObject.h \
    ../../include/BydaoScript/BydaoReal.h \
    ../../include/BydaoScript/BydaoString.h \
    ../../include/BydaoScript/BydaoValue.h \
    ../../include/BydaoScript/BydaoArray.h \
    ../../include/BydaoScript/BydaoNative.h \
    ../../include/BydaoScript/BydaoModule.h \
    ../../include/BydaoScript/BydaoBytecode.h \
    ../../include/BydaoScript/BydaoLexer.h \
    ../../include/BydaoScript/BydaoParser.h \
    ../../include/BydaoScript/BydaoVM.h

SOURCES += \
    BydaoBool.cpp \
    BydaoDisassembler.cpp \
    BydaoInt.cpp \
    BydaoNull.cpp \
    BydaoObject.cpp \
    BydaoReal.cpp \
    BydaoString.cpp \
    BydaoValue.cpp \
    BydaoArray.cpp \
    BydaoNative.cpp \
    BydaoModule.cpp \
    BydaoBytecode.cpp \
    BydaoLexer.cpp \
    BydaoParser.cpp \
    BydaoVM.cpp

# Экспорт символов для Windows
DEFINES += BYDAOCORE_LIBRARY

DESTDIR = ../../bin
