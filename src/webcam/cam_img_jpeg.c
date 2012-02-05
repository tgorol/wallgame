#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <setjmp.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <jpeglib.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_img.h"
#include "include/cam_img_jpeg.h"

#define LINES_PER_READ 32

/*! @defgroup webcam_jpeg Webcam JPEG Conversion Functions
 *  @ingroup webcam 
 */

/*! @{ */

/**
 * @brief Custom memmory source manager
 *
 * This manager reads butes from memory buffer
 */
typedef struct Wg_src_mgr{
    /* This field must must be first in the structure */
    struct jpeg_source_mgr parent_mgr; /*!< jpeg mgr        */

    wg_uchar *buffer;    /*!< buffer location               */
    wg_ssize  size;      /*!< number of bytes in the buffer */

}Wg_src_mgr;

/**
 * @brief Custom error handler
 */
typedef struct Wg_src_err{
    /* This field must must be first in the structure */
    struct jpeg_error_mgr  parent_err;  /*!< jpeg error handler */

    jmp_buf exit_point;                 /*!< exit point in case of error */
}Wg_src_err;

WG_PRIVATE cam_status
set_source_manager(struct jpeg_decompress_struct *jdecomp, Wg_src_mgr* mgr);

WG_PRIVATE cam_status
init_source_manager(Wg_src_mgr *mgr, wg_uchar *buffer, wg_ssize size);

#if 0
WG_PRIVATE cam_status
alloc_jrows(struct jpeg_decompress_struct *jdecomp, Wg_image *img);
#endif

WG_PRIVATE cam_status
set_options(struct jpeg_decompress_struct *jdecomp);

WG_PRIVATE void init_source (j_decompress_ptr cinfo);
WG_PRIVATE boolean fill_input_buffer (j_decompress_ptr cinfo);
WG_PRIVATE void skip_input_data (j_decompress_ptr cinfo, long num_bytes);
WG_PRIVATE boolean resync_to_restart (j_decompress_ptr cinfo, int desired);
WG_PRIVATE void term_source (j_decompress_ptr cinfo);

WG_PRIVATE void error_exit(j_common_ptr cinfo);

/**
 * @brief Decompress jpeg image
 *
 * @param in_buffer  input buffer
 * @param in_size    size of the buffer
 * @param width      width of the image
 * @param height     haight of the image
 * @param img        memory to store decomressed image
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_img_jpeg_decompress(wg_uchar *in_buffer, wg_ssize in_size, 
        wg_uint width, wg_uint height, Wg_image *img)
{
    Wg_src_mgr mgr;
    int status = 0;
    struct jpeg_decompress_struct jdecomp;
    struct Wg_src_err         jerror;
    wg_uint index = 0;
    JDIMENSION readed_lines = 0;

    CHECK_FOR_NULL_PARAM(in_buffer);
    CHECK_FOR_NULL_PARAM(in_size);
    CHECK_FOR_NULL_PARAM(img);

    jdecomp.err = jpeg_std_error(&jerror.parent_err);
    jerror.parent_err.error_exit = error_exit;

    if (setjmp(jerror.exit_point)){
        jpeg_destroy_decompress(&jdecomp);
        return CAM_FAILURE;
    }

    /* initialize custom data manager            */
    init_source_manager(&mgr, in_buffer, in_size);

    /* create a decompression session            */
    jpeg_create_decompress(&jdecomp);

    /* set custom source data manager            */
    set_source_manager(&jdecomp, &mgr);

    /* read headed of the jpeg image             */
    status = jpeg_read_header(&jdecomp, FALSE);
    if (status !=  JPEG_HEADER_OK){
        jpeg_destroy_decompress(&jdecomp);
        return CAM_FAILURE;
    }

    set_options(&jdecomp);

    /* decompress image                           */
    if (FALSE == jpeg_start_decompress(&jdecomp)){
        jpeg_destroy_decompress(&jdecomp);
        return CAM_FAILURE;
    }

    /* allocate memory to store decomressed image */
#if 0
    alloc_jrows(&jdecomp, img);
#endif
    cam_img_fill(jdecomp.output_width ,jdecomp.output_height, 
            jdecomp.output_components, IMG_RGB, img);

    /* read decompressed lines                    */
    for (index = 0; jdecomp.output_scanline < jdecomp.output_height;){
        readed_lines = jpeg_read_scanlines(&jdecomp, &(img->rows[index]),
                LINES_PER_READ);
        if (readed_lines == 0){
            jpeg_destroy_decompress(&jdecomp);
            cam_img_cleanup(img);
            return CAM_FAILURE;
        }
        index += readed_lines;
    }

    /* cleanup */
    jpeg_finish_decompress(&jdecomp);
    jpeg_destroy_decompress(&jdecomp);

    return CAM_SUCCESS;
}


