CC=gcc
CFLAGS=-pedantic -Wall -Wextra -Werror -Wfatal-errors -Wswitch-enum -Ofast -flto -march=native -pipe
LIBS=
SRC=src/main.c

evm: $(SRC)
	$(CC) $(CFLAGS) -o evm $(SRC) $(LIBS)
