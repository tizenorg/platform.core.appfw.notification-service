#include <bundle.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dlog.h>
#include <notification.h>
#include <libwlmessage.h>

typedef enum {
	BT_AGENT_ACCEPT,
	BT_AGENT_REJECT,
	BT_AGENT_CANCEL,
	BT_CORE_AGENT_TIMEOUT,
} bt_agent_accept_type_t;


static DBusGProxy* create_agent_proxy(const char *path)
{
	DBusGConnection *sys_conn;

	sys_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (sys_conn == NULL) {
		LOGD("ERROR: Can't get on system bus");
		return NULL;
	}

	return dbus_g_proxy_new_for_name(sys_conn, "org.projectx.bt", path,
			"org.bluez.Agent1");
}

static DBusGProxy* create_obex_proxy(void)
{
	DBusGConnection *sys_conn;

	sys_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (sys_conn == NULL) {
		LOGD("ERROR: Can't get on system bus");
		return NULL;
	}

	return dbus_g_proxy_new_for_name(sys_conn, "org.bluez.frwk_agent",
			"/org/obex/ops_agent", "org.openobex.Agent");
}

static void send_user_reply (DBusGProxy *agent_proxy,
							 const char *reply_method,
							 bt_agent_accept_type_t user_reply)
{
	dbus_g_proxy_call_no_reply( agent_proxy, reply_method, G_TYPE_UINT,
								user_reply, G_TYPE_INVALID, G_TYPE_INVALID);
}

int display_notification (notification_h noti)
{
	char *pkgname = NULL;
	char *title = NULL;
	char *content = NULL;
	char *image_path = NULL;
	int result;

	bundle *resp_data = NULL;
	const char *buttons_str = NULL;
	const char *timeout_str = NULL;
	const char *textfield = NULL;
	gchar **buttons = NULL;
	int timeout;
	int pos;

	notification_get_pkgname (noti, &pkgname);
	if (pkgname == NULL)
		notification_get_application (noti, &pkgname);
	notification_get_title (noti, &title, NULL);
	notification_get_text (noti, NOTIFICATION_TEXT_TYPE_CONTENT, &content);
	notification_get_image (noti, NOTIFICATION_IMAGE_TYPE_ICON, &image_path);

	LOGD("NOTIFICATION RECEIVED: %s - %s - %s", pkgname, title, content);

	 /* verify if we are supposed to respond to this notification */
	notification_get_execute_option (noti, NOTIFICATION_EXECUTE_TYPE_RESPONDING,
	                                 NULL, &resp_data);
	if (resp_data) {
		buttons_str = bundle_get_val (resp_data, "buttons");
		timeout_str = bundle_get_val (resp_data, "timeout");
		textfield = bundle_get_val (resp_data, "textfield");
	}

	if (!strcasecmp(pkgname, "bluetooth-frwk-bt-service")) {
		bundle *user_data = NULL;
		DBusGProxy *proxy;
		char *reply_method;
		int button = 0;

		const char *device_name = NULL;
		const char *passkey = NULL;
		const char *file = NULL;
		const char *agent_path;
		const char *event_type = NULL;

		notification_get_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH,NULL, &user_data);

		event_type = bundle_get_val(user_data, "event-type");
		if (!event_type) {
			LOGD("Not a bluetooth-related notification...");
		} else {
			LOGD("bluetooth notification type: [%s]", event_type);
		}

		if (!strcasecmp(event_type, "passkey-confirm-request")) {
			device_name = (gchar*) bundle_get_val(user_data, "device-name");
			passkey = (gchar*) bundle_get_val(user_data, "passkey");
			agent_path = bundle_get_val(user_data, "agent-path");
			proxy = create_agent_proxy(agent_path);
			reply_method = "ReplyConfirmation";

		} else if(!strcasecmp(event_type, "pin-request")) {
			LOGD("Not implemented");
		} else if(!strcasecmp(event_type, "passkey-request")) {
			LOGD("Not implemented");
		} else if(!strcasecmp(event_type, "passkey-display-request")) {
			LOGD("Not implemented");

		} else if (!strcasecmp(event_type, "authorize-request")) {
			device_name = (gchar*) bundle_get_val(user_data, "device-name");
			agent_path = bundle_get_val(user_data, "agent-path");
			proxy = create_agent_proxy(agent_path);
			reply_method = "ReplyAuthorize";

		} else if(!strcasecmp(event_type, "app-confirm-request")) {
			LOGD("Not implemented");

		} else if(!strcasecmp(event_type, "push-authorize-request")) {
			file = (gchar*) bundle_get_val(user_data, "file");
			device_name = (gchar*) bundle_get_val(user_data, "device-name");
			proxy = create_obex_proxy();
			reply_method = "ReplyAuthorize";

		} else if(!strcasecmp(event_type, "confirm-overwrite-request")) {
			LOGD("Not implemented");
		} else if(!strcasecmp(event_type, "keyboard-passkey-request")) {
			LOGD("Not implemented");
		} else if(!strcasecmp(event_type, "bt-information")) {
			LOGD("Not implemented");
		} else if(!strcasecmp(event_type, "exchange-request")) {
			LOGD("Not implemented");
		} else if(!strcasecmp(event_type, "phonebook-request")) {
			LOGD("Not implemented");
		} else if(!strcasecmp(event_type, "message-request")) {
			LOGD("Not implemented");
		}

		struct wlmessage *wlmessage = wlmessage_create ();
		wlmessage_set_title (wlmessage, title);
		wlmessage_set_icon (wlmessage, image_path);
		wlmessage_set_message (wlmessage, content);
		wlmessage_add_button (wlmessage, 1, "Yes");
		wlmessage_add_button (wlmessage, 0, "No");

		button = wlmessage_show (wlmessage, NULL);
		if (button < 0) {
			wlmessage_destroy (wlmessage);
			return 0;
		} else if (button == 1) {
			LOGD("user clicked on 'Yes' popup button");
			send_user_reply(proxy, reply_method, BT_AGENT_ACCEPT);
		} else if (button == 0) {
			LOGD("user clicked on 'No' popup button");
			send_user_reply(proxy, reply_method, BT_AGENT_CANCEL);
		}
		wlmessage_destroy (wlmessage);
		return 1;
	} else {
		struct wlmessage *wlmessage = wlmessage_create ();
		wlmessage_set_title (wlmessage, title);
		wlmessage_set_icon (wlmessage, image_path);
		wlmessage_set_message (wlmessage, content);
		if (textfield)
			wlmessage_set_textfield (wlmessage, (char *)textfield);
		if (buttons_str) {
			buttons = g_strsplit (buttons_str, ",", 0);
			for (pos = 0; buttons[pos]; pos++)
				wlmessage_add_button (wlmessage, pos + 1, buttons[pos]);
			g_strfreev (buttons);
		} else {
			wlmessage_add_button (wlmessage, 0, "Ok");
		}
		if (timeout_str) {
			timeout = g_ascii_strtoll (timeout_str, NULL, 10);
			wlmessage_set_timeout (wlmessage, timeout);
		} else {
			wlmessage_set_timeout (wlmessage, 60);
		}

		result = wlmessage_show (wlmessage, NULL);
		if (result < 0) {
			wlmessage_destroy (wlmessage);
			return 0;
		} else if (result > 0) {
			notification_send_response (noti, result, (char *)textfield);
			return 1;
		} else {
			return 1;
		}
	}
}
