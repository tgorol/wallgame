#ifndef _Wgui_screen_H
#define _Wgui_screen_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>

#define gui_screen(obj)    \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), gui_screen_get_type (), Gui_screen))

#define gui_screen_CLASS(klass)                                     \
    G_TYPE_CHECK_CLASS_CAST((klass), gui_screen_get_type (), \
            Gui_screen_class)

#define IS_gui_screen(obj)      \
    G_TYPE_CHECK_INSTANCE_TYPE((obj), gui_screen_get_type ())

typedef struct Gui_screen{
    GtkDrawingArea    area;
}Gui_screen;

typedef struct Gui_screen_class{
    GtkDrawingAreaClass parent_class;
}Gui_screen_class;

WG_PUBLIC guint        gui_screen_get_type(void);
WG_PUBLIC GtkWidget*   gui_screen_new(void);
WG_PUBLIC void         gui_screen_clear(Gui_screen *obj);

#endif
