# Install
BIN = file-watcher

# Flags
CFLAGS += -std=c99 -Wall -Wextra -pedantic -Wno-unused-function
DFLAGS = $(CFLAGS) -g

SRC = main.c file_utils.c
OBJ = $(SRC:.c=.o)

$(BIN):
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
	$(CC) $(SRC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -o bin/$(BIN) -lX11 -lm

debug:
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
	$(CC) $(SRC) $(DFLAGS) -D_POSIX_C_SOURCE=200809L -o bin/$(BIN) -lX11 -lm