#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_cap.h"
#include "include/cam_frame.h"
#include "include/cam_output.h"
#include "include/cam_readwrite.h"
#include "include/cam_img_jpeg.h"
#include "include/cam_format_selector.h"

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
camera_is_active(Camera *cam)
{
    return cam->camera != NULL ? TRUE : FALSE;
}

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
    cam_img_cleanup((Wg_image*)data);

    WG_FREE(data);
}

void*
capture(void *data)
{
    Wg_frame *frame = NULL;
    Wg_image *image = NULL;
    Wg_camera *camera = NULL;
    Camera  *cam = NULL;
    int old_state = 0;
    int old_type = 0;
    cam_status status;
    Wg_cam_decompressor decompressor;

    cam = (Camera*)data;

    camera = cam->camera;

    frame = WG_CALLOC(1, sizeof (Wg_frame));

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type);
//    pthread_cleanup_push(camera->cam_ops.cleanup_frame, frame);

//    cam_select_user_decompressor(camera, v4l2_fourcc('M', 'J', 'P', 'G'));

    cam_decompressor(camera, &decompressor);
    cam_start(camera);

    for(;;){
        if (cam_read(camera, frame) == CAM_SUCCESS){
            image = WG_MALLOC(sizeof (Wg_image));

            status = decompressor.run(frame->start, frame->size, 
                    frame->width, frame->height, image);
            if(CAM_SUCCESS == status){
                cam_discard_frame(camera, frame);

                pthread_mutex_lock(&cam->mutex);

                if (cam->pixbuf != NULL){
                    g_object_unref(cam->pixbuf);
                    cam->pixbuf = NULL;
                }

                cam->pixbuf = gdk_pixbuf_new_from_data(image->image, 
                        GDK_COLORSPACE_RGB, FALSE, 8, 
                        image->width, image->height, 
                        image->row_distance, 
                        xbuf_free, image);

                pthread_mutex_unlock(&cam->mutex);

                gdk_threads_enter(); 

                gtk_widget_queue_draw(cam->area);

                gdk_threads_leave();
            }else{
                cam_discard_frame(camera, frame);

                WG_FREE(image);    
            }
        }else{
            pthread_exit(NULL);
        }
    }

    cam_stop(camera);

    cam_close(camera);

    cam_free_frame(camera, frame);

    WG_FREE(frame);

    //pthread_cleanup_pop(0);

    return data;
}


    void
stop_capture(Camera *cam)
{
    void *retval = NULL;

    if (camera_is_active(cam)){
        pthread_mutex_lock(&cam->mutex);

        pthread_cancel(cam->thread);

        pthread_join(cam->thread, &retval);

        cam_stop(cam->camera);

        cam_close(cam->camera);
        WG_FREE(cam->camera);
        cam->camera = NULL;

        if (cam->pixbuf != NULL){
            g_object_unref(cam->pixbuf);
            cam->pixbuf = NULL;
        }

        pthread_mutex_unlock(&cam->mutex);

        pthread_mutex_destroy(&cam->mutex);
    }

    return;
}

    gint 
delete_event( GtkWidget *widget, GdkEvent  *event, gpointer   data )
{
    Camera *cam = NULL;

    cam = (Camera*)data;

    stop_capture(cam);

    return FALSE;
}

void button_clicked_stop
(GtkWidget *widget, gpointer data){
    Camera    *cam = NULL;

    cam = (Camera*)data;

    stop_capture(cam);

    gtk_widget_set_sensitive(widget, FALSE);
    gtk_widget_set_sensitive(cam->button_start, TRUE);
}

void button_clicked_start
(GtkWidget *widget, gpointer data){
    Wg_camera *camera;
    Camera    *cam = NULL;
    pthread_attr_t attr;
    cam_status status = CAM_FAILURE;


    cam = (Camera*)data;

    camera = malloc(sizeof (Wg_camera));
    if (NULL != camera){
        cam->camera = camera;

        cam_init(cam->camera, "/dev/video0");
        status = cam_open(cam->camera, 0);
        if (CAM_SUCCESS == status){
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            pthread_mutex_init(&cam->mutex, NULL);
            pthread_create(&cam->thread, &attr, capture, cam);
            pthread_attr_destroy(&attr);

            gtk_widget_set_sensitive(widget, FALSE);
            gtk_widget_set_sensitive(cam->button_stop, TRUE);
        }else{
            WG_FREE(cam->camera);
            cam->camera = NULL;
        }
    }

    return;
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

    gtk_signal_connect(GTK_OBJECT(area), "expose-event",
            G_CALLBACK (on_expose_event), camera);
    gtk_signal_connect(GTK_OBJECT(window), "destroy",
            G_CALLBACK (gtk_main_quit), camera);
    gtk_signal_connect(GTK_OBJECT(button_start), "clicked",
            G_CALLBACK (button_clicked_start), camera);
    gtk_signal_connect(GTK_OBJECT(button_stop), "clicked",
            G_CALLBACK (button_clicked_stop), camera);
    gtk_signal_connect (GTK_OBJECT (window), "delete_event",
            GTK_SIGNAL_FUNC (delete_event), camera);

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
