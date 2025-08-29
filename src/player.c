#include "include/utils.h"
#include <stdlib.h>
#include <time.h>

int dir[8][2] = {{0, 1}, 
                {0, -1}, 
                {1, 0},
                {1, 1},
                {1, -1},
                {-1, 0},
                {-1, -1},
                {-1, 1}};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho_tablero> <alto_tablero>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);
    
    int fd = int fd_game = open_shared_memory(SHM_GAME, O_RDONLY, 0);
	size_t game_size = sizeof(game_t) + sizeof(int) * width * height;
	game_t *game = map_shared_memory(fd_game, game_size, PROT_READ, MAP_SHARED);
    
    
}