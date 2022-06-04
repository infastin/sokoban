#ifndef DISTANCE_H_ITF1IFZB
#define DISTANCE_H_ITF1IFZB

#include "Definitions.h"

void pull_goal_distance(
	point *goals,
	point *positions,
	u32 w, u32 h,
	u8 (*board)[h][w],
	u32 ngoals,
	u32 (*ret)[ngoals][h][w]
);

void manhattan_distance(
	point *goals,
	point *positions,
	u32 w, u32 h,
	u8 (*board)[h][w],
	u32 ngoals,
	u32 (*ret)[ngoals][h][w]
);

void pythagorean_distance(
	point *goals,
	point *positions,
	u32 w, u32 h,
	u8 (*board)[h][w],
	u32 ngoals,
	u32 (*ret)[ngoals][h][w]
);

void transform_distances(
	point *positions,
	u32 w, u32 h,
	u32 ngoals,
	u32 (*distances)[ngoals][h][w],
	u32 (*ret)[ngoals][ngoals]
);

#endif /* end of include guard: DISTANCE_H_ITF1IFZB */
