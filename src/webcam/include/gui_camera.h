#ifndef _GUI_CAMERA_H
#define _GUI_CAMERA_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>

/** @brief Gui_camera cating macro                */
#define GUI_CAMERA(obj)    \
    G_TYPE_CHECK_INSTANCE_CAST((obj), gui_camera_get_type (), Gui_camera)

/** @brief Get gui_camera class                   */
#define GUI_CAMERA_CLASS(klass)                                     \
    G_TYPE_CHECK_CLASS_CAST ((klass), gui_camera_get_type (), \
            Gui_camera_class)

/** @brief Check if object is a Gui_camera object */
#define IS_GUI_CAMERA(obj)      \
    G_TYPE_CHECK_INSTANCE_TYPE ((obj), gui_camera_get_type ())

/** 
* @brief Gui_camera instance structure
*/
typedef struct Gui_camera{
    GtkGrid    parent;             /*!< parent structure        */
    GtkWidget  *resolution;        /*!< resolution selector     */
    GtkWidget  *start_button;      /*!< start capturing button  */
    GtkWidget  *stop_button;       /*!< stop capturing button   */
    GtkWidget  *capture_button;    /*!< capture a frame button  */
    GtkWidget  *color_button;      /*!< color selector button   */
    GtkWidget  *device;            /*!< device selector         */
    GtkWidget  *fps;               /*!< FPS display label       */

    /* Frame per secount counter */
    GTimer *fps_timer;             /*!< timer used by fps counter */
    gint    frame_counter;         /*!< number of counted frames  */
    gfloat  fps_val;               /*!< fps value                 */

    /* Layout row counter */ 
    guint row_count;               /*!< row counter               */
}Gui_camera;

/** 
* @brief Gui_camera class structure
*/
typedef struct Gui_camera_class{
    GtkGridClass parent_class;              /*!< parent class structure */
    void (*res_selected) (Gui_camera *obj); /*!< resolution selected signal */
}Gui_camera_class;

WG_PUBLIC guint        gui_camera_get_type(void);
WG_PUBLIC GtkWidget*   gui_camera_new(void);
WG_PUBLIC void         gui_camera_clear(Gui_camera *obj);

WG_PUBLIC wg_status gui_camera_get(Gui_camera *obj, 
        wg_uint *width, wg_uint *height);

WG_PUBLIC GtkWidget *
gui_camera_get_device_widget(Gui_camera *obj);

WG_PUBLIC GtkWidget *
gui_camera_get_stop_widget(Gui_camera *obj);

WG_PUBLIC GtkWidget *
gui_camera_get_start_widget(Gui_camera *obj);

WG_PUBLIC GtkWidget *
gui_camera_get_capture_widget(Gui_camera *obj);

WG_PUBLIC GtkWidget *
gui_camera_get_resolution_widget(Gui_camera *obj);

WG_PUBLIC GtkWidget *
gui_camera_add_checkbox(Gui_camera *obj, gchar *text);

WG_PUBLIC void
gui_camera_add(Gui_camera *obj, GtkWidget *comp);

WG_PUBLIC GtkWidget *
gui_camera_get_color_widget(Gui_camera *obj);

WG_PUBLIC void
gui_camera_fps_update(Gui_camera *obj, int val);

WG_PUBLIC void
gui_camera_fps_stop(Gui_camera *obj);

WG_PUBLIC void
gui_camera_fps_start(Gui_camera *obj);

WG_PUBLIC wg_status
gui_camera_get_active_resolution(Gui_camera *obj, wg_uint *width, 
		wg_uint *height);

#endif
