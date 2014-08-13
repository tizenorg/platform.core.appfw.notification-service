#include <Ecore.h>
#include <notification.h>
#include <unistd.h>

#include <bundle.h>
#ifdef HAVE_WAYLAND
#include <libwlmessage.h>
#endif

#include <bundle.h>

#include <bluetooth.h>
#include <dlog.h>

#define POPUP_TYPE_INFO  "user_info_popup"
#define POPUP_TYPE_USERCONFIRM "user_confirm_popup"
#define POPUP_TYPE_USERPROMPT "user_agreement_popup"

static void display_user_information_popup(void) {};

static void display_user_prompt_popup(void) {};

static void display_user_confirmation_popup(void) 
{
         notification_error_e err = NOTIFICATION_ERROR_NONE;
         int bt_yesno = 1;

         char line[4];

#ifdef HAVE_WAYLAND
         struct wlmessage *wlmessage = wlmessage_create();
         wlmessage_set_message(wlmessage, "Do you confirm ?");
         wlmessage_add_button(wlmessage, 1, "Yes");
         wlmessage_add_button(wlmessage, 0, "No");
         wlmessage_set_default_button(wlmessage, 1);
         bt_yesno = wlmessage_show(wlmessage, NULL);
         wlmessage_destroy(wlmessage);

         if (bt_yesno == 1)
                 bt_agent_reply_sync(0);
         else if (bt_yesno == 0)
                 bt_agent_reply_sync(1);
#else
         fprintf(stdout, "Do you confirm yes or no ? ");
         while ( bt_yesno != 0){
                 if (!fgets(line, sizeof(line), stdin))
                         continue;
                 if ( strncmp("yes", line, 3) == 0) {
                         LOGD("user accepts to pair with device ");
                         bt_agent_reply_sync(0);
                         bt_yesno = 0;
                 } else if ( strncmp("no", line, 2) == 0) {
                         LOGD("user rejects to pair with device ");
                         bt_agent_reply_sync(1);
                         bt_yesno = 0;
                 } else {
                         fprintf(stdout," yes or no ?\n");
                         continue;
                 }
         }
#endif
         err = notification_delete_all_by_type("bt-agent", NOTIFICATION_TYPE_NOTI);
         if (err != NOTIFICATION_ERROR_NONE) {
                  LOGE("Unable to remove notifications");
         }

}

static void __noti_changed_cb(void *data, notification_type_e type)
{
         notification_h noti = NULL;
         notification_list_h notification_list = NULL;
         notification_list_h get_list = NULL;
         notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

         LOGD("listen to new notifications...");

         int count = 0, group_id = 0, priv_id = 0, num = 1;
         char *pkgname = NULL;
         char *title = NULL;
         char *str_count = NULL;
         char *content= NULL;
         bundle *user_data = NULL;

         const char *device_name = NULL;
         const char *passkey = NULL;
         const char *agent_path;
         const char *event_type = NULL;

         char *popup_type = NULL;

         noti_err = notification_get_list(NOTIFICATION_TYPE_NOTI, -1, &notification_list);
         if (noti_err != NOTIFICATION_ERROR_NONE) {
                LOGE("notification_get_list() failed (error code = %d)", noti_err);
         }

         if (notification_list) {
                LOGD("new notificiation received");

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

                notification_get_text(noti, NOTIFICATION_TEXT_TYPE_INFO_1, &popup_type);
                LOGD("'%s' notification type [%s]", pkgname, popup_type);

                if (!strcasecmp(POPUP_TYPE_INFO, popup_type))
                  display_user_information_popup();
                else if (!strcasecmp(POPUP_TYPE_USERCONFIRM, popup_type))
                  display_user_confirmation_popup();
                else if (!strcasecmp(POPUP_TYPE_USERPROMPT, popup_type))
                  display_user_prompt_popup();
                else
                  LOGE("popup type is not recognized !");


                if (notification_list != NULL) {
                        notification_free_list(notification_list);
                        notification_list = NULL;
                }
  }
  return;
}

int
main(int argc, char **argv)
{
    if (!ecore_init()) {
        LOGE("ERROR: Cannot init Ecore!\n");
        return -1;
    }

    bt_initialize();

    bt_agent_register_sync();

    notification_resister_changed_cb(__noti_changed_cb, NULL);
    ecore_main_loop_begin();

    return 1;
}

