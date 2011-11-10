#include <string.h>
#include <pthread.h>
#include <alloca.h>
#include <inttypes.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <ght_hash_table.h>
#include <editline/readline.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <wg_string.h>

#include "include/gpm_console.h"
#include "include/gpm_console_parser.h"

/*! \defgroup  Gameplay_console Gameplay Console
 */

/*! @{ */

typedef struct Async_hook_info_block{
    pthread_t tid;
    wg_char *command;
    List_head list;
}Async_hook_info_block;

typedef struct Cmd_info {
    wg_char *name;
    wg_char *description;
    Console_hook_cb cb_hook;
    HOOK_FLAG flags;
    void *private_data;
}Cmd_info;

WG_PRIVATE wg_status 
def_cb_help(wg_uint argc, wg_char *argv[], void *private_data);

#ifdef WGDEBUG
WG_PRIVATE wg_status 
debug_cb_print(wg_uint argc, wg_char *argv[], void *private_data);
#endif

WG_PRIVATE wg_char *prompt = NULL;
WG_PRIVATE wg_status release_prompt(void);
WG_PRIVATE wg_status set_prompt(wg_char *new_prompt);

WG_PRIVATE ght_hash_table_t *cmd_directory = NULL;

WG_PRIVATE List_head async_cmd;

WG_PRIVATE wg_status add_default_hooks(void);

WG_PRIVATE wg_status get_hook(wg_char *cmd, Console_hook **hook);

WG_PRIVATE wg_boolean execute_command(wg_uint argv, wg_char *command[]);

WG_PRIVATE wg_status print_detail_lines(wg_char *lines[]);

Cmd_info def_cmd_info[] = {
    {
        .name         = "help"                ,
        .description  = "Print help screen"   ,
        .cb_hook      = def_cb_help           ,
        .flags        = HOOK_SYNC             ,
        .private_data = NULL             
    }

#ifdef WGDEBUG
    ,{
        .name         = "print"                ,
        .description  = "DBG: print arguments" ,
        .cb_hook      = debug_cb_print         ,
        .flags        = HOOK_SYNC              ,
        .private_data = NULL             
    }
#endif
};


