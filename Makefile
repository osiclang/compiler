CC=cc
CFLAGS = -std=c89 -pedantic -Wall -Wextra -Wno-unused-parameter -I. -I./src
LDFLAGS = -lm

ifeq ($(OS),Windows_NT)
	CC = gcc
	CFLAGS += -DWINDOWS
	LDFLAGS += -lws2_32 -lShlwapi
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		AR=gcc-ar
		CFLAGS += -DLINUX -D_XOPEN_SOURCE=700 -D_GNU_SOURCE
		LDFLAGS += -Wl,-rpath=./
	endif
	ifeq ($(UNAME_S),Darwin)
		AR=ar
		CFLAGS += -DDARWIN
	endif

	CFLAGS += -fPIC
	LDFLAGS += -ldl
endif

DEBUG ?= 0
ifeq ($(DEBUG),0)
	CFLAGS += -O2 -flto -DNDEBUG
else
	CFLAGS += -g -DDEBUG
endif

ifeq ($USE_MALLOC),1)
	CFLAGS += -DUSE_MALLOC
endif

SRCS  = src/osic.c
SRCS += src/hash.c
SRCS += src/shell.c
SRCS += src/mpool.c
SRCS += src/arena.c
SRCS += src/table.c
SRCS += src/token.c
SRCS += src/input.c
SRCS += src/lexer.c
SRCS += src/scope.c
SRCS += src/syntax.c
SRCS += src/parser.c
SRCS += src/symbol.c
SRCS += src/extend.c
SRCS += src/compiler.c
SRCS += src/peephole.c
SRCS += src/generator.c
SRCS += src/allocator.c
SRCS += src/machine.c

SRCS += src/oNil.c
SRCS += src/oType.c
SRCS += src/oKarg.c
SRCS += src/oVarg.c
SRCS += src/oTable.c
SRCS += src/oVkarg.c
SRCS += src/oArray.c
SRCS += src/oFrame.c
SRCS += src/oClass.c
SRCS += src/oSuper.c
SRCS += src/oObject.c
SRCS += src/oModule.c
SRCS += src/oNumber.c
SRCS += src/oString.c
SRCS += src/oInteger.c
SRCS += src/oBoolean.c
SRCS += src/oInstance.c
SRCS += src/oIterator.c
SRCS += src/oFunction.c
SRCS += src/oSentinel.c
SRCS += src/oAccessor.c
SRCS += src/oException.c
SRCS += src/oCoroutine.c
SRCS += src/oDict.c
SRCS += src/oContinuation.c

SRCS += lib/builtin.c
SRCS += lin/garbagecollector.c

MODULE_OS ?= 1
ifeq ($(MODULE_OS), 1)
	SRCS += lib/os.c
	CFLAGS += -DMODULE_OS
endif

MODULE_SOCKET ?= 1
ifeq ($(MODULE_SOCKET), 1)
	SRCS += lib/socket.c
	CFLAGS += -DMODULE_SOCKET
endif

OBJS = $(addprefix obj/,$(notdir $(SRCS:.c=.o)))

STATIC ?= 0
ifeq ($(STATIC),0)
	ifeq ($(OS),Windows_NT)
		LIB = libosic.dll
	else
		LIB = libosic.so
	endif
	LIBFLAGS = -L. -losic
else
	LIB = libosic.a
	CFLAGS += -DSTATICLIB
endif

VPATH  = ./src:./lib

INCS := $(wildcard src/*.h)

TESTS = $(wildcard test/test_*.lm)

.PHONY: mkdir test

all: mkdir osic

mkdir: obj

obj:
	@mkdir obj

osic: obj/main.o $(LIB) Makefile
	$(CC) $(CFLAGS) obj/main.o $(LIB) $(LIBFLAGS) $(LDFLAGS) -o $@
	@echo CC osic

libosic.a: $(OBJS) $(INCS) Makefile
	@$(AR) rcu $@ $(OBJS)
	@echo CC libosic.a

libosic.so: $(OBJS) $(INCS) Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) -shared $(OBJS) -o $@
	@echo CC libosic.so

libosic.dll: $(OBJS) $(INCS) Makefile
	$(CC) $(CFLAGS) -shared $(OBJS) $(LDFLAGS) -o $@
	@echo CC libosic.dll

obj/%.o: %.c $(INCS) Makefile
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

test: $(TESTS) osic Makefile
	@for test in $(TESTS); do \
		./osic $$test >> /dev/null && echo "$$test [ok]" || \
		{ echo "$$test [fail]" && exit 1; } \
	done

clean:
	@rm -f osic $(OBJS) libosic.a libosic.so libosic.dll obj/main.o
	@rmdir obj
	@echo clean osic $(OBJS)
