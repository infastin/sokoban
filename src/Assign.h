#ifndef ASSIGN_H_TORHCHXI
#define ASSIGN_H_TORHCHXI

#include "Definitions.h"

u32 *hungarian_assignment(u32 ngoals, u32 (*distances)[ngoals][ngoals]);
u32 *greedy_assignment(u32 ngoals, u32 (*distances)[ngoals][ngoals]);
u32 *closest_assignment(u32 ngoals, u32 (*distances)[ngoals][ngoals]);

#endif /* end of include guard: ASSIGN_H_TORHCHXI */
