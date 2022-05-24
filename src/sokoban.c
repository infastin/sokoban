#include "Definitions.h"
#include "Queue.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <tribble/Deque.h>
#include <tribble/Tribble.h>

typedef struct _State State;

struct _State {
	TrbString solution;
	u32 *positions;
};

State *state_new(State *state, State *init, usize ngoals)
{
	if (state == NULL) {
		state = trb_talloc(State, 1);
		assert(state != NULL);
	}

	state->positions = calloc(ngoals + 1, 4);
	assert(state->positions != NULL);

	if (init != NULL)
		memcpy(state->positions, init->positions, (ngoals + 1) * 4);

	trb_string_init0(&state->solution);

	if (init != NULL && init->solution.data != NULL)
		trb_string_assign(&state->solution, init->solution.data);

	return state;
}

i32 state_cmp(const u32 *a, const u32 *b, u32 *data)
{
	u32 ngoals = *data;

	for (u32 i = 0; i <= ngoals; ++i) {
		if (a[i] > b[i])
			return 1;
		if (a[i] < b[i])
			return -1;
	}

	return 0;
}

void state_free(State *state)
{
	free(state->positions);
	trb_string_destroy(&state->solution);
}

typedef struct _Game Game;

struct _Game {
	u32 width;
	u32 height;
	u32 ngoals;

	u32 *goals_pos;
	u8 *board;
	u8 *marks;

	u32 **distances;

	State state;
};

Game *game_init(Game *game)
{
	if (game == NULL) {
		game = trb_talloc(Game, 1);
		assert(game);
	}

	game->width = 0;
	game->height = 0;
	game->ngoals = 0;
	game->board = NULL;
	game->marks = NULL;
	game->goals_pos = NULL;
	game->distances = NULL;

	return game;
}

void mark(Game *game, u32 pos)
{
	if (game->marks[pos])
		return;

	u32 w = game->width;
	u32 h = game->height;
	u32 x = pos % w;
	u32 y = pos / w;

	game->marks[pos] = 1;

	if (x > 2 && game->board[pos - 1] != WALL && game->board[pos - 2] != WALL)
		mark(game, pos - 1);
	if (x < w - 2 && game->board[pos + 1] != WALL && game->board[pos + 2] != WALL)
		mark(game, pos + 1);
	if (y > 2 && game->board[pos - w] != WALL && game->board[pos - 2 * w] != WALL)
		mark(game, pos - w);
	if (y < h - 2 && game->board[pos + w] != WALL && game->board[pos + 2 * w] != WALL)
		mark(game, pos + w);
}

u32 get_box(Game *game, State *state, u32 pos)
{
	for (u32 i = 1; i <= game->ngoals; ++i) {
		if (state->positions[i] == pos)
			return i;
	}

	return -1;
}

bool is_solved(Game *game, State *state)
{
	for (u32 i = 0; i < game->ngoals; ++i) {
		if (get_box(game, state, game->goals_pos[i]) == -1)
			return FALSE;
	}

	return TRUE;
}

bool move(Game *game, State *state, u32 pos, State *ret)
{
	if (game->board[pos] == '#' || game->board[pos] == 0)
		return FALSE;

	state_new(ret, state, game->ngoals);
	ret->positions[0] = pos;

	return TRUE;
}

bool push(Game *game, State *state, u32 ppos, u32 bpos, u32 box_index, State *ret)
{
	if (game->board[bpos] == '#' || game->board[bpos] == 0 || get_box(game, state, bpos) != -1)
		return FALSE;

	state_new(ret, state, game->ngoals);
	ret->positions[0] = ppos;
	ret->positions[box_index] = bpos;

	return TRUE;
}

void show_board(const Game *game, const State *state);

