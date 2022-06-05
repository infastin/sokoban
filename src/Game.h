#ifndef GAME_H_WUFBIG2D
#define GAME_H_WUFBIG2D

#include "Definitions.h"

#include <tribble/Tribble.h>

typedef struct {
	TrbString solution;
	point *positions;
	u32 total_distance;
	u32 distance;
} State;

void state_destroy(State *state);

typedef struct {
	u32 width;
	u32 height;
	u32 ngoals;

	point *goals;
	u8 *board;
	u8 *marks;

	u32 *distances;
	u32 *assignment;

	State state;
} Game;

enum {
	PULL_GOAL_DIST,
	MANHATTAN_DIST,
	PYTHAGOREAN_DIST,
};

enum {
	HUNGARIAN_ASSIGN,
	GREEDY_ASSIGN,
	CLOSEST_ASSIGN,
};

Game *game_init(Game *game);
void game_reset(Game *game);
void game_destroy(Game *game);

void game_parse_board(Game *game, u32 w, u32 h, const char *str);

void game_calc_distances(Game *game, int type);
void game_do_assignment(Game *game, int type);

bool game_solve_dfs(Game *game, State *ret);
bool game_solve_astar(Game *game, State *ret);
bool game_solve_cbfs(Game *game, State *ret);

#endif /* end of include guard: GAME_H_WUFBIG2D */
