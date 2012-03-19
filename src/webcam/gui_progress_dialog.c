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


#define BACK_RESPONSE_ID 0
#define NEXT_RESPONSE_ID 1

struct Gui_progress_dialog_screen{
    action_cb next;
    action_cb back;
    void *user_data;
    wg_char *text;
    List_head list;
};

struct Gui_progress_dialog{
    List_head screens;
    wg_uint active_index;
    wg_uint size;
    Gui_progress_dialog_screen **screens_array;
    GtkTextBuffer *buffer;
    GtkWidget *dialog;
};

WG_STATIC void
response(GtkDialog *dialog, gint response_id, gpointer user_data);

WG_STATIC void
show_screen(Gui_progress_dialog *pd, wg_int val);

Gui_progress_dialog *
gui_progress_dialog_new()
{
    Gui_progress_dialog *pd = NULL;

    pd = WG_CALLOC(1, sizeof (Gui_progress_dialog));
    if (NULL == pd){
        return NULL;
    }

    list_init(&pd->screens);
    pd->active_index = 0;
    pd->screens_array = NULL;
    pd->size = 0;

    return pd;
}

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

Gui_progress_dialog_screen *
gui_progress_dialog_screen_new(action_cb next, action_cb prev, void *user_data,
        const wg_char *text)
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

    pds->next = next;
    pds->back = prev;
    pds->user_data = user_data;

    list_init(&pds->list);

    return pds;
}

void
gui_progress_dialog_screen_cleanup(Gui_progress_dialog_screen *pds)
{
    WG_FREE(pds);

    return;
}

void
gui_progress_dialog_add_screen(Gui_progress_dialog *pd,
        Gui_progress_dialog_screen *pds)
{
    list_add(&pd->screens, &pds->list);

    return;
}

void
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

    builder = gtk_builder_new ();
    gtk_builder_add_from_file (
            builder, "progress_dialog.xml", NULL);
    dialog = GTK_WIDGET (gtk_builder_get_object (builder, "progress_dialog"));
    gtk_builder_connect_signals (builder, NULL);

    pd->dialog = dialog;

    widget = GTK_WIDGET (gtk_builder_get_object (builder, "text_view"));
    pd->buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));

    /* release builder */
    g_object_unref(G_OBJECT (builder));

    g_signal_connect(GTK_DIALOG(dialog), "response", 
      G_CALLBACK(response), pd);

    size = list_size(&pd->screens);
    pds_array = WG_CALLOC(size, sizeof (Gui_progress_dialog_screen*));
    if (NULL == pds_array){
        return;
    }

    pd->size = size;
    pd->screens_array = pds_array;

    iterator_list_init(&itr, &pd->screens,
            GET_OFFSET(Gui_progress_dialog_screen, list));

    for (i = 0; (pds = iterator_list_next(&itr)) != NULL; ++i){
        pd->screens_array[i] = pds; 
    }

    show_screen(pd, 0);

    gtk_widget_show_all(dialog);

    return;
}

WG_STATIC void
show_screen(Gui_progress_dialog *pd, wg_int val)
{
    GtkWidget *widget = NULL;

    pd->active_index += val;
    if (pd->active_index > pd->size){
        pd->active_index -= val;
    }

    if (pd->active_index < pd->size){
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
    }else{
        gtk_widget_destroy(GTK_WIDGET(pd->dialog));
        gui_progress_dialog_cleanup(pd);
    }

    return;
}

WG_STATIC void
response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    Gui_progress_dialog *pd = NULL;
    Gui_progress_dialog_screen *pds = NULL;
    action_cb action = NULL;
    wg_boolean flag = WG_TRUE;

    pd = (Gui_progress_dialog*)user_data;

    switch (response_id){
    case NEXT_RESPONSE_ID:
        pds = pd->screens_array[pd->active_index];
        action =pds->next;
        if (action != NULL){
            flag = action(pds->user_data);
        }

        if (flag == WG_TRUE){
            show_screen(pd, 1);   
        }
        break;
    case BACK_RESPONSE_ID:
        pds = pd->screens_array[pd->active_index];
        action =pds->back;
        if (action != NULL){
            flag = action(pds->user_data);
        }

        if (flag == WG_TRUE){
            show_screen(pd, -1);   
        }
        break;
    case GTK_RESPONSE_DELETE_EVENT:
        gtk_widget_destroy(pd->dialog);
        gui_progress_dialog_cleanup(pd);
        break;
    }

    return;
}
