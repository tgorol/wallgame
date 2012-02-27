
SOURCE= wg_iterator.c      \
		wg_linked_list.c   \
        wg_string.c        \
		wg_sync_linked_list.c     \
		wg_slab.c          \
		wg_msgpipe.c       \
        wg_lsdir.c         \
        wg_wq.c            \
        wg_sort.c

INCLUDE=./include 

OUT_NAME=common

EXTRA_CFLAGS=

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: clean lib

include $(BUILD_PATH)/build.mk
