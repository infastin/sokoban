#include "Game.h"

#include "Assign.h"
#include "Definitions.h"
#include "Distance.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

static State *state_init(State *state, State *init, usize ngoals)
{
	state->positions = calloc(ngoals + 1, sizeof(point));
	assert(state->positions != NULL);

	state->distance = 0;
	state->total_distance = 0;

	if (init != NULL) {
		memcpy(state->positions, init->positions, (ngoals + 1) * sizeof(point));
		state->distance = init->distance;
		state->total_distance = init->total_distance;
	}

	trb_string_init0(&state->solution);

	if (init != NULL && init->solution.data != NULL)
		trb_string_assign(&state->solution, init->solution.data);

	return state;
}

void state_destroy(State *state)
{
	free(state->positions);
	trb_string_destroy(&state->solution);
}

static i32 pos_cmp(const point *a, const point *b, u32 *data)
{
	u32 ngoals = *data;

	for (u32 i = 0; i <= ngoals; ++i) {
		if (a[i].x > b[i].x || a[i].y > b[i].y)
			return 1;
		if (a[i].x < b[i].x || a[i].y < b[i].y)
			return -1;
	}

	return 0;
}

static i32 state_pcmp(const State *a, const State *b)
{
	if (a->total_distance > b->total_distance)
		return -1;
	if (a->total_distance < b->total_distance)
		return 1;
	return 0;
}

static i32 state_cmp(const State *a, const State *b, u32 *data)
{
	return pos_cmp(a->positions, b->positions, data);
}

Game *game_init(Game *game)
{
	game->width = 0;
	game->height = 0;
	game->ngoals = 0;
	game->board = NULL;
	game->goals = NULL;
	game->marks = NULL;
	game->distances = NULL;
	game->assignment = NULL;

	return game;
}

void game_reset(Game *game)
{
	free(game->marks);

	if (game->distances != NULL)
		free(game->distances);

	if (game->assignment != NULL)
		free(game->assignment);
}

void game_destroy(Game *game)
{
	free(game->board);
	free(game->goals);
	free(game->marks);

	if (game->distances != NULL)
		free(game->distances);

	if (game->assignment != NULL)
		free(game->assignment);

	state_destroy(&game->state);
}

static void mark(Game *game, u32 x, u32 y)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*marks)[h][w] = (u8(*)[h][w]) game->marks;
	u8(*board)[h][w] = (u8(*)[h][w]) game->board;

	if ((*marks)[y][x])
		return;

	(*marks)[y][x] = 1;

	if (x > 1 && (*board)[y][x - 1] != WALL && (*board)[y][x - 2] != WALL)
		mark(game, x - 1, y);
	if (x < w - 2 && (*board)[y][x + 1] != WALL && (*board)[y][x + 2] != WALL)
		mark(game, x + 1, y);
	if (y > 1 && (*board)[y - 1][x] != WALL && (*board)[y - 2][x] != WALL)
		mark(game, x, y - 1);
	if (y < h - 2 && (*board)[y + 1][x] != WALL && (*board)[y + 2][x] != WALL)
		mark(game, x, y + 1);
}

static u32 game_get_box(Game *game, State *state, u32 x, u32 y)
{
	for (u32 i = 1; i <= game->ngoals; ++i) {
		point pos = state->positions[i];
		if (pos.x == x && pos.y == y)
			return i;
	}

	return -1;
}

static bool is_solved(Game *game, State *state)
{
	for (u32 i = 0; i < game->ngoals; ++i) {
		point goal = game->goals[i];
		if (game_get_box(game, state, goal.x, goal.y) == -1)
			return FALSE;
	}

	return TRUE;
}

static bool game_move(Game *game, State *state, u32 x, u32 y, State *ret)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*board)[h][w] = (u8(*)[h][w]) game->board;

	if ((*board)[y][x] == '#' || (*board)[y][x] == 0)
		return FALSE;

	state_init(ret, state, game->ngoals);
	ret->positions[0] = (point){ x, y };

	return TRUE;
}

static bool game_push(Game *game, State *state, u32 px, u32 py, u32 bx, u32 by, u32 bi, State *ret)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*board)[h][w] = (u8(*)[h][w]) game->board;
	u8(*marks)[h][w] = (u8(*)[h][w]) game->marks;

	if (
		(*board)[by][bx] == '#' ||
		(*board)[by][bx] == 0 ||
		(*marks)[by][bx] == 0 ||
		game_get_box(game, state, bx, by) != -1
	) {
		return FALSE;
	}

	state_init(ret, state, game->ngoals);
	ret->positions[0] = (point){ px, py };
	ret->positions[bi] = (point){ bx, by };

	return TRUE;
}

