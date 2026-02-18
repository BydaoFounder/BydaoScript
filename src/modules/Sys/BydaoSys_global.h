#pragma once

#include <QtCore/qglobal.h>

#ifdef BYDAOSYS_LIBRARY
#  define BYDAOSYS_EXPORT Q_DECL_EXPORT
#else
#  define BYDAOSYS_EXPORT Q_DECL_IMPORT
#endif