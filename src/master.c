#include "include/mlib.h"
#include "include/utils.h"

int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(stderr, "ERROR: No se ha especificado un jugador.");
		exit(EXIT_FAILURE);
	}

	unsigned short width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT;
	int delay = DEFAULT_DELAY, timeout = DEFAULT_TIMEOUT, seed = DEFAULT_SEED;
	unsigned int player_count = 0;
	char *view = DEFAULT_VIEW;
	char *players[MAX_PLAYERS];
	int pipe_fd[MAX_PLAYERS][2]; // Read-end (0) y write-end (1)

	handle_params(argc, argv, &width, &height, &delay, &timeout, &seed, &view, players, &player_count);

	printf("width: %d\nheight: %d\ndelay: %d\ntimeout: %d\nseed: %d\nview: %s\nnum_players: %d\n", width, height, delay,
		   timeout, seed, view == NULL ? "-" : view, player_count);
	for (unsigned int i = 0; i < player_count; i++) {
		printf("\t%s\n", players[i]);
	}
	sleep(3);

	// Crear memoria compartida
	game_t *game = (game_t *) open_and_map(SHM_GAME, O_CREAT | O_RDWR, sizeof(game_t) + width * height * sizeof(int),
										   PROT_READ | PROT_WRITE, MAP_SHARED);

	game_sync *sync =
		(game_sync *) open_and_map(SHM_SYNC, O_CREAT | O_RDWR, sizeof(game_sync), PROT_READ | PROT_WRITE, MAP_SHARED);

	initialize_sems(sync);

	// Creación de pipes
	for (unsigned int i = 0; i < player_count; i++) {
		if (pipe(pipe_fd[i]) == -1) {
			fprintf(stderr, "pipe error");
			exit(EXIT_FAILURE);
		}
	}

	pid_t view_pid = initialize_game(game, width, height, player_count, seed, players, view, pipe_fd);

	// Imprimir el estado inicial
	if (view != NULL) {
		sem_post(&sync->printNeeded);
		sem_wait(&sync->printDone);
	}

	int max_fd = 0;
	for (unsigned int i = 0; i < player_count; i++) {
		if (pipe_fd[i][READ_END] > max_fd) {
			max_fd = pipe_fd[i][READ_END];
		}
	}

	fd_set read_fd;
	struct timeval tv;
	int round_robin_start = 0;
	time_t last_valid_move = time(NULL);

	// Loop principal de juego
	while (!game->gameFinished) {
		// Dar turno a todos los jugadores activos antes de esperar datos
		for (unsigned int i = 0; i < player_count; i++) {
			if (!game->players[i].isBlocked) {
				sem_post(&sync->playerTurn[i]);
			}
		}

		// Esperar jugadas con select
		FD_ZERO(&read_fd);
		for (unsigned i = 0; i < player_count; i++) {
			FD_SET(pipe_fd[i][READ_END], &read_fd);
		}

		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		int ready = select(max_fd + 1, &read_fd, NULL, NULL, &tv);
		if (ready == -1) {
			fprintf(stderr, "error select.\n");
			exit(EXIT_FAILURE);
		}
		else if (ready == 0) {
			fprintf(stderr, "timeout select.\n");
			break;
		}

		int valid_move_this_round = 0;

		// Procesar jugadas disponibles (round-robin)
		for (unsigned int j = 0; j < player_count; j++) {
			int idx = (round_robin_start + j) % player_count;

			if (!game->players[idx].isBlocked && FD_ISSET(pipe_fd[idx][READ_END], &read_fd)) {
				unsigned char move;
				ssize_t n = read(pipe_fd[idx][READ_END], &move, sizeof(unsigned char));
				if (n <= 0) {
					game->players[idx].isBlocked = 1;
				}
				else {
					sem_wait(&sync->masterMutex);

					int valid = validate_and_apply_move(game, idx, move);
					if (valid) {
						game->players[idx].validMoves++;
						last_valid_move = time(NULL);
						valid_move_this_round = 1;
					}
					else {
						game->players[idx].invalidMoves++;
					}

					sem_post(&sync->masterMutex);

					if (view != NULL) {
						sem_post(&sync->printNeeded);
						sem_wait(&sync->printDone);
						usleep(delay * 1000);
					}
				}
			}
		}
		round_robin_start++;

		int blocked = 0;
		for (unsigned int i = 0; i < player_count; i++) {
			if (game->players[i].isBlocked)
				blocked++;
		}
		if (blocked == (int) player_count) {
			game->gameFinished = true;
		}

		if (!valid_move_this_round && (time(NULL) - last_valid_move) >= timeout) {
			game->gameFinished = true;
		}
	}

	// Esperar a que la vista termine
	if (view != NULL) {
		sem_post(&sync->printNeeded);
		int status;
		waitpid(view_pid, &status, 0);
		printf("La vista terminó con código %d\n", WEXITSTATUS(status));
	}

	// Esperar a que los jugadores terminen
	for (unsigned int i = 0; i < player_count; i++) {
		int status;
		waitpid(game->players[i].player_pid, &status, 0);
		printf("Jugador %d terminó con código %d, puntaje %d / %d / %d\n", i, WEXITSTATUS(status),
			   game->players[i].score, game->players[i].validMoves, game->players[i].invalidMoves);
		close(pipe_fd[i][READ_END]);
		close(pipe_fd[i][WRITE_END]);
	}

	// Limpiar recursos
	sem_destroy(&sync->printNeeded);
	sem_destroy(&sync->printDone);
	sem_destroy(&sync->masterMutex);
	sem_destroy(&sync->gameMutex);
	sem_destroy(&sync->readersMutex);
	for (int i = 0; i < MAX_PLAYERS; i++) {
		sem_destroy(&sync->playerTurn[i]);
	}

	close_and_unmap(SHM_GAME, game, sizeof(game_t) + sizeof(int) * width * height, true);
	close_and_unmap(SHM_SYNC, sync, sizeof(game_sync), true);

	sleep(1); // Espera antes de terminar
	return 0;
}
