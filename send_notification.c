#include <Ecore.h>
#include <Ecore_Getopt.h>
#include <notification.h>
#include <notification_internal.h>
#include <unistd.h>

const char *error_to_string(notification_error_e error)
{
    if (error == NOTIFICATION_ERROR_INVALID_PARAMETER)
        return "NOTIFICATION_ERROR_INVALID_PARAMETER";
    if (error == NOTIFICATION_ERROR_OUT_OF_MEMORY)
        return "NOTIFICATION_ERROR_OUT_OF_MEMORY";
    if (error == NOTIFICATION_ERROR_FROM_DB)
        return "NOTIFICATION_ERROR_FROM_DB";
    if (error == NOTIFICATION_ERROR_ALREADY_EXIST_ID)
        return "NOTIFICATION_ERROR_ALREADY_EXIST_ID";
    if (error == NOTIFICATION_ERROR_FROM_DBUS)
        return "NOTIFICATION_ERROR_FROM_DBUS";
    if (error == NOTIFICATION_ERROR_NOT_EXIST_ID)
        return "NOTIFICATION_ERROR_NOT_EXIST_ID";
    if (error == NOTIFICATION_ERROR_IO_ERROR)
        return "NOTIFICATION_ERROR_IO_ERROR";
    if (error == NOTIFICATION_ERROR_SERVICE_NOT_READY)
        return "NOTIFICATION_ERROR_SERVICE_NOT_READY";
    if (error == NOTIFICATION_ERROR_NONE)
        return "NOTIFICATION_ERROR_NONE";

    return "UNHANDLED ERROR";
}

static Eina_Bool remove_all(const Ecore_Getopt *parser,
                            const Ecore_Getopt_Desc *desc,
                            const char *str,
                            void *data,
                            Ecore_Getopt_Value *storage)
{
    notification_error_e err = NOTIFICATION_ERROR_NONE;

    err = notification_delete_all_by_type("SEND_TEST_PKG", NOTIFICATION_TYPE_NOTI);
    if (err != NOTIFICATION_ERROR_NONE) {
        fprintf(stderr, "Unable to remove notifications: %s\n", error_to_string(err));
        exit(-1);
    }

    exit(0);

    // will never reach here
    return 0;
}

static const Ecore_Getopt optdesc = {
    "send notification test utility",
    NULL,
    "0.0",
    "(C) 2013 Intel Corp",
    "Flora",
    "Test utility for sending notifications",
    0,
    {
        ECORE_GETOPT_STORE_STR('t', "title", "Title"),
        ECORE_GETOPT_STORE_STR('c', "content", "Content"),
        ECORE_GETOPT_STORE_STR('i', "icon", "Path to icon"),
        ECORE_GETOPT_STORE_STR('m', "image", "Path to image"),
        ECORE_GETOPT_STORE_INT('y', "imagetype", "Image type enum value"),
        ECORE_GETOPT_CALLBACK_NOARGS('r', "removeall", "Remove all notifications", remove_all, NULL),
        ECORE_GETOPT_HELP('h', "help"),
        ECORE_GETOPT_SENTINEL
    }
};

int
main(int argc, char **argv)
{
    char *title = NULL;
    char *content = NULL;
    char *icon = NULL;
    char *image = NULL;
    int imageType = 0;
    Eina_Bool quit = EINA_FALSE;
    Eina_Bool remove = EINA_FALSE;
    notification_h noti = NULL;
    notification_error_e err = NOTIFICATION_ERROR_NONE;

    Ecore_Getopt_Value values[] = {
        ECORE_GETOPT_VALUE_STR(title),
        ECORE_GETOPT_VALUE_STR(content),
        ECORE_GETOPT_VALUE_STR(icon),
        ECORE_GETOPT_VALUE_STR(image),
        ECORE_GETOPT_VALUE_INT(imageType),
        ECORE_GETOPT_VALUE_NONE,
        ECORE_GETOPT_VALUE_BOOL(quit)
    };

    if (!ecore_init()) {
        fprintf(stderr, "ERROR: Cannot init Ecore!\n");
        return -1;
    }

    if (ecore_getopt_parse(&optdesc, values, argc, argv) < 0) {
        fprintf(stderr, "Parsing arguments failed!\n");
        return -1;
    }

    if (quit)
       return 0;

    noti = notification_new(NOTIFICATION_TYPE_NOTI,
                            NOTIFICATION_GROUP_ID_NONE,
                            NOTIFICATION_PRIV_ID_NONE);
    if (noti == NULL) {
        fprintf(stderr, "Failed to create notification: %s\n", error_to_string(err));
        return -1;
    }

    err = notification_set_pkgname(noti, "SEND_TEST_PKG");
    if (err != NOTIFICATION_ERROR_NONE) {
        fprintf(stderr, "Unable to set pkgname: %s\n", error_to_string(err));
        return -1;
    }

    err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
                                title ? title : "Default Title",
                                NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    if (err != NOTIFICATION_ERROR_NONE) {
        fprintf(stderr, "Unable to set notification title: %s\n", error_to_string(err));
        return -1;
    }

    err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
                                content ? content : "Default Content",
                                NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    if (err != NOTIFICATION_ERROR_NONE) {
        fprintf(stderr, "Unable to set notification content: %s\n", error_to_string(err));
        return -1;
    }

    if (icon) {
        err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, icon);
        if (err != NOTIFICATION_ERROR_NONE) {
            fprintf(stderr, "Unable to set notification icon path: %s\n", error_to_string(err));
            return -1;
        }
    }

    if (image) {
        err = notification_set_image(noti, imageType, image);
        if (err != NOTIFICATION_ERROR_NONE) {
            fprintf(stderr, "Unable to set notification image path: %s\n", error_to_string(err));
            return -1;
        }
    }

    err = notification_insert(noti, NULL);
    if (err != NOTIFICATION_ERROR_NONE) {
        fprintf(stderr, "Unable to insert notification: %s\n", error_to_string(err));
        return -1;
    }

    fprintf(stdout, "Sent Notification > %s : %s : %s : %s : %i\n",
            title, content, icon, image, imageType);
    return 0;
}

