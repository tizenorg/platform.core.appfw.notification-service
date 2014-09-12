bin_PROGRAMS =
moduledir = $(libdir)/notification-service/plugins

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = $(GCC_CFLAGS)

module_LTLIBRARIES = wlmessage.la

wlmessage_la_SOURCES = wlmessage.c

wlmessage_la_LDFLAGS = -module -avoid-version
wlmessage_la_CPPFLAGS = $(AM_CPPFLAGS)
wlmessage_la_CFLAGS = $(TIZEN_CFLAGS)
wlmessage_la_LIBADD = $(TIZEN_LIBS)
