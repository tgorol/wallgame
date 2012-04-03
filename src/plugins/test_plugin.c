#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>
#include <wg_trans.h>

#include <wg_msg.h>
#include <wgp.h>

#define LOOP_NUM 10000

WG_PRIVATE void get_random_coordinate(float *coord);

int
main(int argc, char *argv[])
{
    wg_status status = WG_FAILURE;
    Transport trans;
    char msg[129];
    float x;
    float y;

    int i = 0;

    printf("Dummy plugin\n");

    if (argc == 1){
        printf("Error - wrong command line\n");
        return EXIT_FAILURE;
    }

    printf("Connecting to socket : %s\n", argv[1]);

    status = trans_unix_new(&trans, argv[1]);
    if (WG_SUCCESS != status){
        printf("Error - couldn't connect to socket\n");
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    for (i = 0; i < LOOP_NUM; ++i){
        trans_unix_connect(&trans);
        get_random_coordinate(&x);
        get_random_coordinate(&y);

        sprintf(msg, "%d %d\n", (int)x, (int)y);

        trans_unix_send(&trans, (wg_uchar*)msg, strlen(msg));
        trans_unix_disconnect(&trans);
    }

    trans_unix_close(&trans);

    return 0;
}


WG_PRIVATE void
get_random_coordinate(float *coord)
{
    *coord = ((float)rand() / (float)RAND_MAX) * 100.0f;

    return;
}
