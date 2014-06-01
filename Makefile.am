bin_PROGRAMS = notification-service notification-display-service sample-display-client send-notification bluetooth_notification_client

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = $(GCC_CFLAGS)

if HAVE_WAYLAND
AM_CPPFLAGS += -DHAVE_WAYLAND
endif

notification_service_SOURCES = \
    main.c \
    notification_service.c \
    service_common.c
notification_service_CFLAGS = -I. $(TIZEN_CFLAGS)
notification_service_LDADD = $(TIZEN_LIBS)

notification_display_service_SOURCES = notification_display_service.c
notification_display_service_CFLAGS = -I. $(TIZEN_CFLAGS)
notification_display_service_LDADD = $(TIZEN_LIBS)

send_notification_SOURCES = send_notification.c
send_notification_CFLAGS = -I. $(TIZEN_CFLAGS)
send_notification_LDADD = $(TIZEN_LIBS)

sample_display_client_SOURCES = sample_display_client.c
sample_display_client_CFLAGS = -I. $(TIZEN_CFLAGS)
sample_display_client_LDADD = $(TIZEN_LIBS)

bluetooth_notification_client_SOURCES = bluetooth_notification_client.c
bluetooth_notification_client_CFLAGS = -I. $(TIZEN_CFLAGS)
bluetooth_notification_client_LDADD = $(TIZEN_LIBS)

SCRIPT_IN_FILES = \
     notifications.service.in

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)/usr/lib/systemd/system/
	install -m 0644 notifications.service $(DESTDIR)/usr/lib/systemd/system/notifications.service
