APP_NAME=dummy_plug

MAIN_C=test_plugin.c          \

INCLUDE=./include/

EXTRA_CFLAGS+=-D_GNU_SOURCE

LIB_LIST= -lwg

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: app

include $(BUILD_PATH)/build.mk
