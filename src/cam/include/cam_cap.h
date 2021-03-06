#ifndef _CAM_CAP_H
#define _CAM_CAP_H

#define CAM_DEVICE_NAME_MAX  32

WG_PUBLIC cam_status cam_cap_get(Wg_camera *cam);

WG_PUBLIC cam_status
cam_cap_get_device_name(Wg_camera *cam, wg_char *device_name, wg_size size);

WG_PUBLIC cam_status cam_cap_print(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_video_capture(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_video_output(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_video_overlay(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_vbi_capture(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_vbi_output(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_sliced_vbi_capture(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_sliced_vbi_output(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_rds_capture(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_video_output_overlay(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_tuner(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_audio(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_radio(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_readwrite(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_asyncio(Wg_camera *cam);

WG_PUBLIC wg_boolean cam_cap_streaming(Wg_camera *cam);

#endif

