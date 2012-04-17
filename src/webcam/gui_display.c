#include <stdlib.h>
#include <stdio.h>

/* @todo create user space include */
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/types.h>
#include <unistd.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <wg_sync_linked_list.h>
#include <wg_trans.h>
#include <wg_plugin_tools.h>
#include <wg_wq.h>
#include <img.h>
#include <cam.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "include/gui_prim.h"
#include "include/sensor.h"
#include "include/vid.h"
#include "include/gui_work.h"

#include "include/collision_detect.h"
#include "include/wg_plugin.h"

#include "include/gui_display.h"

/*! \defgroup gui_display Widget to Display Custom Images
 *  \ingroup plugin_webcam
 */

/*! @{ */

/** 
* @brief Describe a line displayed by gui_display
*/
typedef struct Gui_display_line{
    List_head list;    /*!< list of lines         */
    Wg_point2d p1;     /*!< end of line           */
    Wg_point2d p2;     /*!< end of line           */
    double r;          /*!< red color component   */
    double g;          /*!< green color component */
    double b;          /*!< blue color component  */
    double size;       /*!< size of line          */
}Gui_display_line;

WG_PRIVATE gboolean
on_expose(GtkWidget *widget, cairo_t *cr, gpointer data);

WG_PRIVATE void
refresh(Gui_display *display);

WG_PRIVATE void
update_image_cb(void *data);

WG_PRIVATE gboolean
pressed_mouse(GtkWidget *widget, GdkEvent  *event, gpointer user_data);

WG_PRIVATE gboolean
released_mouse(GtkWidget *widget, GdkEvent  *event, gpointer user_data);

/** 
* @brief Init display
* 
* @param widget   widget to assiociate with display
* @param display  instance of gui_display to initialize
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gui_display_init(GtkWidget *widget, Gui_display *display)
{
    CHECK_FOR_NULL_PARAM(widget);
    CHECK_FOR_NULL_PARAM(display);

    memset(display, '\0', sizeof (Gui_display));

    display->widget = widget;

    list_init(&display->lines);

    display->widget_width = gtk_widget_get_allocated_width(display->widget);
    display->widget_height = gtk_widget_get_allocated_height(display->widget);
    gtk_widget_set_app_paintable(display->widget, TRUE);
    gtk_widget_set_double_buffered(display->widget, TRUE);

    g_signal_connect(display->widget, "draw",
            G_CALLBACK(on_expose), display);

    display->enable_dragging = 0;
    display->is_dragging     = 0;

    return WG_SUCCESS;
}

/** 
* @brief Release gui_display allocated resources
* 
* @param display gui_display instance
*/
void
gui_display_cleanup(Gui_display *display)
{
    gui_display_clean_lines(display);

    return;
}

wg_status
gui_display_set_drag_callback(Gui_display *display, drag_cb cb, 
        void *user_data)
{
    CHECK_FOR_NULL_PARAM(display);

    display->drag_callback = cb;
    display->user_data     = user_data;

    return WG_SUCCESS;
}

wg_status
gui_display_enable_dragging(Gui_display *display)
{
    CHECK_FOR_NULL_PARAM(display);

    display->enable_dragging = 1;

    g_signal_connect(display->widget, "button-press-event", 
            G_CALLBACK(pressed_mouse), display);

    g_signal_connect(display->widget, "button-release-event", 
            G_CALLBACK(released_mouse), display);

    return WG_SUCCESS;
}

wg_status
gui_display_disable_dragging(Gui_display *display)
{
    CHECK_FOR_NULL_PARAM(display);

    g_signal_handlers_disconnect_by_func(display->widget,
            G_CALLBACK(pressed_mouse), display);

    g_signal_handlers_disconnect_by_func(display->widget,
            G_CALLBACK(released_mouse), display);

    display->enable_dragging = 0;

    return WG_SUCCESS;
}

