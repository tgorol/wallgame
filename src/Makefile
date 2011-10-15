OUT_DIR=$(CURDIR)/build/
APP_NAME=wall_game

LIB=$(OUT_DIR)

EXTRA_CFLAGS+=

include $(BUILD_PATH)/env.mk

MODULE_NAMES=main gameplay config common transport

.PHONY: $(MODULE_NAMES) app

all: clean mkdir $(MODULE_NAMES) app
	
gameplay:
	make -C $@ 
	@echo "Copy $@ to output directory"
	cp $@/build/lib/*.o $(OUT_DIR)/

config:
	make -C $@ 
	@echo "Copy $@ to output directory"
	@cp $@/build/lib/*.o $(OUT_DIR)/

common:
	make -C $@ 
	@echo "Copy $@ to output directory"
	@cp $@/build/lib/*.o $(OUT_DIR)/

main:
	make -C $@ 
	@echo "Copy $@ to output directory"
	@cp $@/build/lib/*.o $(OUT_DIR)/

transport:
	make -C $@ 
	@echo "Copy $@ to output directory"
	@cp $@/build/lib/*.o $(OUT_DIR)/

clean: cleanall

cleanall:
	for i in $(MODULE_NAMES) ;do        \
		make -C $$i clean;              \
	done


include $(BUILD_PATH)/build.mk
