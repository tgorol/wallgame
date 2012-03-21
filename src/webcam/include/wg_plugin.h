#ifndef _WG_PLUGIN_H
#define _WG_PLUGIN_H

typedef enum WEBCAM_STATE{
    WEBCAM_STATE_UNINITIALIZED     = 0,
    WEBCAM_STATE_CALLIBRATE           ,
    WEBCAM_STATE_START                ,
    WEBCAM_STATE_STOP                 ,
    WEBCAM_STATE_GET_COLOR            ,
    WEBCAM_STATE_GET_PANE
}WEBCAM_STATE;

typedef struct Camera{
    /* application state */
    WEBCAM_STATE state;

    /* gui variables */
    GtkWidget *menubar;
    GtkWidget *window;
    GtkWidget *resolution_combo;
    GtkWidget *device_combo;
    GtkWidget *start_capturing;
    GtkWidget *stop_capturing;
    GtkWidget *callibrate;
    GtkWidget *left_area;
    GtkWidget *right_area;
    GtkWidget *noise_reduction;
    GtkWidget *fps_display;

    /* Sensor color object range */
    Hsv top;
    Hsv bottom;

    /* Sensor variables */
    Sensor    *sensor;

    /* Collision detector */
    Cd_instance pane;
    
    /* fps counter variables */
    GTimer *fps_timer;             /*!< timer used by fps counter */
    gint    frame_counter;         /*!< number of counted frames  */
    gfloat  fps_val;               /*!< fps value                 */

    GdkPixbuf *right_pixbuf;
    GdkPixbuf *left_pixbuf;
    GtkWidget *status_bar;

    pthread_t  thread;
    Wg_video_out vid;
    wg_boolean dragging;
    wg_uint x1;
    wg_uint y1;
    wg_uint x2;
    wg_uint y2;
}Camera;

WG_PUBLIC wg_boolean
go_to_state(Camera *camera, WEBCAM_STATE new_state);

WG_PUBLIC void
get_selected_resolution(GtkWidget *combo, wg_uint *width, wg_uint *height);

WG_PUBLIC void
default_cb(Sensor *sensor, Sensor_cb_type type, Wg_image *img, void *user_data);

WG_PUBLIC void *
capture(void *data);

WG_PUBLIC void
fill_resolution_combo(GtkComboBoxText *combo);

WG_PUBLIC void
stop_capture(Camera *cam);

#endif