/** 
* @brief Get widget associated with gui_display
* 
* @param display gui_display instance
* @param widget  memory to store widget
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gui_display_get_widget(Gui_display *display, GtkWidget **widget)
{
    CHECK_FOR_NULL_PARAM(display);
    CHECK_FOR_NULL_PARAM(widget);

    *widget = display->widget; 

    return WG_SUCCESS;
}


/** 
* @brief Get dimention
* 
* @param display gui_display instance
* @param width   memory to store width
* @param height  memory to store height
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gui_display_get_size(Gui_display *display, wg_uint *width, wg_uint *height)
{
    CHECK_FOR_NULL_PARAM(display);
    CHECK_FOR_NULL_PARAM(width);
    CHECK_FOR_NULL_PARAM(height);

    *width  = gtk_widget_get_allocated_width(display->widget);
    *height = gtk_widget_get_allocated_height(display->widget);

    return WG_SUCCESS;
}

/** 
* @brief Clean all lines
* 
* @param display gui_display instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gui_display_clean_lines(Gui_display *display)
{
    Iterator itr;
    Gui_display_line *line = NULL;

    iterator_list_init(&itr, &display->lines, 
            GET_OFFSET(Gui_display_line, list));

    while((line = iterator_list_next(&itr)) != NULL){
        WG_FREE(line);
    }

    list_init(&display->lines);

    return WG_SUCCESS;
}

/** 
* @brief Draw line
* 
* @param display gui_display instance
* @param p1      end of line
* @param p2      end of line
* @param r       red color component
* @param g       green color component
* @param b       blue color component
* @param size    width of the line
* 
* @retval
* @todo return line instance for futher reference
*/
wg_status
gui_display_draw_line(Gui_display *display, const Wg_point2d *p1, 
const Wg_point2d *p2, double r, double g, double b, double size)
{
    Gui_display_line *line = NULL;

    CHECK_FOR_NULL_PARAM(display);
    CHECK_FOR_NULL_PARAM(p1);
    CHECK_FOR_NULL_PARAM(p2);

    line = WG_MALLOC(sizeof (Gui_display_line));
    if (NULL == line){
        return WG_FAILURE;
    }

    line->p1 = *p1;
    line->p2 = *p2;
    line->r = r;
    line->g = g;
    line->b = b;
    line->size = size;

    list_add(&display->lines, &line->list);
    refresh(display);

    return WG_SUCCESS;
}

/** 
* @brief Copy part of display
* 
* @param display gui_diaplay instance
* @param rect    rectangle to copy
* @param img
* 
* @return 
*/
wg_status
gui_display_copy(Gui_display *display, Wg_rect *rect, Wg_image *img)
{
    wg_uchar *buffer = NULL;
    Wg_image image;
    Wg_image rect_image;
    wg_uint width  = 0;
    wg_uint height = 0;
    cam_status status = CAM_FAILURE;

    gdk_threads_enter();

    width  = gdk_pixbuf_get_width(display->pixbuf);
    height = gdk_pixbuf_get_height(display->pixbuf);
    buffer = gdk_pixbuf_get_pixels(display->pixbuf); 

    status = img_rgb_from_buffer(buffer, width, height, &image);
    if (CAM_FAILURE == status){
        return WG_FAILURE;
    }

    gdk_threads_leave();

    status = img_fill(rect->width, rect->height,
            RGB24_COMPONENT_NUM, IMG_RGB, &rect_image);
    if (CAM_FAILURE == status){
        img_cleanup(&image);
        return WG_FAILURE;
    }

    status = img_get_subimage(&image, rect->x, rect->y, &rect_image);
    if (CAM_FAILURE == status){
        img_cleanup(&image);
        img_cleanup(&rect_image);
        return WG_FAILURE;
    }

    status = img_rgb_2_hsv_gtk(&rect_image, img);
    if (CAM_FAILURE == status){
        img_cleanup(&image);
        img_cleanup(&rect_image);
        return WG_FAILURE;
    }

    img_cleanup(&image);
    img_cleanup(&rect_image);

    return WG_SUCCESS;
}

