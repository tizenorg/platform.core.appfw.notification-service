#include <bundle.h>
#include <bluetooth.h>
#include <dlog.h>
#include <notification.h>
#include <libwlmessage.h>

#define POPUP_TYPE_INFO "user_info_popup"
#define POPUP_TYPE_USERCONFIRM "user_confirm_popup"
#define POPUP_TYPE_USERPROMPT "user_agreement_popup"
#define REGISTER_PAIRING_AGENT_TITLE "register_pairing_agent"

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

	LOGD("NOTIFICATION RECEIVED: %s - %s - %s", pkgname, title, content);

	if (!strcasecmp(pkgname, "bt-agent")) {
		enum { NOTIF_TYPE_INFO, NOTIF_TYPE_USERPROMPT, NOTIF_TYPE_USERCONFIRM } type = 0;
		int clicked_button = 0;
		char *info1 = NULL;

		if (!strcasecmp(REGISTER_PAIRING_AGENT_TITLE, title)) {
			LOGD("Register pairing agent. Do not display this popup.");
			bt_initialize();
			bt_agent_register_sync();
			return 1;
		}

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

		struct wlmessage *wlmessage = wlmessage_create ();
		wlmessage_set_title (wlmessage, title);
		wlmessage_set_icon (wlmessage, image_path);
		wlmessage_set_message (wlmessage, content);
		wlmessage_add_button (wlmessage, 1, "Yes");
		wlmessage_add_button (wlmessage, 0, "No");

		clicked_button = wlmessage_show (wlmessage, NULL);

		if (clicked_button < 0) {
			wlmessage_destroy (wlmessage);
			return;
		} else if ((clicked_button == 1) && (type == NOTIF_TYPE_USERCONFIRM)) {
			LOGD("user clicked on 'Yes' popup button");
			if (!strcmp(pkgname, "bt-agent")) {
				bt_agent_reply_sync(0);
			}
		} else if ((clicked_button == 0) && (type == NOTIF_TYPE_USERCONFIRM)) {
			LOGD("user clicked on 'No' popup button");
			if (!strcmp(pkgname, "bt-agent")) {
				bt_agent_reply_sync(1);
			}
		}
		wlmessage_destroy (wlmessage);
		return 1;
	} else {
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
}
