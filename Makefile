CC=gcc
CFLAGS=-Wall -Wextra -Werror -g -O2

BIN=bfi
SRC=bfi.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(BIN)

.PHONY: all clean
