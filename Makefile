CC=gcc -std=c11
CFLAGS=-Wall -Wextra -pedantic -Iinclude
D=
FILES=game.c
SOURCES=$(addprefix src/, $(FILES))
EXECUTABLE=game
LIBRARIES=-lm -lncurses

OUTPUT=$(D) $(CFLAGS) $(SOURCES) $(LIBRARIES) -o $(EXECUTABLE)

all:
	$(CC) -DDEBUG -g $(OUTPUT)
prod:
	$(CC) -O3 $(OUTPUT)

clean:
	rm $(EXECUTABLE).exe
