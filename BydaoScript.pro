# BydaoScript.pro
TEMPLATE = subdirs
CONFIG += ordered

# Ядро языка
SUBDIRS += src/core/core.pro

# Модули (зависят от ядра)
SUBDIRS += \
    src/modules/Dir/Dir.pro \
    src/modules/File/File.pro \
    src/modules/Sys/Sys.pro

# CLI интерпретатор (зависит от ядра и модулей)
SUBDIRS += src/cli/cli.pro

# Порядок сборки
src/core/core.pro.depends =
src/modules/Dir/Dir.pro.depends = src/core/core.pro
src/modules/File/File.pro.depends = src/core/core.pro
src/modules/Sys/Sys.pro.depends = src/core/core.pro
src/cli/cli.pro.depends = src/core/core.pro \
    src/modules/Dir/Dir.pro \
    src/modules/File/File.pro \
    src/modules/Sys/Sys.pro