AC_PREREQ([2.68])
AC_INIT([notification-service], [0.0.1], [])
AM_INIT_AUTOMAKE([foreign 1.11])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_PROG_PKG_CONFIG()
AC_ARG_ENABLE(wayland, [  --enable-wayland],, enable_wayland=no)
AM_CONDITIONAL(HAVE_WAYLAND, test x$enable_wayland = xyes)
if test "x$enable_wayland" = "xyes"; then
  wayland_libs="libwlmessage"
fi
PKG_CHECK_MODULES([TIZEN], [eina ecore com-core notification dbus-1 capi-network-bluetooth glib-2.0 gio-2.0 dlog $wayland_libs])
AC_SUBST(TIZEN_CFLAGS)
AC_SUBST(TIZEN_LIBS)

AC_CONFIG_FILES([Makefile notifications.service notifications_display.service])
AC_PROG_RANLIB([ranlib])
AC_OUTPUT
