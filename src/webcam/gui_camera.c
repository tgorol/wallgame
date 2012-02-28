#include <stdlib.h>
#include <string.h>

#include <gtk/gtk.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>

#include <wg_lsdir.h>

#include "include/gui_camera.h"

/*! @defgroup gui GUI components
*/

/*! @defgroup gui_camera camera panel components
 * @ingroup gui
 */

/*! @{ */

/** @brief Row spacing              */
#define ROW_SPACING    5

/** @brief Column spacing           */
#define COL_SPACING    5

/** @brief FPS interwal counter     */
#define FPS_INTERVAL   0.5

/** @brief maximum devixe path size */
#define DEVICE_PATH_MAX  64

/** @brief device path 
 *
 * @todo find a way to get it from the system
 */
#define DEV_PATH  "/dev/"

/** @brief Default resolution */
#define RESOLUTION_DEFAULT_INDEX    0

enum{
    CHANGED_SIGNAL,
    LAST_SIGNAL
};

/** 
* @brief Resolution
*/
typedef struct Resolution{
    wg_char text[16];        /*!< text of the resolution */
    wg_uint  width;          /*!< width in pixels        */
    wg_uint  height;         /*!< height in pixels       */
}Resolution;



/** 
* @brief List of supported resolutions
*/
static const Resolution res_info[] = {
    {"352x288", 352, 288,} ,
    {"320x240", 320, 240,} ,
    {"176x144", 176, 144,} ,
    {"160x120", 160, 120,}
};

/** 
* @brief Signals emited by gui_camera
*/
static gint wsignals[LAST_SIGNAL] = {0};

WG_PRIVATE void 
update_fps(Gui_camera *obj);

WG_PRIVATE void
gui_camera_class_init(Gui_camera_class *wclass)
{
    wsignals[CHANGED_SIGNAL] = g_signal_new("changed",
            G_TYPE_FROM_CLASS(wclass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET(Gui_camera_class, res_selected),
            NULL,
            NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE,
            0);
}

WG_PRIVATE guint
get_row_and_inc(Gui_camera *obj)
{
    return obj->row_count++;
}

WG_PRIVATE guint
get_row(Gui_camera *obj)
{
    return obj->row_count;
}

WG_PRIVATE void
init_row_count(Gui_camera *obj)
{
    obj->row_count = 0;
}


WG_PRIVATE void
gui_camera_init(Gui_camera *obj)
{
    GtkWidget *label = 0;
    List_head head;
    Iterator itr;
    wg_dirent *dir_entry;
    wg_char full_path[DEVICE_PATH_MAX];
    wg_int count = 0;

    init_row_count(obj);

    label = gtk_label_new("Device");
    gtk_grid_attach(GTK_GRID(obj),
            label,
            0, get_row(obj),
            1, 1
            );

    obj->device = gtk_combo_box_text_new();
    gtk_grid_attach(GTK_GRID(obj),
            obj->device,
            1, get_row_and_inc(obj),
            1, 1
            );

    list_init(&head);

    wg_lsdir(DEV_PATH, "video", &head);

    iterator_list_init(&itr, &head, GET_OFFSET(wg_dirent, list));

    count = 0;
    while ((dir_entry = iterator_list_next(&itr)) != NULL){
        strcpy(full_path, DEV_PATH);
        strcat(full_path, dir_entry->d_name);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(obj->device),
            full_path);
        ++count;
    }

    if (count > 0){
        gtk_combo_box_set_active(GTK_COMBO_BOX(obj->device), count - 1);
    }

    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(obj->device), FALSE); 

    wg_lsdir_cleanup(&head);

    label = gtk_label_new("Resolution");
    gtk_grid_attach(GTK_GRID(obj),
            label,
            0, get_row(obj),
            1, 1
            );

    obj->resolution = gtk_combo_box_text_new();
    gtk_grid_attach(GTK_GRID(obj),
            obj->resolution,
            1, get_row_and_inc(obj),
            1, 1
            );

    for (count = 0; count < ELEMNUM(res_info); ++count){
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(obj->resolution),
			res_info[count].text);
    }

    gtk_combo_box_set_active(GTK_COMBO_BOX(obj->resolution), 
		    RESOLUTION_DEFAULT_INDEX);

    gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(obj->resolution), FALSE); 

    label = gtk_label_new("fps");
    gtk_grid_attach(GTK_GRID(obj),
            label,
            0, get_row(obj),
            1, 1
            );

    obj->fps = gtk_label_new("0");
    gtk_grid_attach(GTK_GRID(obj),
            obj->fps,
            1, get_row_and_inc(obj),
            1, 1
            );


    obj->start_button = gtk_button_new_with_label("Start Capturing");
    gtk_grid_attach(GTK_GRID(obj),
            obj->start_button,
            0, get_row_and_inc(obj),
            2, 1
            );

    obj->stop_button  = gtk_button_new_with_label("Stop Capturing");
    gtk_grid_attach(GTK_GRID(obj),
            obj->stop_button,
            0, get_row_and_inc(obj),
            2, 1
            );

    obj->capture_button  = gtk_button_new_with_label("Capture Frame");
    gtk_grid_attach(GTK_GRID(obj),
            obj->capture_button,
            0, get_row_and_inc(obj),
            2, 1
            );

    obj->color_button  = gtk_button_new_with_label("Select Color");
    gtk_grid_attach(GTK_GRID(obj),
            obj->color_button,
            0, get_row_and_inc(obj),
            2, 1
            );

    gtk_grid_set_row_spacing(GTK_GRID(obj), ROW_SPACING); 
    gtk_grid_set_column_spacing(GTK_GRID(obj), COL_SPACING); 
    
    obj->fps_timer = g_timer_new();
    obj->frame_counter = 0;
    obj->fps_val = 0.0;

    return;
}

