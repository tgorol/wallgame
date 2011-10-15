#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_cm.h>
#include <wg_gpm.h>

#include "include/gpm.h"


/*! \defgroup Gameplay Gameplay Manager
 */

/*! @{ */


WG_PRIVATE void
print_help(void);

/**
 * @brief Parse a command line.
 *
 * @param argc     number of arguments passed to main
 * @param argv[]   arguments passed to main
 * @param app_opt structure to store passed parameters
 *
 * @return 0 on success otherwise fails
 */
wg_int
gpm_cmdln_parse(int argc, char *argv[], App_options *app_opt)
{
    int opt = 0;
    wg_int retcode = WG_EXIT_SUCCESS;;

    CHECK_FOR_NULL(app_opt);
    CHECK_FOR_NULL(argv);

    while (((opt = getopt(argc, argv, GETOPT_STRING)) != -1) && (retcode == 0)){
        switch (opt){
            case 'i':
                WG_LOG("Command Line option found : -i %s\n", optarg);
                strncpy(app_opt->ini_file_name.value, optarg, 
                        COMMAND_LINE_OPTION_SIZE);
                break;
            case 'h':
                print_help();
                retcode = WG_EXIT_ERROR;
                break;
            case ':':
                retcode = WG_EXIT_ERROR;
                break;
            case '?':
                retcode = WG_EXIT_ERROR;
                break;
            default:
                retcode = WG_EXIT_ERROR;
                break;
        }
    }

    return retcode;
}

WG_PRIVATE void
print_help(void)
{
    printf("TODO : write help\n");
    return;
}

/*! @} */
