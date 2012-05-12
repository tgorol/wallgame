OUT_DIR=$(CURDIR)/build/
APP_NAME=wall_game
MAIN_C=main/main.c
OUT_NAME=libwg

LIB=$(OUT_DIR)

EXTRA_CFLAGS+=-L$(OUT_DIR)/

LIBLIST+=$(OUT_NAME)

include $(BUILD_PATH)/env.mk

MODULE_NAMES=gameplay config common transport tools image cam         \
             plugin_tools

PLUG_INS=plugins webcam


.PHONY: $(MODULE_NAMES) app $(PLUG_INS)

all: clean mkdir modules app plugins

plugins: $(PLUG_INS)

modules: $(MODULE_NAMES)
	
gameplay:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

config:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

common:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

transport:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

sensor:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

tools:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

image:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

cam:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

plugin_tools:
	$(MAKE) OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

plugins:
	$(MAKE) OUT_DIR=$(OUT_DIR) -C $@

webcam:
	$(MAKE) OUT_DIR=$(OUT_DIR) -C $@

clean: cleanall

cleanall:
	for i in $(MODULE_NAMES) ;do        \
		$(MAKE) -C $$i clean;              \
	done


include $(BUILD_PATH)/build.mk
