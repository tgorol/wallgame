
SOURCE= img_bgrx.c  \
        img.c       \
        img_draw.c  \
        img_gs.c    \
        img_hsv.c   \
        img_jpeg.c  \
        img_rgb24.c \
        img_yuyv.c

INCLUDE=./include 

OUT_NAME=image

EXTRA_CFLAGS+=-D_GNU_SOURCE `pkg-config --cflags gtk+-3.0` \
			  -DGTK_DISABLE_DEPRECATED=1                   \

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: clean lib

include $(BUILD_PATH)/build.mk
