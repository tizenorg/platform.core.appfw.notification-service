#include <Ecore.h>
#include <unistd.h>

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

    ecore_main_loop_begin();

 shutdown:
    ecore_shutdown();
    return 0;
}

