#include <Ecore.h>
#include <notification.h>
#include <notification_list.h>
#include <unistd.h>

static void __noti_changed_cb(void *data, notification_type_e type)
{
    notification_h noti = NULL;
    notification_list_h notification_list = NULL;
    notification_list_h get_list = NULL;
    int count = 0, group_id = 0, priv_id = 0, show_noti = 0, num = 1;
    char *pkgname = NULL;
    char *title = NULL;
    char *str_count = NULL;
    char *content = NULL;
    int i = 1;
    char buf[512] = {0};

    notification_get_list(NOTIFICATION_TYPE_NOTI, -1, &notification_list);
    if (notification_list) {
        get_list = notification_list_get_head(notification_list);
        noti = notification_list_get_data(get_list);
        while(get_list != NULL) {
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
            notification_get_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, &content);


            fprintf(stdout, "NOTIFICATION: %s - %s - %s - %i - %i\n", pkgname, title, content, count, num);

            get_list = notification_list_get_next(get_list);
            noti = notification_list_get_data(get_list);
            num++;
        }
    }
    if (notification_list != NULL) {
        notification_free_list(notification_list);
        notification_list = NULL;
    }
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

