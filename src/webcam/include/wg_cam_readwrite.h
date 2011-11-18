#ifndef _WG_CAMREADWRITE
#define _WG_CAMREADWRITE


WG_PUBLIC wg_cam_status
wg_cam_frame_read(Wg_camera *cam, Wg_frame *frame);

WG_PUBLIC wg_cam_status
wg_cam_frame_get_size(Wg_camera *cam, wg_ssize *frame_size);

WG_PUBLIC wg_cam_status
wg_cam_frame_buffer_release(Wg_frame *fram);

WG_PUBLIC wg_cam_status
wg_cam_frame_buffer_alloc(Wg_camera *cam, Wg_frame *frame);

WG_PUBLIC wg_cam_status
wg_cam_frame_select(wg_uint num, Wg_camera *cameras[], wg_boolean retval[],
        wg_int timeout_ms);

#endif
