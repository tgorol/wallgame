
SOURCE=wg_cam.c                  \
	   wg_cam_cap.c              \
	   wg_cam_output.c           \
	   wg_cam_readwrite.c        \
	   wg_cam_img_jpeg.h         \
	   wg_cam_streaming.c        \
	   wg_cam_frame.c            \
	   wg_cam_img_yuyv.c         \
	   wg_cam_format_selector.c 

APP_NAME=webcam

MAIN_C=wg_cam_main.c

INCLUDE=./include/

EXTRA_CFLAGS+=-D_GNU_SOURCE `pkg-config --cflags --libs gtk+-2.0`

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

LIB_LIST+=./build/lib/lib.a ../build/libwg.a -ljpeg

all: clean lib app

include $(BUILD_PATH)/build.mk
