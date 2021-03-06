DEBUG = 0

SANITIZER = NONE

ifeq ($(SANITIZER),ADDRESS)

SANITIZER_FLAGS = -fsanitize=address
SANITIZER_LIBS = -lasan

else ifeq ($(SANITIZER),UNDEFINED)

SANITIZER_FLAGS = -fsanitize=address
SANITIZER_LIBS = -lubsan
	
else ifeq ($(SANITIZER),LEAK)

SANITIZER_FLAGS = -fsanitize=leak
SANITIZER_LIBS = 

endif

ifeq ($(DEBUG),1)

DEBUG_FLAGS = -g -O0

else

DEBUG_FLAGS = -O2

endif

CC = gcc 
CFLAGS = --std=c11 -Wall -Wextra -fPIC -pedantic -Wall \
	-Wno-missing-braces -Wextra -Wno-missing-field-initializers -Wformat=2 \
	-Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith \
	-Wbad-function-cast -Wstrict-overflow=5 -Wstrict-prototypes -Winline \
	-Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code \
	-Wlogical-op -Wfloat-equal -Wstrict-aliasing=2 -Wredundant-decls \
	-Wold-style-definition -Werror \
	-fno-omit-frame-pointer -ffloat-store -fno-common -fstrict-aliasing \
	$(SANITIZER_FLAGS) $(DEBUG_FLAGS) $(MYFLAGS)
LDFLAGS = $(SANITIZER_LIBS)
RM = rm -r
ECHO = echo -e

OUT = main
CORE_O = main.o files.o

.PHONY: all

all: $(OUT)

$(OUT): $(CORE_O)
	$(CC) $? -o $@ $(LDFLAGS)

.PHONY: clean

clean:
	$(RM) ./*.o ./$(OUT)

.PHONY: test_clean

test_clean:
	$(RM) ./*.test_data

.PHONY: depend

depend:
	$(CC) -MM ./*.c

print:
	$(ECHO) "dupa\n"

.PHONY: test_generate

test_generate:
	# 4 bytes long records
	./$(OUT) generate ./r_4096-l_4.test_data 4096 4
	# for copying
	./$(OUT) generate ./r_65536-l_4.test_data 65536 4
	# 512 bytes long records
	./$(OUT) generate ./r_4096-l_512.test_data 4096 512
	# for copying
	./$(OUT) generate ./r_32768-l_512.test_data 32768 512
	# 4096 bytes long records
	./$(OUT) generate ./r_4096-l_4096.test_data 4096 4096
	# for copying
	./$(OUT) generate ./r_32768-l_4096.test_data 32768 4096
	# 8192 bytes long records
	./$(OUT) generate ./r_4096-l_8192.test_data 4096 8192
	# for copying
	./$(OUT) generate ./r_32768-l_8192.test_data 32768 8192

.PHONY: test_copy

test_copy:
	# 4 bytes long records
	./$(OUT) copy ./r_4096-l_4.test_data ./r_4096-l_4.lib.test_data 4096 4 lib
	./$(OUT) copy ./r_4096-l_4.test_data ./r_4096-l_4.sys.test_data 4096 4 sys
	# 512 bytes long records
	./$(OUT) copy ./r_4096-l_512.test_data ./r_4096-l_512.lib.test_data 4096 512 lib
	./$(OUT) copy ./r_4096-l_512.test_data ./r_4096-l_512.sys.test_data 4096 512 sys
	# 4096 bytes long records
	./$(OUT) copy ./r_4096-l_4096.test_data ./r_4096-l_4096.lib.test_data 4096 4096 lib
	./$(OUT) copy ./r_4096-l_4096.test_data ./r_4096-l_4096.sys.test_data 4096 4096 sys
	# 8192 bytes long records
	./$(OUT) copy ./r_4096-l_8192.test_data ./r_4096-l_8192.lib.test_data 4096 8192 lib
	./$(OUT) copy ./r_4096-l_8192.test_data ./r_4096-l_8192.sys.test_data 4096 8192 sys

	# Copy tests ("big" files)
	# 4 bytes long records
	./$(OUT) copy ./r_65536-l_4.test_data ./r_65536-l_4.lib.test_data 65536 4 lib
	./$(OUT) copy ./r_65536-l_4.test_data ./r_65536-l_4.sys.test_data 65536 4 sys
	# 512 bytes long records
	./$(OUT) copy ./r_32768-l_512.test_data ./r_32768-l_512.lib.test_data 32768 512 lib
	./$(OUT) copy ./r_32768-l_512.test_data ./r_32768-l_512.sys.test_data 32768 512 sys
	# 4096 bytes long records
	./$(OUT) copy ./r_32768-l_4096.test_data ./r_32768-l_4096.lib.test_data 32768 4096 lib
	./$(OUT) copy ./r_32768-l_4096.test_data ./r_32768-l_4096.sys.test_data 32768 4096 sys
	# 8192 bytes long records
	./$(OUT) copy ./r_32768-l_8192.test_data ./r_32768-l_8192.lib.test_data 32768 8192 lib
	./$(OUT) copy ./r_32768-l_8192.test_data ./r_32768-l_8192.sys.test_data 32768 8192 sys


.PHONY: test_sort

test_sort:
	./$(OUT) sort ./r_4096-l_4.lib.test_data 4096 4 lib
	./$(OUT) sort ./r_4096-l_4.sys.test_data 4096 4 sys
	./$(OUT) sort ./r_4096-l_512.lib.test_data 4096 512 lib
	./$(OUT) sort ./r_4096-l_512.sys.test_data 4096 512 sys
	./$(OUT) sort ./r_4096-l_4096.lib.test_data 4096 4096 lib
	./$(OUT) sort ./r_4096-l_4096.sys.test_data 4096 4096 sys
	./$(OUT) sort ./r_4096-l_8192.lib.test_data 4096 8192 lib
	./$(OUT) sort ./r_4096-l_8192.sys.test_data 4096 8192 sys

.PHONY: test

test: test_generate test_copy test_sort

# Generated by `depend'
files.o: files.c files.h
main.o: main.c files.h
