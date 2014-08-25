#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <notification.h>
#ifdef HAVE_WAYLAND
#include <libwlmessage.h>
#endif
#include <bluetooth.h>
#include <glib.h>

#define POPUP_TYPE_INFO "user_info_popup"
#define POPUP_TYPE_USERCONFIRM "user_confirm_popup"
#define POPUP_TYPE_USERPROMPT "user_agreement_popup"

void display_notifications_cb(void *data, notification_type_e notif_type)
{
	notification_h noti = NULL;
	notification_list_h notification_list = NULL;
	notification_list_h get_list = NULL;

	char *pkgname = NULL;
	char *title = NULL;
	char *content = NULL;
	char *image_path = NULL;
	char *info1 = NULL;
	enum { NOTIF_TYPE_INFO, NOTIF_TYPE_USERPROMPT, NOTIF_TYPE_USERCONFIRM } type = 0;
	int clicked_button = 0;

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

			 /* react specifically to the source framework and event (TODO : plugins !) */
			if (!strcmp(pkgname, "bt-agent")) {
printf("ON EST EN BLUETOOTH\n");
				notification_get_text (noti, NOTIFICATION_TEXT_TYPE_INFO_1, &info1);
				if (info1) {
					if (!strcasecmp(POPUP_TYPE_INFO, info1)) {
						type = NOTIF_TYPE_INFO;
					}
					else if (!strcasecmp(POPUP_TYPE_USERCONFIRM, info1)) {
						type = NOTIF_TYPE_USERCONFIRM;
					}
					else if (!strcasecmp(POPUP_TYPE_USERPROMPT, info1)) {
						type = NOTIF_TYPE_USERPROMPT;
					}
				}
			} else { printf ("ON N'EST PAS EN BLUETOOTH !\n"); }

#			ifdef HAVE_WAYLAND
			struct wlmessage *wlmessage = wlmessage_create ();
			if (title)
				wlmessage_set_title (wlmessage, title);
			if (image_path)
				wlmessage_set_icon (wlmessage, image_path);
			if (content)
				wlmessage_set_message (wlmessage, content);
			else
				wlmessage_set_message (wlmessage, "<Default>");
			if (type == NOTIF_TYPE_USERPROMPT)
				wlmessage_set_textfield (wlmessage, "");
			if (type == NOTIF_TYPE_USERCONFIRM) {
				wlmessage_add_button (wlmessage, 1, "Yes");
				wlmessage_add_button (wlmessage, 0, "No");
			} else {
				wlmessage_add_button (wlmessage, 0, "Ok");
			}

			clicked_button = wlmessage_show (wlmessage, NULL);

			if (clicked_button < 0) {
				wlmessage_destroy (wlmessage);
				return;
			} else if ((clicked_button == 1) && (type == NOTIF_TYPE_USERCONFIRM)) {
				printf("CLIQUE SUR OUI !\n");
				if (!strcmp(pkgname, "bt-agent")) {
					printf("ON ENVOIE SYNC(0)\n");
					bt_agent_reply_sync(0);	
				}
			} else if ((clicked_button == 0) && (type == NOTIF_TYPE_USERCONFIRM)) {
				printf("CLIQUE SUR NON !\n");
				if (!strcmp(pkgname, "bt-agent")) {
					printf("ON ENVOIE SYNC(1)\n");
					bt_agent_reply_sync(1);	
				}
			}
			wlmessage_destroy (wlmessage);

#			else
			fprintf(stderr, "\nNew Notification : %s\n", title); 
			fprintf(stderr, "Icon : %s\n", image_path); 
			fprintf(stderr, "Message : %s\n", content); 
#			endif

			get_list = notification_list_remove(get_list, noti);
			notification_delete(noti);
		}
	}
}

int main (int argc, char **argv)
{
	GMainLoop *mainloop = NULL;

	bt_initialize();

	bt_agent_register_sync();

    notification_resister_changed_cb(display_notifications_cb, NULL);

    mainloop = g_main_loop_new(NULL, FALSE);
	if (!mainloop) {
		printf("failed to create glib main loop");
		return -1;
	}
	g_main_loop_run(mainloop);

    return 0;
}
