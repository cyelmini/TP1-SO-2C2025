// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#include "include/mlib.h"
#include "include/utils.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const int DIRS[8][2] = {{0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}};

/* -------------------------------------- Funciones de inicializacion -------------------------------------- */

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
		game->board[game->players[i].y * game->width + game->players[i].x] = -i; // Marcar posicion inicial

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
		sem_init(&sync->playerTurn[i], 1, 1);
	}
}

/* -------------------------------------- Handler de parámetros  -------------------------------------- */

void handle_params(int argc, char *argv[], unsigned short *width, unsigned short *height, int *delay, int *timeout,
				   int *seed, char **view_path, char **players, unsigned int *player_count) {
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

/* -------------------------------------- Funciones de lógica del juego -------------------------------------- */

void playChompChamps(game_t *game, game_sync *sync, int pipe_fd[MAX_PLAYERS][2], unsigned int player_count, int max_fd,
					 int delay, int timeout, char *view){

	struct timeval tv;
	time_t last_valid_move = time(NULL);

	while(1){
		// Entrar como lector para verificar si el juego terminó
		enter_reader(sync);
		bool gameFin = game->gameFinished;
		exit_reader(sync);
		
		if(gameFin){
			break;
		}

		if(!someone_can_move(game, player_count)){
			end_game(game, sync);
			sync_view(view, sync, delay);
			break;
		}
	
		fd_set read_fd;
		FD_ZERO(&read_fd);
		for (unsigned i = 0; i < player_count; i++) {
				FD_SET(pipe_fd[i][READ_END], &read_fd);
		}

		tv.tv_sec = timeout;
		tv.tv_usec = 0;
	
		int ready = select(max_fd + 1, &read_fd, NULL, NULL, &tv);
		if(ready == -1){
			fprintf(stderr, "ERROR: select\n");
			exit(EXIT_FAILURE);
		}
		else if(ready == 0){
			fprintf(stderr, "timeout select.\n");
			end_game(game, sync);
			sync_view(view, sync, delay);
			break; //basicamente return
		}

		round_robin(game, sync, pipe_fd, player_count, delay, view, &read_fd, &last_valid_move);
	
	}

}

void round_robin(game_t * game, game_sync * sync, int pipe_fd[MAX_PLAYERS][2], unsigned int player_count,
				 int delay, char *view, fd_set * read_fd, time_t * last_valid_move){

	int round_robin_start = 0;
	for(unsigned int j=0; j<player_count; j++){
		int idx = (round_robin_start + j) % player_count;

		if(!game->players[idx].isBlocked && FD_ISSET(pipe_fd[idx][READ_END], read_fd)){
			unsigned char move;
			int n = read(pipe_fd[idx][READ_END], &move, sizeof(unsigned char));
			if(n<=0){
				if(n<0)
					fprintf(stderr, "ERROR");
				
				FD_CLR(pipe_fd[idx][READ_END], read_fd);
				game->players[idx].isBlocked = true;

			} else {
				bool valid = validate_and_apply_move(sync, game, idx, move, read_fd, pipe_fd);

				if(valid){
					*last_valid_move = time(NULL);
					sync_view(view, sync, delay);
					if(!someone_can_move(game, player_count)){
						end_game(game, sync);
						sync_view(view, sync, delay);
						break;
					}
				}

				if(!game->players[idx].isBlocked){
					sem_post(&sync->playerTurn[idx]);
				}
			}

			round_robin_start = (idx+1)%player_count;
		}

	}
}

int validate_and_apply_move(game_sync * sync, game_t *game, unsigned int idx, unsigned char move, fd_set *read_fd, int pipe_fd[MAX_PLAYERS][2] ) {
	sem_wait(&sync->masterMutex);
	sem_wait(&sync->gameMutex);
	sem_post(&sync->masterMutex);
	
	int was_valid= 0;

	if (move > 7){
		game->players[idx].invalidMoves++;
	}else{
		int nx = game->players[idx].x + DIRS[move][0];
		int ny = game->players[idx].y + DIRS[move][1];
		
		if(!is_valid_move(nx, ny, game)){
			game->players[idx].invalidMoves++;
		}else{
			int pos = ny * game->width + nx;
			game->players[idx].x = nx;
			game->players[idx].y = ny;
			game->players[idx].score += game->board[pos];
			game->board[pos] = -idx;
			game->players[idx].validMoves++;
			was_valid = 1;
		}

	}
	
	for(unsigned int i=0; i<game->playerCount; i++){
		if(!game->players[i].isBlocked && !has_valid_moves(game, i)){
			FD_CLR(pipe_fd[i][READ_END], read_fd);
			game->players[i].isBlocked = 1;
		}
	}
	sem_post(&sync->gameMutex);

	return was_valid;
}

void end_game(game_t *game, game_sync *sync){
	sem_wait(&sync->masterMutex);
	sem_wait(&sync->gameMutex);
	game->gameFinished = true;
	sem_post(&sync->gameMutex);
	sem_post(&sync->masterMutex);
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


/* ------------------------------------- Funciones de sincronización ------------------------------------- */

void sync_view(char *view, game_sync *sync, int delay) {
	if (view != NULL) {
		sem_post(&sync->printNeeded);
		sem_wait(&sync->printDone);
		usleep(delay * 1000);  // Delay en milisegundos
	}
}

void wait_for_view(char *view, pid_t view_pid, game_sync *sync) {
	if (view != NULL) {
		sem_post(&sync->printNeeded);
		int status;
		waitpid(view_pid, &status, 0);
		printf("La vista terminó con código %d\n", WEXITSTATUS(status));
	}
}

void wait_for_players(game_t *game, unsigned int player_count, int pipe_fd[MAX_PLAYERS][2]) {
	for (unsigned int i = 0; i < player_count; i++) {
		int status;
		if(game->players[i].player_pid != 0){
			waitpid(game->players[i].player_pid, &status, 0);
			close(pipe_fd[i][READ_END]);
			close(pipe_fd[i][WRITE_END]);
		}
				printf("Jugador %u terminó con código %d, puntaje %u / %u / %u\n", i, WEXITSTATUS(status),
			   game->players[i].score, game->players[i].validMoves, game->players[i].invalidMoves);
		
	}
}

void signal_all_players(game_sync * sync, unsigned int player_count){
	for( unsigned int i=0; i<player_count; i++){
		sem_post(&sync->playerTurn[i]);
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

int find_max_fd(int pipe_fd[MAX_PLAYERS][2], unsigned int player_count) {
	int max_fd=0;
	for (unsigned int i = 0; i < player_count; i++) {
		if (pipe_fd[i][READ_END] > max_fd) {
			max_fd = pipe_fd[i][READ_END];
		}
	}
	return max_fd;
}


/* -------------------------------------- Funciones de validación -------------------------------------- */

void check_arguments(int argc) {
	if (argc < 3) {
		fprintf(stderr, "ERROR: No se ha especificado un jugador.");
		exit(EXIT_FAILURE);
	}
}

void check_invalid(const char *c) {
	if (c == NULL || strncmp(c, "-", 1) == 0) {
		fprintf(stderr, "ERROR: Argumento inválido\n");
		exit(EXIT_FAILURE);
	}
}

int is_valid_move(int x, int y, game_t *game) {
	return(x >= 0 && x < game->width && y >= 0 && y < game->height && game->board[y * game->width + x] > 0);
}

int has_valid_moves(game_t *game, unsigned int idx) {
    for (int dir = 0; dir < 8; dir++) {
        int nx = game->players[idx].x + DIRS[dir][0];
        int ny = game->players[idx].y + DIRS[dir][1];
        if(is_valid_move(nx, ny, game)){
			return 1;
		}
    }
    return 0;
}

int someone_can_move(game_t *game, unsigned int player_count){
	for(unsigned int i=0; i<player_count; i++){
		if(!game->players[i].isBlocked && has_valid_moves(game, i)){
			return 1;
		}
	}
	return 0;
}