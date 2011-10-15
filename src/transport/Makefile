include $(BUILD_PATH)/env.mk

MODULE_NAMES=unix_socket

.PHONY: $(MODULE_NAMES) lib cleanall

all: clean module
	
module: mkdir $(MODULE_NAMES)

unix_socket:
	make -C $@ 
	@echo "Copy $@ to output directory"
	cp $@/build/lib/*.o $(OUT_DIR)/

clean: cleanall

cleanall:
	for i in $(MODULE_NAMES) ;do        \
		make -C $$i clean;              \
	done

include $(BUILD_PATH)/build.mk
