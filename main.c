#include <Ecore.h>
#include <notification_service.h>
#include <unistd.h>
#include <vconf.h>
#include <vconf-internal-livebox-keys.h>

int
main(int argc, char **argv)
{
    if (!ecore_init()) {
        fprintf(stderr, "ERROR: Cannot init Ecore!\n");
        return -1;
    }

    if (notification_service_init() != 0) {
        fprintf(stderr, "Unable to initialize notification service!\n");
        goto shutdown;
    }

    vconf_set_bool(VCONFKEY_MASTER_STARTED, 1);
    ecore_main_loop_begin();

 shutdown:
    vconf_set_bool(VCONFKEY_MASTER_STARTED, 0);
    ecore_shutdown();

    return 0;
}

