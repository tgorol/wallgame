
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
       img_draw.c         \
       ef_engine.c    \
       vid.c            \
       sensor.c          \
       gui_work.c        \
       gui_prim.c       \
       collision_detect.c \
       gui_progress_dialog.c  \
       wg_cam_calibrate.c  \
       wg_plugin.c       \
       gui_display.c      \
       wg_config.c


APP_NAME=webcam

MAIN_C=wg_cam_main.c

OUT_NAME=libwebcam

INCLUDE=./include/

EXTRA_CFLAGS+=-D_GNU_SOURCE `pkg-config --cflags gtk+-3.0` \
			  -DGTK_DISABLE_DEPRECATED=1                   \
              -Wl,--export-dynamic

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

LIB_LIST+=./build/lib/$(OUT_NAME).a ../build/libwg.a -ljpeg -lm -lgthread-2.0 `pkg-config --libs gtk+-3.0` -lavcodec -lavutil -lswscale

all: clean lib app

include $(BUILD_PATH)/build.mk
