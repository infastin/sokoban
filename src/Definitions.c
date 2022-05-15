#include "Definitions.h"

#include <assert.h>
#include <memory.h>
#include <stdlib.h>

i32 cmp_edges(const Edge *a, const Edge *b)
{
	if (a->distance > b->distance)
		return 1;

	if (a->distance < b->distance)
		return -1;

	return 0;
}

void **alloc_2d(u32 elemsize, u32 rows, u32 cols)
{
	void **arr = (void **) malloc((rows * sizeof(void *)) + (rows * cols * elemsize));
	assert(arr != NULL);

	for (u32 i = 0; i < rows; ++i) {
		arr[i] = (void *) (((char *) (arr + rows)) + (i * cols * elemsize));
	}

	return arr;
}

void **memset_2d(void **arr, u32 elemsize, i32 c, u32 rows, u32 cols)
{
	return (void **) memset(arr + rows, c, rows * cols * elemsize);
}

void **copy_2d(void **dst, const void **src, u32 elemsize, u32 rows, u32 cols)
{
	if (dst == NULL) {
		dst = alloc_2d(elemsize, rows, cols);
		assert(dst != NULL);
	}

	memcpy(dst + rows, src + rows, rows * cols * elemsize);

	return dst;
}
