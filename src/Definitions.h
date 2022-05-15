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

struct _Edge {
	u32 goal;
	u32 box;
	u32 distance;
};

i32 cmp_edges(const Edge *a, const Edge *b);

void **alloc_2d(u32 elemsize, u32 rows, u32 cols);
void **copy_2d(void **dst, const void **src, u32 elemsize, u32 rows, u32 cols);
void **memset_2d(void **arr, u32 elemsize, i32 c, u32 rows, u32 cols);

#endif /* end of include guard: DEFINITIONS_H_KKTMTE8L */
