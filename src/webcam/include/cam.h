#ifndef _CAM_H
#define _CAM_H


#define DEV_PATH_MAX   64

/*! Number of buffers to allocate for streaming mode */
#define DEFAULT_BUFFER_NUM 4


enum {CAM_FMT_CAPTURE, CAM_FMT_NUM};

/**
 * @brief Camera modes
 */
typedef enum CAM_MODE {
    /* impl. decision: CAM_MODE_INVALID must be equal to 0 */
    CAM_MODE_INVALID     = 0,       /*!< Invalid mode   */
    CAM_MODE_STREAMING      ,       /*!< Streaming mode */
    CAM_MODE_READWRITE      ,       /*!< Read mode      */
    CAM_MODE_UNKNOWN                /*!< Unknown mode   */
}CAM_MODE;

typedef enum CAM_FLAGS {
    ENABLE_DECOMPRESSOR = 1<<0
}CAM_FLAGS;

#define IS_FLAG_SET(flag, flag_name)          \
    ((flag) & (flag_name))

/**
 * @brief Status codes returned by the Camera module
 */
typedef enum cam_status{
    CAM_SUCCESS = WG_SUCCESS ,     /*!< Success               */
    CAM_FAILURE = WG_FAILURE ,     /*!< Failure               */
    CAM_BUSY    = 10         ,     /*!< Busy                  */
    CAM_INVAL                ,     /*!< Invalid value         */
    CAM_IO                   ,     /*!< Input/Output error    */
    CAM_TIMEOUT              ,     /*!< Timeout error         */
    CAM_NO_SUPPORT                 /*!< Feature not supported */
}cam_status;

typedef enum cam_state{
    CAM_STATE_UNINIT = 0,
    CAM_STATE_STOP      ,
    CAM_STATE_START     
}cam_state;

/**
 * @brief Camera buffer
 */
typedef struct Wg_cam_buf{
        void   *start;     /*!< Start of the buffer         */
        size_t length;     /*!< Number of bytes in a buffer */
}Wg_cam_buf;

typedef struct Wg_camera Wg_camera;
typedef struct Wg_frame Wg_frame;

typedef wg_uint8 gray_pixel;


/**
 * @brief Camera operations
 *
 * Callback from this structure are called by the driver
 */
typedef struct Wg_cam_ops{
        cam_status (*close)(Wg_camera *cam);
        /*!< on close */

        cam_status (*open)(Wg_camera *cam);
        /*!< on open  */

        cam_status (*start)(Wg_camera *cam);
        /*!< on start */

        cam_status (*stop)(Wg_camera *cam);
        /*!< on stopt */

        cam_status (*read)(Wg_camera *cam, Wg_frame *frame); 
        /*!< on read */

        cam_status (*cleanup_frame)(Wg_camera *cam, Wg_frame *frame);
        /*!< on releasing a frame buffer */

        cam_status (*empty_frame)(Wg_camera *cam, Wg_frame *frame);
        /*!< on marking a buffer as empty */
}Wg_cam_ops;

typedef enum img_type {
    IMG_INVALI  ,
    IMG_RGB     ,
    IMG_BGRX    ,
    IMG_YUYV    ,
    IMG_HSV     ,
    IMG_GS      ,
    IMG_USER
} img_type;

/**
 * @brief Represents an image returned by a decompressor
 */
typedef struct Wg_image{
    img_type type;         /*!< type of image                          */
    wg_uchar **rows;       /*!< array of rows                          */
    wg_uchar *image;       /*!< start of the image                     */
    wg_ssize size;         /*!< number of bytes in the image           */
    wg_uint  width;        /*!< width in pixels                        */
    wg_uint  height;       /*!< height in pixels                       */
    wg_uint  row_distance; /*!< distanse in bytes between rows         */
    wg_uint  components_per_pixel; /*!< number of components per pixel */
}Wg_image;

typedef cam_status
    (*cam_decomp)(wg_uchar *in_buffer, wg_ssize in_size, 
            wg_uint width, wg_uint height, Wg_image *img);

/**
 * @brief Decompressor
 */
