DEBUG = 0

SANITIZER = NONE

ifeq ($(SANITIZER),ADDRESS)

SANITIZER_FLAGS = -fsanitize=address
SANITIZER_LIBS = -lasan

else ifeq ($(SANITIZER),UNDEFINED)

SANITIZER_FLAGS = -fsanitize=undefined
SANITIZER_LIBS = -lubsan

else ifeq ($(SANITIZER),LEAK)

SANITIZER_FLAGS = -fsanitize=leaks
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
LDFLAGS = $(SANITIZER_LIBS) -lpthread -lm
RM = rm -r
ECHO = echo -e

OUT = imgfilter
CORE_O = imgfilter.o apgm.o filter.o

OUT_REQ = $(CORE_O)

.PHONY: all

all: $(OUT)

$(OUT): $(OUT_REQ)
	$(CC) $^ -o $@ $(LDFLAGS)

.PHONY: clean

clean:
	$(RM) ./*.o ./$(OUT)

.PHONY: depend

depend:
	$(CC) -MM ./*.c

test:
	./$(OUT) 1 totem.ascii.pgm sobel.filter totem-sobel.pgm
	./$(OUT) 2 x31_f18.ascii.pgm sobel.filter x31_f18-sobel.pgm
	./$(OUT) 4 roi_14.ascii.pgm sobel.filter roi_14-sobel.pgm
	./$(OUT) 8 coins.ascii.pgm sobel.filter coins-sobel.pgm


# Generated by `depend'
apgmloader.o: apgmloader.c
imgfilter.o: imgfilter.c