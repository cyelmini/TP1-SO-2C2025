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

void check_arguments(int argc);
void initialize_board(unsigned short width, unsigned short height, int board[], int seed) ;
pid_t initialize_game(game_t *game, unsigned short width, unsigned short height, int player_count, int seed,
					  char **players, char *view, int pipe_fd[][2]) ;
pid_t initialize_players_and_view(game_t *game, char **players, char *view, int pipe_fd[][2]);
void initialize_sems(game_sync *sync);
void check_invalid(const char *c) ;
void handle_params(int argc, char *argv[], unsigned short *width, unsigned short *height, int *delay, int *timeout,
				   int *seed, char **view_path, char **players, unsigned int *player_count);
void sync_view(char *view, game_sync *sync, int delay) ;
int validate_and_apply_move(game_sync * sync, game_t *game, unsigned int idx, unsigned char move, fd_set *read_fd, int pipe_fd[MAX_PLAYERS][2] );
int is_valid_move(int x, int y, game_t *game);
int has_valid_moves(game_t *game, unsigned int idx);
int someone_can_move(game_t *game, unsigned int player_count);
void playChompChamps(game_t *game, game_sync *sync, int pipe_fd[MAX_PLAYERS][2], unsigned int player_count, int max_fd,
					 int delay, int timeout, char *view);
void round_robin(game_t * game, game_sync * sync, int pipe_fd[MAX_PLAYERS][2], unsigned int player_count, int max_fd,
				 int delay, int timeout, char *view, fd_set * read_fd, time_t * last_valid_move);
void end_game(game_t *game, game_sync *sync);
void calculate_winner(game_t *game, int player_count) ;
void destroy_semaphones(game_sync *sync);
void wait_for_players(game_t *game, unsigned int player_count, int pipe_fd[MAX_PLAYERS][2]);
int find_max_fd(int pipe_fd[MAX_PLAYERS][2], unsigned int player_count);
void wait_for_view(char *view, pid_t view_pid, game_sync *sync);


#endif // MLIB_H