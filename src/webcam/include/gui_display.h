#ifndef _GUI_DISPLAY_H
#define _GUI_DISPLAY_H

typedef struct Gui_display{
    GtkWidget *widget;
    GdkPixbuf *pixbuf;
    wg_uint widget_width;
    wg_uint widget_height;
    wg_uint x;
    wg_uint y;
    List_head lines;
}Gui_display;

WG_PUBLIC wg_status
gui_display_init(GtkWidget *widget, Gui_display *display);

WG_PUBLIC void
gui_display_cleanup(Gui_display *display);

WG_PUBLIC wg_status
gui_display_set_pixbuf(Gui_display *display, wg_uint x, wg_uint y, 
        GdkPixbuf *pixbuf);

WG_PUBLIC wg_status
gui_display_get_size(Gui_display *display, wg_uint *width, wg_uint *height);

WG_PUBLIC wg_status
gui_display_get_widget(Gui_display *display, GtkWidget **widget);

WG_PUBLIC wg_status
gui_display_copy(Gui_display *display, Wg_rect *rect, Wg_image *img);

WG_PUBLIC wg_status
gui_display_draw_line(Gui_display *display, const Wg_point2d *p1, 
const Wg_point2d *p2, double r, double g, double b);

WG_PUBLIC wg_status
gui_display_clean_lines(Gui_display *display);

#endif


