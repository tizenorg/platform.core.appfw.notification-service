#include <bundle.h>
#include <bluetooth.h>
#include <dlog.h>
#include <notification.h>
#include <libwlmessage.h>
#include <stdlib.h>

#define POPUP_TYPE_INFO "user_info_popup"
#define POPUP_TYPE_USERCONFIRM "user_confirm_popup"
#define POPUP_TYPE_USERPROMPT "user_agreement_popup"
#define REGISTER_PAIRING_AGENT_TITLE "register_pairing_agent"
#define REGISTER_OPP_AGENT_TITLE "register_opp_agent"
#define BLUETOOTH_ICON_PATH	"/usr/share/icons/default/bt-icon.png"

static void bt_opp_server_transfer_progress_cb_test(const char *file,
							long long size,
							int percent,
							void *user_data)
{
	LOGD("file = %s / size = %lld / percent = %d", file, size, percent);

}

static void bt_opp_server_transfer_finished_cb_test(int result,
						const char *file,
						long long size,
						void *user_data)
{
	LOGD("file = %s / size = %lld / result = %d", file, size, result);

}

void bt_opp_server_push_requested_cb_test(const char *file, int size, void *user_data)
{
	int transfer_id;
	int clicked_button = 0;
	LOGD("file = %s / size = %d", file, size);

	int len;
	char *content;

	len = snprintf(NULL, 0, "Do you want to accept the received file: %s (size: %d) ?", file, size);
	content = malloc(len + 1);
	snprintf(content, len, "Do you want to accept the received file: %s (size: %d) ?", file, size);

	struct wlmessage *wlmessage = wlmessage_create ();
	wlmessage_set_title (wlmessage, "Bluetooth file transfer");
	wlmessage_set_icon (wlmessage, BLUETOOTH_ICON_PATH);
	wlmessage_set_message (wlmessage, content);
	wlmessage_add_button (wlmessage, 1, "Yes");
	wlmessage_add_button (wlmessage, 0, "No");

	clicked_button = wlmessage_show (wlmessage, NULL);

	if (clicked_button < 0) {
		wlmessage_destroy (wlmessage);
		return;
	} else if (clicked_button == 1) {
		LOGD("user clicked on 'Yes' popup button");
		bt_opp_server_accept(bt_opp_server_transfer_progress_cb_test,
				bt_opp_server_transfer_finished_cb_test, file, NULL, &transfer_id);
	} else if (clicked_button == 0){
		LOGD("user clicked on 'No' popup button");
		bt_opp_server_reject();
	}
	wlmessage_destroy (wlmessage);
}

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

		if (!strcasecmp(REGISTER_OPP_AGENT_TITLE, title)) {
			LOGD("Register opp agent. Do not display this popup.");
			bt_opp_server_initialize("/tmp", bt_opp_server_push_requested_cb_test, NULL);
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
