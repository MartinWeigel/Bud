CC = gcc
CFLAGS += -std=c11 -Wall
CLIBS = -lm
FILES = $(wildcard src/*.c)

ifeq ($(OS),Windows_NT) 
APPLICATION_NAME = bud.exe
RM = del /Q /F
else
APPLICATION_NAME = bud
RM = rm -rf
endif

all: clean build

build:
	$(CC) $(CFLAGS) -o $(APPLICATION_NAME) $(FILES) $(CLIBS)

clean:
	$(RM) $(APPLICATION_NAME)
