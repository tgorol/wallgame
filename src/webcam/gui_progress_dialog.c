#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>


#include "include/gui_progress_dialog.h"

/*! \defgroup plugin_webcam Plug-in Webcam
 */

/*! \defgroup gui_wizard_progress Wizard Screen List
    \ingroup plugin_webcam
*/

/*! @{ */

/*! \brief Back button id */
#define BACK_RESPONSE_ID 0

/*! \brief Next button id */
#define NEXT_RESPONSE_ID 1

/*! \brief Next dialog id */
#define NEXT_SCREEN      1

/*! \brief Prievious dialog id */
#define PREVIUOS_SCREEN -1

/** 
* @brief Dialogs
*/
struct Gui_progress_dialog_screen{
    action_cb action;        /*!< action callback */
    void *user_data;         /*!< user data       */
    wg_char *text;           /*!< text to display */
    GtkWidget *extra_widget; /*!< extra widget displayed below text */
    List_head list;          /*!< list            */
};

/** 
* @brief Progress dialogs
*/
struct Gui_progress_dialog{
    List_head screens;          /*!< dialog list                      */
    wg_uint active_index;       /*!< displaying dialog id             */
    wg_uint size;               /*!< number of displays               */ 
    Gui_progress_dialog_screen **screens_array; /*!< dialogs in array */
    GtkTextBuffer *buffer;      /*!< widget used to display text      */
    GtkWidget *dialog;          /*!< dialog gtk instance              */
    GtkWidget *box;             /*!< box                              */
    exit_action_cb exit_action; /*!< exit callback                    */
};

WG_PRIVATE void
response(GtkDialog *dialog, gint response_id, gpointer user_data);

WG_PRIVATE void
show_screen(Gui_progress_dialog *pd, wg_int val);

WG_PRIVATE wg_boolean
call_action(Gui_progress_dialog *pd, Gui_progress_action action_id);

WG_PRIVATE wg_uint
increment_screen_index(Gui_progress_dialog *pd, wg_int val);

WG_PRIVATE wg_boolean
is_last_screen(Gui_progress_dialog *pd);

WG_PRIVATE void
show_extra_widget(Gui_progress_dialog *pd);

WG_PRIVATE void
hide_extra_widget(Gui_progress_dialog *pd);

/** 
* @brief Create new progress dialogs.
* 
* @return insrance of new progress dialogs
*/
Gui_progress_dialog *
gui_progress_dialog_new()
{
    Gui_progress_dialog *pd = NULL;

    pd = WG_CALLOC(1, sizeof (Gui_progress_dialog));
    if (NULL == pd){
        return NULL;
    }

    list_init(&pd->screens);
    pd->active_index  = 0;
    pd->size          = 0;
    pd->screens_array = NULL;
    pd->exit_action   = NULL;

    return pd;
}

/** 
* @brief Clean all resources alloctes by gui_progress_dialog_new()
* 
* @param pd instance of progress dialogs
*/
void
gui_progress_dialog_cleanup(Gui_progress_dialog *pd)
{
    Gui_progress_dialog_screen *pds = NULL;
    Iterator itr;

    iterator_list_init(&itr, &pd->screens,
            GET_OFFSET(Gui_progress_dialog_screen, list));

    while((pds = iterator_list_next(&itr)) != NULL){
        gui_progress_dialog_screen_cleanup(pds);
    }

    WG_FREE(pd->screens_array);

    WG_FREE(pd);

    return;
}

/** 
* @brief Create new screen
* 
* @param action       action associated with screen
* @param user_data    user data
* @param text         text to display
* 
* @return dialog instance
*/
Gui_progress_dialog_screen *
gui_progress_dialog_screen_new(action_cb action, void *user_data,
        const wg_char *text, GtkWidget *extra_widget)
{
    Gui_progress_dialog_screen *pds = NULL;
    wg_size size = 0;

    size = sizeof (Gui_progress_dialog_screen) + strlen(text) + 1;

    pds = WG_CALLOC(1, size);
    if (NULL == pds){
        return NULL;
    }

    pds->text = (wg_char*)(pds + 1);
    strcpy(pds->text, text);

    pds->action = action;
    pds->user_data = user_data;

    list_init(&pds->list);

    if (NULL != extra_widget){
        g_object_ref(extra_widget);
        pds->extra_widget = extra_widget;
    }

    return pds;
}

