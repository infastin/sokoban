#include "Assign.h"
#include "Definitions.h"
#include "Distance.h"
#include "Game.h"

#include <assert.h>
#include <getopt.h>
#include <memory.h>
#include <ncurses.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <tribble/tribble.h>
#include <unistd.h>

#define WALL_PAIR (COLOR_PAIR(1))
#define FLOOR_PAIR (COLOR_PAIR(2))
#define PLAYER_PAIR (COLOR_PAIR(3))
#define BOX_PAIR (COLOR_PAIR(4))
#define GOAL_PAIR (COLOR_PAIR(5))
#define PLAYER_ON_GOAL_PAIR (COLOR_PAIR(6))
#define BOX_ON_GOAL_PAIR (COLOR_PAIR(7))

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

	for (u32 y = 0; y < h; ++y) {
		for (u32 x = 0; x < w; ++x) {
			switch ((*board)[y][x]) {
			case WALL:
				attron(WALL_PAIR);
				addch(' ');
				attroff(WALL_PAIR);
				break;
			case 0:
			case FLOOR:
				attron(FLOOR_PAIR);
				addch(' ');
				attroff(FLOOR_PAIR);
				break;
			case PLAYER:
				attron(PLAYER_PAIR);
				addch('@');
				attroff(PLAYER_PAIR);
				break;
			case BOX:
				attron(BOX_PAIR);
				addch('$');
				attroff(BOX_PAIR);
				break;
			case GOAL:
				attron(GOAL_PAIR);
				addch('.');
				attroff(GOAL_PAIR);
				break;
			case PLAYER_ON_GOAL:
				attron(PLAYER_ON_GOAL_PAIR);
				addch('+');
				attroff(PLAYER_ON_GOAL_PAIR);
				break;
			case BOX_ON_GOAL:
				attron(BOX_ON_GOAL_PAIR);
				addch('.');
				attroff(BOX_ON_GOAL_PAIR);
				break;
			}
		}

		addch('\n');
	}

	free(board);
}

typedef struct {
	u32 px, py;
	u32 bx, by;
	char type;
} Action;

Action *parse_solution(const char *solution, usize len, u32 init_x, u32 init_y)
{
	Action *actions = malloc(len * sizeof(Action));
	assert(actions != NULL);

	u32 px = init_x;
	u32 py = init_y;

	u32 bx = init_x;
	u32 by = init_y;

	const char *s = solution;
	for (u32 i = 0; *s; ++s, ++i) {
		switch (*s) {
		case 'l':
			px -= 1;
			actions[i].type = 'm';
			break;
		case 'u':
			py -= 1;
			actions[i].type = 'm';
			break;
		case 'r':
			px += 1;
			actions[i].type = 'm';
			break;
		case 'd':
			py += 1;
			actions[i].type = 'm';
			break;
		case 'L':
			px -= 1;
			bx = px - 1;
			by = py;
			actions[i].type = 'p';
			break;
		case 'U':
			py -= 1;
			bx = px;
			by = py - 1;
			actions[i].type = 'p';
			break;
		case 'R':
			px += 1;
			bx = px + 1;
			by = py;
			actions[i].type = 'p';
			break;
		case 'D':
			py += 1;
			bx = px;
			by = py + 1;
			actions[i].type = 'p';
			break;
		}

		actions[i].px = px;
		actions[i].py = py;
		actions[i].bx = bx;
		actions[i].by = by;
	}

	return actions;
}

void visual_move(Game *game, u32 old_x, u32 old_y, u32 new_x, u32 new_y)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*board)[h][w] = (u8(*)[h][w]) game->board;

	if ((*board)[old_y][old_x] == GOAL) {
		attron(GOAL_PAIR);
		mvaddch(old_y, old_x, '.');
		attroff(GOAL_PAIR);
	} else {
		attron(FLOOR_PAIR);
		mvaddch(old_y, old_x, ' ');
		attroff(FLOOR_PAIR);
	}

	if ((*board)[new_y][new_x] == GOAL) {
		attron(PLAYER_ON_GOAL_PAIR);
		mvaddch(new_y, new_x, '+');
		attroff(PLAYER_ON_GOAL_PAIR);
	} else {
		attron(PLAYER_PAIR);
		mvaddch(new_y, new_x, '@');
		attroff(PLAYER_PAIR);
	}
}

void visual_push(Game *game, u32 old_x, u32 old_y, u32 new_x, u32 new_y)
{
	u32 w = game->width;
	u32 h = game->height;

	u8(*board)[h][w] = (u8(*)[h][w]) game->board;

	if ((*board)[old_y][old_x] == GOAL) {
		attron(GOAL_PAIR);
		mvaddch(old_y, old_x, '.');
		attroff(GOAL_PAIR);
	} else {
		attron(FLOOR_PAIR);
		mvaddch(old_y, old_x, ' ');
		attroff(FLOOR_PAIR);
	}

	if ((*board)[new_y][new_x] == GOAL) {
		attron(BOX_ON_GOAL_PAIR);
		mvaddch(new_y, new_x, '*');
		attroff(BOX_ON_GOAL_PAIR);
	} else {
		attron(BOX_PAIR);
		mvaddch(new_y, new_x, '$');
		attroff(BOX_PAIR);
	}
}

#define handle_error(str)   \
	{                       \
		perror(str);        \
		exit(EXIT_FAILURE); \
	}

