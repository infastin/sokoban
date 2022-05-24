#include "Queue.h"

Queue *queue_init(Queue *queue)
{
	return trb_list_init(queue);
}

void __queue_free(void *ptr)
{
	QueueNode *node = trb_list_entry(ptr, QueueNode, entry);
	free(node);
}

void queue_push(Queue *queue, u32 val)
{
	trb_return_if_fail(queue != NULL);

	QueueNode *node = trb_talloc(QueueNode, 1);
	trb_return_if_fail(node != NULL);

	node->val = val;
	trb_list_node_init(&node->entry);

	trb_list_push_front(queue, &node->entry);
}

u32 queue_pop(Queue *queue)
{
	trb_return_val_if_fail(queue != NULL, 0);

	Queue *node = trb_list_pop_back(queue);
	QueueNode *qnode = trb_list_entry(node, QueueNode, entry);

	u32 val = qnode->val;
	free(qnode);

	return val;
}

bool queue_empty(const Queue *queue)
{
	return trb_list_empty(queue);
}

void queue_destroy(Queue *queue)
{
	trb_list_destroy(queue, __queue_free);
}