bool game_solve_dfs(Game *game, State *ret)
{
	State init_state;
	state_init(&init_state, &game->state, game->ngoals);

	TrbHashTable visited;
	trb_hash_table_init_data(&visited, (game->ngoals + 1) * sizeof(point), 1, 0xdeadbeef, trb_jhash, (TrbCmpDataFunc) pos_cmp, &game->ngoals);
	trb_hash_table_insert(&visited, init_state.positions, trb_get_ptr(bool, TRUE));

	TrbDeque vertices;
	trb_deque_init(&vertices, TRUE, sizeof(State));
	trb_deque_push_back(&vertices, &init_state);

	while (vertices.len != 0) {
		State vertex;
		trb_deque_pop_front(&vertices, &vertex);

		point pos = vertex.positions[0];
		u32 x = pos.x;
		u32 y = pos.y;

		struct {
			u32 px, py;
			u32 bx, by;
			char move_char;
			char push_char;
		} dirs[4] = {
			{x - 1,  y,     x - 2, y,     'l', 'L'},
			{ x,     y - 1, x,     y - 2, 'u', 'U'},
			{ x + 1, y,     x + 2, y,     'r', 'R'},
			{ x,     y + 1, x,     y + 2, 'd', 'D'},
		};

		for (u32 i = 0; i < 4; ++i) {
			u32 px = dirs[i].px;
			u32 py = dirs[i].py;
			u32 bx = dirs[i].bx;
			u32 by = dirs[i].by;

			bool do_something;
			State next;
			char csol;

			u32 bi = game_get_box(game, &vertex, px, py);
			if (bi != -1) {
				do_something = game_push(game, &vertex, px, py, bx, by, bi, &next);
				csol = dirs[i].push_char;
			} else {
				do_something = game_move(game, &vertex, px, py, &next);
				csol = dirs[i].move_char;
			}

			if (do_something) {
				if (trb_hash_table_lookup(&visited, next.positions, NULL)) {
					state_destroy(&next);
					continue;
				}

				trb_string_push_back_c(&next.solution, csol);

				if (is_solved(game, &next)) {
					trb_hash_table_destroy(&visited, NULL, NULL);
					trb_deque_destroy(&vertices, (TrbFreeFunc) state_destroy);
					state_destroy(&vertex);

					*ret = next;
					return TRUE;
				}

				trb_deque_push_back(&vertices, &next);
				trb_hash_table_insert(&visited, next.positions, trb_get_ptr(bool, TRUE));
			}
		}

		state_destroy(&vertex);
	}

	trb_hash_table_destroy(&visited, NULL, NULL);
	trb_deque_destroy(&vertices, (TrbFreeFunc) state_destroy);

	return FALSE;
}

static u32 heuristic(Game *game, State *state)
{
	u32 total = 0;

	u32 w = game->width;
	u32 h = game->height;

	u32 *assignment = game->assignment;
	u32(*distances)[game->ngoals][h][w] = (u32(*)[game->ngoals][h][w]) game->distances;

	for (u32 goal = 0; goal < game->ngoals; ++goal) {
		u32 box = assignment[goal] + 1;
		point bp = state->positions[box];
		total += (*distances)[goal][bp.y][bp.x];
	}

	return total;
}

