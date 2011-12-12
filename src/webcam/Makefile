
SOURCE=cam.c                  \
	   cam_cap.c              \
	   cam_output.c           \
	   cam_readwrite.c        \
	   cam_img_jpeg.h         \
	   cam_streaming.c        \
	   cam_frame.c            \
	   cam_img_yuyv.c         \
	   cam_img.h              \
	   cam_format_selector.c 

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
