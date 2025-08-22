CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g
LDFLAGS = -lrt -lpthread -lm

# fuentes
OBJS_VIEW = view.o utils/utils.o
OBJS_PRUEBA = player_prueba.o utils/utils.o

all: view player_prueba

view: $(OBJS_VIEW)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

player_prueba: $(OBJS_PRUEBA)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o utils/*.o view player_prueba

.PHONY: all
