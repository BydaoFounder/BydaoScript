# BydaoScript.pri
QT -= gui
QT += core

CONFIG += c++17
CONFIG += debug_and_release
CONFIG += strict_c++
#CONFIG += precompile_header

# Общие макросы
DEFINES += BYDAOSCRIPT_LIBRARY
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

# Общие пути
INCLUDEPATH += $$PWD/include

# Оптимизация
QMAKE_CXXFLAGS_RELEASE += -O2
QMAKE_CXXFLAGS_DEBUG += -O0 -g

# Предкомпилированные заголовки (опционально)
# PRECOMPILED_HEADER = $$PWD/include/BydaoScript/stable.h

HEADERS += \
    $$PWD/include/BydaoScript/BydaoIntRange.h
