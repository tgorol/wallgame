#ifndef _CAM_FRAME_H
#define _CAM_FRAME_H

/**
 * @brief Frame state
 */
typedef enum WG_FRAME_STATE {
   WG_FRAME_INVALID = 0 ,   /*!< Invalid state */
   WG_FRAME_EMPTY       ,   /*!< Frame Empty   */
   WG_FRAME_FULL            /*!< Frame Full    */
}WG_FRAME_STATE;

/**
 * @brief Frame
 *
 * Each readed frame from the camera is returned in thih format
 */
struct Wg_frame{
    WG_FRAME_STATE state;   /*!< State of frame     */
    wg_uchar *start;        /*!< Start of frame     */
    wg_size  size;          /*!< Size of frame      */
    wg_uint  width;         /*!< Width in pixels    */
    wg_uint  height;        /*!< Height in pixels   */
    /* used only by streaming */
    __u32 pixelformat;      
    /*!< Pixel format       */
    struct v4l2_buffer stream_buf;
    /*!< Stream buffer associated with this frame */
};

WG_PUBLIC cam_status
cam_frame_init(Wg_frame *frame);

#endif
