CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=gnu99 -g
OBJS = main.o game.o map.o terminal.o util.o

escape: $(OBJS)
	$(CC) $(CFLAGS) -o escape $(OBJS)

main.o: main.c game.h map.h terminal.h util.h
	$(CC) $(CFLAGS) -c main.c

game.o: game.c game.h map.h terminal.h
	$(CC) $(CFLAGS) -c game.c

map.o: map.c map.h util.h
	$(CC) $(CFLAGS) -c map.c

terminal.o: terminal.c terminal.h map.h util.h
	$(CC) $(CFLAGS) -c terminal.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

clean:
	rm -f escape $(OBJS) state.txt

.PHONY: clean
