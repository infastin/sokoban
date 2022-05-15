#include "Assign.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	u32 **table = (u32 **) alloc_2d(4, 4, 4);

	table[0][0] = 82;
	table[0][1] = 82;
	table[0][2] = 82;
	table[0][3] = 82;

	table[1][0] = 82;
	table[1][1] = 82;
	table[1][2] = 82;
	table[1][3] = 82;

	table[2][0] = 82;
	table[2][1] = 82;
	table[2][2] = 82;
	table[2][3] = 82;

	table[3][0] = 82;
	table[3][1] = 82;
	table[3][2] = 82;
	table[3][3] = 82;

	Edge *matching = hungarian_assignment(table, 4);

	for (u32 i = 0; i < 4; ++i) {
		printf("Worker %u and Job %u -> %u\n", matching[i].goal, matching[i].box, matching[i].distance);
	}

	free(table);
	free(matching);

	return 0;
}
