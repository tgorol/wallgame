#ifndef _WG_CAMREADWRITE
#define _WG_CAMREADWRITE

WG_PUBLIC cam_status
cam_readwrite_init(Wg_camera *cam);

WG_PUBLIC cam_status
cam_frame_select(wg_uint num, Wg_camera *cameras[], wg_boolean retval[],
        wg_int timeout_ms);

#endif
