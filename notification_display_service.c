#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/inotify.h>
#include <notification.h>
#ifdef HAVE_WAYLAND
#include <libwlmessage.h>
#endif


int fd, wd;

void sigint_handler (int s)
{
	inotify_rm_watch (fd, wd);
	close (fd);
	exit (0);
}

void display_notifications ()
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
		if (get_list) {
			noti = notification_list_get_data (get_list);
			notification_get_pkgname (noti, &pkgname);
			if (pkgname == NULL)
				notification_get_application (noti, &pkgname);
			notification_get_title (noti, &title, NULL);
			notification_get_text (noti, NOTIFICATION_TEXT_TYPE_CONTENT, &content);
			notification_get_image (noti, NOTIFICATION_IMAGE_TYPE_ICON, &image_path);

#			ifdef HAVE_WAYLAND
			struct wlmessage *wlmessage = wlmessage_create ();
			wlmessage_set_title (wlmessage, title);
			wlmessage_set_icon (wlmessage, image_path);
			wlmessage_set_message (wlmessage, content);
			wlmessage_add_button (wlmessage, 0, "Ok");
			wlmessage_show (wlmessage, NULL);
			wlmessage_destroy (wlmessage);
#			endif

			notification_delete (noti);
		}
	}
}

int main (int argc, char **argv)
{
	char buffer[8192];

	 /* display notifications once, so it stays useful without inotify  */
	display_notifications ();

	fd = inotify_init ();
	if (fd < 0) {
		fprintf (stderr, "ERROR: cannot initialize inotify\n");
		fprintf (stderr, "Verify that your kernel integrates it\n");
		return -1;
	}

	signal (SIGINT, sigint_handler);
	wd = inotify_add_watch (fd, "/opt/dbspace/.notification.db", IN_MODIFY);
	while (1) {
		read (fd, buffer, sizeof(buffer));
		display_notifications ();
	}

	return 0;
}
