# Set your board here
BOARD =

# Set your cross compile prefix with CROSS_COMPILE variable
CROSS_COMPILE ?=

CMDSEP = ;

CC =		$(CROSS_COMPILE)gcc
AR =		$(CROSS_COMPILE)ar
LD =		$(CROSS_COMPILE)ld
OBJDUMP =	$(CROSS_COMPILE)objdump
OBJCOPY =	$(CROSS_COMPILE)objcopy
SIZE =		$(CROSS_COMPILE)size
MAKE =		make

INSTALL_DIR ?= /usr/lib
export INSTALL_DIR

# Config variables suitable for creating shared libraries
LIB_VER = 1.0.0

# General C flags
CFLAGS = -Wall

# Debug flags -D<flasg_name>=<value>
CFLAGS_DEBUG = -g

# Specific platform Flags
CFLAGS_PLATFORM = 
LDFLAGS_PLATFORM =

# General library flags -L<libdir>
LFLAGS = 

# Specific platform objects
OBJS_PLATFORM = 

# Include other Makefiles as needed here
include libsllpserver/libsllpserver.mk
include libsllpclient/libsllpclient.mk

# Include directories
INCLUDE_DIRS = -Iinclude   \
	-I.

# Merge all flags. Optimize for size (-Os)
CFLAGS += $(CFLAGS_PLATFORM) $(INCLUDE_DIRS) \
	$(CFLAGS_DEBUG) -Os

LDFLAGS = $(LDFLAGS_PLATFORM) \
	-ffunction-sections -fdata-sections -Wl,--gc-sections \
	-Os -Iinclude

# Output library names
LIBSERVER = libsllpserver
LIBCLIENT = libsllpclient
OUT = $(LIBSERVER) $(LIBCLIENT)

.SECONDEXPANSION:

# Objects common for both server and client libraries
common_OBJS = $(OBJS_PLATFORM) $(OBJS_BOARD)

# Objects for each version of library
$(LIBSERVER)_OBJS = $(common_OBJS) $($(LIBSERVER)_OBJS_LIB)
$(LIBCLIENT)_OBJS = $(common_OBJS) $($(LIBCLIENT)_OBJS_LIB)

# Save a git repository description
REVISION = $(shell git describe --dirty --always)
REVISION_NAME = revision
OBJ_REVISION = $(addsuffix .o, $(REVISION_NAME))

OBJS_all = $(common_OBJS) $($(LIBSERVER)_OBJS_LIB) \
	$($(LIBCLIENT)_OBJS_LIB) \
	$(OBJ_REVISION)

# Libraries suffixes
LIB_STATIC_SUFFIX = .a
LIB_SHARED_SUFFIX = .so

# Generate suitable names for static libraries
TARGET_STATIC = $(addsuffix $(LIB_STATIC_SUFFIX), $(OUT))
TARGET_SHARED = $(addsuffix $(LIB_SHARED_SUFFIX), $(OUT))
TARGET_SHARED_VER = $(addsuffix $(LIB_SHARED_SUFFIX).$(LIB_VER), $(OUT))

.PHONY: all clean mrproper install uninstall tests

# Avoid deletion of intermediate files, such as objects
.SECONDARY: $(OBJS_all)

# Makefile rules 
all: $(TARGET_STATIC) $(TARGET_SHARED_VER)

# Compile static library. libsslpclient.a libsslpserver.a
%.a: $$($$*_OBJS) $(OBJ_REVISION)
	$(AR) rcs $@ $?
	$(SIZE) $@

# Compile dynamic library. libsslpclient.so libsslpserver.so
%.so.$(LIB_VER): $$($$*_OBJS) $(OBJ_REVISION)
	$(CC) -shared -fPIC -Wl,-soname,$@ -o $@ $? $(LDFLAGS)
	$(SIZE) $@

$(REVISION_NAME).o: $(REVISION_NAME).c
	$(CC) $(CFLAGS) -DGIT_REVISION=\"$(REVISION)\" -c $<
	$(SIZE) -t $@
	
# Pull in dependency info for *existing* .o files and don't complain if the
# corresponding .d file is not found
-include $(OBJS_all:.o=.d)

# Compile with position-independent objects.
# Autodependencies generatation by Scott McPeak, November 2001,
# from article "Autodependencies with GNU make"
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c -fPIC $*.c -o $@
	
# create the dependency files "target: pre-requisites"
	${CC} -MM $(CFLAGS) $*.c > $*.d
	
# Workaround to make objects in different folders have
# the correct target path. e.g., "dir/bar.o: dir/bar.c dir/foo.h"
# instead of "bar.o: dir/bar.c dir/foo.h"
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|$*.o:|' < $*.d.tmp > $*.d

# All prereqs listed will also become command-less,
# prereq-less targets. In this way, the prereq file will be
# treated as changed and the target will be rebuilt
#   sed:    strip the target (everything before colon)
#   sed:    remove any continuation backslashes
#   fmt -1: list words one per line
#   sed:    strip leading spaces
#   sed:    add trailing colons
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		sed -e 's/^ *//' -e 's/$$/:/' >> $*.d
	@rm -f $*.d.tmp

tests:
	$(MAKE) -C $@ all

install:
	@install -m 755 $(TARGET_SHARED_VER) $(INSTALL_DIR)	
	$(foreach lib,$(TARGET_SHARED),ln -sf $(lib).$(LIB_VER) $(INSTALL_DIR)/$(lib) $(CMDSEP))

uninstall:
	$(foreach lib,$(TARGET_SHARED),rm -f $(INSTALL_DIR)/$(lib).$(LIB_VER) $(CMDSEP))
	$(foreach lib,$(TARGET_SHARED),rm -f $(INSTALL_DIR)/$(lib) $(CMDSEP))

clean:
	rm -f $(OBJS_all) $(OBJS_all:.o=.d)
	$(MAKE) -C tests clean

mrproper: clean
	rm -f *.a *.so.$(LIB_VER)
