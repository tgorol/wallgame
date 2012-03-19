#ifndef _GUI_PROGRESS_DIALOG_H
#define _GUI_PROGRESS_DIALOG_H


typedef wg_boolean (*action_cb)(void *data);

typedef struct Gui_progress_dialog_screen Gui_progress_dialog_screen;

typedef struct Gui_progress_dialog Gui_progress_dialog;

WG_PUBLIC Gui_progress_dialog_screen *
gui_progress_dialog_screen_new(action_cb next, action_cb prev, void *user_data, 
        const wg_char *text);

WG_PUBLIC void
gui_progress_dialog_screen_cleanup(Gui_progress_dialog_screen *pds);

WG_PUBLIC Gui_progress_dialog *
gui_progress_dialog_new();

WG_PUBLIC void
gui_progress_dialog_cleanup(Gui_progress_dialog *pd);

WG_PUBLIC void
gui_progress_dialog_add_screen(Gui_progress_dialog *pd,
        Gui_progress_dialog_screen *pds);

WG_PUBLIC void
gui_progress_dialog_show(Gui_progress_dialog *pd);

#endif
