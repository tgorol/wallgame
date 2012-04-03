#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>
#include <wg_linked_list.h>
#include <wg_iterator.h>
#include <img.h>
#include <cam.h>

#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "include/vid.h"

#define IS_POW_2(val) ((((~val) + 1) & (val)) == val)

WG_INLINE wg_uint
get_pow_2(wg_uint value)
{
    wg_uint est = 0;
    wg_uint ret = 0;
    if (IS_POW_2(value) == WG_TRUE){
        ret = value;
    }else{
        for (est = 1; (est != 0) && (est < value); est <<= 1)
            /* void scope */;
        ret = est;
    }

    return ret;
}

WG_INLINE wg_uint
get_ceil_even(wg_uint value)
{
    return value & 0x1 ? value + 1 : value;
}
/*! \defgroup video Video library
 */

/*! @{ */

/** 
* @brief Open output stream
* 
* @param filename  filename to bind with stream
* @param vid       memory to store stream instance
* @param width     width of the image
* @param height    height of the image
* 
* @retval WG_SUCCESS
* @retval WG_FAILURE
*/
wg_status
video_open_output_stream(const char *filename, Wg_video_out *vid, 
        wg_uint width, wg_uint height)
{
    static wg_boolean init_flag = WG_FALSE;

    if (WG_FALSE == init_flag){
        avcodec_init();
        /* register all the codecs */
        avcodec_register_all();

        init_flag = WG_TRUE;
    }

    vid->codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
    if (NULL == vid->codec) {
        WG_LOG("Codec not found\n");
        video_close_output_stream(vid);
        return WG_FAILURE;
    }

    vid->context= avcodec_alloc_context();
    if (NULL == vid->context){
        WG_LOG("Could not allocate codec context\n");
        video_close_output_stream(vid);
        return WG_FAILURE;
    }
    /* put sample parameters */
    vid->context->bit_rate = 400000;
    /* resolution must be a multiple of two */
    vid->context->width = width + width;
    vid->context->height = height + height;
    /* frames per second */
    vid->context->time_base= (AVRational){1, 30};
    vid->context->gop_size = 10; /* emit one intra frame every ten frames */
    vid->context->max_b_frames=1;
    vid->context->pix_fmt = PIX_FMT_YUV420P;

    /* open codec */
    if (avcodec_open(vid->context, vid->codec) < 0) {
        WG_LOG("Could not open codec\n");
        exit(1);
    }

    /* open output file */
    vid->out_stream = fopen(filename, "wb");
    if (NULL == vid->out_stream){
        WG_LOG("%s\n", strerror(errno));
        video_close_output_stream(vid);
        return WG_FAILURE;
    }

    vid->ts = 0;

    return WG_SUCCESS;
}

/** 
* @brief Close output stream
* 
* @param vid stream instance
*/
void
video_close_output_stream(Wg_video_out *vid)
{
    uint8_t outbuf[4];

    /* add sequence end code to have a real mpeg file */
    outbuf[0] = 0x00;
    outbuf[1] = 0x00;
    outbuf[2] = 0x01;
    outbuf[3] = 0xb7;

    fwrite(outbuf, 1, 4, vid->out_stream);

    if (NULL != vid->out_stream){
        fclose(vid->out_stream);
        vid->out_stream = NULL;
    }

    if (NULL != vid->context){
        avcodec_close(vid->context);
        av_free(vid->context);
    }

    return;
}

/** 
* @brief Write a frame to the stream
* 
* @param vid stream instance
* @param img image(frame) to write to the stream
* 
* @return 
*/
wg_status
video_encode_frame(Wg_video_out *vid, Wg_image *img)
{
    uint8_t *outbuf = NULL;
    uint8_t *picture_buf = NULL;
    AVFrame *rgb24_pix = NULL;
    AVFrame *yuv420_pix = NULL;
    struct SwsContext *img_convert_ctx = NULL;
    const uint8_t* const* src_data = NULL;
    uint8_t* const* dest_data = NULL;
    int out_size;
    int outbuf_size;
    wg_uint num_bytes = 0;

    /* allocate memory for frames */
    rgb24_pix  = avcodec_alloc_frame();
    yuv420_pix = avcodec_alloc_frame();

    /* initialize av picture with data from img */
    avpicture_fill((AVPicture*)rgb24_pix, img->rows[0], PIX_FMT_RGB24,
                                img->width, img->height);

    /* get number of bytes needed for YUV420 picture */
    num_bytes = avpicture_get_size(vid->context->pix_fmt, vid->context->width,
                                vid->context->height);

    /* alloc image and output buffer */
    outbuf_size = 100000;
    outbuf =av_malloc(outbuf_size);

    /* allocate data for YUV420 picture */
    picture_buf = av_malloc(num_bytes); /* size for YUV 420 */

    /* fill YUV420 picture with allocated data */
    avpicture_fill((AVPicture*)yuv420_pix, picture_buf, vid->context->pix_fmt,
                                vid->context->width, vid->context->height);

    /* convert RGB24 to image supported by codec */
    img_convert_ctx = sws_getCachedContext(img_convert_ctx, 
             img->width, img->height, PIX_FMT_RGB24, 
             vid->context->width, vid->context->height, vid->context->pix_fmt,
             SWS_BICUBIC, NULL, NULL, NULL);
    if(img_convert_ctx == NULL) {
        WG_ERROR("Cannot initialize the conversion context!\n");
        return WG_FAILURE;
    }

    src_data = (const uint8_t* const*)rgb24_pix->data;
    dest_data = (uint8_t * const *)yuv420_pix->data;

    /* convert RGB24 to format supported by a codex */
    sws_scale(img_convert_ctx,
            src_data, rgb24_pix->linesize, 0,
            img->height, dest_data, yuv420_pix->linesize);

    sws_freeContext(img_convert_ctx);

    yuv420_pix->pts = vid->ts;

    vid->ts += 1;

    /* encode frame */
    out_size = avcodec_encode_video(vid->context, outbuf, outbuf_size, 
            yuv420_pix);

    /* write frame to the output stream */
    fwrite(outbuf, 1, out_size, vid->out_stream);

    av_free(picture_buf);
    av_free(rgb24_pix);
    av_free(yuv420_pix);
    av_free(outbuf);

    return WG_SUCCESS;
}

/*! @} */
