#include "Queue.h"

Queue *queue_init(Queue *queue)
{
	return list_init(queue);
}

void __queue_free(void *ptr)
{
	QueueNode *node = list_entry(ptr, QueueNode, entry);
	free(node);
}

void queue_push(Queue *queue, u32 val)
{
	return_if_fail(queue != NULL);

	QueueNode *node = talloc(QueueNode, 1);
	return_if_fail(node != NULL);

	node->val = val;
	list_node_init(&node->entry);

	list_push_front(queue, &node->entry);
}

u32 queue_pop(Queue *queue)
{
	return_val_if_fail(queue != NULL, 0);

	Queue *node = list_pop_back(queue);
	QueueNode *qnode = list_entry(node, QueueNode, entry);

	u32 val = qnode->val;
	free(qnode);

	return val;
}

bool queue_empty(const Queue *queue)
{
	return list_empty(queue);
}

void queue_destroy(Queue *queue)
{
	list_destroy(queue, __queue_free);
}
