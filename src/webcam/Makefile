
SOURCE=cam.c                  \
	   cam_cap.c              \
	   cam_output.c           \
	   cam_readwrite.c        \
	   cam_streaming.c        \
	   cam_frame.c            \
	   cam_img_yuyv.c         \
	   cam_format_selector.c  \
	   cam_img.c              \
	   cam_img_jpeg.h         \
	   cam_img_hsv.c          \
	   cam_img_bgrx.c         \
	   cam_img_rgb.c          \
	   cam_img_grayscale.c    \
	   gui_camera.c           \
	   gui_screen.c           \
           extraction_engine.c

APP_NAME=webcam

MAIN_C=wg_cam_main.c

INCLUDE=./include/

EXTRA_CFLAGS+=-D_GNU_SOURCE `pkg-config --cflags gtk+-3.0` \
			  -DGTK_DISABLE_DEPRECATED=1

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

LIB_LIST+=./build/lib/lib.a ../build/libwg.a -ljpeg -lm -lgthread-2.0 `pkg-config --libs gtk+-3.0`

all: clean lib app

include $(BUILD_PATH)/build.mk
