#pragma once

#include <QtCore/qglobal.h>

#ifdef BYDAODIR_LIBRARY
#  define BYDAODIR_EXPORT Q_DECL_EXPORT
#else
#  define BYDAODIR_EXPORT Q_DECL_IMPORT
#endif