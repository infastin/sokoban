#include "Distance.h"

#include "Definitions.h"
#include "Queue.h"

#include <assert.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <tribble/Tribble.h>

u32 **pull_goal_distance(u32 *board, u32 *goals_pos, u32 *positions, u32 w, u32 h, u32 ngoals)
{
	u32 **distances = (u32 **) alloc_2d(4, ngoals, w * h);
	memset_2d((void **) distances, 4, 0xff, ngoals, w * h);

	Queue queue;
	queue_init(&queue);

	for (u32 goal = 0; goal < ngoals; ++goal) {
		distances[goal][goals_pos[goal]] = 0;
		queue_push(&queue, goals_pos[goal]);

		while (!queue_empty(&queue)) {
			u32 pos = queue_pop(&queue);
			u32 x = pos % w;
			u32 y = pos / w;

			for (u32 dir = 0; dir < 4; ++dir) {
				u32 ppos = 0;
				u32 bpos = 0;

				switch (dir) {
				case LEFT:
					if (x <= 1)
						continue;
					bpos = pos - 1;
					ppos = pos - 2;
					break;
				case UP:
					if (y <= 1)
						continue;
					bpos = pos - w;
					ppos = pos - (w << 1);
					break;
				case RIGHT:
					if (x >= w - 2)
						continue;
					bpos = pos + 1;
					ppos = pos + 2;
					break;
				case DOWN:
					if (y >= h - 2)
						continue;
					bpos = pos + w;
					ppos = pos + (w << 1);
					break;
				}

				if (distances[goal][bpos] == U32_MAX) {
					if (board[bpos] != WALL && board[ppos] != WALL) {
						distances[goal][bpos] = distances[goal][pos] + 1;
						queue_push(&queue, bpos);
					}
				}
			}
		}
	}

	u32 **result = (u32 **) alloc_2d(4, ngoals, ngoals);

	for (u32 goal = 0; goal < ngoals; ++goal) {
		for (u32 box = 0; box < ngoals; ++box) {
			result[goal][box] = distances[goal][positions[box + 1]];
		}
	}

	free(distances);

	return result;
}

u32 **manhattan_distance(u32 *board, u32 *goals_pos, u32 *positions, u32 w, u32 h, u32 ngoals)
{
	u32 **result = (u32 **) alloc_2d(4, ngoals, ngoals);

	for (u32 goal = 0; goal < ngoals; ++goal) {
		i32 goal_x = goals_pos[goal] % w;
		i32 goal_y = goals_pos[goal] / w;

		for (u32 box = 0; box < ngoals; ++box) {
			i32 box_x = positions[box + 1] % w;
			i32 box_y = positions[box + 1] / w;

			result[goal][box] = (u32) (abs(goal_x - box_x) + abs(goal_y - box_y));
		}
	}

	return result;
}

u32 **pythagorean_distance(u32 *board, u32 *goals_pos, u32 *positions, u32 w, u32 h, u32 ngoals)
{
	u32 **result = (u32 **) alloc_2d(4, ngoals, ngoals);

	for (u32 goal = 0; goal < ngoals; ++goal) {
		i32 goal_x = goals_pos[goal] % w;
		i32 goal_y = goals_pos[goal] / w;

		for (u32 box = 0; box < ngoals; ++box) {
			i32 box_x = positions[box + 1] % w;
			i32 box_y = positions[box + 1] / w;

			i32 diff_x = goal_x - box_x;
			i32 diff_y = goal_y - box_y;

			result[goal][box] = (u32) sqrt((double) ((diff_x * diff_x) + (diff_y * diff_y)));
		}
	}

	return result;
}
