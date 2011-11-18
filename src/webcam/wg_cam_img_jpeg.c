#include <stdio.h>
#include <sys/types.h>
#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <jpeglib.h>

#include <linux/videodev2.h>

#include "include/wg_cam.h"

#define LINES_PER_READ 32


typedef struct Wg_src_mgr{
    /* This field must must be first in the structure */
    struct jpeg_source_mgr parent_mgr;

    wg_uchar *buffer;
    wg_ssize  size;

}Wg_src_mgr;

WG_PRIVATE wg_cam_status
set_source_manager(struct jpeg_decompress_struct *jdecomp, Wg_src_mgr* mgr);

WG_PRIVATE wg_cam_status
init_source_manager(Wg_src_mgr *mgr, wg_uchar *buffer, wg_ssize size);
WG_PRIVATE wg_cam_status
alloc_jrows(struct jpeg_decompress_struct *jdecomp, JSAMPARRAY *rows);

WG_PRIVATE void init_source (j_decompress_ptr cinfo);
WG_PRIVATE boolean fill_input_buffer (j_decompress_ptr cinfo);
WG_PRIVATE void skip_input_data (j_decompress_ptr cinfo, long num_bytes);
WG_PRIVATE boolean resync_to_restart (j_decompress_ptr cinfo, int desired);
WG_PRIVATE void term_source (j_decompress_ptr cinfo);

wg_cam_status
wg_cam_img_jpeg_decompress(wg_uchar *in_buffer, wg_ssize in_size,
        wg_uchar **out_buffer, wg_ssize *out_size, wg_uint *width, 
        wg_uint *height)
{
    Wg_src_mgr mgr;
    int status = 0;
    struct jpeg_decompress_struct jdecomp;
    struct jpeg_error_mgr         jerror;
    JSAMPARRAY rows = NULL;
    wg_uint index = 0;
    JDIMENSION readed_lines = 0;

    CHECK_FOR_NULL_PARAM(in_buffer);
    CHECK_FOR_NULL_PARAM(in_size);
    CHECK_FOR_NULL_PARAM(out_buffer);
    CHECK_FOR_NULL_PARAM(out_size);
    CHECK_FOR_NULL_PARAM(width);
    CHECK_FOR_NULL_PARAM(height);

    init_source_manager(&mgr, in_buffer, in_size);

    jdecomp.err = jpeg_std_error(&jerror);

    jpeg_create_decompress(&jdecomp);

    set_source_manager(&jdecomp, &mgr);

    status = jpeg_read_header(&jdecomp, TRUE);
    if (status !=  JPEG_HEADER_OK){
        jpeg_finish_decompress(&jdecomp);
        return WG_CAM_FAILURE;
    }

    jdecomp.do_fancy_upsampling = FALSE;
    jdecomp.do_block_smoothing  = FALSE;
    jdecomp.dither_mode         = JDITHER_NONE;
    jdecomp.two_pass_quantize   = FALSE;
#if 0
    jdecomp.quantize_colors     = TRUE;
#endif

    if (FALSE == jpeg_start_decompress(&jdecomp)){
        jpeg_destroy_decompress(&jdecomp);
        return WG_CAM_FAILURE;
    }

    alloc_jrows(&jdecomp, &rows);

    for (index = 0; jdecomp.output_scanline < jdecomp.output_height;){
        readed_lines = jpeg_read_scanlines(&jdecomp, &rows[index],
                LINES_PER_READ);
        index += readed_lines;
    }

    *out_buffer =  rows[0];
    *width      = jdecomp.output_width;
    *height     = jdecomp.output_height;
    *out_size   = jdecomp.output_components;

    jpeg_finish_decompress(&jdecomp);
    jpeg_destroy_decompress(&jdecomp);

    WG_FREE(rows);

    return WG_CAM_SUCCESS;
}

WG_PRIVATE wg_cam_status
alloc_jrows(struct jpeg_decompress_struct *jdecomp, JSAMPARRAY *rows)
{
    JSAMPROW *row_array = NULL;
    wg_uint  row_size = 0;
    JSAMPLE  *raw_data = NULL;
    wg_int   i = 0;

    CHECK_FOR_NULL(jdecomp);
    CHECK_FOR_NULL(rows);

    row_array = WG_CALLOC(jdecomp->output_height, sizeof (JSAMPROW));
    if (NULL == row_array){
        return WG_CAM_FAILURE;
    }

    row_size = jdecomp->output_width * jdecomp->output_components * 
        sizeof (JSAMPLE);

    raw_data = WG_CALLOC(jdecomp->output_height, row_size);
    if (NULL == row_array){
        WG_FREE(row_array);
        return WG_CAM_FAILURE;
    }

    /** @todo Optimize loop */
    for (i = 0; i < jdecomp->output_height; ++i){
        row_array[i] = &(raw_data[i * row_size]);
    }

    *rows = row_array;

    return WG_CAM_SUCCESS;
}


WG_PRIVATE wg_cam_status
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

    return WG_CAM_SUCCESS;
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
    return TRUE;
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

WG_PRIVATE wg_cam_status
set_source_manager(struct jpeg_decompress_struct *jdecomp, Wg_src_mgr *mgr)
{
    CHECK_FOR_NULL_PARAM(jdecomp);
    CHECK_FOR_NULL_PARAM(mgr);

    jdecomp->src = &mgr->parent_mgr;

    return WG_CAM_SUCCESS;
}
