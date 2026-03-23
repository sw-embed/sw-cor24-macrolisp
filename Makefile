CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11
SRCDIR = src
SRCS = $(wildcard $(SRCDIR)/*.c)
BIN = tml24c

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) -I$(SRCDIR) -o $@ $^

clean:
	rm -f $(BIN) *.o
