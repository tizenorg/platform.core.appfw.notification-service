bin_PROGRAMS = notification-service

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = $(GCC_CFLAGS)

notification_service_SOURCES = \
    main.c \
    notification_service.c \
    service_common.c
notification_service_CFLAGS = -I. $(TIZEN_CFLAGS)
notification_service_LDADD = $(TIZEN_LIBS)

SCRIPT_IN_FILES = \
     notifications.service.in \
     notifications.socket.in

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)/usr/lib/systemd/system/sockets.target.wants
	install -m 0644 notifications.service $(DESTDIR)/usr/lib/systemd/system/notifications.service
	install -m 0644 notifications.socket $(DESTDIR)/usr/lib/systemd/system/notifications.socket
	ln -fs ../notifications.socket $(DESTDIR)/usr/lib/systemd/system/sockets.target.wants/notifications.socket

