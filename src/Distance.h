#ifndef DISTANCE_H_ITF1IFZB
#define DISTANCE_H_ITF1IFZB

#include "Definitions.h"

u32 **pull_goal_distance(u32 *board, u32 *goals_pos, u32 *positions, u32 w, u32 h, u32 ngoals);
u32 **manhattan_distance(u32 *board, u32 *goals_pos, u32 *positions, u32 w, u32 h, u32 ngoals);
u32 **pythagorean_distance(u32 *board, u32 *goals_pos, u32 *positions, u32 w, u32 h, u32 ngoals);

#endif /* end of include guard: DISTANCE_H_ITF1IFZB */
