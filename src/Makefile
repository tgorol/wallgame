OUT_DIR=$(CURDIR)/build/
APP_NAME=wall_game
MAIN_C=main/main.c
OUT_NAME=libwg

LIB=$(OUT_DIR)

EXTRA_CFLAGS+=-L$(OUT_DIR)/

LIBLIST+=$(OUT_NAME)

include $(BUILD_PATH)/env.mk

MODULE_NAMES=gameplay config common transport sensor tools image cam         \
             plugin_tools

PLUG_INS=webcam plugins


.PHONY: $(MODULE_NAMES) app $(PLUG_INS)

all: clean mkdir modules app plugins

plugins: $(PLUGINS)

modules: $(MODULE_NAMES)
	
gameplay:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

config:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

common:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

transport:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

sensor:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

tools:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

image:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

cam:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

plugin_tools:
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

plugins:
	make OUT_DIR=$(OUT_DIR) -C $@



#webcam:
#	make OUT_DIR=$(OUT_DIR) -C $@


clean: cleanall

cleanall:
	for i in $(MODULE_NAMES) ;do        \
		make -C $$i clean;              \
	done


include $(BUILD_PATH)/build.mk
