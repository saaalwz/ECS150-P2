#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <queue.h>

#define TEST_ASSERT(assert)				\
do {									\
	printf("ASSERT: " #assert " ... ");	\
	if (assert) {						\
		printf("PASS\n");				\
	} else	{							\
		printf("FAIL\n");				\
		exit(1);						\
	}									\
} while(0)

/* Create */
void test_create(void)
{
	fprintf(stderr, "*** TEST create ***\n");

	TEST_ASSERT(queue_create() != NULL);
}

/* Enqueue/Dequeue simple */
void test_queue_simple(void)
{
	int data = 3, *ptr;
	queue_t q;

	fprintf(stderr, "*** TEST queue_simple ***\n");

	q = queue_create();
	queue_enqueue(q, &data);
	queue_dequeue(q, (void**)&ptr);
	TEST_ASSERT(ptr == &data);
}

void test_queue_advanced(void)
{
	int data[2] = {1024, 2048};
	int *ptr[2] = {NULL, NULL};
	queue_t q = queue_create();

	fprintf(stderr, "*** TEST queue_advanced ***\n");

	TEST_ASSERT(q != NULL);

	TEST_ASSERT(queue_dequeue(q, (void **)&ptr[0]) == -1);	// dequeue when queue is empty

	TEST_ASSERT(queue_enqueue(q, &data[0]) == 0);
	TEST_ASSERT(queue_enqueue(q, &data[1]) == 0);

	TEST_ASSERT(queue_destroy(q) == -1);					// destroy an non-empty queue

	TEST_ASSERT(queue_dequeue(q, (void **)&ptr[0]) == 0);
	TEST_ASSERT(queue_dequeue(q, (void **)&ptr[1]) == 0);

	TEST_ASSERT(queue_length(q) == 0);						// it should be an empty queue

	TEST_ASSERT(ptr[0] == &data[0]);
	TEST_ASSERT(ptr[1] == &data[1]);

	TEST_ASSERT(queue_dequeue(q, (void **)&ptr[0]) == -1);	// dequeue times > enqueue times

	TEST_ASSERT(queue_destroy(q) == 0);						// destroy an empty queue
}

int __find_data(queue_t queue, void *data, void *arg)
{
	(void)queue;
	int *ptr_cur = (int *)data;
	int *ptr_dst = (int *)arg;
	if (ptr_cur != NULL && ptr_cur == ptr_dst) {
		return 1;
	}
	return 0;
}

void test_queue_iterate(void)
{
	queue_t q = queue_create();
	int data[5] = {0, 1024, 2048, 4096, 8196};
	int *ptr = NULL;

	fprintf(stderr, "*** TEST queue_iterate ***\n");

	TEST_ASSERT(q != NULL);

	TEST_ASSERT(queue_enqueue(q, &data[0]) == 0);
	TEST_ASSERT(queue_enqueue(q, &data[1]) == 0);
	TEST_ASSERT(queue_enqueue(q, &data[2]) == 0);

	ptr = NULL;
	TEST_ASSERT(queue_iterate(q, __find_data, &data[3], (void **)&ptr) == 0 && ptr == NULL);		// find a non-existent element
	TEST_ASSERT(queue_iterate(q, __find_data, &data[1], (void **)&ptr) == 0 && ptr == &data[1]);	// find an existing element

	TEST_ASSERT(queue_dequeue(q, (void **)&ptr) == 0 && ptr == &data[0]);
	TEST_ASSERT(queue_dequeue(q, (void **)&ptr) == 0 && ptr == &data[1]);
	TEST_ASSERT(queue_dequeue(q, (void **)&ptr) == 0 && ptr == &data[2]);

	TEST_ASSERT(queue_length(q) == 0);

	TEST_ASSERT(queue_destroy(q) == 0);
}

void test_queue_delete(void)
{
	queue_t q = queue_create();
	int data[5] = {0, 1024, 2048, 4096, 8196};

	fprintf(stderr, "*** TEST queue_delete ***\n");

	TEST_ASSERT(q != NULL);

	TEST_ASSERT(queue_delete(q, &data[3]) == -1);	// delete a non-existent element from the empty queue

	TEST_ASSERT(queue_enqueue(q, &data[0]) == 0);
	TEST_ASSERT(queue_enqueue(q, &data[1]) == 0);
	TEST_ASSERT(queue_enqueue(q, &data[2]) == 0);

	TEST_ASSERT(queue_delete(q, &data[4]) == -1);	// delete a non-existent element from the non-empty queue
	TEST_ASSERT(queue_delete(q, &data[1]) == 0);	// delete the element in the middle
	TEST_ASSERT(queue_delete(q, &data[0]) == 0);	// delete the element at the head
	TEST_ASSERT(queue_delete(q, &data[2]) == 0);	// delete the element at the end / delete the only remaining element

	TEST_ASSERT(queue_length(q) == 0);

	TEST_ASSERT(queue_destroy(q) == 0);
}

int main(void)
{
	test_create();
	test_queue_simple();
	test_queue_advanced();
	test_queue_iterate();
	test_queue_delete();

	return 0;
}