/** 
* @brief Set background pixbuf
* 
* @param display gui_display imnstance
* @param x      horizontal offset
* @param y      vertical offset
* @param pixbuf pixbuf instance
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
gui_display_set_pixbuf(Gui_display *display, wg_uint x, wg_uint y, 
        GdkPixbuf *pixbuf){
    Update_image *work = NULL;

    CHECK_FOR_NULL_PARAM(display);
    CHECK_FOR_NULL_PARAM(pixbuf);

    g_object_ref(pixbuf);

    work = gui_work_create(sizeof (Update_image), 
            update_image_cb);

    work->pixbuf  = pixbuf;
    work->display = display;
    work->x = x;
    work->y = y;

    gui_work_add(work);

    return WG_SUCCESS;
}

WG_PRIVATE void
refresh_image_cb(void *data)
{
    Update_image *work = NULL;
    GtkWidget *widget = NULL;

    work = (Update_image*)data;

    gui_display_get_widget(work->display, &widget);

    gtk_widget_queue_draw(widget);

    return;
}

WG_PRIVATE void
refresh(Gui_display *display)
{
    Update_image *work = NULL;

    work = gui_work_create(sizeof (Update_image), 
            refresh_image_cb);

    work->display = display;

    gui_work_add(work);

    return;
}


WG_PRIVATE wg_status
set_pixbuf(Gui_display *display, wg_uint x, wg_uint y, 
        GdkPixbuf *pixbuf)
{
    gdk_threads_enter();

    g_object_ref(pixbuf);

    if (display->pixbuf != NULL){
        g_object_unref(display->pixbuf);
    }
 
    display->pixbuf = pixbuf;
    display->x = x;
    display->y = y;

    gtk_widget_queue_draw(display->widget);

    gdk_threads_leave();

    return WG_SUCCESS;
}

WG_PRIVATE void
paint_line(cairo_t *cr, const Gui_display_line *line)
{
    cairo_set_source_rgb(cr, line->r, line->g, line->b);
    cairo_move_to(cr, (double)line->p1.x, (double)line->p1.y);
    cairo_line_to(cr, (double)line->p2.x, (double)line->p2.y);
    cairo_set_line_width (cr, line->size);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_stroke(cr);

    return;
}

WG_PRIVATE gboolean
on_expose(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    Gui_display *display = NULL;
    Gui_display_line *line = NULL;
    Iterator itr;

    display = (Gui_display*)data;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    if (display->pixbuf != NULL){
        gdk_cairo_set_source_pixbuf(cr,
                display->pixbuf, 
                display->widget_width, display->widget_height);

        cairo_paint(cr);

        iterator_list_init(&itr, &display->lines, 
                GET_OFFSET(Gui_display_line, list));
        while((line = iterator_list_next(&itr)) != NULL){
            paint_line(cr, line);
        }

        cairo_fill(cr);

    }

    return TRUE;
}

WG_PRIVATE void
update_image_cb(void *data)
{
    Update_image *work = NULL;

    work = (Update_image*)data;

    set_pixbuf(work->display, work->x, work->y, work->pixbuf); 

    g_object_unref(work->pixbuf);

    return;
}

WG_PRIVATE gboolean
pressed_mouse(GtkWidget *widget, GdkEvent  *event, gpointer user_data) 
{
    Gui_display *display = NULL;
    GdkEventButton *e = NULL;

    display = user_data;
    e = &event->button;

    if (display->is_dragging == 0){
        display->is_dragging = 1;
        
        wg_point2d_new((int)e->x, (int)e->y, &display->drag_pt_1);
    }

    return FALSE;
}

WG_PRIVATE gboolean
released_mouse(GtkWidget *widget, GdkEvent  *event, gpointer user_data) 
{
    Gui_display *display = NULL;
    GdkEventButton *e = NULL;
    Wg_rect rect;

    display = user_data;
    e = &event->button;

    if (display->is_dragging == 1){
        display->is_dragging = 0;

        wg_point2d_new((int)e->x, (int)e->y, &display->drag_pt_2);

        wg_rect_new_from_point2d(&display->drag_pt_1, &display->drag_pt_2,
                &rect);

        if (display->drag_callback != NULL){
            display->drag_callback(display, &rect, display->user_data);
        }
    }

    return FALSE;
}

/*! @} */
