#include <stdio.h>
#include <sys/stat.h>
#include <glib.h>
#include <notification.h>
#include <dlog.h>
#include <libwlmessage.h>


void display_notifications_cb (void *data, notification_type_e notif_type)
{
	notification_h noti = NULL;
	notification_list_h notification_list = NULL;
	notification_list_h get_list = NULL;

	char *pkgname = NULL;
	char *title = NULL;
	char *content = NULL;
	char *image_path = NULL;

	notification_get_list (NOTIFICATION_TYPE_NOTI, -1, &notification_list);
	if (notification_list) {
		get_list = notification_list_get_head (notification_list);
		while (get_list) {
			noti = notification_list_get_data (get_list);
			notification_get_pkgname (noti, &pkgname);
			if (pkgname == NULL)
				notification_get_application (noti, &pkgname);
			notification_get_title (noti, &title, NULL);
			notification_get_text (noti, NOTIFICATION_TEXT_TYPE_CONTENT, &content);
			notification_get_image (noti, NOTIFICATION_IMAGE_TYPE_ICON, &image_path);

			struct wlmessage *wlmessage = wlmessage_create ();
			wlmessage_set_title (wlmessage, title);
			wlmessage_set_icon (wlmessage, image_path);
			wlmessage_set_message (wlmessage, content);
			wlmessage_add_button (wlmessage, 0, "Ok");
			if (wlmessage_show (wlmessage, NULL) < 0) {
				wlmessage_destroy (wlmessage);
				return;
			}
			wlmessage_destroy (wlmessage);

			LOGD("\nNew Notification : %s\n", title);
			LOGD("Icon : %s\n", image_path);
			LOGD("Message : %s\n", content);

			get_list = notification_list_remove(get_list, noti);
			notification_delete(noti);
		}
	}
}

int main (int argc, char **argv)
{
	GMainLoop *mainloop = NULL;
	notification_error_e error_n;
	int error_s;
	struct stat buf;

retry_socket:
	LOGD("Checking if the notifications server socket exists...");
	error_s = stat ("/tmp/.notification.service", &buf);
	if (error_s == -1) {
		LOGD("Could not find the notifications server socket");
		sleep (5);
		goto retry_socket;
	}

retry_service:
	LOGD("Checking if the notifications server is available...");
	error_n = notification_resister_changed_cb (display_notifications_cb, NULL);
	if (error_n != NOTIFICATION_ERROR_NONE) {
		LOGD("Could not register with notifications server");
		sleep (5);
		goto retry_service;
	}

	mainloop = g_main_loop_new (NULL, FALSE);
	if (!mainloop) {
		printf ("Failed to create the GLib main loop\n");
		return -1;
	}

	g_main_loop_run (mainloop);

	return 0;
}
