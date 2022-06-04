#include "Assign.h"
#include "Definitions.h"
#include "Distance.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <tribble/Tribble.h>

typedef struct {
	TrbString solution;
	point *positions;
	u32 total_distance;
	u32 distance;
} State;

State *state_init(State *state, State *init, usize ngoals)
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

i32 pos_cmp(const point *a, const point *b, u32 *data)
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

i32 state_cmp(const State *a, const State *b, u32 *data)
{
	if (a->total_distance > b->total_distance)
		return -1;
	if (a->total_distance < b->total_distance)
		return 1;

	return pos_cmp(a->positions, b->positions, data);
}

typedef struct _Game Game;

struct _Game {
	u32 width;
	u32 height;
	u32 ngoals;

	point *goals;
	u8 *board;
	u8 *marks;

	u32 *distances;
	u32 *assignment;

	State state;
};

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

void game_destroy(Game *game)
{
	free(game->board);
	free(game->goals);
	free(game->marks);

	if (game->distances != NULL)
		free(game->distances);

	if (game->assignment != NULL)
		free(game->assignment);
}

void mark(Game *game, u32 x, u32 y)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*marks)[h][w] = (u8(*)[h][w]) game->marks;
	u8(*board)[h][w] = (u8(*)[h][w]) game->board;

	if ((*marks)[y][x])
		return;

	(*marks)[y][x] = 1;

	if (x > 2 && (*board)[y][x - 1] != WALL && (*board)[y][x - 2] != WALL)
		mark(game, x - 1, y);
	if (x < w - 2 && (*board)[y][x + 1] != WALL && (*board)[y][x + 2] != WALL)
		mark(game, x + 1, y);
	if (y > 2 && (*board)[y - 1][x] != WALL && (*board)[y - 2][x] != WALL)
		mark(game, x, y - 1);
	if (y > 2 && (*board)[y + 1][x] != WALL && (*board)[y + 2][x] != WALL)
		mark(game, x, y + 1);
}

u32 get_box(Game *game, State *state, u32 x, u32 y)
{
	for (u32 i = 1; i <= game->ngoals; ++i) {
		point pos = state->positions[i];
		if (pos.x == x && pos.y == y)
			return i;
	}

	return -1;
}

bool is_solved(Game *game, State *state)
{
	for (u32 i = 0; i < game->ngoals; ++i) {
		point goal = game->goals[i];
		if (get_box(game, state, goal.x, goal.y) == -1)
			return FALSE;
	}

	return TRUE;
}

bool move(Game *game, State *state, u32 x, u32 y, u32 dist, State *ret)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*board)[h][w] = (u8(*)[h][w]) game->board;

	if ((*board)[y][x] == '#' || (*board)[y][x] == 0)
		return FALSE;

	state_init(ret, state, game->ngoals);
	ret->positions[0] = (point){ x, y };
	ret->distance = dist;

	return TRUE;
}

bool push(Game *game, State *state, u32 px, u32 py, u32 bx, u32 by, u32 bi, u32 dist, State *ret)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*board)[h][w] = (u8(*)[h][w]) game->board;
	u8(*marks)[h][w] = (u8(*)[h][w]) game->marks;

	if (
		(*board)[by][bx] == '#' ||
		(*board)[by][bx] == 0 ||
		(*marks)[by][bx] == 0 ||
		get_box(game, state, bx, by) != -1
	) {
		return FALSE;
	}

	state_init(ret, state, game->ngoals);
	ret->positions[0] = (point){ px, py };
	ret->positions[bi] = (point){ bx, by };
	ret->distance = dist;

	return TRUE;
}

