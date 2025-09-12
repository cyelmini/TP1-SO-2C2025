// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/mlib.h"
#include "include/utils.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int DIRS[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

void initialize_board(unsigned short width, unsigned short height, int board[], int seed) {
	srand(seed);
	for (unsigned short i = 0; i < height; i++) {
		for (unsigned short j = 0; j < width; j++) {
			board[i * width + j] = rand() % 9 + 1;
		}
	}
}

pid_t initialize_game(game_t *game, unsigned short width, unsigned short height, int player_count, int seed,
					  char **players, char *view, int pipe_fd[][2]) {
	game->width = width;
	game->height = height;
	game->playerCount = player_count;
	game->gameFinished = 0;
	initialize_board(game->width, game->height, game->board, seed);
	return initialize_players_and_view(game, players, view, pipe_fd);
}

pid_t initialize_players_and_view(game_t *game, char **players, char *view, int pipe_fd[][2]) {
	char width[16];
	char height[16];
	sprintf(width, "%d", game->width);
	sprintf(height, "%d", game->height);
	char *argv[4] = {NULL, width, height, NULL};

	// Lanzar jugadores
	for (unsigned int i = 0; i < game->playerCount; i++) {
		snprintf(game->players[i].name, sizeof(game->players[i].name), "%s", players[i]);
		game->players[i].score = 0;
		game->players[i].invalidMoves = 0;
		game->players[i].validMoves = 0;
		game->players[i].x = (i * game->width) / game->playerCount;
		game->players[i].y = (i * game->height) / game->playerCount;
		game->players[i].isBlocked = 0;

		pid_t pid = fork();
		if (pid < 0) {
			fprintf(stderr, "ERROR: fork");
			exit(EXIT_FAILURE);
		}
		else if (pid == 0) { // Hijo
			// Cerrar todas las pipe ends no usadas
			for (unsigned int j = 0; j < game->playerCount; j++) {
				if (j != i) {
					close(pipe_fd[j][READ_END]);
					close(pipe_fd[j][WRITE_END]);
				}
			}
			// Cerrar read end del pipe de este player
			close(pipe_fd[i][READ_END]);
			// Redirigir stdout al write de este player
			dup2(pipe_fd[i][WRITE_END], STDOUT_FILENO);
			close(pipe_fd[i][WRITE_END]);

			argv[0] = game->players[i].name;
			if (execve(game->players[i].name, argv, NULL) == -1) {
				fprintf(stderr, "ERROR: execve");
				exit(EXIT_FAILURE);
			}
		}
		else { // Padre
			game->players[i].player_pid = pid;
			close(pipe_fd[i][WRITE_END]);
		}
	}

	// Lanzar vista
	pid_t pid_view = -1;
	if (view != NULL) {
		argv[0] = view;
		pid_view = fork();

		if (pid_view < 0) {
			fprintf(stderr, "ERROR: fork");
			exit(EXIT_FAILURE);
		}
		else if (pid_view == 0) {
			if (execve(view, argv, NULL) == -1) {
				fprintf(stderr, "ERROR: execve");
				exit(EXIT_FAILURE);
			}
		}
	}
	return pid_view;
}

void initialize_sems(game_sync *sync) {
	sem_init(&sync->printNeeded, 1, 0);
	sem_init(&sync->printDone, 1, 0);
	sem_init(&sync->masterMutex, 1, 1);
	sem_init(&sync->gameMutex, 1, 1);
	sem_init(&sync->readersMutex, 1, 1);
	sync->readersCount = 0;
	for (int i = 0; i < MAX_PLAYERS; i++) {
		sem_init(&sync->playerTurn[i], 1, 0);
	}
}

void check_invalid(const char *c) {
	if (c == NULL || strncmp(c, "-", 1) == 0) {
		fprintf(stderr, "ERROR: Argumento inválido\n");
		exit(EXIT_FAILURE);
	}
}

void handle_params(int argc, char *argv[], unsigned short *width, unsigned short *height, int *delay, int *timeout,
				   int *seed, char **view_path, char **players, unsigned int *player_count) {
	*width = DEFAULT_WIDTH;
	*height = DEFAULT_HEIGHT;
	*delay = DEFAULT_DELAY;
	*timeout = DEFAULT_TIMEOUT;
	*seed = DEFAULT_SEED;
	*view_path = DEFAULT_VIEW;
	*player_count = 0;
	int param;

	int opt;
	while ((opt = getopt(argc, argv, "w:h:d:t:s:v:p:")) != -1) {
		switch (opt) {
			case 'w':
				check_invalid(optarg);
				if ((param = atoi(optarg)) < 10) {
					perror("ERROR: El ancho debe ser al menos 10\n");
					exit(EXIT_FAILURE);
				}
				*width = param;
				break;
			case 'h':
				check_invalid(optarg);
				if ((param = atoi(optarg)) < 10) {
					perror("ERROR: El alto debe ser al menos 10\n");
					exit(EXIT_FAILURE);
				}
				*height = param;
				break;
			case 'd':
				check_invalid(optarg);
				*delay = atoi(optarg);
				break;
			case 't':
				check_invalid(optarg);
				*timeout = atoi(optarg);
				break;
			case 's':
				check_invalid(optarg);
				*seed = atoi(optarg);
				break;
			case 'v':
				check_invalid(optarg);
				*view_path = optarg;
				break;
			case 'p': {
				int index_player = optind - 1;
				if (index_player >= argc) {
					fprintf(stderr, "ERROR: Debe especificar al menos un jugador después de -p");
					exit(EXIT_FAILURE);
				}
				unsigned int added = 0;
				int k = index_player;
				while (k < argc && argv[k][0] != '-') {
					if ((*player_count + added) >= MAX_PLAYERS) {
						fprintf(stderr, "ERROR: Máximo 9 jugadores");
						exit(EXIT_FAILURE);
					}
					players[*player_count + added] = argv[k];
					added++;
					k++;
				}
				*player_count += added;
				optind = index_player + added;
			} break;
			default:
				fprintf(stderr, "ERROR: Opción no reconocida: -%c\n", optopt);
				exit(EXIT_FAILURE);
		}
	}
}

