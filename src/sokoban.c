#include "Definitions.h"
#include "Queue.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <tribble/Tribble.h>

typedef struct _State State;

struct _State {
	u32 *positions;
};

State *state_new()
{
	State *state = talloc(State, 1);
	state->positions = NULL;
	return state;
}

typedef struct _Game Game;

struct _Game {
	u32 width;
	u32 height;
	u32 nboxes;
	u32 ngoals;

	u32 *goals_pos;
	u8 *board;
	u8 *marks;

	u32 **distances;

	State *state;
};

Game *game_init(Game *game)
{
	if (game == NULL) {
		game = talloc(Game, 1);
		assert(game);
	}

	game->width = 0;
	game->height = 0;
	game->nboxes = 0;
	game->ngoals = 0;
	game->board = NULL;
	game->marks = NULL;
	game->goals_pos = NULL;
	game->distances = NULL;
	game->state = state_new();

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

	if (y > 1 && game->board[pos - w] != WALL && game->board[pos - (w << 1)] != WALL)
		mark(game, pos - w);
	if (y < h - 2 && game->board[pos + w] != WALL && game->board[pos + (w << 1)] != WALL)
		mark(game, pos + w);
	if (x > 1 && game->board[pos - 1] != WALL && game->board[pos - 1] != WALL)
		mark(game, pos - 1);
	if (x < w - 2 && game->board[pos + 1] != WALL && game->board[pos + 2] != WALL)
		mark(game, pos + 1);
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
			game->nboxes++;
		case PLAYER:
		case FLOOR:
			game->board[i] = FLOOR;
			continue;

		case BOX_ON_GOAL:
			game->nboxes++;
		case GOAL:
		case PLAYER_ON_GOAL:
			game->ngoals++;
			game->board[i] = GOAL;
			continue;

		default:
			continue;
		}
	}

	game->goals_pos = calloc(game->ngoals, 4);
	game->state->positions = calloc(game->nboxes + 1, 4);

	for (u32 i = 0, j = 0, k = 0; i < width * height; ++i) {
		if (game->board[i] == GOAL)
			mark(game, i);

		switch (str[i]) {
		case GOAL:
			game->goals_pos[k++] = i;
			continue;

		case PLAYER_ON_GOAL:
			game->goals_pos[k++] = i;
		case PLAYER:
			game->state->positions[0] = i;
			continue;

		case BOX_ON_GOAL:
			game->goals_pos[k++] = i;
		case BOX:
			game->state->positions[++j] = i;
			continue;

		default:
			continue;
		}
	}

	calc_distances(game);
	assign_goals_and_boxes(game);
}

void show_board(const Game *game)
{
	u8 board[game->width * game->height];
	memcpy(board, game->board, game->width * game->height);

	if (board[game->state->positions[0]] == GOAL)
		board[game->state->positions[0]] = PLAYER_ON_GOAL;
	else
		board[game->state->positions[0]] = PLAYER;

	for (u32 i = 1; i < game->nboxes + 1; ++i) {
		if (board[game->state->positions[i]] == GOAL)
			board[game->state->positions[i]] = BOX_ON_GOAL;
		else
			board[game->state->positions[i]] = BOX;
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

	Game game;
	game_init(&game);
	parse_board(&game, 7, 8, s);

	show_board(&game);

	return 0;
}
