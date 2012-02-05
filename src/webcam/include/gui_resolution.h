#ifndef _WGUI_RESOLUTION_H
#define _WGUI_RESOLUTION_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define GUI_RESOLUTION(obj)    \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), gui_resolution_get_type (), Gui_resolution))

#define GUI_RESOLUTION_CLASS(klass)                                     \
    G_TYPE_CHECK_CLASS_CAST((klass), gui_resolution_get_type (), \
            Gui_resolution_class)

#define IS_GUI_RESOLUTION(obj)      \
    G_TYPE_CHECK_INSTANCE_TYPE((obj), gui_resolution_get_type ())

typedef struct Gui_resolution{
    GtkComboBoxText    hbox;
}Gui_resolution;

typedef struct Gui_resolution_class{
    GtkComboBoxTextClass parent_class;
}Gui_resolution_class;

WG_PUBLIC guint        gui_resolution_get_type(void);
WG_PUBLIC GtkWidget*   gui_resolution_new(void);
WG_PUBLIC void         gui_resolution_clear(Gui_resolution *obj);

WG_PUBLIC wg_status gui_resolution_get(Gui_resolution *obj, 
        wg_uint *width, wg_uint *height);

#endif