/** 
* @brief Create new gui_camera object
* 
* @return pointer to new object or NULL if error
*/
GtkWidget*
gui_camera_new(void)
{
    return GTK_WIDGET(g_object_new(gui_camera_get_type(), NULL));
}

/** 
* @brief Add new check box
* 
* @param obj    wg_camera widget
* @param text   checkbock text
* 
* @return pointer to created checkbox
*/
GtkWidget *
gui_camera_add_checkbox(Gui_camera *obj, gchar *text)
{
    GtkWidget *button = NULL;

    button = gtk_check_button_new_with_label(text);

    gtk_grid_attach(GTK_GRID(obj),
            button,
            0, get_row_and_inc(obj),
            2, 1
            );

    return button;
}

/** 
* @brief Add widget
* 
* @param obj   wg_camera widget
* @param comp  widget to add
*/
void
gui_camera_add(Gui_camera *obj, GtkWidget *comp)
{
    gtk_grid_attach(GTK_GRID(obj),
            comp,
            0, get_row_and_inc(obj),
            2, 1
            );

    return;
}

/** 
* @brief Get type
* 
* @return type
*/
guint
gui_camera_get_type()
{
    static guint obj_type = 0;
    static GTypeInfo obj_info = {
        sizeof (Gui_camera_class),
        NULL,
        NULL,
        (GClassInitFunc) gui_camera_class_init,
        NULL,
        NULL,
        sizeof (Gui_camera),
        0,
        (GInstanceInitFunc) gui_camera_init
    };

    if (obj_type == 0){
        obj_type = g_type_register_static(GTK_TYPE_GRID, 
            "gui_camera", &obj_info, 0);
    }

    return obj_type;
}

/** 
* @brief Get start widget
* 
* @param obj wg_camera widget
* 
* @return start widget
*/
GtkWidget *
gui_camera_get_start_widget(Gui_camera *obj)
{
    return obj->start_button;
}

/** 
* @brief Get start capture widget
* 
* @param obj wg_camera widget
* 
* @return start capturet widget
*/
GtkWidget *
gui_camera_get_capture_widget(Gui_camera *obj)
{
    return obj->capture_button;
}

/** 
* @brief Get stop widget
* 
* @param obj wg_camera widget
* 
* @return stop widget
*/
GtkWidget *
gui_camera_get_stop_widget(Gui_camera *obj)
{
    return obj->stop_button;
}

/** 
* @brief Get color select widget
* 
* @param obj wg_camera widget
* 
* @return color select widget
*/
GtkWidget *
gui_camera_get_color_widget(Gui_camera *obj)
{
    return obj->color_button;
}

/** 
* @brief Get device path widget
* 
* @param obj wg_camera widget
* 
* @return device path widget
*/
GtkWidget *
gui_camera_get_device_widget(Gui_camera *obj)
{
    return obj->device;
}

/** 
* @brief Get resolution widget
* 
* @param obj wg_camera widget
* 
* @return resolution widget
*/
GtkWidget *
gui_camera_get_resolution_widget(Gui_camera *obj)
{
    return obj->resolution;
}

/** 
* @brief Get selected resolution
* 
* @param obj wg_camera widget
* @param[out] width memory to store selected width
* @param[out] height memory to store selected height
* 
* @return start widget
*/
wg_status
gui_camera_get_active_resolution(Gui_camera *obj, wg_uint *width, 
		wg_uint *height)
{
    gint res_index = 0;

    res_index = gtk_combo_box_get_active(GTK_COMBO_BOX(obj->resolution));

    CHECK_FOR_RANGE_GE(res_index, ELEMNUM(res_info));
    
    *width  = res_info[res_index].width;
    *height = res_info[res_index].height;

    return WG_SUCCESS;
}

/** 
* @brief Start fps counter
* 
* @param obj wg_camera widget
*/
void
gui_camera_fps_start(Gui_camera *obj)
{
    g_timer_start(obj->fps_timer);
}

/** 
* @brief Update fps counter
* 
* @param obj wg_camera widget
* @param val number of frames to update
*/
void
gui_camera_fps_update(Gui_camera *obj, int val)
{
    gulong micro = 0UL;
    double elapsed = 0.0;

    obj->frame_counter += val;

    if ((elapsed = g_timer_elapsed(obj->fps_timer, &micro)) >= FPS_INTERVAL){
        obj->fps_val = obj->frame_counter / elapsed;
        obj->frame_counter = 0;
        g_timer_start(obj->fps_timer);
        update_fps(obj);
    }
}

/** 
* @brief Stop fps counter
* 
* @param obj wg_camera widget
*/
void
gui_camera_fps_stop(Gui_camera *obj)
{
    g_timer_stop(obj->fps_timer);
}

WG_PRIVATE void 
update_fps(Camera *obj)
{
    char text[32];

    sprintf(text, "%.1f", (float)obj->fps_val);

    gtk_label_set_text(GTK_LABEL(obj->fps), text);
}

/*! @} */
