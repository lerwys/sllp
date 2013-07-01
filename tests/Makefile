CC = gcc
CFLAGS = -Wall
INCLUDE_DIRS = -I. -I../libsllpserver -I../libsllpclient

sllpserver_LIB = -lsllpserver 
sllpclient_LIB = -lsllpclient
LIBS = $(sllpserver_LIB) $(sllpclient_LIB)
LIBS_DIR = -L../

.SECONDEXPANSION:

# Test's application names. Add new tests here!
TESTS = test_server

# Generated output tests
TESTS_OUT = $(addsuffix .static, $(TESTS))
TESTS_OUT += $(addsuffix .dynamic, $(TESTS))

# Add test source files in a new variable. Add source files here!
test_server_SRCS = test_server.c

OUT = $(TESTS_OUT)

all: $(OUT) 

%.static: $(patsubst %.c, %.o, $$($$*_SRCS))
	$(CC) $(CFLAGS) -static $(INCLUDE_DIRS) $? -o $@ $(LIBS_DIR) $(LIBS) 

%.dynamic: $(patsubst %.c, %.o, $$($$*_SRCS))
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) $? -o $@ $(LIBS_DIR) $(LIBS) 

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $*.c -o $@ $(LIBS_DIR) $(LIBS) 

.PHONY: all clean

clean:
	rm -f *.o $(TESTS_OUT)

