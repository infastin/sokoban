#ifndef QUEUE_H_VBINDBPX
#define QUEUE_H_VBINDBPX

#include <tribble/List.h>
#include <tribble/Messages.h>

typedef List Queue;
typedef struct _QueueNode QueueNode;

struct _QueueNode {
	Queue entry;
	u32 val;
};

void queue_push(Queue *queue, u32 val);
u32 queue_pop(Queue *queue);
bool queue_empty(const Queue *queue);
void queue_destroy(Queue *queue);
Queue *queue_init(Queue *queue);

#endif /* end of include guard: QUEUE_H_VBINDBPX */
