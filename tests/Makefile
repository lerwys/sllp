CC = gcc
CFLAGS = -Wall
INCLUDE_DIRS = -I. -I../libsllpserver -I../libsllpclient

# If not defined by top make or if user called this directly
INSTALL_DIR ?= /usr/lib
LIBS_DIR = -L$(INSTALL_DIR) -L../

.SECONDEXPANSION:

# Test's application names. Add new tests here!
TESTS = test_server

# Generated output tests
TESTS_OUT = $(addsuffix .static, $(TESTS))
TESTS_OUT += $(addsuffix .dynamic, $(TESTS))

# Add test source files in a new variable! It must have the
# same name as specified in TESTS variable. Follow test_server example
test_server_SRCS = test_server.c
# Add test source files in a new variable! It must have the
# same name as specified in TESTS variable. Follow test_server example
test_server_LIBS = -lsllpserver

OUT = $(TESTS_OUT)

all: $(OUT) 

%.static: $(patsubst %.c, %.o, $$($$*_SRCS))
	$(CC) $(CFLAGS) -static $(INCLUDE_DIRS) $? -o $@ $(LIBS_DIR) $($*_LIBS)

%.dynamic: $(patsubst %.c, %.o, $$($$*_SRCS))
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $? -o $@ $(LIBS_DIR) $($*_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $*.c -o $@

.PHONY: all clean

clean:
	rm -f *.o $(TESTS_OUT)

