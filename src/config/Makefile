
SOURCE=cm_init.c \
	   cm_access.c

INCLUDE=./include 

ifdef $(WG_DEBUG)
	EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: clean lib

include $(BUILD_PATH)/build.mk
