#ifndef _WG_CAM_CAP_H
#define _WG_CAM_CAP_H

WG_PUBLIC wg_cam_status wg_cam_cap_read(Wg_camera *cam);

WG_PUBLIC wg_cam_status wg_cam_cap_print(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_video_capture(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_video_output(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_video_overlay(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_vbi_capture(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_vbi_output(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_sliced_vbi_capture(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_sliced_vbi_output(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_rds_capture(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_video_output_overlay(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_tuner(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_audio(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_radio(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_readwrite(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_asyncio(Wg_camera *cam);

WG_PUBLIC wg_boolean wg_cam_cap_streaming(Wg_camera *cam);

#endif

