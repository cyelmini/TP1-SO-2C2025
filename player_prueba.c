#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "./utils/utils.h"

#define SHM_GAME "/game_state"
#define SHM_SYNC "/game_sync"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ancho> <alto>\n", argv[0]);
        return 1;
    }

    int width = atoi(argv[1]);
    int height = atoi(argv[2]);

    // shm del juego
    int fd_game = shm_open(SHM_GAME, O_RDWR, 0);
    if (fd_game == -1) { perror("shm_open game"); exit(1); }
    size_t game_size = sizeof(game_t) + sizeof(int) * width * height;
    game_t *game = mmap(NULL, game_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_game, 0);

    // shm de sync
    int fd_sync = shm_open(SHM_SYNC, O_RDWR, 0);
    if (fd_sync == -1) { perror("shm_open sync"); exit(1); }
    game_sync *sync = mmap(NULL, sizeof(game_sync), PROT_READ | PROT_WRITE, MAP_SHARED, fd_sync, 0);

    pause();

    return 0;
}
