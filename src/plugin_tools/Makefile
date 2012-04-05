
SOURCE= wg_plugin_tools.c

INCLUDE=./include 

OUT_NAME=plugin_tools

EXTRA_CFLAGS=

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: clean lib

include $(BUILD_PATH)/build.mk