/** 
* @brief Get extra widget
* 
* @param pds screen 
* 
* @return widget instance or NULL
*/
GtkWidget *
gui_progress_dialog_screen_get_widget(Gui_progress_dialog_screen *pds)
{
    return pds->extra_widget;
}

/** 
* @brief Clean all resources allocated by gui_progress_dialog_screen_new()
* 
* @param pds dialog instance
*/
void
gui_progress_dialog_screen_cleanup(Gui_progress_dialog_screen *pds)
{
    if (NULL != pds->extra_widget){
        g_object_unref(pds->extra_widget);
    }

    WG_FREE(pds);

    return;
}

/** 
* @brief Add new dialog to progress dialog
* 
* @param pd   progress dialogs instance
* @param pds  dialog instance to add
*/
wg_status
gui_progress_dialog_add_screen(Gui_progress_dialog *pd,
        Gui_progress_dialog_screen *pds)
{
    CHECK_FOR_NULL_PARAM(pd);
    CHECK_FOR_NULL_PARAM(pds);

    list_add(&pd->screens, &pds->list);

    return WG_SUCCESS;
}

/** 
* @brief Add exit callback
*
* Exit callback is called as last before closing dialog progress
* 
* @param pd      progress screen instance
* @param action  exit action
*/
wg_status
gui_progress_dialog_set_exit_action(Gui_progress_dialog *pd,
        exit_action_cb action)
{
    CHECK_FOR_NULL_PARAM(pd);

    pd->exit_action = action;

    return WG_SUCCESS;
}

/** 
* @brief Show progress dialogs
* 
* @param pd progress dialogs instance
*/
wg_status
gui_progress_dialog_show(Gui_progress_dialog *pd)
{
    GtkBuilder *builder = NULL;
    GtkWidget  *dialog  = NULL; 
    GtkWidget  *widget  = NULL;
    wg_uint size = 0;
    wg_int i = 0;
    Gui_progress_dialog_screen **pds_array = NULL;
    Gui_progress_dialog_screen *pds       = NULL;
    Iterator itr;

    CHECK_FOR_NULL_PARAM(pd);

    builder = gtk_builder_new ();
    gtk_builder_add_from_file (
            builder, "progress_dialog.xml", NULL);
    dialog = GTK_WIDGET (gtk_builder_get_object (builder, "progress_dialog"));
    gtk_builder_connect_signals (builder, NULL);

    pd->dialog = dialog;

    /* get text instance */
    widget = GTK_WIDGET (gtk_builder_get_object (builder, "text_view"));
    pd->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));

    /* get box instance */
    widget = GTK_WIDGET (gtk_builder_get_object (builder, "box"));
    pd->box = widget;

    /* release builder */
    g_object_unref(G_OBJECT (builder));

    g_signal_connect(GTK_DIALOG(dialog), "response", 
      G_CALLBACK(response), pd);

    size = list_size(&pd->screens);
    pds_array = WG_CALLOC(size, sizeof (Gui_progress_dialog_screen*));
    if (NULL == pds_array){
        return WG_FAILURE;
    }

    pd->size = size;
    pd->screens_array = pds_array;

    iterator_list_init(&itr, &pd->screens,
            GET_OFFSET(Gui_progress_dialog_screen, list));

    for (i = 0; (pds = iterator_list_next(&itr)) != NULL; ++i){
        pd->screens_array[i] = pds; 
    }

    show_screen(pd, 0);
    show_extra_widget(pd);
    call_action(pd, GUI_PROGRESS_ENTER);

    gtk_widget_show_all(dialog);

    return WG_SUCCESS;
}

WG_PRIVATE wg_boolean
is_last_screen(Gui_progress_dialog *pd)
{
    return pd->active_index >= pd->size;
}

WG_PRIVATE wg_uint
increment_screen_index(Gui_progress_dialog *pd, wg_int val)
{
    pd->active_index += val;
    if (pd->active_index > pd->size){
        pd->active_index -= val;
    }

    return pd->active_index;
}

