#include <stdlib.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_cm.h>
#include <wg_gpm.h>

#include "include/gpm.h"
#include "include/gpm_ini.h"
#include "include/gpm_console.h"
#include "include/gpm_cmdln.h"
#include "include/gpm_hooks.h"

typedef struct Cmd_info {
    wg_char *name;
    wg_char *description;
    Console_hook_cb cb_hook;
    HOOK_FLAG flags;
    void *private_data;
    wg_char **detail_lines;
}Cmd_info;

wg_char *details_lsg[] = {
    "    lsg            -   print all available games in the system with id",
    "    lsg <id list>  -   print details about a game",
    "    example:",
    "        lsg 1 3 5  -   print game details for games with id 1, 3, 5",
    NULL
};

Cmd_info cmd_info[] = {
    {
        .name         = "exit"           ,
        .description  = "Quit program"   ,
        .cb_hook      = cb_exit          ,
        .flags        = HOOK_SYNC | HOOK_EXIT   ,
        .private_data = NULL             ,
        .detail_lines = NULL
    }
    ,
    {
        .name         = "connect"           ,
        .description  = "Connect to the game"   ,
        .cb_hook      = cb_connect       ,
        .flags        = HOOK_SYNC | HOOK_EXIT   ,
        .private_data = NULL             ,
        .detail_lines = NULL
    }
    ,
    {
        .name         = "version"        ,
        .description  = "Print version"  ,
        .cb_hook      = cb_info          ,
        .flags        = HOOK_SYNC        ,
        .private_data = NULL             ,
        .detail_lines = NULL
    }
    ,
    {
        .name         = "start"          ,
        .description  = "Start a game "  ,
        .cb_hook      = cb_start         ,
        .flags        = HOOK_ASYNC       ,
        .private_data = NULL             
    }
    ,
    {
        .name         = "stop"           ,
        .description  = "Stop a game"    ,
        .cb_hook      = cb_stop          ,
        .flags        = HOOK_SYNC        ,
        .private_data = NULL             
    }
    ,
    {
        .name         = "pause"          ,
        .description  = "Stop a game"    ,
        .cb_hook      = cb_pause         ,
        .flags        = HOOK_SYNC        ,
        .private_data = NULL             
    }
    ,
    {
        .name         = "lsg"            ,
        .description  = "List available games"    ,
        .cb_hook      = cb_lsg           ,
        .flags        = HOOK_SYNC        ,
        .private_data = NULL             ,      
        .detail_lines = details_lsg
    }
    ,
    {
        .name         = "send"            ,
        .description  = "Send data to the game"    ,
        .cb_hook      = cb_send           ,
        .flags        = HOOK_SYNC        ,
        .private_data = NULL             ,      
        .detail_lines = NULL
    }
};

WG_PRIVATE wg_status setup_console();
WG_PRIVATE wg_status set_default_options(App_options *cmdln);

/**
 * @brief Entry point to the Server
 *
 * @param argc    argc passed to the main()
 * @param argv[]  argv passed to the main()
 *
 * @return Exit Code or 0 on success
 */
int
wg_start(int argc, char *argv[])
{
    wg_status status = WG_FAILURE;
    wg_int exit_code = WG_EXIT_SUCCESS;
    App_options options;

    set_default_options(&options);

    /* parse input command line */
    exit_code = gpm_cmdln_parse(argc, argv, &options);
    if (WG_EXIT_SUCCESS != exit_code){
        return exit_code;
    }
    WG_PRINT("Reading Config File : %s\n",
            options.ini_file_name.value);

    /* read an ini file */
    status = gpm_ini_read(options.ini_file_name.value);
    if (WG_SUCCESS != status){
        return WG_EXIT_ERROR;
    }

    /* prepare a console */
    status = setup_console();
    if (WG_SUCCESS != status){
        return WG_EXIT_ERROR;
    }

    /* start receiving input from the user */
    status = gpm_console_start();

    /* clean console */
    gpm_console_cleanup();

    return WG_EXIT_SUCCESS;
}

WG_PRIVATE wg_status 
set_default_options(App_options *cmdln)
{
    char *env_var = NULL;

    CHECK_FOR_NULL(cmdln);

    memset(cmdln, '\0', sizeof (App_options));

    /* 
     * create a default ini file path. Try ${HOME} and then
     * /home/${USER} it fails then try INI_BACKUP_DIR
     */
    env_var = getenv("HOME");
    if (NULL == env_var){
        WG_LOG("No HOME defined try /home/$(USER)/\n");
        env_var = getenv("USER");
        if (NULL == env_var){
            WG_LOG("No USER defined\n");
            env_var = INI_BACKUP_DIR;
        }
    }

    snprintf(cmdln->ini_file_name.value , COMMAND_LINE_OPTION_SIZE, "%s/%s", 
            env_var, INI_DEFAULT_NAME);

    return WG_SUCCESS;
}


WG_PRIVATE wg_status
setup_console()
{
    Console_hook *hook = NULL;
    wg_int index = 0; 
    Cmd_info *info = NULL;

    wg_status status = WG_FAILURE;

    status = gpm_console_init(0);
    if (WG_FAILURE == status){
        return WG_FAILURE;
    }

    status = gpm_console_set_prompt(PROMPT_DEFAULT);
    if (WG_FAILURE == status){
        gpm_console_cleanup();
        return WG_FAILURE;
    }

    for (index = 0; index < ELEMNUM(cmd_info); ++index){
        info = &(cmd_info[index]);
        status = gpm_console_new_hook(info->name, info->cb_hook,
                info->private_data, &hook);
        if (WG_FAILURE == status){
            gpm_console_cleanup();
            return WG_FAILURE;
        }

        gpm_console_set_hook_flags(hook, info->flags);

        gpm_console_add_hook_description(hook, info->description, 
                info->detail_lines);

        gpm_console_register_hook(hook);
    }

    return WG_SUCCESS;
}

