AC_PREREQ([2.68])
AC_INIT([notification-service], [0.0.1], [])
AM_INIT_AUTOMAKE([foreign 1.11])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC

LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_PROG_PKG_CONFIG()

AC_CHECK_FUNC([dlopen], [],
              AC_CHECK_LIB([dl], [dlopen], DLOPEN_LIBS="-ldl"))
AC_SUBST(DLOPEN_LIBS)

AC_ARG_WITH(notif-display-service, AS_HELP_STRING([--with-notif-display-service=],[]),,
            with_notif_display_service="notifications-display.service")
notif_display_service=$with_notif_display_service
AC_SUBST(notif_display_service)

PKG_CHECK_MODULES([TIZEN], [eina ecore com-core notification dbus-1 capi-network-bluetooth dlog libwlmessage])
AC_SUBST(TIZEN_CFLAGS)
AC_SUBST(TIZEN_LIBS)

AC_CONFIG_FILES([Makefile plugins/Makefile notifications.service notifications-display.service notifications-display-ivi.service])
AC_PROG_RANLIB([ranlib])
AC_OUTPUT
