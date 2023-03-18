CC=gcc
CFLAGS=-pedantic -Wall -Wextra -Werror -Wfatal-errors -Wswitch-enum -Wmissing-prototypes -Wconversion -Ofast -flto -march=native -pipe
LIBS=

all: easm evmi deasm edbug easm2nasm

easm: src/easm.c src/evm.h
	$(CC) $(CFLAGS) -o easm src/easm.c $(LIBS)

evmi: src/evmi.c src/evm.h
	$(CC) $(CFLAGS) -o evmi src/evmi.c $(LIBS)

deasm: src/deasm.c src/evm.h
	$(CC) $(CFLAGS) -o deasm src/deasm.c $(LIBS)

edbug: src/edbug.c src/evm.h
	$(CC) $(CFLAGS) -o edbug src/edbug.c $(LIBS)

easm2nasm: src/easm2nasm.c src/evm.h
	$(CC) $(CFLAGS) -o easm2nasm src/easm2nasm.c $(LIBS)
