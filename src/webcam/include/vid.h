#ifndef VID_H
#define VID_H

#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

/** 
* @brief Video output stream
*/
typedef struct Wg_video_out{
    FILE *out_stream;           /*!< file stream */
    AVCodec *codec;             /*!< codec */
    AVCodecContext *context;    /*!< encoding context */
    wg_uint64 ts;
}Wg_video_out;

WG_PUBLIC wg_status
video_encode_frame(Wg_video_out *vid, Wg_image *img);

WG_PUBLIC void
video_close_output_stream(Wg_video_out *vid);

WG_PUBLIC wg_status
video_open_output_stream(const char *filename, Wg_video_out *vid,
        wg_uint width, wg_uint height);


#endif
