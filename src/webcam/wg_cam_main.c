#include <stdlib.h>
#include <stdio.h>

#include <wg.h>
#include <wgtypes.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_sync_linked_list.h>
#include <wg_wq.h>
#include <img.h>
#include <cam.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "include/gui_prim.h"
#include "include/sensor.h"
#include "include/vid.h"

#include "include/collision_detect.h"
#include "include/wg_plugin.h"


int
main(int argc, char *argv[])
{
    wg_status status = WG_FAILURE;
    Camera camera;

    MEMLEAK_START;

    status = wg_plugin_init(argc, argv, &camera);

    if (WG_SUCCESS == status){
        wg_plugin_start(&camera);
    }

    wg_plugin_cleanup(&camera);

    MEMLEAK_STOP;

    return 0;
}

#if 0
wg_status
create_histogram(Wg_image *img, wg_uint width, wg_uint height, Wg_image *hist)
{
    wg_status status = CAM_FAILURE;
    wg_int i = 0;
    wg_uint max_val = 0;
    wg_uint histogram[GS_PIXEL_MAX + 1];
    wg_float my = WG_FLOAT(0.0);
    wg_float mx = WG_FLOAT(0.0);
    wg_float x = WG_FLOAT(0);
    Img_draw ctx;

    img_gs_histogram(img, histogram, ELEMNUM(histogram));

    status = img_fill(width, height, GS_COMPONENT_NUM, IMG_GS, hist);
    if (WG_SUCCESS != status){
        return WG_FAILURE;
    }

    for (i = 0; i < ELEMNUM(histogram); ++i){
        max_val = WG_MAX(max_val, histogram[i]);
    }

    if (max_val == 0){
        return WG_SUCCESS;
    }

    mx = WG_FLOAT(width) / ELEMNUM(histogram);
    my = WG_FLOAT(height) / max_val;

    img_draw_get_context(hist->type, &ctx);

    for (i = 0, x = 0; i < ELEMNUM(histogram); ++i, x += 1.0){
        img_draw_line(&ctx, hist, 
                height, mx * x, height - histogram[i] * my, mx * x, 128);   
    }

    img_draw_cleanup_context(&ctx);

    return WG_SUCCESS;
}

#endif
