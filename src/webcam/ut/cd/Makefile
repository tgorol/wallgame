APP_NAME=unit_test
SOURCE=unit_test.c
OUT_NAME=libut

INCLUDE=$(ROOT_DIR)/src/ut/include/ $(ROOT_DIR)/src/ ../../

LIBLIST+=$(OUT_NAME)  webcam

LIB+=$(OUT_DIR) $(ROOT_DIR)/src/build/ ../../build/lib 

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG 
endif

include $(BUILD_PATH)/env.mk

EXTRA_CFLAGS+=-L$(OUT_DIR)

all: clean lib app

include $(BUILD_PATH)/build.mk