WG_PRIVATE void
show_screen(Gui_progress_dialog *pd, wg_int val)
{
    GtkWidget *widget = NULL;

    increment_screen_index(pd, val);

    if (!is_last_screen(pd)){
        gtk_text_buffer_set_text(pd->buffer, 
                pd->screens_array[pd->active_index]->text, -1);

        if (pd->active_index == 0){
            widget = gtk_dialog_get_widget_for_response(GTK_DIALOG(pd->dialog), 
                    BACK_RESPONSE_ID); 

            gtk_widget_set_sensitive(widget, FALSE);
        }else{
            widget = gtk_dialog_get_widget_for_response(GTK_DIALOG(pd->dialog), 
                    BACK_RESPONSE_ID); 
            gtk_widget_set_sensitive(widget, TRUE);

        }

        if (pd->active_index == (pd->size - 1)){
            widget = gtk_dialog_get_widget_for_response(GTK_DIALOG(pd->dialog), 
                    NEXT_RESPONSE_ID); 

            gtk_button_set_label(GTK_BUTTON(widget), "Finish");
        }else{
            widget = gtk_dialog_get_widget_for_response(GTK_DIALOG(pd->dialog), 
                    NEXT_RESPONSE_ID); 
            gtk_button_set_label(GTK_BUTTON(widget), "Next");
        }
    }

    return;
}

WG_PRIVATE wg_boolean
call_action(Gui_progress_dialog *pd, Gui_progress_action action_id)
{
    wg_boolean flag = WG_TRUE;
    action_cb action = NULL;
    Gui_progress_dialog_screen *pds = NULL;

    if (!is_last_screen(pd)){
        pds = pd->screens_array[pd->active_index];
        action = pds->action;
        if (action != NULL){
            flag = action(pds, action_id, pds->user_data);
        }
    }

    return flag;
}

WG_PRIVATE void
call_exit_action(Gui_progress_dialog *pd)
{
    exit_action_cb action = NULL;
    wg_uint i = 0;

    action = pd->exit_action;
    if (action != NULL){
        for (i = 0; i < pd->size; ++i){
            action(i, pd->screens_array[i]->user_data);
        }
    }

    return;
}

WG_PRIVATE void
show_extra_widget(Gui_progress_dialog *pd)
{
    Gui_progress_dialog_screen *pds = NULL;

    pds = pd->screens_array[pd->active_index];
    if (!is_last_screen(pd)){
        if (NULL != pds->extra_widget){
            gtk_box_pack_end(GTK_BOX(pd->box), 
                    pds->extra_widget, FALSE, TRUE, 0);
        }
    }

    return;
}

WG_PRIVATE void
hide_extra_widget(Gui_progress_dialog *pd)
{
    Gui_progress_dialog_screen *pds = NULL;

    pds = pd->screens_array[pd->active_index];

    if (!is_last_screen(pd)){
        if (NULL != pds->extra_widget){
            gtk_container_remove(GTK_CONTAINER(pd->box), pds->extra_widget);
        }
    }

    return;
}

WG_PRIVATE void
response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    Gui_progress_dialog *pd = NULL;
    wg_boolean flag = WG_TRUE;

    pd = (Gui_progress_dialog*)user_data;

    switch (response_id){
    case NEXT_RESPONSE_ID:
        flag = call_action(pd, GUI_PROGRESS_NEXT);

        if (flag == WG_TRUE){
            call_action(pd, GUI_PROGRESS_LEAVE);
            hide_extra_widget(pd);
            show_screen(pd, NEXT_SCREEN);   
            show_extra_widget(pd);
            call_action(pd, GUI_PROGRESS_ENTER);
        }
        break;
    case BACK_RESPONSE_ID:
        flag = call_action(pd, GUI_PROGRESS_BACK);

        if (flag == WG_TRUE){
            call_action(pd, GUI_PROGRESS_LEAVE);
            hide_extra_widget(pd);
            show_screen(pd, PREVIUOS_SCREEN);   
            show_extra_widget(pd);
            call_action(pd, GUI_PROGRESS_ENTER);
        }
        break;
    }

    if ((is_last_screen(pd) == WG_TRUE) ||
            (response_id == GTK_RESPONSE_DELETE_EVENT)){
        call_action(pd, GUI_PROGRESS_LEAVE);
        hide_extra_widget(pd);
        call_exit_action(pd);
        gtk_widget_destroy(pd->dialog);
        gui_progress_dialog_cleanup(pd);
    }

    return;
}

/*! @} */
