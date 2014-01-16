AC_PREREQ([2.68])
AC_INIT([notification-service], [0.0.1], [])
AM_INIT_AUTOMAKE([foreign 1.11])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_PROG_PKG_CONFIG()
PKG_CHECK_MODULES([TIZEN], [eina ecore com-core notification dbus-1 bluetooth-api])
AC_SUBST(TIZEN_CFLAGS)
AC_SUBST(TIZEN_LIBS)

AC_CONFIG_FILES([Makefile notifications.service])
AC_PROG_RANLIB([ranlib])
AC_OUTPUT
