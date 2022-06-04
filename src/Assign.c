#include "Assign.h"

#include "Definitions.h"

#include <assert.h>
#include <memory.h>
#include <tribble/Tribble.h>

enum {
	NONE,
	MARK,
	PRIME
};

static void __hungarian_reduce_rows(u32 ngoals, u32 (*table)[ngoals][ngoals])
{
	for (u32 i = 0; i < ngoals; ++i) {
		u32 lowest = (*table)[i][0];

		for (u32 j = 1; j < ngoals; ++j) {
			if ((*table)[i][j] < lowest)
				lowest = (*table)[i][j];
		}

		for (u32 j = 0; j < ngoals; ++j) {
			(*table)[i][j] -= lowest;
		}
	}
}

static void __hungarian_reduce_cols(u32 ngoals, u32 (*table)[ngoals][ngoals])
{
	for (u32 j = 0; j < ngoals; ++j) {
		u32 lowest = (*table)[0][j];

		for (u32 i = 1; i < ngoals; ++i) {
			if ((*table)[i][j] < lowest)
				lowest = (*table)[i][j];
		}

		if (lowest == 0)
			continue;

		for (u32 i = 0; i < ngoals; ++i) {
			(*table)[i][j] -= lowest;
		}
	}
}

static void __hungarian_mark(u32 ngoals, u32 (*table)[ngoals][ngoals], u8 (*marks)[ngoals][ngoals])
{
	memset(marks, 0, sizeof *marks);

	u8 covered_rows[ngoals];
	memset(covered_rows, 0, ngoals);

	u8 covered_cols[ngoals];
	memset(covered_cols, 0, ngoals);

	for (u32 i = 0; i < ngoals; ++i) {
		for (u32 j = 0; j < ngoals; ++j) {
			if (covered_rows[i] || covered_cols[j])
				break;

			if ((*table)[i][j] == 0) {
				(*marks)[i][j] = MARK;
				covered_rows[i] = 1;
				covered_cols[j] = 1;
			}
		}
	}
}

static u32 __hungarian_count_covered(
	u32 ngoals,
	u8 (*marks)[ngoals][ngoals],
	u8 covered_rows[ngoals],
	u8 covered_cols[ngoals]
)
{
	u32 count = 0;

	for (u32 i = 0; i < ngoals; ++i) {
		for (u32 j = 0; j < ngoals; ++j) {
			if ((covered_rows[i] || covered_cols[j]) && ((*marks)[i][j] == MARK))
				count++;
		}
	}

	return count;
}

static void __hungarian_cover_cols(u32 ngoals, u8 (*marks)[ngoals][ngoals], u8 covered_cols[ngoals])
{
	for (u32 i = 0; i < ngoals; ++i) {
		for (u32 j = 0; j < ngoals; ++j) {
			if ((*marks)[i][j] == MARK) {
				covered_cols[j] = 1;
			}
		}
	}
}

static bool __hungarian_find_prime(
	u32 ngoals,
	u32 (*table)[ngoals][ngoals],
	u8 (*marks)[ngoals][ngoals],
	u8 covered_rows[ngoals],
	u8 covered_cols[ngoals],
	u32 *prime_row,
	u32 *prime_col
)
{
	for (u32 i = 0; i < ngoals; ++i) {
		u32 mark_col = U32_MAX;

		for (u32 j = 0; j < ngoals; ++j) {
			if ((*marks)[i][j] == MARK) {
				mark_col = j;
				continue;
			}

			if (covered_cols[j])
				continue;

			if ((*table)[i][j] != 0)
				continue;

			(*marks)[i][j] = PRIME;

			if (mark_col != U32_MAX) {
				covered_rows[i] = 1;
				covered_cols[mark_col] = 0;
				continue;
			}

			*prime_row = i;
			*prime_col = j;

			return TRUE;
		}
	}

	return FALSE;
}

static void __hungarian_alt_marks(
	u32 ngoals,
	u8 (*marks)[ngoals][ngoals],
	u32 prime_row,
	u32 prime_col
)
{
	while (1) {
		u32 mark_col = prime_col;
		u32 mark_row = U32_MAX;

		for (u32 i = 0; i < ngoals; ++i) {
			if (i == prime_row)
				continue;

			if ((*marks)[i][prime_col] == MARK) {
				mark_row = i;
				break;
			}
		}

		(*marks)[prime_row][prime_col] = MARK;

		if (mark_row == U32_MAX)
			break;

		(*marks)[mark_row][mark_col] = NONE;

		for (u32 j = 0; j < ngoals; ++j) {
			if (j == prime_col)
				continue;

			if ((*marks)[mark_row][j] == PRIME) {
				prime_row = mark_row;
				prime_col = j;
				break;
			}
		}
	}
}

