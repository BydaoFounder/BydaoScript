#pragma once

#include <QtCore/qglobal.h>

#ifdef BYDAOFILE_LIBRARY
#  define BYDAOFILE_EXPORT Q_DECL_EXPORT
#else
#  define BYDAOFILE_EXPORT Q_DECL_IMPORT
#endif