
SOURCE=gpm.c          \
	   gpm_cmdln.c    \
	   gpm_ini.c      \
	   gpm_console.c  \
	   gpm_console_parser.c    \
	   gpm_hooks.c             \

INCLUDE=./include/

EXTRA_CFLAGS+=-D_GNU_SOURCE

ifdef WG_DEBUG
EXTRA_CFLAGS+=-DWGDEBUG
endif

include $(BUILD_PATH)/env.mk

all: clean lib

include $(BUILD_PATH)/build.mk
