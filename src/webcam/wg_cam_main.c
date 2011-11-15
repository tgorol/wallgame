#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>

#include <linux/videodev2.h>

#include "include/wg_cam.h"
#include "include/wg_cam_cap.h"
#include "include/wg_cam_image.h"

int
main(int argc, char *argv[])
{
    Wg_camera camera;
    struct v4l2_format format;

    List_head fmt;

    wg_cam_init(&camera, "/dev/video0");

    wg_cam_open(&camera);

    wg_cam_cap_print(&camera);

    wg_cam_image_format_get(&camera, WG_CAM_OUT_VIDEO_CAPTURE, &format);

    wg_cam_image_format_print(&format);

    list_init(&fmt);

    wg_cam_image_fmtdesc_list(&camera, WG_CAM_OUT_VIDEO_CAPTURE, &fmt);

    wg_cam_image_fmtdesc_list_print(&fmt);

    wg_cam_image_fmtdesc_list_cleanup(&fmt);

    wg_cam_close(&camera);

    return 0;
}
