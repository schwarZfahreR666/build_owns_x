CC = gcc
AR = ar
LD = ld

CRT_LIB_SRC = entry.c heap.c stdio.c string.c printf.c
CRT_LIB_OBJS = heap.o printf.o stdio.o string.o
ENTRY_OBJ = entry.o
CRT_STATIC_LIB = crt.a

CFLAGS = -fno-builtin -nostdlib
ARFLAGS = -rs

TEST_SRC = test.c
TEST_OBJ = test.o
TEST_LD_SRC = $(ENTRY_OBJ) $(TEST_OBJ) $(CRT_STATIC_LIB)
TEST_CFLAGS = -ggdb -fno-builtin -nostdlib
TEST_LDFLAGS = -static
TEST_LD_ENTRY_FLAGS = -e crt_entry

#----------------------------------------------
ifeq ($(HIDE),)
hide := @
else
hide :=
endif

#----------------------------------------------
all: $(CRT_STATIC_LIB) test

test: $(TEST_LD_SRC)
	$(hide) $(LD) $(TEST_LDFLAGS) $(TEST_LD_ENTRY_FLAGS) $^ -o $@

$(TEST_OBJ): $(TEST_SRC)
	$(hide) $(CC) -c $(TEST_CFLAGS) $<

lib: $(CRT_STATIC_LIB)

$(CRT_STATIC_LIB): $(CRT_LIB_OBJS)
ifeq ($(HIDE),)
	$(hide) $(AR) $(ARFLAGS) $@ $^ >/dev/null 2>&1
else
	$(hide) $(AR) $(ARFLAGS) $@ $^
endif

$(CRT_LIB_OBJS): $(CRT_LIB_SRC)

%.o : %.c
	$(hide) $(CC) -c $(CFLAGS) $<

.PHONY: clean
clean:
	$(hide) find -type f 	\
		\( -name '*.o' -o 	\
		-name 'test.txt' -o \
		-name '*.a' -o      \
		-name 'test' \)|xargs rm -f

help:
	@echo "Usage:"
	@echo "	help 			: show help messages"
	@echo "	all  			: build crt.a and test"
	@echo "	lib  			: build crt.a"
	@echo "	test 			: build test"
	@echo "	clean			: clean *.o test.txt"
	@echo "	make HIDE=1 [target]	: print out all build commands, ex: make HIDE=1 lib"

