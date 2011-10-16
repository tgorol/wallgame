OUT_DIR=$(CURDIR)/build/
APP_NAME=wall_game
MAIN_C=main/main.c
OUT_NAME=libwg

LIB=$(OUT_DIR)

EXTRA_CFLAGS+=-L$(OUT_DIR)/

LIBLIST+=$(OUT_NAME)

include $(BUILD_PATH)/env.mk

MODULE_NAMES=gameplay config common transport

.PHONY: $(MODULE_NAMES) app

all: clean mkdir $(MODULE_NAMES) app
	
gameplay:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

config:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

common:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

transport:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

clean: cleanall

cleanall:
	for i in $(MODULE_NAMES) ;do        \
		make -C $$i clean;              \
	done


include $(BUILD_PATH)/build.mk
