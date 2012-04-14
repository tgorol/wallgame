#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

/* @todo create seperate user include file */
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <unistd.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>
#include <wg_trans.h>

#include <wg_plugin_tools.h>


#define LOOP_NUM     30
#define DELAY_IN_SEC 5

WG_PRIVATE void get_random_coordinate(wg_double *coord);

int
main(int argc, char *argv[])
{
    wg_status status = WG_FAILURE;
    Wg_msg_transport msg_transport;
    wg_double x;
    wg_double y;

    int i = 0;

    MEMLEAK_START;

    printf("Dummy plugin\n");

    if (argc == 1){
        printf("Error - wrong command line\n");
        return EXIT_FAILURE;
    }

    printf("Connecting to socket : %s\n", argv[1]);

    status = wg_msg_transport_init(argv[1], &msg_transport);
    if (WG_SUCCESS != status){
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    for (i = 0; i < LOOP_NUM; ++i){
        get_random_coordinate(&x);
        get_random_coordinate(&y);
        wg_msg_transport_send_hit(&msg_transport, x, y);
        get_random_coordinate(&x);
        get_random_coordinate(&y);
        wg_msg_transport_send_hit(&msg_transport, x, y);
        get_random_coordinate(&x);
        get_random_coordinate(&y);
        wg_msg_transport_send_hit(&msg_transport, x, y);

        sleep(DELAY_IN_SEC);
    }

    wg_msg_transport_cleanup(&msg_transport);

    MEMLEAK_STOP;

    return 0;
}


WG_PRIVATE void
get_random_coordinate(wg_double *coord)
{
    *coord = ((wg_double)rand() / RAND_MAX) * 100.0;

    return;
}
