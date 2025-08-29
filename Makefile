CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g -Isrc/include
LDFLAGS = -lrt -lpthread -lm

all: view player

view: src/view.o src/utils.o
	$(CC) $(CFLAGS) -o view src/view.o src/utils.o $(LDFLAGS)

player: src/player.o src/utils.o
	$(CC) $(CFLAGS) -o player src/player.o src/utils.o $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o view player
