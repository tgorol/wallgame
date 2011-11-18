OUT_DIR?=$(CURDIR)/build/lib/
CPU_ARCH?=i386
C_STD?=c99

ifndef OUT_NAME
OUT_NAME=lib
endif

ifndef VERSION
	VERSION="N/A"
endif

ifndef AUTHOR
	AUTHOR="Unknown"
endif


INCLUDE+= /opt/include/               \
          $(ROOT_DIR)/src/include/    

LIB+= /opt/lib/      \
		  

LIBLIST+= ini      \
		  readline \
		  ghthash  \
		  pthread  \
		  dl       \
		  jpeg

ifneq "$(strip $(INCLUDE))" ""
	INC_PATH=$(foreach inc, $(INCLUDE), -I$(inc))
else
	INC_PATH=""
endif

ifneq "$(strip $(LIB))" ""
	LIB_PATH=$(foreach lib, $(LIB), -L$(lib))
else
	LIB_PATH=""
endif

ifneq "$(strip $(LIBLIST))" ""
	LIB_LIST=$(foreach lib, $(LIBLIST), -l$(subst lib,,$(basename $(lib))))
else
	LIB_LIST=""
endif

# Create object names
OBJ=$(foreach file, $(SOURCE), $(basename $(file)).o)

# Add archiceture
CFLAGS+=-march=$(CPU_ARCH) -std=$(C_STD) -DAUTHOR=$(AUTHOR) -DVERSION=$(VERSION)

ifeq ($(BUILD_TYPE),  RELEASE)
	CFLAGS+=-O2  -Wall -Werror
else
	CFLAGS+=-g  -Wall -Werror
endif
