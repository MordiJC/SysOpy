CC= gcc 
CFLAGS= --std=c11 -Wall -Wextra -fPIC -pedantic -Wall \
    -Wno-missing-braces -Wextra -Wno-missing-field-initializers -Wformat=2 \
    -Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith \
    -Wbad-function-cast -Wstrict-overflow=5 -Wstrict-prototypes -Winline \
    -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code \
    -Wlogical-op -Wfloat-equal -Wstrict-aliasing=2 -Wredundant-decls \
    -Wold-style-definition -Werror \
    -fno-omit-frame-pointer -ffloat-store -fno-common -fstrict-aliasing \
	$(MYFLAGS)
RM= rm -r
ECHO= echo -e

OUT= main
CORE_O= main.o

.PHONY: all

all: $(OUT)

$(OUT): $(CORE_O)
	$(CC) $? -o $@

.PHONY: clean

clean:
	$(RM) ./*.o ./main

main.o: main.c