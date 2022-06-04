#ifndef DEFINITIONS_H_KKTMTE8L
#define DEFINITIONS_H_KKTMTE8L

#include <tribble/Types.h>

enum {
	FLOOR = ' ',
	WALL = '#',
	PLAYER = '@',
	BOX = '$',
	GOAL = '.',
	PLAYER_ON_GOAL = '+',
	BOX_ON_GOAL = '*',
};

enum {
	LEFT,
	UP,
	RIGHT,
	DOWN,
};

typedef struct _Edge Edge;

typedef struct _Edge {
	u32 goal;
	u32 box;
	u32 distance;
} Edge;

i32 cmp_edges(const Edge *a, const Edge *b);

typedef struct {
	u32 x, y;
} point;

#endif /* end of include guard: DEFINITIONS_H_KKTMTE8L */
