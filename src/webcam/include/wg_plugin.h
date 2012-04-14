#ifndef _WG_PLUGIN_H
#define _WG_PLUGIN_H

/* todo get rid of this from here */
#include "gui_display.h"

/** 
* @brief Update image task structure
*/
typedef struct Update_image{
    GdkPixbuf   *pixbuf;      /*!< image to paint                      */
    Gui_display *display;     /*!< display to use to paint             */
    wg_uint x;                /*!< x offset in display                 */
    wg_uint y;                /*!< y offet in display                  */
}Update_image;

/** 
* @brief Camera plugin state
*/
typedef enum WEBCAM_STATE{
    WEBCAM_STATE_UNINITIALIZED     = 0,     /*!< uninitalized          */
    WEBCAM_STATE_CALLIBRATE           ,     /*!< callibtation          */
    WEBCAM_STATE_START                ,     /*!< detection started     */
    WEBCAM_STATE_STOP                 ,     /*!< detection stoped      */
    WEBCAM_STATE_GET_COLOR            ,     /*!< get color range       */
    WEBCAM_STATE_GET_PANE                   /*!< get pane              */
}WEBCAM_STATE;

/** 
* @brief Camera plugin structure
*/
typedef struct Camera{
    /* application state                                                  */
    WEBCAM_STATE state;            /*!< state of plugin                   */

    /* gui variables                                                      */
    GtkWidget *menubar;            /*!< menu bar                          */
    GtkWidget *window;             /*!< main window                       */
    GtkWidget *resolution_combo;   /*!< resolution select combo box       */
    GtkWidget *device_combo;       /*!< device select combo box           */
    GtkWidget *start_capturing;    /*!< start capturing button            */
    GtkWidget *stop_capturing;     /*!< stop capturing button             */
    GtkWidget *callibrate;         /*!< calibrate button                  */
    GtkWidget *noise_reduction;    /*!< noise reduction check button      */
    GtkWidget *fps_display;        /*!< fps diaplay label                 */
    GtkWidget *device_name;        /*!< devoce name label                 */

    /* Message Transport                                                  */
    Wg_msg_transport msg_transport; /*!< transport                        */

    /* Displays display                                                   */
    Gui_display left_display;      /*!< left display widget               */
    Gui_display right_display;     /*!< right display widget              */

    /* Sensor color object range                                          */
    Hsv top;                       /*!< color range top                   */
    Hsv bottom;                    /*!< color range bottom                */

    /* Sensor variables                                                   */
    Sensor    *sensor;             /*!< sensor instance                   */

    /* Collision detector                                                 */
    Cd_instance cd;                /*!< collision detector instance       */

    /* fps counter variables                                              */
    GTimer *fps_timer;             /*!< timer used by fps counter         */
    gint    frame_counter;         /*!< number of counted frames          */
    gfloat  fps_val;               /*!< fps value                         */

    GdkPixbuf *right_pixbuf;       /*!< right pixbuf                      */
    GdkPixbuf *left_pixbuf;        /*!< left pixbuf                       */
    GtkWidget *status_bar;         /*!< status bar                        */

    pthread_t  thread;             /*!< capturing thread                  */
    Wg_video_out vid;              /*!< video stream instance             */

    /* used for gathering color from screen                               */
    wg_boolean dragging;           /*!< dragging flag                     */
    wg_uint x1;                    /*!< color selector x1                 */
    wg_uint y1;                    /*!< color selector y1                 */
    wg_uint x2;                    /*!< color selector x2                 */
    wg_uint y2;                    /*!< color selector y2                 */
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

WG_PUBLIC wg_status
wg_plugin_init(int argc, char *argv[], Camera *camera);

WG_PUBLIC void
wg_plugin_start(Camera *camera);

WG_PUBLIC void
wg_plugin_cleanup(Camera *camera);


WG_PUBLIC void
wg_plugin_start_fps(Camera *obj);

WG_PUBLIC void 
wg_plugin_print_fps(Camera *obj);

WG_PUBLIC void
wg_plugin_update_fps(Camera *obj, wg_int val);

WG_PUBLIC void
wg_plugin_stop_fps(Camera *obj);

#endif
