#ifndef _GPM_CONSOLE_H
#define _GPM_CONSOLE_H

#define DEFAULT_CMD_NUM         24

typedef wg_status (*Console_hook_cb)(wg_uint argc, wg_char *args[], void *private_data);

typedef enum HOOK_FLAG {
    HOOK_SYNC   = 1 << 0,
    HOOK_ASYNC  = 1 << 1,
    HOOK_EXIT   = 1 << 2
}HOOK_FLAG;

typedef struct Console_hook{
    wg_char *name;
    wg_char *description;
    wg_char **detail_lines;
    Console_hook_cb cb_hook;
    HOOK_FLAG flags;
    void *private_data;
}Console_hook;


#define DEFAULT_HOOK_FLAGS  (HOOK_SYNC)

wg_status
gpm_console_init(wg_uint cmd_num);

wg_status
gpm_console_cleanup(void);

wg_status
gpm_console_set_prompt(wg_char *propmt);

wg_status
gpm_console_start(void);

wg_status
gpm_console_register_hook(Console_hook *hook);

wg_status
gpm_console_add_hook_description(Console_hook *hook, wg_char* description,
        wg_char **details);

wg_status
gpm_console_new_hook(wg_char *name, Console_hook_cb cb_hook, 
        void *private_data, Console_hook **hook);

wg_status
gpm_console_set_hook_flags(Console_hook *hook, HOOK_FLAG flags);

wg_status
gpm_console_add_hook_flags(Console_hook *hook, HOOK_FLAG flags);

void
gpm_console_hook_release(Console_hook *hook);


#endif
