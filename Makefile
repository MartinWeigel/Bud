CC = gcc
CFLAGS += -std=c11 -Wall -Wextra -pedantic -O3
CLIBS = -lm
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

ifeq ($(OS), Windows_NT)
RM = del /F
BIN = bud.exe
else
RM = rm -f
BIN = bud
endif

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(CLIBS)

.PHONY: clean
clean:
	$(RM) $(BIN) $(OBJS)
