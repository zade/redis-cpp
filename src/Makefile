# Redis Makefile
# Copyright (C) 2009 Salvatore Sanfilippo <antirez at gmail dot com>
# This file is released under the BSD license, see the COPYING file
#
# The Makefile composes the final FINAL_CFLAGS and FINAL_LDFLAGS using
# what is needed for Redis plus the standard CFLAGS and LDFLAGS passed.
# However when building the dependencies (Jemalloc, Lua, Hiredis, ...)
# CFLAGS and LDFLAGS are propagated to the dependencies, so to pass
# flags only to be used when compiling / linking Redis itself REDIS_CFLAGS
# and REDIS_LDFLAGS are used instead (this is the case of 'make gcov').
#
# Dependencies are stored in the Makefile.dep file. To rebuild this file
# Just use 'make dep', but this is only needed by developers.

release_hdr := $(shell sh -c './mkreleasehdr.sh')
uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
CC := $(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc')

STD=-pedantic -D__STDC_CONSTANT_MACROS
WARN=-Wall
OPTIMIZATION?=-O2
OPT=$(OPTIMIZATION)
DEPENDENCY_TARGETS=hiredis linenoise lua

# Default allocator
ifeq ($(uname_S),Linux)
  MALLOC=jemalloc
else
  MALLOC=libc
endif

CFLAGS=
LDFLAGS= 

REDIS_CFLAGS= 
REDIS_LDFLAGS= 

ifeq ($(uname_S),SunOS)
  DEBUG= -g -ggdb
  FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS) $(REDIS_CFLAGS) -D__EXTENSIONS__ -D_XPG6
  FINAL_LDFLAGS= $(LDFLAGS) $(REDIS_LDFLAGS) -g -ggdb
  FINAL_LIBS= -ldl -lnsl -lsocket -lm -lpthread -lstdc++
else
  DEBUG= -g -rdynamic -ggdb
  FINAL_CFLAGS= $(STD) $(WARN) $(OPT) $(DEBUG) $(CFLAGS) $(REDIS_CFLAGS)
  FINAL_LDFLAGS= $(LDFLAGS) $(REDIS_LDFLAGS) -g -rdynamic -ggdb
  FINAL_LIBS= -lm -pthread -lstdc++
endif

FINAL_CFLAGS+= -I../deps/hiredis -I../deps/linenoise -I../deps/lua/src

ifeq ($(MALLOC),jemalloc)
  DEPENDENCY_TARGETS+= jemalloc
  FINAL_CFLAGS+= -DUSE_JEMALLOC -I../deps/jemalloc/include
  FINAL_LIBS+= ../deps/jemalloc/lib/libjemalloc.a -ldl
endif

PREFIX?=/usr/local
INSTALL_BIN= $(PREFIX)/bin
INSTALL= cp -pf

CCCOLOR="\033[34m"
LINKCOLOR="\033[34;1m"
SRCCOLOR="\033[33m"
BINCOLOR="\033[37;1m"
MAKECOLOR="\033[32;1m"
ENDCOLOR="\033[0m"

QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_INSTALL = @printf '    %b %b\n' $(LINKCOLOR)INSTALL$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;

REDIS_CC=$(QUIET_CC)$(CC) $(FINAL_CFLAGS)
REDIS_LD=$(QUIET_LINK)$(CC) $(FINAL_LDFLAGS)
REDIS_INSTALL=$(QUIET_INSTALL)$(INSTALL)

REDIS_SERVER_NAME= redis-server
REDIS_SENTINEL_NAME= redis-sentinel
#REDIS_SERVER_OBJ= adlist.o ae.o anet.o dict.o redis.o sds.o zmalloc.o lzf_c.o lzf_d.o pqsort.o zipmap.o sha1.o ziplist.o release.o networking.o util.o object.o db.o replication.o rdb.o t_string.o t_list.o t_set.o t_zset.o t_hash.o config.o aof.o pubsub.o multi.o debug.o sort.o intset.o syncio.o migrate.o endianconv.o slowlog.o scripting.o bio.o rio.o rand.o memtest.o crc64.o bitops.o sentinel.o
REDIS_SERVER_OBJ= ae.o anet.o consts.o crc64.o debug.o log.o redis.o release.o rio.o util.o zmalloc.o
REDIS_CLI_NAME= redis-cli
#REDIS_CLI_OBJ= anet.o sds.o adlist.o redis-cli.o zmalloc.o release.o anet.o ae.o
REDIS_CLI_OBJ=
REDIS_BENCHMARK_NAME= redis-benchmark
#REDIS_BENCHMARK_OBJ= ae.o anet.o redis-benchmark.o sds.o adlist.o zmalloc.o redis-benchmark.o
REDIS_BENCHMARK_OBJ=
#REDIS_CHECK_DUMP_NAME= redis-check-dump
REDIS_CHECK_DUMP_NAME=
#REDIS_CHECK_DUMP_OBJ= redis-check-dump.o lzf_c.o lzf_d.o crc64.o
REDIS_CHECK_DUMP_OBJ=
#REDIS_CHECK_AOF_NAME= redis-check-aof
REDIS_CHECK_AOF_NAME=
#REDIS_CHECK_AOF_OBJ= redis-check-aof.o
REDIS_CHECK_AOF_OBJ=

