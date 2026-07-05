#pragma once

#include <QtCore/qglobal.h>

#ifdef BYDAOWEB_LIBRARY
#  define BYDAOWEB_EXPORT Q_DECL_EXPORT
#else
#  define BYDAOWEB_EXPORT Q_DECL_IMPORT
#endif
