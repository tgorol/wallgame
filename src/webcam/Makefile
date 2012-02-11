
SOURCE=cam.c                  \
	   cam_cap.c              \
	   cam_output.c           \
	   cam_readwrite.c        \
	   cam_streaming.c        \
	   cam_frame.c            \
	   cam_format_selector.c  \
	   img.c              \
	   img_jpeg.h         \
	   img_hsv.c          \
	   img_bgrx.c         \
	   img_rgb24.c          \
	   img_gs.c    \
	   img_yuyv.c         \
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

LIB_LIST+=./build/lib/lib.a ../build/libwg.a -ljpeg -lm -lgthread-2.0 `pkg-config --libs gtk+-3.0` -lavcodec -lavutil -lswscale

all: clean lib app

include $(BUILD_PATH)/build.mk
