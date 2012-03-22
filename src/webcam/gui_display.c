#include <stdlib.h>
#include <stdio.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_sync_linked_list.h>
#include <wg_wq.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/img.h"
#include "include/gui_prim.h"
#include "include/sensor.h"
#include "include/vid.h"

#include "include/collision_detect.h"
#include "include/wg_plugin.h"

#include "include/gui_display.h"

WG_PRIVATE gboolean
on_expose(GtkWidget *widget, cairo_t *cr, gpointer data);

wg_status
gui_display_init(GtkWidget *widget, Gui_display *display)
{
    CHECK_FOR_NULL_PARAM(widget);
    CHECK_FOR_NULL_PARAM(display);

    memset(display, '\0', sizeof (Gui_display));

    display->widget = widget;

    display->widget_width = gtk_widget_get_allocated_width(display->widget);
    display->widget_height = gtk_widget_get_allocated_height(display->widget);

    g_signal_connect(display->widget, "draw",
            G_CALLBACK(on_expose), display);

    return WG_SUCCESS;
}

void
gui_display_cleanup(Gui_display *display)
{
    return;
}

wg_status
gui_display_set_pixbuf(Gui_display *display, wg_uint x, wg_uint y, 
        GdkPixbuf *pixbuf)
{
    CHECK_FOR_NULL_PARAM(display);
    CHECK_FOR_NULL_PARAM(pixbuf);

    g_object_ref(pixbuf);

    
    gdk_threads_enter();

    if (display->pixbuf != NULL){
        g_object_unref(display->pixbuf);
    }
 
    display->pixbuf = pixbuf;
    display->x = x;
    display->y = y;

    gdk_threads_leave();

    return WG_SUCCESS;
}

WG_PRIVATE gboolean
on_expose(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    Gui_display *display = NULL;

    display = (Gui_display*)data;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    if (display->pixbuf != NULL){
        gdk_cairo_set_source_pixbuf(cr,
                display->pixbuf, 
                display->widget_width, display->widget_height);

        cairo_paint(cr);
    }

    return TRUE;
}

