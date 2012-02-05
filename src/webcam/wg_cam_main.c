#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include <cairo.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_lsdir.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_cap.h"
#include "include/cam_frame.h"
#include "include/cam_output.h"
#include "include/cam_readwrite.h"
#include "include/cam_img_jpeg.h"
#include "include/cam_format_selector.h"
#include "include/cam_img.h"

#include "include/gui_resolution.h"
#include "include/gui_camera.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

typedef struct Camera{
    GtkWidget *window;
    GdkPixbuf *pixbuf;
    GtkWidget *area;
    GtkWidget *gui_camera;
    GtkWidget *show_gray;
    GtkWidget *normalize;
    GThread   *thread;
    Wg_camera *camera;
    gint fps;
    gint frame_count;
}Camera;

static gboolean
camera_is_active(Camera *cam)
{
    return cam->camera != NULL ? TRUE : FALSE;
}

static void
set_pixbuf(Camera *cam, GdkPixbuf *pixbuf)
{
    cam->pixbuf = pixbuf;

    return;
}


static gboolean
on_expose_event(GtkWidget *widget,
        cairo_t *cr,
        gpointer data)
{
    Camera *cam = NULL;
    wg_uint width = 0;
    wg_uint height = 0;
    int w_width = 0;
    int w_height = 0;
    GtkWidget *resolution;

    cam = (Camera*)data;

    if (cam->pixbuf != NULL){
        resolution = gui_camera_get_resolution_widget(
                GUI_CAMERA(cam->gui_camera));

        gui_resolution_get(GUI_RESOLUTION(resolution), &width, &height);

        w_width = gtk_widget_get_allocated_width(widget);
        w_height = gtk_widget_get_allocated_height(widget);

        gdk_cairo_set_source_pixbuf(cr,
                cam->pixbuf, 
                (double)((w_width - width) >> 1),
                (double)((w_height - height) >> 1));

        cairo_paint(cr);
    }

    return TRUE;
}

void
xbuf_free(guchar *pixels, gpointer data)
{
    cam_img_cleanup((Wg_image*)data);

    WG_FREE(data);
}

gpointer
capture(gpointer data)
{
    register Camera  *cam = NULL;
    Wg_frame *frame = NULL;
    Wg_image *image_sub = NULL;
//    Wg_image  image;
    Wg_image  hsv_img;
//    Wg_camera *camera = NULL;
//    Wg_rgb base;
//    Wg_rgb thresh;
    cam_status status;
    wg_uint width = 0;
    wg_uint height = 0;
    Wg_cam_decompressor decompressor;
    GtkWidget *resolution = NULL;

    cam = (Camera*)data;

    frame = WG_CALLOC(1, sizeof (Wg_frame));

//    cam_select_user_decompressor(camera, v4l2_fourcc('M', 'J', 'P', 'G'));

    cam_decompressor(cam->camera, &decompressor);

    resolution = gui_camera_get_resolution_widget(GUI_CAMERA(cam->gui_camera));

    gui_resolution_get(GUI_RESOLUTION(resolution), &width, &height);
    cam_set_resolution(cam->camera, width, height);

    cam_start(cam->camera);

    gui_camera_fps_start(GUI_CAMERA(cam->gui_camera));

    for(;;){
        gdk_threads_enter();
        if ((cam->camera != NULL) && 
                (cam_read(cam->camera, frame) == CAM_SUCCESS)){
            gdk_threads_leave();

            image_sub = WG_MALLOC(sizeof (Wg_image));

            status = invoke_decompressor(&decompressor, 
                    frame->start, frame->size, 
                    frame->width, frame->height, image_sub);
            if(CAM_SUCCESS == status){
                cam_discard_frame(cam->camera, frame);

//               cam_img_rgb_2_bgrx(image_sub, &hsv_img);

//               cam_img_cleanup(&hsv_img);
//
                 if (gtk_toggle_button_get_active(
                             GTK_TOGGLE_BUTTON(cam->show_gray))){
                         cam_img_rgb_2_grayscale(image_sub, &hsv_img);

                         cam_img_cleanup(image_sub);

                         if (gtk_toggle_button_get_active(
                                     GTK_TOGGLE_BUTTON(cam->normalize))){

                             cam_img_grayscale_normalize(&hsv_img, 255, 0);
                         }

                         cam_img_grayscale_2_rgb(&hsv_img, image_sub);

                         cam_img_cleanup(&hsv_img);
                 }

//               cam_img_rgb_2_hsv_gtk(image_sub, &hsv_img);

                
                 gdk_threads_enter();

                 if (cam->pixbuf != NULL){
                     g_object_unref(cam->pixbuf);
                     cam->pixbuf = NULL;
                 }

                 set_pixbuf(cam,  gdk_pixbuf_new_from_data(image_sub->image, 
                        GDK_COLORSPACE_RGB, FALSE, 8, 
                        image_sub->width, image_sub->height, 
                        image_sub->row_distance, 
                        xbuf_free, image_sub));

                 gtk_widget_queue_draw(cam->area);

                 gui_camera_fps_update(GUI_CAMERA(cam->gui_camera), 1);

                 gdk_threads_leave();
            }else{
                 cam_discard_frame(cam->camera, frame);

                 WG_FREE(image_sub);    
            }
        }else{
            gdk_threads_leave();
            g_thread_exit(data);
        }
    }

    gui_camera_fps_stop(GUI_CAMERA(cam->gui_camera));

    cam_stop(cam->camera);

    cam_close(cam->camera);

    cam_free_frame(cam->camera, frame);

    WG_FREE(frame);

    return data;
}


    void