static void __hungarian_add_and_subtract(
	u32 ngoals,
	u32 (*table)[ngoals][ngoals],
	u8 covered_rows[ngoals],
	u8 covered_cols[ngoals]
)
{
	u32 lowest = U32_MAX;

	for (u32 i = 0; i < ngoals; ++i) {
		if (covered_rows[i])
			continue;

		for (u32 j = 0; j < ngoals; ++j) {
			if (covered_cols[j])
				continue;

			if ((*table)[i][j] < lowest) {
				lowest = (*table)[i][j];
			}
		}
	}

	for (u32 i = 0; i < ngoals; ++i) {
		for (u32 j = 0; j < ngoals; ++j) {
			if (!covered_rows[i] && !covered_cols[j])
				(*table)[i][j] -= lowest;
			else if (covered_rows[i] && covered_cols[j])
				(*table)[i][j] += lowest;
		}
	}
}

/* Used for debugging */
TRB_UNUSED static void __hungarian_print_table(
	u32 ngoals,
	u32 (*table)[ngoals][ngoals],
	u8 (*marks)[ngoals][ngoals],
	u8 covered_rows[ngoals],
	u8 covered_cols[ngoals]
)
{
	for (u32 i = 0; i < ngoals; ++i) {
		for (u32 j = 0; j < ngoals; ++j) {
			if (covered_rows[i] || covered_cols[j]) {
				printf("\e[30;47m");
			}

			if (marks != NULL && (*marks)[i][j]) {
				if ((*marks)[i][j] == MARK)
					printf("%4d*", (*table)[i][j]);
				else if ((*marks)[i][j] == PRIME)
					printf("%4d'", (*table)[i][j]);
			} else {
				printf("%4d ", (*table)[i][j]);
			}

			if (covered_rows[i] || covered_cols[j]) {
				printf("\e[0m");
			}
		}

		printf("\n");
	}
}

u32 *hungarian_assignment(u32 ngoals, u32 (*distances)[ngoals][ngoals])
{
	u8 covered_rows[ngoals];
	memset(covered_rows, 0, ngoals);

	u8 covered_cols[ngoals];
	memset(covered_cols, 0, ngoals);

	u32(*table)[ngoals][ngoals] = malloc(sizeof *table);
	assert(table != NULL);

	memcpy(table, distances, sizeof *table);

	__hungarian_reduce_rows(ngoals, table);
	__hungarian_reduce_cols(ngoals, table);

	u8(*marks)[ngoals][ngoals] = malloc(sizeof *marks);
	assert(marks != NULL);

	__hungarian_mark(ngoals, table, marks);

	while (1) {
		__hungarian_cover_cols(ngoals, marks, covered_cols);

		u32 n_covered = __hungarian_count_covered(ngoals, marks, covered_rows, covered_cols);
		if (n_covered == ngoals)
			break;

		u32 prime_row, prime_col;

		while (1) {
			bool found_prime = __hungarian_find_prime(
				ngoals, table, marks,
				covered_rows, covered_cols,
				&prime_row, &prime_col
			);

			if (found_prime)
				break;

			__hungarian_add_and_subtract(ngoals, table, covered_rows, covered_cols);
		}

		__hungarian_alt_marks(ngoals, marks, prime_row, prime_col);

		memset(covered_rows, 0, ngoals);
		memset(covered_cols, 0, ngoals);
	}

	u32 *matching = malloc(ngoals * sizeof(u32));
	assert(matching != NULL);

	for (u32 i = 0; i < ngoals; ++i)
		for (u32 j = 0; j < ngoals; ++j)
			if ((*marks)[i][j] == MARK)
				matching[i] = j;

	free(table);
	free(marks);

	return matching;
}