bool game_solve_astar(Game *game, State *ret)
{
	State init_state;
	state_init(&init_state, &game->state, game->ngoals);

	TrbHashTable visited;
	trb_hash_table_init_data(&visited, (game->ngoals + 1) * sizeof(point), 1, 0xdeadbeef, trb_jhash, (TrbCmpDataFunc) pos_cmp, &game->ngoals);
	trb_hash_table_insert(&visited, init_state.positions, trb_get_ptr(bool, TRUE));

	TrbHeap vertices;
	trb_heap_init_data(&vertices, sizeof(State), (TrbCmpDataFunc) state_pcmp, &game->ngoals);
	trb_heap_insert(&vertices, &init_state);

	while (vertices.vector.len != 0) {
		State vertex;
		trb_heap_pop_front(&vertices, &vertex);
		trb_hash_table_add(&visited, vertex.positions, trb_get_ptr(bool, TRUE));

		point pos = vertex.positions[0];
		u32 x = pos.x;
		u32 y = pos.y;

		struct {
			u32 px, py;
			u32 bx, by;
			char move_char;
			char push_char;
		} dirs[4] = {
			{x - 1,  y,     x - 2, y,     'l', 'L'},
			{ x,     y - 1, x,     y - 2, 'u', 'U'},
			{ x + 1, y,     x + 2, y,     'r', 'R'},
			{ x,     y + 1, x,     y + 2, 'd', 'D'},
		};

		for (u32 i = 0; i < 4; ++i) {
			u32 px = dirs[i].px;
			u32 py = dirs[i].py;
			u32 bx = dirs[i].bx;
			u32 by = dirs[i].by;

			bool do_something;
			State next;
			char csol;

			u32 bi = game_get_box(game, &vertex, px, py);
			if (bi != -1) {
				do_something = game_push(game, &vertex, px, py, bx, by, bi, &next);
				csol = dirs[i].push_char;
			} else {
				do_something = game_move(game, &vertex, px, py, &next);
				csol = dirs[i].move_char;
			}

			if (do_something) {
				if (trb_hash_table_lookup(&visited, next.positions, NULL)) {
					state_destroy(&next);
					continue;
				}

				trb_string_push_back_c(&next.solution, csol);

				if (is_solved(game, &next)) {
					trb_hash_table_destroy(&visited, NULL, NULL);
					trb_heap_destroy(&vertices, (TrbFreeFunc) state_destroy);
					state_destroy(&vertex);

					*ret = next;
					return TRUE;
				}

				next.distance = vertex.distance + 1;
				next.total_distance = next.distance + heuristic(game, &next);

				usize index;

				if (trb_heap_search_data(&vertices, &next, (TrbCmpDataFunc) state_cmp, &game->ngoals, &index)) {
					State *old = trb_heap_ptr(&vertices, State, index);

					if (next.distance < old->distance) {
						old->distance = next.distance;
						old->total_distance = next.total_distance;
						trb_heap_fix(&vertices);
					}

					state_destroy(&next);
				} else {
					trb_heap_insert(&vertices, &next);
				}
			}
		}

		state_destroy(&vertex);
	}

	trb_hash_table_destroy(&visited, NULL, NULL);
	trb_heap_destroy(&vertices, (TrbFreeFunc) state_destroy);

	return FALSE;
}

bool game_solve_cbfs(Game *game, State *ret)
{
	State init_state;
	state_init(&init_state, &game->state, game->ngoals);

	TrbHashTable visited;
	trb_hash_table_init_data(&visited, (game->ngoals + 1) * sizeof(point), 1, 0xdeadbeef, trb_jhash, (TrbCmpDataFunc) pos_cmp, &game->ngoals);
	trb_hash_table_insert(&visited, init_state.positions, trb_get_ptr(bool, TRUE));

	TrbHeap vertices;
	trb_heap_init_data(&vertices, sizeof(State), (TrbCmpDataFunc) state_pcmp, &game->ngoals);
	trb_heap_insert(&vertices, &init_state);

	while (vertices.vector.len != 0) {
		State vertex;
		trb_heap_pop_front(&vertices, &vertex);
		trb_hash_table_add(&visited, vertex.positions, trb_get_ptr(bool, TRUE));

		point pos = vertex.positions[0];
		u32 x = pos.x;
		u32 y = pos.y;

		struct {
			u32 px, py;
			u32 bx, by;
			char move_char;
			char push_char;
		} dirs[4] = {
			{x - 1,  y,     x - 2, y,     'l', 'L'},
			{ x,     y - 1, x,     y - 2, 'u', 'U'},
			{ x + 1, y,     x + 2, y,     'r', 'R'},
			{ x,     y + 1, x,     y + 2, 'd', 'D'},
		};

		for (u32 i = 0; i < 4; ++i) {
			u32 px = dirs[i].px;
			u32 py = dirs[i].py;
			u32 bx = dirs[i].bx;
			u32 by = dirs[i].by;

			bool do_something;
			State next;
			char csol;

			u32 bi = game_get_box(game, &vertex, px, py);
			if (bi != -1) {
				do_something = game_push(game, &vertex, px, py, bx, by, bi, &next);
				csol = dirs[i].push_char;
			} else {
				do_something = game_move(game, &vertex, px, py, &next);
				csol = dirs[i].move_char;
			}

			if (do_something) {
				if (trb_hash_table_lookup(&visited, next.positions, NULL)) {
					state_destroy(&next);
					continue;
				}

				trb_string_push_back_c(&next.solution, csol);

				if (is_solved(game, &next)) {
					trb_hash_table_destroy(&visited, NULL, NULL);
					trb_heap_destroy(&vertices, (TrbFreeFunc) state_destroy);
					state_destroy(&vertex);

					*ret = next;
					return TRUE;
				}

				next.total_distance = vertex.distance + 1 + heuristic(game, &next);

				if (!trb_heap_search_data(&vertices, &next, (TrbCmpDataFunc) state_cmp, &game->ngoals, NULL)) {
					trb_heap_insert(&vertices, &next);
				} else {
					state_destroy(&next);
				}
			}
		}

		state_destroy(&vertex);
	}

	trb_hash_table_destroy(&visited, NULL, NULL);
	trb_heap_destroy(&vertices, (TrbFreeFunc) state_destroy);

	return FALSE;
}

