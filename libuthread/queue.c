#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

typedef struct node* node_t;

struct node {
	void *data;
	node_t next;
};

node_t node_create(void *data, node_t next) {
	node_t node = (node_t) malloc(sizeof(struct node));
	if (node) {
		node->data = data;
		node->next = next;
	}
	return node;
}

struct queue {
	node_t front;
	node_t rear;
	int length;
};

queue_t queue_create(void)
{
	queue_t candidates = (queue_t) malloc(sizeof(struct queue));
	if (candidates) {
		candidates->front = candidates->rear = NULL;
		candidates->length = 0;
	}
	return candidates;
}

int queue_destroy(queue_t queue)
{
	if (!queue || queue->length > 0) {
		return -1;
	}

	free(queue);	

	return 0;
}

int queue_enqueue(queue_t queue, void *data)
{
	if (!queue || !data) {
		return -1;
	}

	node_t node = node_create(data, NULL);
	if (!node) {
		return -1;
	}
	if (queue->rear) {
		queue->rear->next = node;
	}
	if (!queue->front) {
		queue->front = node;
	}
	queue->rear = node;
	queue->length++;

	return 0;
}

int queue_dequeue(queue_t queue, void **data)
{
	if (!queue || !data || !queue->front) {
		return -1;
	}

	node_t node_oldest = queue->front;
	(*data) = node_oldest->data;
	queue->front = node_oldest->next;

	if (node_oldest == queue->rear) {
		queue->rear = NULL;
	}
	free(node_oldest);
	queue->length--;

	return 0;
}

int queue_delete(queue_t queue, void *data)
{
	if (!queue || !data || !queue->front) {
		return -1;
	}

	node_t prev = NULL, node = queue->front;
	while (node) {
		if (node->data == data) {
			if (node == queue->front) {
				queue->front = node->next;
			}
			if (node == queue->rear) {
				queue->rear = prev;
			}
			if (prev) {
				prev->next = node->next;
			}
			free(node);
			queue->length--;
			return 0;
		}
		prev = node;
		node = node->next;
	}

	return -1;
}

int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data)
{
	if (!queue || !func) {
		return -1;
	}

	node_t prev, node;
	node = queue->front;
	while (node) {
		prev = node;
		node = node->next;
		if ((*func)(queue, prev->data, arg) == 1) {
			if (data) {
				*data = prev->data;
			}
			break;
		}
	}

	return 0;
}

int queue_length(queue_t queue)
{
	return queue->length;
}

