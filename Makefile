CC=gcc
CFLAGS=-Wall -Wextra -Werror -g -O2

BIN_INT=bfi
SRC_INT=bfi.c

BIN_COMP=bfc
SRC_COMP=bfc.c

all: $(BIN_INT) $(BIN_COMP)

$(BIN_INT): $(SRC_INT)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_COMP): $(SRC_COMP)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BIN_INT) $(BIN_COMP)

.PHONY: all clean
