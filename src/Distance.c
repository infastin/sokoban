#include "Distance.h"

#include "Definitions.h"

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <tribble/tribble.h>

void pull_goal_distance(
	point *goals,
	point *positions,
	u32 w, u32 h,
	u8 (*board)[h][w],
	u32 ngoals,
	u32 (*ret)[ngoals][h][w]
)
{
	memset(ret, 0xff, sizeof *ret);

	TrbDeque queue;
	trb_deque_init(&queue, TRUE, sizeof(point));

	for (u32 goal = 0; goal < ngoals; ++goal) {
		point gpos = goals[goal];

		(*ret)[goal][gpos.y][gpos.x] = 0;
		trb_deque_push_back(&queue, trb_get_ptr(point, gpos.x, gpos.y));

		while (queue.len != 0) {
			point pos;
			trb_deque_pop_front(&queue, &pos);

			u32 x = pos.x;
			u32 y = pos.y;

			struct {
				u32 px, py;
				u32 bx, by;
				bool skip;
			} dirs[4] = {
				{x - 2,  y,     x - 1, y,     x - 2 > x},
				{ x,     y - 2, x,     y - 1, y - 2 > y},
				{ x + 2, y,     x + 1, y,     FALSE    },
				{ x,     y + 2, x,     y + 1, FALSE    },
			};

			for (u32 i = 0; i < 4; ++i) {
				if (dirs[i].skip)
					continue;

				u32 px = dirs[i].px;
				u32 py = dirs[i].py;
				u32 bx = dirs[i].bx;
				u32 by = dirs[i].by;

				if ((*ret)[goal][by][bx] == U32_MAX) {
					if ((*board)[by][bx] != WALL && (*board)[py][px] != WALL) {
						(*ret)[goal][by][bx] = (*ret)[goal][y][x] + 1;
						trb_deque_push_back(&queue, trb_get_ptr(point, bx, by));
					}
				}
			}
		}
	}

	trb_deque_destroy(&queue, NULL);
}

void manhattan_distance(
	point *goals,
	point *positions,
	u32 w, u32 h,
	u8 (*board)[h][w],
	u32 ngoals,
	u32 (*ret)[ngoals][h][w]
)
{
	for (u32 goal = 0; goal < ngoals; ++goal) {
		point gpos = goals[goal];
		i32 goal_x = gpos.x;
		i32 goal_y = gpos.y;

		for (i32 y = 0; y < h; ++y) {
			for (i32 x = 0; x < w; ++x) {
				(*ret)[goal][y][x] = trb_abs_32(goal_x - x) + trb_abs_32(goal_y - y);
			}
		}
	}
}

void pythagorean_distance(
	point *goals,
	point *positions,
	u32 w, u32 h,
	u8 (*board)[h][w],
	u32 ngoals,
	u32 (*ret)[ngoals][h][w]
)
{
	for (u32 goal = 0; goal < ngoals; ++goal) {
		point gpos = goals[goal];
		i32 goal_x = gpos.x;
		i32 goal_y = gpos.y;

		for (i32 y = 0; y < h; ++y) {
			i32 diff_y = goal_y - y;

			for (i32 x = 0; x < w; ++x) {
				i32 diff_x = goal_x - x;

				(*ret)[goal][y][x] = (u32) sqrt((double) ((diff_x * diff_x) + (diff_y * diff_y)));
			}
		}
	}
}

void transform_distances(
	point *positions,
	u32 w, u32 h,
	u32 ngoals,
	u32 (*distances)[ngoals][h][w],
	u32 (*ret)[ngoals][ngoals]
)
{
	for (u32 goal = 0; goal < ngoals; ++goal) {
		for (u32 box = 0; box < ngoals; ++box) {
			point bpos = positions[box + 1];
			(*ret)[goal][box] = (*distances)[goal][bpos.y][bpos.x];
		}
	}
}