void game_parse_board(Game *game, u32 w, u32 h, const char *str)
{
	game->width = w;
	game->height = h;

	game->board = calloc(w * h, 1);
	assert(game->board);

	game->marks = calloc(w * h, 1);
	assert(game->marks);

	for (u32 i = 0; str[i]; ++i) {
		switch (str[i]) {
		case WALL:
			game->board[i] = WALL;
			continue;

		case BOX:
		case PLAYER:
		case FLOOR:
			game->board[i] = FLOOR;
			continue;

		case BOX_ON_GOAL:
		case GOAL:
		case PLAYER_ON_GOAL:
			game->ngoals++;
			game->board[i] = GOAL;
			continue;

		default:
			continue;
		}
	}

	state_init(&game->state, NULL, game->ngoals);
	game->goals = calloc(game->ngoals, sizeof(point));
	assert(game->goals != NULL);

	u8(*board)[h][w] = (u8(*)[h][w]) game->board;

	for (u32 y = 0, i = 0, j = 1, k = 0; y < h; ++y) {
		for (u32 x = 0; x < w; ++x, ++i) {
			if ((*board)[y][x] == GOAL)
				mark(game, x, y);

			switch (str[i]) {
			case GOAL:
				game->goals[k++] = (point){ x, y };
				continue;

			case PLAYER_ON_GOAL:
				game->goals[k++] = (point){ x, y };
			case PLAYER:
				game->state.positions[0] = (point){ x, y };
				continue;

			case BOX_ON_GOAL:
				game->goals[k++] = (point){ x, y };
			case BOX:
				game->state.positions[j++] = (point){ x, y };
				continue;

			default:
				continue;
			}
		}
	}
}

void game_calc_distances(Game *game, int type)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*board)[h][w] = (u8(*)[h][w]) game->board;
	u32(*distances)[game->ngoals][h][w] = malloc(sizeof *distances);
	assert(distances != NULL);

	switch (type) {
	case PULL_GOAL_DIST:
		pull_goal_distance(game->goals, game->state.positions, w, h, board, game->ngoals, distances);
		break;
	case MANHATTAN_DIST:
		manhattan_distance(game->goals, game->state.positions, w, h, board, game->ngoals, distances);
		break;
	case PYTHAGOREAN_DIST:
	default:
		pythagorean_distance(game->goals, game->state.positions, w, h, board, game->ngoals, distances);
		break;
	}

	game->distances = (u32 *) distances;
}

void game_do_assignment(Game *game, int type)
{
	u32 w = game->width;
	u32 h = game->height;

	u32(*distances)[game->ngoals][h][w] = (u32(*)[game->ngoals][h][w]) game->distances;
	u32(*transformed)[game->ngoals][game->ngoals] = malloc(sizeof *transformed);
	assert(transformed != NULL);

	transform_distances(game->state.positions, w, h, game->ngoals, distances, transformed);

	switch (type) {
	case HUNGARIAN_ASSIGN:
		game->assignment = hungarian_assignment(game->ngoals, transformed);
		break;
	case GREEDY_ASSIGN:
		game->assignment = greedy_assignment(game->ngoals, transformed);
		break;
	default:
	case CLOSEST_ASSIGN:
		game->assignment = closest_assignment(game->ngoals, transformed);
		break;
	}

	free(transformed);
}
