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
AR= ar rcu  

STATIC_LIB=libblockarray.a
DYNAMIC_LIB=libblockarray.so
MAIN_EXEC= main

LIB_O= block_array.o
MAIN_O= main.o args.o

all: $(DYNAMIC_LIB) $(MAIN_EXEC)

clean:
	$(RM) $(STATIC_LIB) $(DYNAMIC_LIB) ./*.o $(MAIN_EXEC) || true

$(MAIN_EXEC): $(MAIN_O) $(STATIC_LIB) 
	$(CC) $? -o $@

$(STATIC_LIB): $(LIB_O)
	$(AR) $@ $?

$(DYNAMIC_LIB): $(LIB_O)
	$(CC) $? -shared -o $@

test:
	./$(MAIN_EXEC) dynamic create_table 10000 512 search_element 30000 remove 3000 add 3000

depend:
	@$(CC) $(CFLAGS) -MM *.c

# Generated by `depend'

args.o: args.c args.h
block_array.o: block_array.c block_array.h
main.o: main.c args.h block_array.h