/**
 * @brief Initialize a console
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_init(wg_uint cmd_num)
{
    wg_status status = WG_FAILURE;

    cmd_num = cmd_num == 0 ? DEFAULT_CMD_NUM : cmd_num;
    if (NULL == cmd_directory){
        cmd_directory = ght_create(cmd_num + ELEMNUM(def_cmd_info));
        CHECK_FOR_NULL(cmd_directory);
        list_init(&async_cmd);

        status = add_default_hooks();
        if (WG_FAILURE == status){
            ght_finalize(cmd_directory);
        }
    }

    return status;
}

/**
 * @brief Release resources allocated by the console
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_cleanup(void)
{
    ght_iterator_t iterator;
    const void *hook_name;
    void *hook;

    gpm_console_set_prompt(NULL);

    for(hook = ght_first(cmd_directory, &iterator, &hook_name); 
            hook != NULL;
            hook = ght_next(cmd_directory, &iterator, &hook_name))
    {
        gpm_console_hook_release(hook);
    }

    if (NULL != cmd_directory){
        ght_finalize(cmd_directory);
    }

    return WG_SUCCESS;
}

/**
 * @brief Set a prompt for a console.
 *
 * @param new_prompt new prompt to set or NULL do delete prompt.
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_set_prompt(wg_char *new_prompt)
{
    wg_status status = WG_FAILURE;

    if (NULL == new_prompt){
        status = release_prompt();
    }else{
        status = set_prompt(new_prompt);
    }

    return status;
}

/**
 * @brief Add a hook for a command.
 *
 * @param hook a function to call when the command enterd
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_register_hook(Console_hook *hook)
{
    int status = 0;
    DECLARE_FUNC_STR_BUFFER(cptr);

    CHECK_FOR_NULL(hook);
    CHECK_FOR_NULL(hook->name);
    CHECK_FOR_NULL(hook->cb_hook);


    wg_fptr_2_str((fvoid)hook->cb_hook, cptr);


    WG_DEBUG("New command hook: %s 0x%s %p\n", (char*)hook->name, 
            cptr, hook->private_data);

    status = ght_insert(cmd_directory, hook, strlen(hook->name), hook->name);

    return status == 0 ? WG_SUCCESS : WG_FAILURE;
}

/**
 * @brief Start a console.
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_start(void)
{
    wg_char *line = NULL;
    wg_char **argv = NULL;
    wg_status status = WG_FAILURE;
    List_head tok_list = {0};
    wg_int argc = 0;

    do{
        argv = NULL;
        line = NULL;

        do{
            line = readline((prompt != NULL) ? prompt : "");
        }while (line == NULL);
        if ('\0' != *line){
            add_history(line);
        }

        dlist_init(&tok_list);
        status = gpm_console_parse(line, &tok_list);
        if (WG_FAILURE == status){
            WG_FREE(line);
            printf("syntax error\n");
            break;
        }

        WG_FREE(line);

        status = gpm_console_tokens_to_array(&tok_list, &argv);

        argc = list_size(&tok_list) - 1;

        gpm_console_remove_token_list(&tok_list);

        CHECK_FOR_FAILURE(status);

    } while (execute_command(argc, argv) == WG_FALSE);

    return WG_SUCCESS;
}

/**
 * @brief Create a new hook for a console
 *
 * @param hook   memory to store a new hook
 * @param name   name of the hook (command name)
 * @param cb_hook   function too call when name entered
 * @param private_data private data to pass to a callback
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_new_hook(wg_char *name, Console_hook_cb cb_hook, void *private_data,
        Console_hook **hook)
{
    Console_hook *console_hook = NULL;

    CHECK_FOR_NULL(name);
    CHECK_FOR_NULL(hook);
    CHECK_FOR_NULL(cb_hook);

    console_hook = WG_CALLOC(1, sizeof (Console_hook));
    CHECK_FOR_NULL(console_hook);

    console_hook->name = name;
    console_hook->cb_hook = cb_hook;
    console_hook->private_data = private_data;
    console_hook->flags = DEFAULT_HOOK_FLAGS;

    *hook = console_hook;

    return WG_SUCCESS;
}

/**
 * @brief Set custom flags for a hook
 *
 * @param hook    hook instance
 * @param flags   flags to set
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_set_hook_flags(Console_hook *hook, HOOK_FLAG flags)
{
    CHECK_FOR_NULL(hook);

    hook->flags = flags;

    return WG_SUCCESS;
}

/**
 * @brief Add custom flags for a hook without overwriting existing ones.
 *
 * @param hook    hook instance
 * @param flags   flags to add
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_add_hook_flags(Console_hook *hook, HOOK_FLAG flags)
{
    CHECK_FOR_NULL(hook);

    hook->flags |= flags;

    return WG_SUCCESS;
}

/**
 * @brief Add detailed description to the hook
 *
 * @param hook  hook instance
 * @param desc  description
 * @param details  get detailed description of the command
 *
 * @return 
 */
wg_status
gpm_console_add_hook_description(Console_hook *hook, wg_char *desc, 
        wg_char **details)
{
    CHECK_FOR_NULL(hook);

    hook->description = desc;
    hook->detail_lines = details;

    return WG_SUCCESS;
}

/**
 * @brief Get hook flags
 *
 * @param hook    hook instance
 * @param flags   memory to store flags
 *
 * @retval WG_SUCCESS
 * @retval WG_FAILURE
 */
wg_status
gpm_console_get_hook_flags(Console_hook *hook, HOOK_FLAG *flags)
{
    CHECK_FOR_NULL(hook);
    CHECK_FOR_NULL(flags);

    *flags =hook->flags ;

    return WG_SUCCESS;
}

/**
 * @brief Clean a hook
 * 
 * This function doesnt release a hook itself !!!
 *
 * @param hook hook to clean
 */
