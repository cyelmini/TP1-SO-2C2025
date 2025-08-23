CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g
LDFLAGS = -lrt -lpthread -lm

# fuentes
OBJS_VIEW = src/view.o src/utils.o
OBJS_PRUEBA = src/player_prueba.o src/utils.o

all: view player_prueba

view: $(OBJS_VIEW)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

player_prueba: $(OBJS_PRUEBA)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compilar y correr tests
test: src/tests/test_utils
	./src/tests/test_utils

src/tests/test_utils: src/tests/test_utils.c src/utils.c src/tests/cutest/CuTest.c
	$(CC) -Isrc/include -Isrc/tests/cutest -o src/tests/test_utils src/tests/test_utils.c src/utils.c src/tests/cutest/CuTest.c

clean:
	rm -f src/*.o view player_prueba src/tests/test_utils

.PHONY: all test clean
