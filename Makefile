##### Available defines for IPFOREST_CFLAGS #####
##
## ENABLE_IPFOREST_GLOBAL: register global ipforest name

##### Build defaults #####
LUA_VERSION =       5.1
TARGET =            ipforest.so
PREFIX =            /usr
#CFLAGS =            -g -Wall -pedantic -fno-inline
CFLAGS =            -g -O0 -Wall -pedantic -DNDEBUG
IPFOREST_CFLAGS =   -fpic
IPFOREST_LDFLAGS =  -shared
LUA_INCLUDE_DIR =   $(PREFIX)/include
LUA_CMODULE_DIR =   $(PREFIX)/lib/lua/$(LUA_VERSION)
LUA_MODULE_DIR =    $(PREFIX)/share/lua/$(LUA_VERSION)
LUA_BIN_DIR =       $(PREFIX)/bin

## Linux

## FreeBSD
#LUA_INCLUDE_DIR =  $(PREFIX)/include/lua51

## MacOSX (Macports)
#PREFIX =           /opt/local
#IPFOREST_LDFLAGS = -bundle -undefined dynamic_lookup

## Solaris
#CC           =     gcc
#IPFOREST_CFLAGS =  -fpic -DUSE_INTERNAL_ISINF

## Windows (MinGW)
#TARGET =           ipforest.dll
#PREFIX =           /home/user/opt
#IPFOREST_CFLAGS =  -DDISABLE_INVALID_NUMBERS
#IPFOREST_LDFLAGS = -shared -L$(PREFIX)/lib -llua51
#LUA_BIN_SUFFIX =   .lua

##### End customisable sections #####

EXECPERM =          755

BUILD_CFLAGS =      -I$(LUA_INCLUDE_DIR) $(IPFOREST_CFLAGS)
OBJS =              lua_ipforest.o ipforest_radix_tree.o ipforest_parser.o

.PHONY: all clean install test

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(BUILD_CFLAGS) -o $@ $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(IPFOREST_LDFLAGS) -o $@ $(OBJS)

install: $(TARGET)
	mkdir -p $(DESTDIR)/$(LUA_CMODULE_DIR)
	cp $(TARGET) $(DESTDIR)/$(LUA_CMODULE_DIR)
	chmod $(EXECPERM) $(DESTDIR)/$(LUA_CMODULE_DIR)/$(TARGET)

clean:
	rm -f *.o $(TARGET)

test: all
	lunit -i /usr/bin/luajit test_ipforest.lua
