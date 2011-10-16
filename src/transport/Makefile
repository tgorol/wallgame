MODULE_NAMES=unix_socket

include $(BUILD_PATH)/env.mk

.PHONY: $(MODULE_NAMES) lib cleanall

all: clean lib
	
lib: $(MODULE_NAMES)

unix_socket: mkdir
	make OUT_DIR=$(OUT_DIR) OUT_NAME=$(OUT_NAME) -C $@ lib

clean: cleanall

cleanall:
	for i in $(MODULE_NAMES) ;do        \
		make -C $$i clean;              \
	done

include $(BUILD_PATH)/build.mk