WG_PRIVATE cam_status
set_options(struct jpeg_decompress_struct *jdecomp)
{
    CHECK_FOR_NULL_PARAM(jdecomp);

    jdecomp->do_fancy_upsampling = FALSE;
    jdecomp->do_block_smoothing  = FALSE;
    jdecomp->dither_mode         = JDITHER_NONE;
    jdecomp->two_pass_quantize   = FALSE;
#if 0
    jdecomp.quantize_colors     = TRUE;
#endif

    return CAM_SUCCESS;
}


#if 0

WG_PRIVATE cam_status
alloc_jrows(struct jpeg_decompress_struct *jdecomp, Wg_image *img)
{
    JSAMPROW *row_array = NULL;
    wg_uint  row_size = 0;
    JSAMPLE  *raw_data = NULL;
    JSAMPROW *tmp_raw_array = NULL;
    wg_int   i = 0;

    CHECK_FOR_NULL(jdecomp);
    CHECK_FOR_NULL(img);

    /* allocate memory for arrayf of pointers to rows */
    row_array = WG_CALLOC(jdecomp->output_height, sizeof (JSAMPROW));
    if (NULL == row_array){
        return CAM_FAILURE;
    }

    /* calculate size of a row                      */
    row_size = jdecomp->output_width * jdecomp->output_components * 
        sizeof (JSAMPLE);

    /* allocate memory for decompressed image       */
    raw_data = WG_CALLOC(jdecomp->output_height, row_size);
    if (NULL == row_array){
        WG_FREE(row_array);
        return CAM_FAILURE;
    }

    /* fill an array of pointers to rows            */
     tmp_raw_array = row_array;
    *tmp_raw_array++ = raw_data;
    for (i = 1; i < jdecomp->output_height; ++i){
        *tmp_raw_array = *(tmp_raw_array - 1) + row_size;
        ++tmp_raw_array;
    }

    /* return image parameters to te caller         */
    img->image                =  raw_data;
    img->width                = jdecomp->output_width;
    img->height               = jdecomp->output_height;
    img->size                 = jdecomp->output_height * row_size;
    img->components_per_pixel = jdecomp->output_components;
    img->rows                 = row_array;
    img->row_distance         = row_size;

    return CAM_SUCCESS;
}

#endif

WG_PRIVATE cam_status
init_source_manager(Wg_src_mgr *mgr, wg_uchar *buffer, wg_ssize size)
{
    struct jpeg_source_mgr *parent_mgr = NULL;

    parent_mgr = &(mgr->parent_mgr);

    memset(mgr, '\0', sizeof (Wg_src_mgr));

    mgr->buffer  = buffer;
    mgr->size = size;

    parent_mgr->init_source       = init_source;
    parent_mgr->fill_input_buffer = fill_input_buffer;
    parent_mgr->skip_input_data   = skip_input_data;
    parent_mgr->resync_to_restart = resync_to_restart;
    parent_mgr->term_source       = term_source;

    return CAM_SUCCESS;
}

WG_PRIVATE void
init_source (j_decompress_ptr cinfo)
{
    Wg_src_mgr *mgr = NULL;

    mgr = (Wg_src_mgr*)cinfo->src;

    mgr->parent_mgr.next_input_byte = mgr->buffer;
    mgr->parent_mgr.bytes_in_buffer = mgr->size;

    return;
}

WG_PRIVATE boolean
fill_input_buffer (j_decompress_ptr cinfo)
{
    return FALSE;
}

WG_PRIVATE void
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
    Wg_src_mgr *mgr = NULL;

    mgr = (Wg_src_mgr*)cinfo->src;

    mgr->parent_mgr.next_input_byte += num_bytes;
    mgr->parent_mgr.bytes_in_buffer -= num_bytes;

    return;
}

WG_PRIVATE boolean
resync_to_restart (j_decompress_ptr cinfo, int desired)
{
    return jpeg_resync_to_restart(cinfo, desired);
}

WG_PRIVATE void
term_source (j_decompress_ptr cinfo)
{
    return;
}

WG_PRIVATE cam_status
set_source_manager(struct jpeg_decompress_struct *jdecomp, Wg_src_mgr *mgr)
{
    CHECK_FOR_NULL_PARAM(jdecomp);
    CHECK_FOR_NULL_PARAM(mgr);

    jdecomp->src = &mgr->parent_mgr;

    return CAM_SUCCESS;
}

WG_PRIVATE void
error_exit(j_common_ptr cinfo)
{
    Wg_src_err *err = NULL;

    err = (Wg_src_err*)cinfo->err;

    err->parent_err.output_message(cinfo);

    longjmp(err->exit_point, 1);

    return;
}



/*! @} */
