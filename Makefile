CC=gcc
CFLAGS=-pedantic -Wall -Wextra -Werror -Wfatal-errors -Wswitch-enum -Ofast -flto -march=native -pipe
LIBS=

all: easm evmi deasm

easm: src/easm.c src/evm.h
	$(CC) $(CFLAGS) -o easm src/easm.c $(LIBS)

evmi: src/evmi.c src/evm.h
	$(CC) $(CFLAGS) -o evmi src/evmi.c $(LIBS)

deasm: src/deasm.c src/evm.h
	$(CC) $(CFLAGS) -o deasm src/deasm.c $(LIBS)
