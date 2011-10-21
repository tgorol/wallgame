
SOURCE= wg_iterator.c      \
		wg_linked_list.c   \
        wg_string.c        \
		wg_workqueue.c     \
		wg_slab.c

INCLUDE=./include 

OUT_NAME=common

EXTRA_CFLAGS=-D_REENTRANT

include $(BUILD_PATH)/env.mk

all: clean lib

include $(BUILD_PATH)/build.mk