bool solve(Game *game, State *ret)
{
	State *init_state = &game->state;

	TrbHashTable visited;
	trb_hash_table_init_data(&visited, (game->ngoals + 1) * 4, 1, 0xdeadbeef, trb_jhash, (TrbCmpDataFunc) state_cmp, &game->ngoals);
	trb_hash_table_insert(&visited, init_state->positions, trb_get_ptr(bool, TRUE));

	TrbDeque vertices;
	trb_deque_init(&vertices, sizeof(State));
	trb_deque_push_back(&vertices, init_state);

	while (vertices.len != 0) {
		State vertex;
		trb_deque_pop_front(&vertices, &vertex);

		u32 pos = vertex.positions[0];
		u32 w = game->width;

		struct {
			u32 ppos;
			u32 bpos;
			char mc;
			char pc;
		} dirs[4] = {
			{pos - 1,     pos - 2, 'l', 'L'},
			{pos - w, pos - 2 * w, 'u', 'U'},
			{pos + 1,     pos + 2, 'r', 'R'},
			{pos + w, pos + 2 * w, 'd', 'D'},
		};

		for (u32 i = 0; i < 4; ++i) {
			u32 ppos = dirs[i].ppos;
			u32 bpos = dirs[i].bpos;

			bool do_something;
			State next;
			char csol;

			u32 box_index = get_box(game, &vertex, ppos);
			if (box_index != -1) {
				do_something = push(game, &vertex, ppos, bpos, box_index, &next);
				csol = dirs[i].pc;
			} else {
				do_something = move(game, &vertex, ppos, &next);
				csol = dirs[i].mc;
			}

			if (do_something) {
				if (trb_hash_table_lookup(&visited, next.positions, NULL)) {
					state_free(&next);
					continue;
				}

				trb_string_push_back_c(&next.solution, csol);

				if (is_solved(game, &next)) {
					trb_hash_table_destroy(&visited, NULL, NULL);
					trb_deque_destroy(&vertices, (TrbFreeFunc) state_free);
					state_free(&vertex);

					*ret = next;
					return TRUE;
				}

				trb_deque_push_back(&vertices, &next);
				trb_hash_table_insert(&visited, next.positions, trb_get_ptr(bool, TRUE));
			}
		}

		state_free(&vertex);
	}

	trb_hash_table_destroy(&visited, NULL, NULL);
	trb_deque_destroy(&vertices, (TrbFreeFunc) state_free);

	return FALSE;
}

void parse_board(Game *game, u32 width, u32 height, const char *str)
{
	game->width = width;
	game->height = height;

	game->board = calloc(width * height, 1);
	assert(game->board);

	game->marks = calloc(width * height, 1);
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

	state_new(&game->state, NULL, game->ngoals);
	game->goals_pos = calloc(game->ngoals, 4);

	for (u32 i = 0, j = 1, k = 0; i < width * height; ++i) {
		if (game->board[i] == GOAL)
			mark(game, i);

		switch (str[i]) {
		case GOAL:
			game->goals_pos[k++] = i;
			continue;

		case PLAYER_ON_GOAL:
			game->goals_pos[k++] = i;
		case PLAYER:
			game->state.positions[0] = i;
			continue;

		case BOX_ON_GOAL:
			game->goals_pos[k++] = i;
		case BOX:
			game->state.positions[j++] = i;
			continue;

		default:
			continue;
		}
	}
}

void show_board(const Game *game, const State *state)
{
	u8 board[game->width * game->height];
	memcpy(board, game->board, game->width * game->height);

	if (board[state->positions[0]] == GOAL)
		board[state->positions[0]] = PLAYER_ON_GOAL;
	else
		board[state->positions[0]] = PLAYER;

	for (u32 i = 1; i < game->ngoals + 1; ++i) {
		if (board[state->positions[i]] == GOAL)
			board[state->positions[i]] = BOX_ON_GOAL;
		else
			board[state->positions[i]] = BOX;
	}

	for (u32 i = 0; i < game->width * game->height; ++i) {
		if (board[i]) {
			putchar(board[i]);
		} else {
			putchar(FLOOR);
		}

		if ((i + 1) % game->width == 0)
			putchar('\n');
	}
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

	show_board(&game, &game.state);

	State kek;
	solve(&game, &kek);

	printf("%s\n", kek.solution.data);

	free(game.board);
	free(game.goals_pos);
	free(game.marks);

	state_free(&kek);

	return 0;
}
