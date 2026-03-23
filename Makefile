CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11
SRC = src/main.c
BIN = tml24c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -Iinclude -o $@ $^

clean:
	rm -f $(BIN) *.o
