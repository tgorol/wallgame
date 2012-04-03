
SOURCE= cam.c                 \
        cam_cap.c             \
        cam_format_selector.c \
        cam_frame.c           \
        cam_output.c          \
        cam_readwrite.c       \
        cam_streaming.c

INCLUDE=./include 

OUT_NAME=cam

EXTRA_CFLAGS+=-D_GNU_SOURCE `pkg-config --cflags gtk+-3.0` \
			  -DGTK_DISABLE_DEPRECATED=1                   \

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: clean lib

include $(BUILD_PATH)/build.mk
