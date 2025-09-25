CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g -Isrc/include
LDFLAGS = -lrt -lpthread -lm

all: master view player playerTonto

master: src/master.o src/mlib.o src/utils.o
	$(CC) $(CFLAGS) -o master src/master.o src/mlib.o src/utils.o $(LDFLAGS)

view: src/view.o src/utils.o
	$(CC) $(CFLAGS) -o view src/view.o src/utils.o $(LDFLAGS)

player: src/player.o src/utils.o
	$(CC) $(CFLAGS) -o player src/player.o src/utils.o $(LDFLAGS)

playerTonto: src/playerTonto.o src/utils.o
	$(CC) $(CFLAGS) -o playerTonto src/playerTonto.o src/utils.o $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o master view player
