#include "include/utils.h"
#include <stdlib.h>

int dir[8][2] = {{0, 1}, {0, -1}, {1, 0}, {1, 1}, {1, -1}, {-1, 0}, {-1, -1}, {-1, 1}};

static int is_valid(game_t *game, int x, int y) {
	if (x < 0 || x >= game->width || y < 0 || y >= game->height)
		return 0;
	for (unsigned int i = 0; i < game->playerCount; ++i) {
		if (game->players[i].x == x && game->players[i].y == y && !game->players[i].isBlocked)
			return 0;
	}
	return 1;
}

static int simulate_moves(game_t *game, int x, int y, int depth, int *board, int width, int height) {
	if (depth == 0)
		return 0;
	int max_score = 0;
	for (int d = 0; d < 8; ++d) {
		int nx = x + dir[d][0];
		int ny = y + dir[d][1];
		if (!is_valid(game, nx, ny))
			continue;
		int cell_score = board[ny * width + nx];
		int path_score = cell_score + simulate_moves(game, nx, ny, depth - 1, board, width, height);
		if (path_score > max_score)
			max_score = path_score;
	}
	if (max_score == 0)
		return 0;
	return max_score;
}

static int pathfinder(game_t *game, player_t *player) {
	int best_dir = -1;
	int best_score = 0;
	int width = game->width;
	int height = game->height;
	int *board = game->board;

	for (int d = 0; d < 8; ++d) {
		int nx = player->x + dir[d][0];
		int ny = player->y + dir[d][1];
		if (!is_valid(game, nx, ny))
			continue;
		int cell_score = board[ny * width + nx];
		int path_score = cell_score + simulate_moves(game, nx, ny, 2, board, width, height);
		if (path_score > best_score) {
			best_score = path_score;
			best_dir = d;
		}
	}
	return best_dir;
}

int main(int argc, char *argv[]) {
	if (argc != 1) {
		fprintf(stderr, "Uso: %s\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// Mapear memoria compartida
	game_t *game = (game_t *) open_and_map(SHM_GAME, O_RDWR, 0, sizeof(game_t) + sizeof(int) * 100 * 100,
										   PROT_READ | PROT_WRITE, MAP_SHARED);
	game_sync *sync =
		(game_sync *) open_and_map(SHM_SYNC, O_RDWR, 0, sizeof(game_sync), PROT_READ | PROT_WRITE, MAP_SHARED);

	pid_t pid = getpid();
	int player_number = -1;
	for (unsigned int i = 0; i < game->playerCount; i++) {
		if (game->players[i].player_pid == pid) {
			player_number = i;
			break;
		}
	}
	if (player_number == -1) {
		fprintf(stderr, "Jugador no encontrado.\n");
		exit(EXIT_FAILURE);
	}

	player_t *player = &game->players[player_number];

	while (!player->isBlocked && !game->gameFinished) {
		// Ver si puede pasar a leer
		sem_wait(&sync->masterMutex);
		sem_post(&sync->masterMutex);

		// Lectura segura del estado del juego
		sem_wait(&sync->readersMutex);
		sync->readersCount++;
		if (sync->readersCount == 1) {
			sem_wait(&sync->gameMutex);
		}
		sem_post(&sync->readersMutex);

		unsigned char move = pathfinder(game, player);

		// Fin de lectura
		sem_wait(&sync->readersMutex);
		sync->readersCount--;
		if (sync->readersCount == 0)
			sem_post(&sync->gameMutex);
		sem_post(&sync->readersMutex);

		// Envia el movimiento al master por stdout
		if (write(STDOUT_FILENO, &move, sizeof(unsigned char)) == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}

	close_and_unmap(SHM_GAME, (void *) game, sizeof(game_t) + sizeof(int) * 100 * 100, false);
	close_and_unmap(SHM_SYNC, (void *) sync, sizeof(game_sync), false);

	return 0;
}