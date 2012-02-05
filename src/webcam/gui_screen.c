#include <stdlib.h>
#include <string.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include "include/gui_screen.h"

static void
gui_screen_class_init(Gui_screen_class *wclass)
{
}

static void
gui_screen_init(Gui_screen *obj)
{
    return;
}


guint
gui_screen_get_type()
{
    static guint obj_type = 0;
    static GTypeInfo obj_info = {
        sizeof (Gui_screen_class),
        NULL,
        NULL,
        (GClassInitFunc) gui_screen_class_init,
        NULL,
        NULL,
        sizeof (Gui_screen),
        0,
        (GInstanceInitFunc) gui_screen_init
    };

    if (obj_type == 0){
        obj_type = g_type_register_static(GTK_TYPE_COMBO_BOX_TEXT, 
            "gui_screen", &obj_info, 0);
    }

    return obj_type;
}


GtkWidget*
gui_screen_new(void)
{
    return GTK_WIDGET(g_object_new(gui_screen_get_type(), NULL));
}