#all: $(REDIS_SERVER_NAME) $(REDIS_SENTINEL_NAME) $(REDIS_CLI_NAME) $(REDIS_BENCHMARK_NAME) $(REDIS_CHECK_DUMP_NAME) $(REDIS_CHECK_AOF_NAME)
all: $(REDIS_SERVER_NAME)
	@echo ""
	@echo "Hint: To run 'make test' is a good idea ;)"
	@echo ""

.PHONY: all


persist-settings: distclean
	echo STD=$(STD) >> .make-settings
	echo WARN=$(WARN) >> .make-settings
	echo OPT=$(OPT) >> .make-settings
	echo MALLOC=$(MALLOC) >> .make-settings
	echo CFLAGS=$(CFLAGS) >> .make-settings
	echo LDFLAGS=$(LDFLAGS) >> .make-settings
	echo REDIS_CFLAGS=$(REDIS_CFLAGS) >> .make-settings
	echo REDIS_LDFLAGS=$(REDIS_LDFLAGS) >> .make-settings
	echo PREV_FINAL_CFLAGS=$(FINAL_CFLAGS) >> .make-settings
	echo PREV_FINAL_LDFLAGS=$(FINAL_LDFLAGS) >> .make-settings
	-(cd ../deps && $(MAKE) $(DEPENDENCY_TARGETS))

.PHONY: persist-settings

# Prerequisites target
.make-prerequisites:
	@touch $@

# Clean everything, persist settings and build dependencies if anything changed
ifneq ($(strip $(PREV_FINAL_CFLAGS)), $(strip $(FINAL_CFLAGS)))
.make-prerequisites: persist-settings
endif

ifneq ($(strip $(PREV_FINAL_LDFLAGS)), $(strip $(FINAL_LDFLAGS)))
.make-prerequisites: persist-settings
endif


# redis-server
$(REDIS_SERVER_NAME): $(REDIS_SERVER_OBJ)
	$(REDIS_LD) -o $@ $^ ../deps/hiredis/libhiredis.a ../deps/lua/src/liblua.a  $(FINAL_LIBS)

# redis-sentinel
#$(REDIS_SENTINEL_NAME): $(REDIS_SERVER_NAME)
#	$(REDIS_INSTALL) $(REDIS_SERVER_NAME) $(REDIS_SENTINEL_NAME)

# redis-cli
#$(REDIS_CLI_NAME): $(REDIS_CLI_OBJ)
#	$(REDIS_LD) -o $@ $^ ../deps/hiredis/libhiredis.a ../deps/linenoise/linenoise.o $(FINAL_LIBS)

# redis-benchmark
#$(REDIS_BENCHMARK_NAME): $(REDIS_BENCHMARK_OBJ)
#	$(REDIS_LD) -o $@ $^ ../deps/hiredis/libhiredis.a $(FINAL_LIBS)

# redis-check-dump
#$(REDIS_CHECK_DUMP_NAME): $(REDIS_CHECK_DUMP_OBJ)
#	$(REDIS_LD) -o $@ $^ $(FINAL_LIBS)

# redis-check-aof
#$(REDIS_CHECK_AOF_NAME): $(REDIS_CHECK_AOF_OBJ)
#	$(REDIS_LD) -o $@ $^ $(FINAL_LIBS)

# Because the jemalloc.h header is generated as a part of the jemalloc build,
# building it should complete before building any other object. Instead of
# depending on a single artifact, build all dependencies first.
%.o: %.cpp 
#.make-prerequisites
	$(REDIS_CC) -c $<

clean:
	rm -rf $(REDIS_SERVER_NAME) $(REDIS_SENTINEL_NAME) $(REDIS_CLI_NAME) $(REDIS_BENCHMARK_NAME) $(REDIS_CHECK_DUMP_NAME) $(REDIS_CHECK_AOF_NAME) *.o *.gcda *.gcno *.gcov redis.info lcov-html

.PHONY: clean

distclean: clean
	-(cd ../deps && $(MAKE) distclean)
	-(rm -f .make-*)

.PHONY: distclean

install: all
	mkdir -p $(INSTALL_BIN)
	$(REDIS_INSTALL) $(REDIS_SERVER_NAME) $(INSTALL_BIN)
#	$(REDIS_INSTALL) $(REDIS_BENCHMARK_NAME) $(INSTALL_BIN)
#	$(REDIS_INSTALL) $(REDIS_CLI_NAME) $(INSTALL_BIN)
#	$(REDIS_INSTALL) $(REDIS_CHECK_DUMP_NAME) $(INSTALL_BIN)
#	$(REDIS_INSTALL) $(REDIS_CHECK_AOF_NAME) $(INSTALL_BIN)
	

ae.o: ae.cpp ae.h config.h
anet.o: anet.cpp anet.h fmacros.h
consts.o: consts.cpp
crc64.o: crc64.cpp util.h
debug.o: debug.cpp debug.h consts.h config.h
log.o: log.cpp log.h
redis.o: redis.cpp redis.h
release.o: release.cpp release.h
rio.o : rio.cpp rio.h 
util.o: util.cpp util.h
zmalloc.o: zmalloc.cpp zmalloc.h