int validate_and_apply_move(game_t *game, unsigned int idx, unsigned char move) {
	if (move > 7)
		return 0;
	int nx = game->players[idx].x + DIRS[move][0];
	int ny = game->players[idx].y + DIRS[move][1];

	if (nx < 0 || nx >= game->width || ny < 0 || ny >= game->height)
		return 0;

	int pos = ny * game->width + nx;
	if (game->board[pos] <= 0)
		return 0;

	for (unsigned int i = 0; i < game->playerCount; i++) {
		if (i != idx && !game->players[i].isBlocked && game->players[i].x == nx && game->players[i].y == ny) {
			return 0;
		}
	}

	game->players[idx].x = nx;
	game->players[idx].y = ny;
	game->players[idx].score += game->board[pos];
	game->board[pos] = -idx;

	return 1;
}

void playChompChamps(game_t *game, game_sync *sync, int pipe_fd[MAX_PLAYERS][2], unsigned int player_count, int max_fd,
					 int delay, int timeout, char *view) {
	fd_set read_fd;
	struct timeval tv;
	int round_robin_start = 0;
	time_t last_valid_move = time(NULL);

	while (!game->gameFinished) {
		for (unsigned int i = 0; i < player_count; i++) {
			if (!game->players[i].isBlocked) {
				sem_post(&sync->playerTurn[i]);
			}
		}

		FD_ZERO(&read_fd);
		for (unsigned i = 0; i < player_count; i++) {
			FD_SET(pipe_fd[i][READ_END], &read_fd);
		}

		tv.tv_sec = timeout;
		tv.tv_usec = 0;

		int ready = select(max_fd + 1, &read_fd, NULL, NULL, &tv);
		if (ready == -1) {
			fprintf(stderr, "ERROR: select\n");
			exit(EXIT_FAILURE);
		}
		else if (ready == 0) {
			fprintf(stderr, "timeout select.\n");
			break;
		}

		int valid_move_this_round = 0;

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
}

void calculate_winner(game_t *game, int player_count) {
	int winner_count = 1;
	int winners[MAX_PLAYERS];
	winners[0] = 0;
	unsigned int best_score = game->players[0].score;
	unsigned int best_valid_moves = game->players[0].validMoves;
	unsigned int best_invalid_moves = game->players[0].invalidMoves;

	for (int i = 1; i < player_count; i++) {
		if (game->players[i].score > best_score) {
			best_score = game->players[i].score;
			best_valid_moves = game->players[i].validMoves;
			best_invalid_moves = game->players[i].invalidMoves;
			winner_count = 1;
			winners[0] = i;
		}
		else if (game->players[i].score == best_score) {
			if (game->players[i].validMoves < best_valid_moves) {
				best_valid_moves = game->players[i].validMoves;
				best_invalid_moves = game->players[i].invalidMoves;
				winner_count = 1;
				winners[0] = i;
			}
			else if (game->players[i].validMoves == best_valid_moves) {
				if (game->players[i].invalidMoves < best_invalid_moves) {
					best_invalid_moves = game->players[i].invalidMoves;
					winner_count = 1;
					winners[0] = i;
				}
				else if (game->players[i].invalidMoves == best_invalid_moves) {
					winner_count++;
					winners[winner_count - 1] = i;
				}
			}
		}
	}
	if (winner_count == 1) {
		printf("\nEl ganador es Jugador %u :%s\n", winners[0], game->players[winners[0]].name);
	}
	else {
		printf("\nHay un empate entre %d jugadores:\n", winner_count);
		for (int i = 0; i < winner_count; i++) {
			printf("\tJugador %u : %s\n", winners[i], game->players[winners[i]].name);
		}
	}
}

void destroy_semaphones(game_sync *sync) {
	sem_destroy(&sync->printNeeded);
	sem_destroy(&sync->printDone);
	sem_destroy(&sync->masterMutex);
	sem_destroy(&sync->gameMutex);
	sem_destroy(&sync->readersMutex);
	for (int i = 0; i < MAX_PLAYERS; i++) {
		sem_destroy(&sync->playerTurn[i]);
	}
}