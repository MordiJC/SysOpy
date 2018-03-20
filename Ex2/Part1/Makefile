DEBUG = 0

SANITIZER = NONE

ifeq ($(SANITIZER),ADDRESS)

SANITIZER_FLAGS = -fsanitize=address
SANITIZER_LIBS = -lasan

else ifeq ($(SANITIZER),UNDEFINED)

SANITIZER_FLAGS = -fsanitize=address
SANITIZER_LIBS = -lubsan

else ifeq ($(SANITIZER),LEAK)

SANITIZER_FLAGS = -fsanitize=LEAK
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

.PHONY: test_generate

test_generate:
	./$(OUT) generate ./r_26214400-l_4.test_data 26214400 4
	./$(OUT) generate ./r_204800-l_512.test_data 204800 512
	./$(OUT) generate ./r_25600-l_4096.test_data 25600 4096
	./$(OUT) generate ./r_12800-l_8192.test_data 12800 8192

.PHONY: test_copy

test_copy:
	./$(OUT) copy ./r_26214400-l_4.test_data ./r_26214400-l_4.lib.test_data 26214400 4 lib
	./$(OUT) copy ./r_26214400-l_4.test_data ./r_26214400-l_4.sys.test_data 26214400 4 sys
	./$(OUT) copy ./r_204800-l_512.test_data ./r_204800-l_512.lib.test_data 204800 512 lib
	./$(OUT) copy ./r_204800-l_512.test_data ./r_204800-l_512.sys.test_data 204800 512 sys
	./$(OUT) copy ./r_25600-l_4096.test_data ./r_25600-l_4096.lib.test_data 25600 4096 lib
	./$(OUT) copy ./r_25600-l_4096.test_data ./r_25600-l_4096.sys.test_data 25600 4096 sys
	./$(OUT) copy ./r_12800-l_8192.test_data ./r_12800-l_8192.lib.test_data 12800 8192 lib
	./$(OUT) copy ./r_12800-l_8192.test_data ./r_12800-l_8192.sys.test_data 12800 8192 sys

.PHONY: test

test: test_generate test_copy
	
	

files.o: files.c files.h
main.o: main.c files.h
