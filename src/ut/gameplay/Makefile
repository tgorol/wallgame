APP_NAME=unit_test
SOURCE=gpm_game.c
OUT_NAME=libut

INCLUDE=$(ROOT_DIR)/src/ut/include/ $(ROOT_DIR)/src/

LIBLIST+=$(OUT_NAME) wg

LIB+=$(OUT_DIR) $(ROOT_DIR)/src/build/

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG 
endif

include $(BUILD_PATH)/env.mk

EXTRA_CFLAGS+=-L$(OUT_DIR)

all: clean lib app

include $(BUILD_PATH)/build.mk
