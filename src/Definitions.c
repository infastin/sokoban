#include "Definitions.h"

#include <assert.h>
#include <memory.h>
#include <stdlib.h>

i32 cmp_edges(const Edge *a, const Edge *b)
{
	if (a->distance > b->distance)
		return -1;

	if (a->distance < b->distance)
		return 1;

	return 0;
}
