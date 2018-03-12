CC= gcc 
CFLAGS= --std=c11 -Wall -Wextra -fPIC -pedantic -Wall \
    -Wno-missing-braces -Wextra -Wno-missing-field-initializers -Wformat=2 \
    -Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith \
    -Wbad-function-cast -Wstrict-overflow=5 -Wstrict-prototypes -Winline \
    -Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code \
    -Wlogical-op -Wfloat-equal -Wstrict-aliasing=2 -Wredundant-decls \
    -Wold-style-definition -Werror \
    -fno-omit-frame-pointer -ffloat-store -fno-common -fstrict-aliasing
RM= rm -r 
AR= ar rcu 
RANLIB= ranlib

LIBS= -lm 

STATIC_LIB= libblockarray.a
DYNAMIC_LIB= libblockarray.so

LIB_O= block_array.o

all: $(STATIC_LIB) $(DYNAMIC_LIB)

clean:
	$(RM) $(STATIC_LIB) $(DYNAMIC_LIB) ./*.o

$(STATIC_LIB): $(LIB_O)
	$(AR) $@ $?
	$(RANLIB) $@

$(DYNAMIC_LIB): $(LIB_O)
	gcc $? -shared -o $@

depend:
	@$(CC) $(CFLAGS) -MM *.c

block_array.o: block_array.c block_array.h config.h