# lume0 — Stage 0 bootstrap (C11, no dependencies)
CC      ?= cc
CFLAGS  ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wno-sign-conversion
SRC      = main.c lexer.c diag.c ast.c parser.c value.c interp.c lbe.c asmgen.c
BIN      = lume0

all: $(BIN)

$(BIN): $(SRC) lume0.h
	$(CC) $(CFLAGS) -o $@ $(SRC) -lm

test: $(BIN)
	./tests/run.sh

asan: CFLAGS += -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer
asan: clean $(BIN)
	./tests/run.sh

clean:
	rm -f $(BIN)

.PHONY: all test asan clean
