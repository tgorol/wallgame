OUT_DIR?=$(CURDIR)/build/lib/
CPU_ARCH?=i686
C_STD?=gnu99

DOXYGEN=doxygen

ifndef OUT_NAME
OUT_NAME=lib
endif

VERSION="1.0"
AUTHOR="Tomasz Gorol"


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
CFLAGS+=-march=$(CPU_ARCH) -std=$(C_STD) -D_REENTRANT 

ifeq ($(BUILD_TYPE),  RELEASE)
	CFLAGS+=-g -Wall -O3 -pedantic
else
	CFLAGS+=-g -Wall -pedantic -DWGDEBUG -DMEMLEAK_CHECK
endif

ifdef PROF
	CFLAGS+=-pg
endif

ifdef MEMLEAK_CHECK
    CFLAGS+=-DMEMLEAK_CHECK
endif