bool solve_dfs(Game *game, State *ret)
{
	State *init_state = &game->state;

	TrbHashTable visited;
	trb_hash_table_init_data(&visited, (game->ngoals + 1) * sizeof(point), 1, 0xdeadbeef, trb_jhash, (TrbCmpDataFunc) pos_cmp, &game->ngoals);
	trb_hash_table_insert(&visited, init_state->positions, trb_get_ptr(bool, TRUE));

	TrbDeque vertices;
	trb_deque_init(&vertices, TRUE, sizeof(State));
	trb_deque_push_back(&vertices, init_state);

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

			u32 bi = get_box(game, &vertex, px, py);
			if (bi != -1) {
				do_something = push(game, &vertex, px, py, bx, by, bi, 0, &next);
				csol = dirs[i].push_char;
			} else {
				do_something = move(game, &vertex, px, py, 0, &next);
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

u32 heuristic(Game *game, State *state)
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

bool solve_astar(Game *game, State *ret)
{
	State *init_state = &game->state;
	u32 init_x = init_state->positions[0].x;
	u32 init_y = init_state->positions[0].y;

	TrbHashTable visited;
	trb_hash_table_init_data(&visited, (game->ngoals + 1) * sizeof(point), 1, 0xdeadbeef, trb_jhash, (TrbCmpDataFunc) pos_cmp, &game->ngoals);
	trb_hash_table_insert(&visited, init_state->positions, trb_get_ptr(bool, TRUE));

	TrbHeap vertices;
	trb_heap_init_data(&vertices, sizeof(State), (TrbCmpDataFunc) state_cmp, &game->ngoals);
	trb_heap_insert(&vertices, init_state);

	while (vertices.deque.len != 0) {
		State vertex;
		trb_heap_pop_front(&vertices, &vertex);
		trb_hash_table_add(&visited, vertex.positions, trb_get_ptr(bool, TRUE));

		point pos = vertex.positions[0];
		u32 x = pos.x;
		u32 y = pos.y;
		u32 vd = vertex.distance;

		u32 dl, du, dr, dd;

		if (x == init_x) {
			dl = 1;
			dr = 1;
		} else {
			dl = (x > init_x) ? vd - 1 : vd + 1;
			dr = (x > init_x) ? vd + 1 : vd - 1;
		}

		if (y == init_y) {
			du = 1;
			dd = 1;
		} else {
			du = (y > init_y) ? vd - 1 : vd + 1;
			dd = (y > init_y) ? vd + 1 : vd - 1;
		}

		struct {
			u32 px, py;
			u32 bx, by;
			u32 dist;
			char move_char;
			char push_char;
		} dirs[4] = {
			{x - 1,  y,     x - 2, y,     dl, 'l', 'L'},
			{ x,     y - 1, x,     y - 2, du, 'u', 'U'},
			{ x + 1, y,     x + 2, y,     dr, 'r', 'R'},
			{ x,     y + 1, x,     y + 2, dd, 'd', 'D'},
		};

		for (u32 i = 0; i < 4; ++i) {
			u32 px = dirs[i].px;
			u32 py = dirs[i].py;
			u32 bx = dirs[i].bx;
			u32 by = dirs[i].by;
			u32 d = dirs[i].dist;

			bool do_something;
			State next;
			char csol;

			u32 bi = get_box(game, &vertex, px, py);
			if (bi != -1) {
				do_something = push(game, &vertex, px, py, bx, by, bi, d, &next);
				csol = dirs[i].push_char;
			} else {
				do_something = move(game, &vertex, px, py, d, &next);
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

				u32 total_distance = vertex.distance + 1 + heuristic(game, &next);
				usize index;

				if (trb_heap_search(&vertices, &next, &index)) {
					if (next.distance < vertex.distance + 1) {
						State *old = trb_heap_ptr(&vertices, State, index);
						state_destroy(old);

						next.distance = vertex.distance + 1;
						next.total_distance = total_distance;
						trb_heap_set(&vertices, index, &next);
					} else {
						state_destroy(&next);
					}
				} else {
					next.distance = vertex.distance + 1;
					next.total_distance = total_distance;
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

bool solve_cbfs(Game *game, State *ret)
{
	State *init_state = &game->state;
	u32 init_x = init_state->positions[0].x;
	u32 init_y = init_state->positions[0].y;

	TrbHashTable visited;
	trb_hash_table_init_data(&visited, (game->ngoals + 1) * sizeof(point), 1, 0xdeadbeef, trb_jhash, (TrbCmpDataFunc) pos_cmp, &game->ngoals);
	trb_hash_table_insert(&visited, init_state->positions, trb_get_ptr(bool, TRUE));

	TrbHeap vertices;
	trb_heap_init_data(&vertices, sizeof(State), (TrbCmpDataFunc) state_cmp, &game->ngoals);
	trb_heap_insert(&vertices, init_state);

	while (vertices.deque.len != 0) {
		State vertex;
		trb_heap_pop_front(&vertices, &vertex);
		trb_hash_table_add(&visited, vertex.positions, trb_get_ptr(bool, TRUE));

		point pos = vertex.positions[0];
		u32 x = pos.x;
		u32 y = pos.y;
		u32 vd = vertex.distance;

		u32 dl, du, dr, dd;

		if (x == init_x) {
			dl = 1;
			dr = 1;
		} else {
			dl = (x > init_x) ? vd - 1 : vd + 1;
			dr = (x > init_x) ? vd + 1 : vd - 1;
		}

		if (y == init_y) {
			du = 1;
			dd = 1;
		} else {
			du = (y > init_y) ? vd - 1 : vd + 1;
			dd = (y > init_y) ? vd + 1 : vd - 1;
		}

		struct {
			u32 px, py;
			u32 bx, by;
			u32 dist;
			char move_char;
			char push_char;
		} dirs[4] = {
			{x - 1,  y,     x - 2, y,     dl, 'l', 'L'},
			{ x,     y - 1, x,     y - 2, du, 'u', 'U'},
			{ x + 1, y,     x + 2, y,     dr, 'r', 'R'},
			{ x,     y + 1, x,     y + 2, dd, 'd', 'D'},
		};

		for (u32 i = 0; i < 4; ++i) {
			u32 px = dirs[i].px;
			u32 py = dirs[i].py;
			u32 bx = dirs[i].bx;
			u32 by = dirs[i].by;
			u32 d = dirs[i].dist;

			bool do_something;
			State next;
			char csol;

			u32 bi = get_box(game, &vertex, px, py);
			if (bi != -1) {
				do_something = push(game, &vertex, px, py, bx, by, bi, d, &next);
				csol = dirs[i].push_char;
			} else {
				do_something = move(game, &vertex, px, py, d, &next);
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

				u32 total_distance = next.distance + heuristic(game, &next);

				if (!trb_heap_search(&vertices, &next, NULL)) {
					next.total_distance = total_distance;
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

void parse_board(Game *game, u32 w, u32 h, const char *str)
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

enum {
	PULL_GOAL_DIST,
	MANHATTAN_DIST,
	PYTHAGOREAN_DIST,
};

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

enum {
	HUNGARIAN_ASSIGN,
	GREEDY_ASSIGN,
	CLOSEST_ASSIGN,
};

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

void show_board(const Game *game, const State *state)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*board)[h][w] = malloc(sizeof *board);
	assert(board != NULL);

	memcpy(board, game->board, w * h);

	point p = state->positions[0];
	if ((*board)[p.y][p.x] == GOAL)
		(*board)[p.y][p.x] = PLAYER_ON_GOAL;
	else
		(*board)[p.y][p.x] = PLAYER;

	for (u32 i = 1; i < game->ngoals + 1; ++i) {
		point b = state->positions[i];
		if ((*board)[b.y][b.x] == GOAL)
			(*board)[b.y][b.x] = BOX_ON_GOAL;
		else
			(*board)[b.y][b.x] = BOX;
	}

	u8(*marks)[h][w] = (u8(*)[h][w]) game->marks;

	for (u32 y = 0; y < h; ++y) {
		for (u32 x = 0; x < w; ++x) {
			if ((*board)[y][x]) {
				if ((*board)[y][x] != FLOOR)
					putchar((*board)[y][x]);
				else if ((*marks)[y][x] == 0)
					putchar('x');
				else
					putchar(FLOOR);

			} else {
				putchar(FLOOR);
			}
		}

		putchar('\n');
	}

	free(board);
}

int main(int argc, char *argv[])
{
	const char *s = "#######"
					"#     #"
					"#     #"
					"#. #  #"
					"#. $$ #"
					"#.$$  #"
					"#.#  @#"
					"#######";

	const char *p = "  ###   "
					"  #.#   "
					"  # ####"
					"###$ $.#"
					"#. $@###"
					"####$#  "
					"   #.#  "
					"   ###  ";

	Game game;
	game_init(&game);
	parse_board(&game, 7, 8, s);
	game_calc_distances(&game, PULL_GOAL_DIST);
	game_do_assignment(&game, HUNGARIAN_ASSIGN);

	show_board(&game, &game.state);

	State kek;
	bool solution = solve_cbfs(&game, &kek);

	if (solution) {
		printf("%s\n", kek.solution.data);
		state_destroy(&kek);
	}

	game_destroy(&game);

	return 0;
}
