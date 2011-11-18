#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>

#include <linux/videodev2.h>

#include "include/wg_cam.h"
#include "include/wg_cam_cap.h"
#include "include/wg_cam_image.h"
#include "include/wg_cam_readwrite.h"
#include "include/wg_cam_img_jpeg.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct Camera{
    GdkPixbuf *pixbuf;
    GtkWidget *area;
    GtkWidget *button_start;
    GtkWidget *button_stop;
    pthread_mutex_t mutex;
    pthread_t thread;
    Wg_camera *camera;
}Camera;

static gboolean
on_expose_event(GtkWidget *widget,
        GdkEventExpose *event,
        gpointer data)
{
    Camera *cam = NULL;
    cairo_t *cr;

    cam = (Camera*)data;
    cr = gdk_cairo_create(widget->window);

    pthread_mutex_lock(&cam->mutex);

    if (cam->pixbuf != NULL){
        gdk_cairo_set_source_pixbuf(cr,
                cam->pixbuf, 
                0.0, 0.0);
    }

    cairo_paint(cr);

    pthread_mutex_unlock(&cam->mutex);

    cairo_destroy(cr);

    return TRUE;
}

void
xbuf_free(guchar *pixels, gpointer data)
{
    WG_FREE(data);
}

void
cancel_handler(void *data)
{
    wg_cam_frame_buffer_release((Wg_frame*)data);

    WG_FREE(data);
}

void*
capture(void *data)
{
    Wg_frame *frame = NULL;
    Wg_camera *camera = NULL;
    wg_uchar *out_buffer = NULL;
    wg_ssize unpacked_size = 0;
    wg_uint width = 0;
    wg_uint height = 0;
    Camera  *cam = NULL;
    int old_state = 0;
    int old_type = 0;

    cam = (Camera*)data;

    camera = cam->camera;

    frame = WG_CALLOC(1, sizeof (Wg_frame));

    frame->start = NULL;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);
    pthread_cleanup_push(cancel_handler, frame);

    for(;;){
        if (wg_cam_frame_read(camera, frame) == WG_CAM_SUCCESS){
            if (WG_CAM_SUCCESS == wg_cam_img_jpeg_decompress(
                        frame->start, frame->size, &out_buffer, 
                        &unpacked_size, &width, &height)){

                pthread_mutex_lock(&cam->mutex);

                if (cam->pixbuf != NULL){
                    g_object_unref(cam->pixbuf);
                    cam->pixbuf = NULL;
                }

                cam->pixbuf = gdk_pixbuf_new_from_data(out_buffer, 
                        GDK_COLORSPACE_RGB, FALSE, 8, width, height, 
                        width * 3, 
                        xbuf_free, out_buffer);

                pthread_mutex_unlock(&cam->mutex);

                gdk_threads_enter(); 

                gtk_widget_queue_draw(cam->area);

                gdk_threads_leave();
            }
        }
    }

    wg_cam_close(camera);

    wg_cam_frame_buffer_release(frame);

    WG_FREE(frame);

    pthread_cleanup_pop(0);

    return data;
}



void button_clicked_stop
(GtkWidget *widget, gpointer data){
    Camera    *cam = NULL;
    void *retval = NULL;

    cam = (Camera*)data;

    pthread_cancel(cam->thread);

    pthread_join(cam->thread, &retval);

    wg_cam_close(cam->camera);
    WG_FREE(cam->camera);
    cam->camera = NULL;

    if (cam->pixbuf != NULL){
        g_object_unref(cam->pixbuf);
        cam->pixbuf = NULL;
    }

    pthread_mutex_destroy(&cam->mutex);

    gtk_widget_set_sensitive(widget, FALSE);
    gtk_widget_set_sensitive(cam->button_start, TRUE);
}

void button_clicked_start
(GtkWidget *widget, gpointer data){
    Wg_camera *camera;
    Camera    *cam = NULL;


    cam = (Camera*)data;

    camera = malloc(sizeof (Wg_camera));
    if (NULL != camera){
        cam->camera = camera;

        wg_cam_init(cam->camera, "/dev/video0");
        wg_cam_open(cam->camera);

        pthread_mutex_init(&cam->mutex, NULL);
        pthread_create(&cam->thread, NULL, capture, cam);
    }

    gtk_widget_set_sensitive(widget, FALSE);
    gtk_widget_set_sensitive(cam->button_stop, TRUE);
}


int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *button_start = NULL;
    GtkWidget *button_stop = NULL;
    GtkWidget *hbox = NULL;
    GtkWidget *area = NULL;
    Camera    *camera = NULL;

    g_thread_init(NULL);
    gdk_threads_init();
    gdk_threads_enter();

    gtk_init(&argc, &argv);

    camera = WG_CALLOC(1, sizeof (Camera));

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    area   = gtk_drawing_area_new();
    button_start = gtk_button_new_with_label("Start");
    button_stop  = gtk_button_new_with_label("Stop");

    camera->area = area;
    camera->button_stop  = button_stop;
    camera->button_start = button_start; 

    gtk_widget_set_sensitive(button_start, TRUE);
    gtk_widget_set_sensitive(button_stop, FALSE);

    g_signal_connect(area, "expose-event",
            G_CALLBACK (on_expose_event), camera);
    g_signal_connect(window, "destroy",
            G_CALLBACK (gtk_main_quit), camera);
    g_signal_connect(button_start, "clicked",
            G_CALLBACK (button_clicked_start), camera);
    g_signal_connect(button_stop, "clicked",
            G_CALLBACK (button_clicked_stop), camera);

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 550); 

    hbox = gtk_vbox_new (FALSE, 3);

    gtk_box_pack_start(GTK_BOX(hbox), area, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button_start, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button_stop, FALSE, TRUE, 0);

    gtk_container_add (GTK_CONTAINER(window), hbox);

    gtk_widget_show_all(window);

    gtk_main();

    gdk_threads_leave();

    return 0;
}