int main(int argc, char *argv[])
{
	char *filename = NULL;
	bool (*solver)(Game * game, State * ret) = NULL;

	int choice;
	while (1) {
		static struct option long_options[] = {
			{"astar", no_argument, 0, 'a'},
			{ "cbfs", no_argument, 0, 'c'},
			{ "dfs",  no_argument, 0, 'd'},
			{ "help", no_argument, 0, 'h'},

			{ 0,      0,           0, 0  }
		};

		int option_index = 0;

		choice = getopt_long(argc, argv, "acdhf:", long_options, &option_index);
		if (choice == -1)
			break;

		switch (choice) {
		case 'h':
			printf("%s: <solver> <file>\n", argv[0]);
			printf("\nSolvers:\n");
			printf(" -c, --cbfs \tComplete Best First Search algorithm\n");
			printf(" -a, --astar\tA* Search algorithm\n");
			printf(" -d, --dfs  \tDepth First Search algorithm\n");
			return 0;
		case 'a':
			solver = game_solve_astar;
			break;
		case 'c':
			solver = game_solve_cbfs;
			break;
		case 'd':
			solver = game_solve_dfs;
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	if (optind == argc) {
		fprintf(stderr, "The file with a level wasn't specified!\n");
		exit(EXIT_FAILURE);
	}

	filename = argv[optind];

	if (solver == NULL) {
		fprintf(stderr, "No solver specified!\n");
		exit(EXIT_FAILURE);
	}

	FILE *level = fopen(filename, "r");
	if (level == NULL)
		handle_error("fopen");

	u32 w, h;
	if (fscanf(level, " %u %u", &w, &h) < 2) {
		fprintf(stderr, "Wrong level format!\n");
		exit(EXIT_FAILURE);
	}

	char(*board)[h][w] = malloc(sizeof *board);
	assert(board != NULL);

	for (u32 y = 0; y < h; ++y) {
		for (u32 x = 0; x < w;) {
			int c = fgetc(level);
			if (c == '\n')
				continue;
			(*board)[y][x++] = c;
		}
	}

	initscr();
	noecho();
	cbreak();
	curs_set(0);

	if (has_colors() == FALSE) {
		endwin();
		fprintf(stderr, "Your terminal doesn't support colors!\n");
		exit(EXIT_FAILURE);
	}

	start_color();

	init_pair(1, COLOR_WHITE, COLOR_WHITE);
	init_pair(2, COLOR_BLACK, COLOR_BLACK);
	init_pair(3, COLOR_BLACK, COLOR_BLUE);
	init_pair(4, COLOR_BLACK, COLOR_YELLOW);
	init_pair(5, COLOR_BLACK, COLOR_RED);
	init_pair(6, COLOR_BLACK, COLOR_CYAN);
	init_pair(7, COLOR_BLACK, COLOR_GREEN);

	Game game;
	game_init(&game);
	game_parse_board(&game, w, h, (const char *) board);

	show_board(&game, &game.state);
	printw("Calculating...");
	refresh();

	game_calc_distances(&game, PULL_GOAL_DIST);
	game_do_assignment(&game, HUNGARIAN_ASSIGN);

	State sol;
	bool solved = game_solve_dfs(&game, &sol);

	clear();

	if (solved) {
		show_board(&game, &game.state);
		printw("Press 'q' or Ctrl-C to exit\n");
		refresh();

		u32 init_x = game.state.positions[0].x;
		u32 init_y = game.state.positions[0].y;

		u32 actions_len = sol.solution.len;
		Action *actions = parse_solution(sol.solution.data, actions_len, init_x, init_y);

		int tfd = timerfd_create(CLOCK_REALTIME, 0);
		if (tfd < 0)
			handle_error("timerfd_create");

		struct pollfd ufds[2] = {
			{tfd,           POLLIN},
			{ STDIN_FILENO, POLLIN},
		};

		struct itimerspec timer = {
			.it_value = {1,  0        },
			.it_interval = { 0, 500000000},
		};

		if (timerfd_settime(tfd, 0, &timer, NULL) < 0)
			handle_error("timerfd_settime");

		timer.it_value.tv_sec = 0;
		timer.it_value.tv_nsec = 500000000;

		u32 old_px = init_x;
		u32 old_py = init_y;
		u32 i = 0;

		while (1) {
			if (poll(ufds, 2, -1) == -1) {
				refresh();
				continue;
			}

			if (ufds[0].revents & POLLIN) {
				u32 new_px = actions[i].px;
				u32 new_py = actions[i].py;
				u32 new_bx = actions[i].bx;
				u32 new_by = actions[i].by;

				u32 old_bx = new_px;
				u32 old_by = new_py;

				if (actions[i].type == 'p') {
					visual_push(&game, old_bx, old_by, new_bx, new_by);
				}

				visual_move(&game, old_px, old_py, new_px, new_py);

				old_px = new_px;
				old_py = new_py;
				i++;

				refresh();

				if (i == actions_len) {
					timer.it_value.tv_sec = 0;
					timer.it_value.tv_nsec = 0;
				}

				if (timerfd_settime(tfd, 0, &timer, NULL) < 0)
					handle_error("timerfd_settime");
			}

			if (ufds[1].revents & POLLIN) {
				if (getch() == 'q')
					break;
			}
		}
	} else {
		printw("No solution was found!\n");
		printw("Press 'q' or Ctrl-C to exit\n");
		refresh();
	}

	endwin();
	game_destroy(&game);

	return 0;
}
