
SOURCE= ef_engine.c            \
        sensor.c               \
        gui_work.c             \
        gui_prim.c             \
        collision_detect.c     \
        gui_progress_dialog.c  \
        wg_cam_calibrate.c     \
        wg_plugin.c            \
        gui_display.c          \
        wg_config.c


APP_NAME=webcam

L_LIB_NAME=webcam

OUT_NAME=lib${L_LIB_NAME}

MAIN_C=wg_cam_main.c

INCLUDE=./include/

EXTRA_CFLAGS+=-D_GNU_SOURCE `pkg-config --cflags gtk+-3.0` \
			  -DGTK_DISABLE_DEPRECATED=1                   \
              -Wl,--export-dynamic

LIB_LIST=-l${L_LIB_NAME} -lwg -ljpeg -lm -lgthread-2.0 \
         `pkg-config --libs gtk+-3.0`

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: lib app

include $(BUILD_PATH)/build.mk
