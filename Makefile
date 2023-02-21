CC=gcc
CFLAGS=-pedantic -Wall -Wextra -Werror -Wfatal-errors -Wswitch-enum -Ofast -flto -march=native -pipe
LIBS=

all: easm evmi

easm: src/easm.c src/evm.c
	$(CC) $(CFLAGS) -o easm src/easm.c $(LIBS)

evmi: src/evmi.c src/evm.c
	$(CC) $(CFLAGS) -o evmi src/evmi.c $(LIBS)
