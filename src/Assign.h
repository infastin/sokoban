#ifndef ASSIGN_H_TORHCHXI
#define ASSIGN_H_TORHCHXI

#include "Definitions.h"

Edge *hungarian_assignment(u32 **distances, u32 ngoals);
Edge *greedy_assignment(u32 **distances, u32 ngoals);
Edge *closest_assignment(u32 **distances, u32 ngoals);

#endif /* end of include guard: ASSIGN_H_TORHCHXI */