typedef struct Wg_cam_decompressor{
    cam_decomp run;  /*!< pointer to a decompressor function */
}Wg_cam_decompressor;


/**
 * @brief Camera
 */
struct Wg_camera{
    CAM_MODE mode;                         /*!< Working mode              */
    cam_state state;                       /*!< camera state              */
    char dev_path[DEV_PATH_MAX + 1];       /*!< Device path               */
    int fd_cam;                            /*!< Device handler            */
    struct v4l2_capability cap;            /*!< Capabilities              */
    struct v4l2_format fmt[CAM_FMT_NUM];   /*!< Selected format           */
    Wg_cam_buf *buffers;                   /*!< Buffers used by streaming */
    size_t buf_num;                        /*!< Number of buffers         */
    Wg_cam_ops cam_ops;                    /*!< Camera operations         */
    Wg_cam_decompressor dcomp;             /*!< Selected decompressor     */
};

/* 
 * Inline functions
 */

inline static cam_status invoke_decompressor(
        Wg_cam_decompressor *decomp,
        wg_uchar *in_buffer, wg_ssize in_size, 
        wg_uint width, wg_uint height, Wg_image *img)
{
    CHECK_FOR_NULL_PARAM(decomp);

    return decomp->run(in_buffer, in_size, width, height, img);
}


WG_PUBLIC 
cam_status cam_init(Wg_camera *cam, const wg_char* dev_path);

WG_PUBLIC
cam_status cam_open(Wg_camera *cam, CAM_MODE mode, wg_uint flags);

WG_PUBLIC 
cam_status cam_close(Wg_camera *cam);

WG_PUBLIC 
cam_status cam_free_frame(Wg_camera *cam, Wg_frame *frame);

WG_PUBLIC 
cam_status cam_discard_frame(Wg_camera *cam, Wg_frame *frame);

WG_PUBLIC 
cam_status cam_read(Wg_camera *cam, Wg_frame *frame);

WG_PUBLIC 
cam_status cam_start(Wg_camera *cam);

WG_PUBLIC 
cam_status cam_stop(Wg_camera *cam);

WG_PUBLIC cam_status
cam_decompressor(Wg_camera *cam, Wg_cam_decompressor *dcomp);

WG_PUBLIC cam_status
img_rgb_2_hsv(Wg_image *rgb_img, Wg_image *hsv_img);

WG_PUBLIC cam_status
img_rgb_2_hsv_fast(Wg_image *rgb_img, Wg_image *hsv_img);

WG_PUBLIC cam_status
img_rgb_2_bgrx(Wg_image *rgb_img, Wg_image *bgrx_img);

WG_PUBLIC cam_status
img_rgb_2_hsv_gtk(Wg_image *rgb_img, Wg_image *hsv_img);

WG_PUBLIC wg_status
img_rgb_2_grayscale(Wg_image *rgb_img, Wg_image *grayscale_img);

WG_PUBLIC wg_status
img_grayscale_2_rgb(Wg_image *grayscale_img, Wg_image *rgb_img);

WG_PUBLIC wg_status
img_grayscale_max_min(Wg_image* grayscale_img, gray_pixel *gs_max,
        gray_pixel *gs_min);

WG_PUBLIC wg_status
img_grayscale_normalize(Wg_image* grayscale_img, gray_pixel new_max,
        gray_pixel new_min);

WG_PUBLIC cam_status
img_grayscale_histogram(Wg_image* grayscale_img, wg_int *histogram, 
        wg_size size);

WG_PUBLIC cam_status
img_grayscale_save(Wg_image *img, wg_char *filename, wg_char *ext);

WG_PUBLIC cam_status
ef_acc_get_max(Wg_image *acc, wg_uint *row_par, wg_uint *col_par);

cam_status
ef_paint_cross(Wg_image *img, wg_uint y, wg_uint x, gray_pixel color);

WG_PUBLIC cam_status
cam_set_resolution(Wg_camera *cam, wg_uint width, wg_uint height);

WG_PUBLIC cam_status
cam_get_resolution(Wg_camera *cam, wg_uint *width, wg_uint *height);


#endif
