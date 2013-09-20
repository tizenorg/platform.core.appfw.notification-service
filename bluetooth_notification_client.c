#include <Ecore.h>
#include <notification.h>
#include <unistd.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <bundle.h>

typedef enum {
           BT_AGENT_ACCEPT,
           BT_AGENT_REJECT,
           BT_AGENT_CANCEL,
           BT_CORE_AGENT_TIMEOUT,
} bt_agent_accept_type_t;


typedef void (*bt_notification)(DBusGProxy *proxy);

static DBusGProxy*
__bluetooth_create_agent_proxy(DBusGConnection *sys_conn, const char *path)
{
        return dbus_g_proxy_new_for_name (sys_conn,
                                          "org.projectx.bt",
                                          path,
                                          "org.bluez.Agent");
}

static DBusGProxy*
__bluetooth_create_obex_proxy(DBusGConnection *sys_conn)
{
        return dbus_g_proxy_new_for_name(sys_conn,
                                         "org.bluez.frwk_agent",
                                         "/org/obex/ops_agent",
                                         "org.openobex.Agent");
}

static void
__notify_passkey_confirm_request_accept_cb( DBusGProxy* agent_proxy)
{
        dbus_g_proxy_call_no_reply( agent_proxy, "ReplyConfirmation",
                                   G_TYPE_UINT, BT_AGENT_ACCEPT,
                                   G_TYPE_INVALID, G_TYPE_INVALID);

}

static void
__notify_passkey_confirm_request_cancel_cb(DBusGProxy* agent_proxy)
{

        dbus_g_proxy_call_no_reply( agent_proxy, "ReplyConfirmation",
                                    G_TYPE_UINT, BT_AGENT_CANCEL,
                                    G_TYPE_INVALID, G_TYPE_INVALID);

}

static void
__notify_push_authorize_request_accept_cb(DBusGProxy* obex_proxy)
{

        dbus_g_proxy_call_no_reply( obex_proxy, "ReplyAuthorize",
                                    G_TYPE_UINT, BT_AGENT_ACCEPT,
                                    G_TYPE_INVALID, G_TYPE_INVALID);

}

static void
__notify_push_authorize_request_cancel_cb(DBusGProxy* obex_proxy)
{

        dbus_g_proxy_call_no_reply( obex_proxy, "ReplyAuthorize",
                                    G_TYPE_UINT, BT_AGENT_CANCEL,
                                    G_TYPE_INVALID, G_TYPE_INVALID);

}

static void
__notify_authorize_request_accept_cb(DBusGProxy* agent_proxy)
{

         dbus_g_proxy_call_no_reply( agent_proxy, "ReplyAuthorize",
                                     G_TYPE_UINT, BT_AGENT_ACCEPT,
                                     G_TYPE_INVALID, G_TYPE_INVALID);
}

static void
__notify_authorize_request_cancel_cb(DBusGProxy* agent_proxy)
{

         dbus_g_proxy_call_no_reply( agent_proxy, "ReplyAuthorize",
                                     G_TYPE_UINT, BT_AGENT_CANCEL,
                                     G_TYPE_INVALID, G_TYPE_INVALID);

}

static int
__display_notification(bt_notification cb_1, bt_notification cb_2, DBusGProxy *proxy)
{

         notification_error_e err = NOTIFICATION_ERROR_NONE;
         int bt_yesno;
         bt_yesno = 1;
         char line[256];

         fprintf(stdout, "Do you confirm yes or no ? ");
         while ( bt_yesno != 0){
                 gets(line);
                 if ( strcmp(line,"yes") == 0){
                         (cb_1) (proxy);
                         bt_yesno = 0;
                 } else if ( strcmp(line,"no") == 0){
                         (cb_2) (proxy);
                         bt_yesno = 0;
                 } else {
                         fprintf(stdout," yes or no :");
                 }
         }
         err = notification_delete_all_by_type("bluetooth-frwk-bt-service", NOTIFICATION_TYPE_NOTI);
         if (err != NOTIFICATION_ERROR_NONE) {
                  fprintf(stdout, "Unable to remove notifications");
         }

}