stop_capture(Camera *cam)
{

    if (camera_is_active(cam)){
        cam_stop(cam->camera);

        cam_close(cam->camera);
        WG_FREE(cam->camera);
        cam->camera = NULL;
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
    GtkWidget *start_button = NULL;
    GtkWidget *resolution = NULL;
    GtkWidget *dev_path = NULL;

    cam = (Camera*)data;

    stop_capture(cam);

    start_button = gui_camera_get_start_widget(GUI_CAMERA(cam->gui_camera));
    resolution = gui_camera_get_resolution_widget(GUI_CAMERA(cam->gui_camera));
    dev_path = gui_camera_get_device_widget(GUI_CAMERA(cam->gui_camera));

    gtk_widget_set_sensitive(widget, FALSE);
    gtk_widget_set_sensitive(start_button, TRUE);
    gtk_widget_set_sensitive(resolution, TRUE);
    gtk_widget_set_sensitive(dev_path, TRUE);
    gtk_window_set_focus(GTK_WINDOW(cam->window), start_button);
}

void button_clicked_start
(GtkWidget *widget, gpointer data){
    Wg_camera *camera;
    Camera    *cam = NULL;
    const gchar *device = NULL;
    cam_status status = CAM_FAILURE;
    GtkWidget *dev_path = NULL;
    GtkWidget *stop_button = NULL;
    GtkWidget *resolution = NULL;

    cam = (Camera*)data;

    camera = malloc(sizeof (Wg_camera));
    if (NULL != camera){
        cam->camera = camera;

        dev_path = gui_camera_get_device_widget(GUI_CAMERA(cam->gui_camera));
        stop_button = gui_camera_get_stop_widget(GUI_CAMERA(cam->gui_camera));
        resolution = gui_camera_get_resolution_widget(
                GUI_CAMERA(cam->gui_camera));

        device = gtk_entry_get_text(GTK_ENTRY(dev_path));

        cam_init(cam->camera, device);
        status = cam_open(cam->camera, 0, ENABLE_DECOMPRESSOR);
        if (CAM_SUCCESS == status){
            cam->thread = g_thread_create(capture, cam, FALSE, NULL);

            gtk_widget_set_sensitive(widget, FALSE);
            gtk_widget_set_sensitive(stop_button, TRUE);
            gtk_widget_set_sensitive(resolution, FALSE);
            gtk_widget_set_sensitive(dev_path, FALSE);
            gtk_window_set_focus(GTK_WINDOW(cam->window), stop_button);
        }else{
            WG_FREE(cam->camera);
            cam->camera = NULL;
        }
    }

    return;
}

static void button_clicked_color
(GtkWidget *widget, gpointer data){
    GtkWidget *color_sel = NULL;

    color_sel = gtk_color_selection_dialog_new("Pick up a color");

    gtk_dialog_run(GTK_DIALOG(color_sel));
}

static void button_clicked_capture
(GtkWidget *widget, gpointer data){
   Camera *cam = NULL;
   GError *gerr = NULL;
   GtkWidget *error_msg = NULL;

   cam = (Camera*)data;

   if (cam->pixbuf != NULL){
      gdk_pixbuf_save(cam->pixbuf, "frame.png", "png", &gerr, NULL);
   }else{
      error_msg = gtk_message_dialog_new(GTK_WINDOW(cam->window),
              GTK_DIALOG_MODAL,
              GTK_MESSAGE_ERROR,
              GTK_BUTTONS_CLOSE,
              "No captured frames"
              );

      gtk_dialog_run(GTK_DIALOG(error_msg));
      gtk_widget_destroy(error_msg);
   }
}

static void
select_resolution(Gui_resolution *obj, gpointer data)
{
    wg_uint width = 0;
    wg_uint height = 0;

    gui_resolution_get(obj, &width, &height);
    WG_LOG("New resolution w:%u h:%u\n", width, height);

}


int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *button_start = NULL;
    GtkWidget *button_stop = NULL;
    GtkWidget *capture_button = NULL;
    GtkWidget *resolution = NULL;
    GtkWidget *hbox = NULL;
    GtkWidget *area = NULL;
    GtkWidget *color_button = NULL;
    Camera    *camera = NULL;
    GtkWidget *gtk_cam = NULL;
    List_head  video;

    list_init(&video);

    wg_lsdir("/dev/", "video", &video);

    wg_lsdir_cleanup(&video);


    if (!g_thread_supported()){
        g_thread_init(NULL);
        gdk_threads_init();
        WG_LOG("g_thread supported\n");
    }else{
        WG_LOG("g_thread not supported\n");
    }

    gtk_init(&argc, &argv);

    camera = WG_CALLOC(1, sizeof (Camera));

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    area   = gtk_drawing_area_new();
    gtk_cam = gui_camera_new();

    camera->area = area;
    camera->gui_camera  = gtk_cam;
    camera->window = window;
    camera->fps = 0;
    camera->frame_count = 0;

    button_start = gui_camera_get_start_widget(GUI_CAMERA(gtk_cam));
    button_stop = gui_camera_get_stop_widget(GUI_CAMERA(gtk_cam));
    resolution  = gui_camera_get_resolution_widget(GUI_CAMERA(gtk_cam));
    capture_button  = gui_camera_get_capture_widget(GUI_CAMERA(gtk_cam));
    color_button = gui_camera_get_color_widget(GUI_CAMERA(gtk_cam));

    camera->normalize = gui_camera_add_checkbox(GUI_CAMERA(gtk_cam), 
            "Contract normalization");

    camera->show_gray = gui_camera_add_checkbox(GUI_CAMERA(gtk_cam), 
            "Show gray scale");

    gtk_widget_set_sensitive(button_start, TRUE);
    gtk_widget_set_sensitive(button_stop, FALSE);

    gtk_window_set_focus(GTK_WINDOW(window), button_start);

    gtk_widget_set_size_request(area, 640, 480);

    g_signal_connect(gtk_widget_get_toplevel(area), "draw",
            G_CALLBACK(on_expose_event), camera);
    g_signal_connect(gtk_widget_get_toplevel(window), "destroy",
            G_CALLBACK(gtk_main_quit), camera);

    g_signal_connect(button_start, "clicked",
            G_CALLBACK(button_clicked_start), camera);
    g_signal_connect(button_stop, "clicked",
            G_CALLBACK(button_clicked_stop), camera);
    g_signal_connect(capture_button, "clicked",
            G_CALLBACK(button_clicked_capture), camera);
    g_signal_connect(color_button, "clicked",
            G_CALLBACK(button_clicked_color), camera);

    g_signal_connect (gtk_widget_get_toplevel (window), "delete_event",
            G_CALLBACK(delete_event), camera);

    g_signal_connect(gtk_widget_get_toplevel(resolution), "changed",
            G_CALLBACK(select_resolution), camera);

    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

    gtk_box_pack_start(GTK_BOX(hbox), area, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_cam, FALSE, TRUE, 5);

    gtk_container_add (GTK_CONTAINER(window), hbox);

    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}
