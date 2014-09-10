#include <notification.h>
#include <libwlmessage.h>


int display_notification (notification_h noti)
{
	char *pkgname = NULL;
	char *title = NULL;
	char *content = NULL;
	char *image_path = NULL;

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
		return 0;
	}

	return 1;
}
