#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <linux/videodev2.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <img.h>
#include <cam.h>

#include "include/cam_frame.h"

/*! @defgroup webcam_frame frame
 *  @ingroup webcam
 */

/*! @{ */

/**
 * @brief Initialize frame.
 *
 * This function must be call before first use with any of webcam function
 *
 * @param frame Frame to initialize
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_frame_init(Wg_frame *frame)
{
    CHECK_FOR_NULL_PARAM(frame);

    memset(frame, '\0', sizeof (Wg_frame));

    frame->state = WG_FRAME_INVALID;

    return CAM_SUCCESS;
}

/*! @} */
