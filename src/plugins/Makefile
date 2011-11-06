OUT_NAME=libdummy

SOURCE=test_plugin.c          \

INCLUDE=./include/

EXTRA_CFLAGS+=-D_GNU_SOURCE -fPIC

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: shared

include $(BUILD_PATH)/build.mk
