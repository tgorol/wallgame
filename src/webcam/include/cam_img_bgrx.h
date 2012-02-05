#ifndef CAM_IMG_BGRX_G
#define CAM_IMG_BGRX_G

typedef wg_uint32 bgrx_pixel;

#define BGRX_R_SHIFT   16
#define BGRX_G_SHIFT   8
#define BGRX_B_SHIFT   0

#define RGB_2_BGRX(r, g, b)                   \
    (((wg_uint32)(b) << (BGRX_B_SHIFT)) | ((wg_uint32)(g) << (BGRX_G_SHIFT)) | \
     ((wg_uint32)(r) << (BGRX_R_SHIFT)))


#define BGRX_B(bgrx)                          \
    (((wg_uint32)(bgrx) >> (BGRX_B_SHIFT)) & 0xff)        

#define BGRX_G(bgrx)                          \
    (((wg_uint32)(bgrx) >> (BGRX_G_SHIFT)) & 0xff)        

#define BGRX_R(bgrx)                          \
    (((wg_uint32)(bgrx) >> (BGRX_R_SHIFT)) & 0xff)            

#endif
