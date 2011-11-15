
SOURCE=wg_cam.c wg_cam_cap.c wg_cam_image.c

APP_NAME=webcam

MAIN_C=wg_cam_main.c

INCLUDE=./include/

EXTRA_CFLAGS+=-D_GNU_SOURCE

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

LIB_LIST+=./build/lib/lib.a ../build/libwg.a

all: clean lib app

include $(BUILD_PATH)/build.mk