void
gpm_console_hook_release(Console_hook *hook)
{
    WG_FREE(hook);

    return;
}


WG_PRIVATE wg_boolean
execute_command(wg_uint argc, char *command[])
{
    Console_hook *hook = NULL;
    wg_status status = WG_FAILURE;
    wg_boolean exit_flag = WG_FALSE;

    if (NULL != command[0]){
        status = get_hook(command[0], &hook);
        if (WG_FAILURE == status){
            fprintf(stdout, "Unknown command \"%s\"\n", command[0]);
            gpm_console_remove_args(command);
            return exit_flag;
        }

        status = hook->cb_hook(argc, command, hook->private_data);
        if (hook->flags & HOOK_EXIT){
            exit_flag = (status == WG_SUCCESS) ? WG_TRUE : WG_FALSE;
        }else{
            if (WG_FAILURE == status){
                fprintf(stdout, "Syntax error\n");
            }
            exit_flag = WG_FALSE;
        }
    }
    gpm_console_remove_args(command);

    return exit_flag;
}

WG_PRIVATE wg_status
add_default_hooks(void)
{
    wg_uint index = 0;
    Cmd_info *def_cmd = NULL;
    Console_hook *hook = NULL;
    wg_status  status = WG_FAILURE;

    for (index = 0; index < ELEMNUM(def_cmd_info); ++index){
        def_cmd = &(def_cmd_info[index]);
        status = gpm_console_new_hook(def_cmd->name, def_cmd->cb_hook, NULL,
                &hook);
        CHECK_FOR_FAILURE(status);

        gpm_console_set_hook_flags(hook, def_cmd->flags);
        gpm_console_add_hook_description(hook, def_cmd->description, NULL);
        gpm_console_register_hook(hook);
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
get_hook(wg_char *cmd, Console_hook **hook)
{
    Console_hook *cmd_hook = NULL;

    CHECK_FOR_NULL(hook);
    CHECK_FOR_NULL(cmd);

    cmd_hook = ght_get(cmd_directory, strlen(cmd), cmd);

    *hook = cmd_hook;

    return (cmd_hook != NULL) ? WG_SUCCESS : WG_FAILURE;
}

WG_PRIVATE wg_status
set_prompt(wg_char *new_prompt)
{
    wg_char *buffer = NULL;

    release_prompt();

    buffer = WG_MALLOC(strlen(new_prompt) + 1);
    CHECK_FOR_NULL(buffer);

    strcpy(buffer, new_prompt);

    prompt = buffer;
    buffer = NULL;

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
release_prompt(void)
{
    WG_FREE(prompt);

    return WG_SUCCESS;
}


WG_PRIVATE wg_status 
def_cb_help(wg_uint argc, wg_char *argv[], void *private_data)
{
    ght_iterator_t iterator;
    const void *hook_name = NULL;
    Console_hook *hook = NULL;

    if (argc == 1){
        for(hook = ght_first(cmd_directory, &iterator, &hook_name); 
                hook != NULL;
                hook = ght_next(cmd_directory, &iterator, &hook_name))
        {
            printf("%-13s %s\n", hook->name, 
                    (hook->description ? hook->description : ""));
        }
    }else{
        for (++argv; *argv != NULL; ++argv){
            hook = ght_get(cmd_directory, strlen(*argv), *argv);
            if (NULL != hook){
                printf("%-13s %s\n", hook->name, 
                        (hook->description ? hook->description : ""));

                print_detail_lines(hook->detail_lines);

            }
        }
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
print_detail_lines(wg_char *lines[])
{
    while ((lines != NULL) && (*lines != NULL)){
        printf("%s\n", *lines++);
    }

    return WG_SUCCESS;
}

#ifdef WGDEBUG

WG_PRIVATE wg_status 
debug_cb_print(wg_uint argc, wg_char *argv[], void *private_data)
{
    while (*argv != NULL){
        printf("%s\n", *argv++);
    }

    return WG_SUCCESS;
}

#endif

/*! @} */
