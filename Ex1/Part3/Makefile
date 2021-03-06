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
ECHO= echo -e

STATIC_LIB=libblockarray.a
DYNAMIC_LIB=libblockarray.so
MAIN_EXEC= main

LIB_O= block_array.o
MAIN_O= main.o args.o

TESTCOMMAND_DYNAMIC= dynamic create_table 10000 512 search_element 30000 remove 3000 add 3000
TESTCOMMAND_STATIC= static create_table 1000 512 search_element 30000 remove 300 add 300

all: $(STATIC_LIB) $(DYNAMIC_LIB) $(MAIN_EXEC)_static $(MAIN_EXEC)_dynamic $(MAIN_EXEC)_runtime

clean:
	$(RM) $(STATIC_LIB) $(DYNAMIC_LIB) ./*.o $(MAIN_EXEC)_static $(MAIN_EXEC)_dynamic $(MAIN_EXEC)_runtime

$(STATIC_LIB): $(LIB_O)
	$(AR) $@ $?

$(DYNAMIC_LIB): $(LIB_O)
	$(CC) $(CFLAGS) -c block_array.c -o block_array.o
	$(CC) $? -shared -o $@

$(MAIN_EXEC): $(MAIN_O) $(STATIC_LIB) 
	$(CC) $? -o $@_static

$(MAIN_EXEC)_static: $(MAIN_EXEC)

$(MAIN_EXEC)_dynamic: $(DYNAMIC_LIB) $(MAIN_O)
	$(CC) $(MAIN_O) -o $@ -dynamic -L . -l:./$(DYNAMIC_LIB)

$(MAIN_EXEC)_runtime: $(DYNAMIC_LIB) $(MAIN_O)
	$(CC) $(MAIN_O) $(LIB_O) -o $@ -DLOAD_LIBRARY_AT_RUNTIME

.PHONY: test_static
test_static: 
	$(ECHO) "STATIC" >> ./log.txt
	./$(MAIN_EXEC)_static $(TESTCOMMAND_STATIC)

.PHONY: test_dynamic
test_dynamic: 
	$(ECHO) "DYNAMIC" >> ./log.txt
	./$(MAIN_EXEC)_dynamic $(TESTCOMMAND_DYNAMIC)

.PHONY: test_runtime
test_runtime: 
	$(ECHO) "RUNTIME" >> ./log.txt
	./$(MAIN_EXEC)_runtime $(TESTCOMMAND_DYNAMIC)

clear_report:
	$(RM) log.txt || true

test: clear_report test_static test_dynamic test_runtime

run:
	$(RUN)

help:
	@$(ECHO) "all              - build all"
	@$(ECHO) "clean            - clean build files"
	@$(ECHO) "test             - run all tests (WARNING! Each test will overwrite report file)"
	@$(ECHO) "test_static      - run tests with static version of program"
	@$(ECHO) "test_dynamic     - run tests with dynamic version of program"
	@$(ECHO) "test_runtime     - run tests with runtime lib loading version of program"
	@$(ECHO) "$(STATIC_LIB)  - build static lib"
	@$(ECHO) "$(DYNAMIC_LIB) - build dynamic lib"
	@$(ECHO) "$(MAIN_EXEC)             - build main executable (with static linking)"
	@$(ECHO) "$(MAIN_EXEC)_dynamic     - build main executable (with dynamic linking)"
	@$(ECHO) "$(MAIN_EXEC)_runtime     - build main executable (with liading library on runtime)"
	@$(ECHO) "Optimization flags: -O0 -O1 -O2 -O3 -Og -Os"
	@$(ECHO) "             Usage: make <target> MYFLAGS=-Ox"

depend:
	@$(CC) $(CFLAGS) -MM *.c

# Generated by `depend'

args.o: args.c args.h
block_array.o: block_array.c block_array.h
main.o: main.c args.h block_array.h