u32 *closest_assignment(u32 ngoals, u32 (*distances)[ngoals][ngoals])
{
	u32 *matching = malloc(ngoals * sizeof(u32));
	assert(matching != NULL);

	TrbVector unmatched_boxes;
	trb_vector_init(&unmatched_boxes, TRUE, 4);
	for (u32 box = 0; box < ngoals; ++box) {
		trb_vector_push_back(&unmatched_boxes, trb_get_ptr(u32, box));
	}

	TrbVector unmatched_goals;
	trb_vector_init(&unmatched_goals, TRUE, 4);
	for (u32 goal = 0; goal < ngoals; ++goal) {
		trb_vector_push_back(&unmatched_goals, trb_get_ptr(u32, goal));
	}

	for (u32 i = 0; i < unmatched_goals.len; ++i) {
		u32 goal = trb_vector_get(&unmatched_goals, u32, i);
		u32 closest_j = 0;
		u32 closest_box = trb_vector_get(&unmatched_boxes, u32, 0);
		u32 closest_dist = (*distances)[goal][closest_box];

		for (u32 j = 1; j < unmatched_boxes.len; ++j) {
			u32 box = trb_vector_get(&unmatched_boxes, u32, j);
			u32 dist = (*distances)[goal][box];

			if (dist > closest_dist) {
				closest_j = j;
				closest_box = box;
				closest_dist = dist;
			}
		}

		trb_vector_remove(&unmatched_boxes, closest_j, NULL);
		matching[goal] = closest_box;
	}

	trb_vector_destroy(&unmatched_boxes, NULL);
	trb_vector_destroy(&unmatched_goals, NULL);

	return matching;
}

u32 *greedy_assignment(u32 ngoals, u32 (*distances)[ngoals][ngoals])
{
	TrbHeap pqueue;
	trb_heap_init(&pqueue, sizeof(Edge), (TrbCmpFunc) cmp_edges);

	for (u32 goal = 0; goal < ngoals; ++goal) {
		for (u32 box = 0; box < ngoals; ++box) {
			Edge edge = {
				.goal = goal,
				.box = box,
				.distance = (*distances)[goal][box],
			};

			trb_heap_insert(&pqueue, &edge);
		}
	}

	u32 *matching = malloc(ngoals * sizeof(u32));
	assert(matching != NULL);

	TrbVector unmatched_boxes;
	trb_vector_init(&unmatched_boxes, TRUE, 4);
	for (u32 box = 0; box < ngoals; ++box) {
		trb_vector_push_back(&unmatched_boxes, trb_get_ptr(u32, box));
	}

	TrbVector unmatched_goals;
	trb_vector_init(&unmatched_goals, TRUE, 4);
	for (u32 goal = 0; goal < ngoals; ++goal) {
		trb_vector_push_back(&unmatched_goals, trb_get_ptr(u32, goal));
	}

	while (pqueue.deque.len != 0) {
		Edge edge;
		trb_heap_pop_back(&pqueue, &edge);

		usize umb_index = 0;
		usize umg_index = 0;

		bool umb_contains = trb_vector_search(&unmatched_boxes, &edge.box, (TrbCmpFunc) trb_u32cmp, &umb_index);
		bool umg_contains = trb_vector_search(&unmatched_goals, &edge.goal, (TrbCmpFunc) trb_u32cmp, &umg_index);

		if (umb_contains && umg_contains) {
			matching[edge.goal] = edge.box;
			trb_vector_remove(&unmatched_boxes, umb_index, NULL);
			trb_vector_remove(&unmatched_goals, umg_index, NULL);
		}
	}

	for (u32 i = 0; i < unmatched_goals.len; ++i) {
		u32 goal = trb_vector_get(&unmatched_goals, u32, i);
		u32 closest_j = 0;
		u32 closest_box = trb_vector_get(&unmatched_boxes, u32, 0);
		u32 closest_dist = (*distances)[goal][closest_box];

		for (u32 j = 1; j < unmatched_boxes.len; ++j) {
			u32 box = trb_vector_get(&unmatched_boxes, u32, j);
			u32 dist = (*distances)[goal][box];

			if (dist > closest_dist) {
				closest_j = j;
				closest_box = box;
				closest_dist = dist;
			}
		}

		trb_vector_remove(&unmatched_boxes, closest_j, NULL);
		matching[goal] = closest_box;
	}

	trb_vector_destroy(&unmatched_boxes, NULL);
	trb_vector_destroy(&unmatched_goals, NULL);
	trb_heap_destroy(&pqueue, NULL);

	return matching;
}
