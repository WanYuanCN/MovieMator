#ifndef TEXTDOCK_GLOBAL_H
#define TEXTDOCK_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(TEXTDOCK_LIBRARY)
#  define TEXTDOCKSHARED_EXPORT Q_DECL_EXPORT
#else
#  define TEXTDOCKSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // TEXTDOCK_GLOBAL_H
