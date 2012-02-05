#include <stdlib.h>
#include <string.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include "include/gui_resolution.h"

typedef struct Resolution{
    wg_char text[16];
    wg_uint  width;
    wg_uint  height;
    wg_char  *id;
}Resolution;

#define RESOLUTION_DEFAULT_INDEX    1

static const Resolution res_info[] = {
    {"640x480", 640, 480, "1"} ,
    {"352x288", 352, 288, "2"} ,
    {"320x240", 320, 240, "3"} ,
    {"176x144", 176, 144, "4"} ,
    {"160x120", 160, 120, "5"}
};

static void
gui_resolution_class_init(Gui_resolution_class *wclass)
{
}

static void
gui_resolution_init(Gui_resolution *obj)
{
    gint i = 0;

    for (i = 0; i < ELEMNUM(res_info); ++i){
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(obj),
			res_info[i].text);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(obj), RESOLUTION_DEFAULT_INDEX);

    return;
}


guint
gui_resolution_get_type()
{
    static guint obj_type = 0;
    static GTypeInfo obj_info = {
        sizeof (Gui_resolution_class),
        NULL,
        NULL,
        (GClassInitFunc) gui_resolution_class_init,
        NULL,
        NULL,
        sizeof (Gui_resolution),
        0,
        (GInstanceInitFunc) gui_resolution_init
    };

    if (obj_type == 0){
        obj_type = g_type_register_static(GTK_TYPE_COMBO_BOX_TEXT, 
            "Gui_resolution", &obj_info, 0);
    }

    return obj_type;
}


GtkWidget*
gui_resolution_new(void)
{
    return GTK_WIDGET(g_object_new(gui_resolution_get_type(), NULL));
}

wg_status
gui_resolution_get(Gui_resolution *obj, wg_uint *width, wg_uint *height)
{
    gint res_index = 0;

    res_index = gtk_combo_box_get_active(GTK_COMBO_BOX(obj));

    CHECK_FOR_RANGE_GE(res_index, ELEMNUM(res_info));
    
    *width  = res_info[res_index].width;
    *height = res_info[res_index].height;

    return WG_SUCCESS;
}

