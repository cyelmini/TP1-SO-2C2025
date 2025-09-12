// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

#ifndef MLIB_H
#define MLIB_H

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define READ_END 0
#define WRITE_END 1

#define DEFAULT_WIDTH 10
#define DEFAULT_HEIGHT 10
#define DEFAULT_DELAY 200
#define DEFAULT_TIMEOUT 10
#define DEFAULT_SEED time(NULL)
#define DEFAULT_VIEW NULL

#define MIN_PLAYERS 1
#define MAX_PLAYERS 9

game_t *global_game;
game_sync *global_sync;
unsigned int global_player_count;
unsigned int global_width;
unsigned int global_height;
int (*global_pipe_fd)[2];

void check_invalid(const char *c);
void handle_params(int argc, char *argv[], unsigned short *width, unsigned short *height, int *delay, int *timeout,
				   int *seed, char **view_path, char **players, unsigned int *player_count);
void initialize_sems(game_sync *sync);
void initialize_board(unsigned short width, unsigned short height, int board[], int seed);
pid_t initialize_game(game_t *game, unsigned short width, unsigned short height, unsigned int player_count, int seed,
					  char **players, char *view, int pipe_fd[][2]);
pid_t initialize_players_and_view(game_t *game, char **players, char *view, int pipe_fd[][2]);
int validate_and_apply_move(game_t *game, unsigned int idx, unsigned char move);
void playChompChamps(game_t *game, game_sync *sync, int pipe_fd[MAX_PLAYERS][2], unsigned int player_count, int max_fd,
					 int delay, int timeout, char *view);

void destroy_semaphones(game_sync * sync);
void calculate_winner(game_t *game, unsigned int player_count);
void cleanup(int signo);

#endif // MLIB_H