static int __noti_changed_cb(void *data, notification_type_e type)
{
         notification_h noti = NULL;
         notification_list_h notification_list = NULL;
         notification_list_h get_list = NULL;
         int count = 0, group_id = 0, priv_id = 0, show_noti = 0, num = 1;
         char *pkgname = NULL;
         char *title = NULL;
         char *str_count = NULL;
         char *content= NULL;
         bundle *user_data = NULL;
         DBusGConnection *sys_conn;
         DBusGProxy *agent_proxy;
         DBusGProxy *obex_proxy;


         const char *device_name = NULL;
         const char *passkey = NULL;
         const char *file = NULL;
         const char *agent_path;
         const char *event_type = NULL;

         notification_get_list(NOTIFICATION_TYPE_NOTI, -1, &notification_list);
         if (notification_list) {
                get_list = notification_list_get_head(notification_list);
                noti = notification_list_get_data(get_list);
                notification_get_id(noti, &group_id, &priv_id);
                notification_get_pkgname(noti, &pkgname);
                if(pkgname == NULL){
                       notification_get_application(noti, &pkgname);
                }

                notification_get_text(noti, NOTIFICATION_TEXT_TYPE_EVENT_COUNT, &str_count);
                if (!str_count) {
                        count = 0;
                } else {
                        count = atoi(str_count);
                }
                notification_get_title(noti, &title, NULL);
                notification_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT , &content);
                notification_get_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH,NULL, &user_data);

                fprintf(stdout, "NOTIFICATION: %s - %s - %s - %i - %i \n", pkgname, title, content, count, num);

                event_type = bundle_get_val(user_data, "event-type");

                if(!strcasecmp(event_type, "pin-request")) {
                        /* Not implemented */
                        fprintf(stdout," Not implemented\n");

                } else if (!strcasecmp(event_type, "passkey-confirm-request")){
                        device_name = (gchar*) bundle_get_val(user_data, "device-name");
                        passkey = (gchar*) bundle_get_val(user_data, "passkey");
                        agent_path = bundle_get_val(user_data, "agent-path");

                        sys_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
                        if (sys_conn == NULL) {
                                fprintf(stdout,"ERROR: Can't get on system bus");
                                return EXIT_FAILURE;
                        }

                        agent_proxy = __bluetooth_create_agent_proxy(sys_conn, agent_path);
                        if (!agent_proxy){
                                fprintf(stdout,"create new agent_proxy failed\n");
                                return EXIT_FAILURE;
                        }

                         __display_notification(__notify_passkey_confirm_request_accept_cb, __notify_passkey_confirm_request_cancel_cb,agent_proxy);
                } else if (!strcasecmp(event_type, "passkey-request")) {
                        /* Not implemented */
                        fprintf(stdout," Not implemented\n");

                } else if (!strcasecmp(event_type, "passkey-display-request")) {
                        /* Not implemented */
                        fprintf(stdout," Not implemented\n");

                } else if (!strcasecmp(event_type, "authorize-request")) {
                        device_name = (gchar*) bundle_get_val(user_data, "device-name");
                        agent_path = bundle_get_val(user_data, "agent-path");

                        sys_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
                        if (sys_conn == NULL) {
                                fprintf(stdout,"ERROR: Can't get on system bus");
                                return EXIT_FAILURE;
                        }

                        agent_proxy = __bluetooth_create_agent_proxy(sys_conn, agent_path);
                        if (!agent_proxy){
                                fprintf(stdout,"create new agent_proxy failed\n");
                                return EXIT_FAILURE;
                        }

                        __display_notification( __notify_authorize_request_accept_cb, __notify_authorize_request_cancel_cb,agent_proxy);
                } else if (!strcasecmp(event_type, "app-confirm-request")) {
                        /* Not implemented */
                        fprintf(stdout," Not implemented\n");

                } else if (!strcasecmp(event_type, "push-authorize-request")) {
                        file = (gchar*) bundle_get_val(user_data, "file");
                        device_name = (gchar*) bundle_get_val(user_data, "device-name");

                        sys_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
                        if (sys_conn == NULL) {
                                fprintf(stdout,"ERROR: Can't get on system bus");
                                return EXIT_FAILURE;
                        }

                        obex_proxy = __bluetooth_create_obex_proxy(sys_conn);
                        if (!obex_proxy){
                                fprintf(stdout,"create new obex_proxy failed\n");
                                return EXIT_FAILURE;
                        }

                        __display_notification( __notify_push_authorize_request_accept_cb, __notify_push_authorize_request_cancel_cb,obex_proxy);
                } else if (!strcasecmp(event_type, "confirm-overwrite-request")) {
                        /* Not implemented */
                        fprintf(stdout," Not implemented\n");

                } else if (!strcasecmp(event_type, "keyboard-passkey-request")) {
                        /* Not implemented */
                        fprintf(stdout," Not implemented\n");

                } else if (!strcasecmp(event_type, "bt-information")) {
                        /* Not implemented */
                        fprintf(stdout," Not implemented\n");

                } else if (!strcasecmp(event_type, "exchange-request")) {
                        device_name = (gchar*) bundle_get_val(user_data, "device-name");
                        agent_path = bundle_get_val(user_data, "agent-path");

                        sys_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
                        if (sys_conn == NULL) {
                                fprintf(stdout,"ERROR: Can't get on system bus");
                                return EXIT_FAILURE;
                        }

                        agent_proxy = __bluetooth_create_agent_proxy(sys_conn, agent_path);
                        if (!agent_proxy){
                                fprintf(stdout,"create new agent_proxy failed\n");
                                return EXIT_FAILURE;
                        }

                        __display_notification( __notify_authorize_request_accept_cb, __notify_authorize_request_cancel_cb,agent_proxy);
                } else if (!strcasecmp(event_type, "phonebook-request")) {
                        device_name = bundle_get_val(user_data, "device-name");
                        agent_path = bundle_get_val(user_data, "agent-path");

                        sys_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
                        if (sys_conn == NULL) {
                                fprintf(stdout,"ERROR: Can't get on system bus");
                                return EXIT_FAILURE;
                        }

                        agent_proxy = __bluetooth_create_agent_proxy(sys_conn, agent_path);
                        if (!agent_proxy){
                                fprintf(stdout,"create new agent_proxy failed\n");
                                return EXIT_FAILURE;
                        }

                        __display_notification( __notify_authorize_request_accept_cb, __notify_authorize_request_cancel_cb,agent_proxy);
                } else if (!strcasecmp(event_type, "message-request")) {
                        device_name = bundle_get_val(user_data, "device-name");
                        agent_path = bundle_get_val(user_data, "agent-path");

                        sys_conn = dbus_g_bus_get(DBUS_BUS_SYSTEM, NULL);
                        if (sys_conn == NULL) {
                                fprintf(stdout,"ERROR: Can't get on system bus");
                                return EXIT_FAILURE;
                        }

                        agent_proxy = __bluetooth_create_agent_proxy(sys_conn, agent_path);
                        if (!agent_proxy){
                                fprintf(stdout,"create new agent_proxy failed\n");
                                return EXIT_FAILURE;
                        }

                        __display_notification( __notify_authorize_request_accept_cb,  __notify_authorize_request_cancel_cb,agent_proxy);
                }
        }
        if (notification_list != NULL) {
                 notification_free_list(notification_list);
                 notification_list = NULL;
        }

        return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
    if (!ecore_init()) {
        fprintf(stderr, "ERROR: Cannot init Ecore!\n");
        return -1;
    }

    notification_resister_changed_cb(__noti_changed_cb, NULL);
    ecore_main_loop_begin();

 shutdown:
    ecore_shutdown();
    return 0;
}

