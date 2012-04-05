#include <stdlib.h>
#include <sys/types.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <stdlib.h>

#include <wg_cm.h>
#include <wg_gpm.h>


/**
 * @brief Entry point to the program
 *
 * @param argc
 * @param argv[]
 *
 * @return EXIT_SUCCESS
 */
int
main(int argc, char *argv[])
{
    int exit_code = EXIT_FAILURE;

    MEMLEAK_START;
    exit_code = wg_start(argc, argv);
    MEMLEAK_STOP;

    return exit_code;
}


