#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"
#include "queue.h"

// #define __DEBUG

enum states
{
	READY,
	RUNNING,
	BLOCKED,
	ZOMBIE
};

struct tcb
{
	uthread_t tid; // Range: [0, USHRT_MAX)
	enum states state;
	uthread_ctx_t context;
	void *stack;
	int retval;
	uthread_t joining_tid; // Range: [0, USHRT_MAX), it hasn't corresponding joining thread if joining_tid == USHRT_MAX
};

typedef struct tcb *tcb_t;

static uthread_t tid_idle = 0;		// Used to allocate free tid
static queue_t candidates = NULL;	// The scheduling queue of candidate threads
static tcb_t current_tcb = NULL;	// Currently working thread

int __is_alive_joined_tcb(queue_t queue, void *data, void *arg)
{
	(void)queue;
	uthread_t tid = *((uthread_t *)arg);
	tcb_t t = (tcb_t)data;
	if (!t)
	{
		return 0;
	}
	if (t->joining_tid == tid && t->state != ZOMBIE)
	{
		return 1;
	}
	return 0;
}

int __is_available_tcb_with_tid(queue_t queue, void *data, void *arg)
{
	(void)queue;
	tcb_t t = (tcb_t)data;
	uthread_t tid = *((uthread_t *)arg);
	if (!t || t->tid != tid)
	{
		return 0;
	}
	if (t->state == READY || t->state == BLOCKED)
	{
		return 1;
	}
	return 0;
}

int uthread_start(int preempt)
{
	// TODO: preempt
	(void)preempt;
	candidates = queue_create();
	if (!candidates)
	{
		return -1; // memory allocation
	}

	current_tcb = (tcb_t)calloc(1, sizeof(struct tcb));
	if (!current_tcb)
	{
		return -1; // memory allocation
	}
	current_tcb->tid = tid_idle++;
	current_tcb->state = RUNNING;
	if (getcontext(&current_tcb->context))
	{
		return -1;
	}
	current_tcb->stack = current_tcb->context.uc_stack.ss_sp;
	current_tcb->joining_tid = USHRT_MAX;
	return 0;
}

int uthread_stop(void)
{
#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] uthread_stop()\n");
#endif
	if (uthread_self() != 0)
	{
		return -1; // This function should only be called by the main execution thread
	}
	// if (queue_length(candidates) > 0)
	// {
	// 	return -1;
	// }
	while (queue_length(candidates) > 0)
	{ // Wait until all threads exited
		uthread_yield();
	}
	queue_destroy(candidates);
	candidates = NULL;
	current_tcb = NULL;
	return 0;
}

int uthread_create(uthread_func_t func)
{
	tcb_t t = (tcb_t)calloc(1, sizeof(struct tcb));
	if (!t)
	{
		return -1; // memory allocation
	}
	if (tid_idle == USHRT_MAX)
	{
		return -1; // TID overflow
	}
	t->tid = tid_idle++;
	t->state = READY;
	t->stack = uthread_ctx_alloc_stack();
	if (uthread_ctx_init(&t->context, t->stack, func) == -1)
	{
		return -1; // context creation
	}
	t->retval = 0;
	t->joining_tid = USHRT_MAX;
	queue_enqueue(candidates, t);
	return t->tid;
}

void uthread_yield(void)
{
#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] yield()::0 [%d]\n", queue_length(candidates));
#endif
	tcb_t tcb_old = current_tcb;
	tcb_t tcb_new = NULL;

#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] yield()::1\n");
#endif
	while (queue_length(candidates) > 0)
	{
		queue_dequeue(candidates, (void **)&tcb_new);
		if (!tcb_new)
		{
			return;
		}
		if (tcb_new->state == ZOMBIE)
		{
			if (tcb_new->joining_tid == USHRT_MAX)
			{
				if (tcb_new->stack)
				{
					uthread_ctx_destroy_stack(tcb_new->stack);
				}
				free(tcb_new);
			}
			else
			{
				queue_enqueue(candidates, (void *)tcb_new);
			}
			tcb_new = NULL;
			continue;
		}
		if (tcb_new->state == READY || tcb_new->state == BLOCKED)
		{
			tcb_t tcb_alive_joined = NULL;
			if (queue_iterate(candidates, __is_alive_joined_tcb, (void *)&tcb_new->tid, (void **)&tcb_alive_joined) == 0 && tcb_alive_joined)
			{
				queue_enqueue(candidates, (void *)tcb_new);
				tcb_new = NULL;
				continue;
			}
			else if (current_tcb->joining_tid == tcb_new->tid && current_tcb->state != ZOMBIE)
			{
				queue_enqueue(candidates, (void *)tcb_new);
				tcb_new = NULL;
				continue;
			}
		}
		break;
	}

#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] yield()::2\n");
#endif
	if (!tcb_new || queue_enqueue(candidates, tcb_old) != 0)
	{
		return;
	}
#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] yield()::{%d [%d]} => {%d [%d]}\n", tcb_old->tid, tcb_old->state, tcb_new->tid, tcb_new->state);
#endif
	// printf("	%d\n", tcb_old->joining_tid);

	if (tcb_old->state != ZOMBIE)
	{
		tcb_old->state = BLOCKED;
	}
	tcb_new->state = RUNNING;
	current_tcb = tcb_new;
#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] yield()::uthread_ctx_switch() : BEG\n");
#endif
	uthread_ctx_switch(&tcb_old->context, &tcb_new->context);
#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] yield()::uthread_ctx_switch() : END\n");
#endif
}

uthread_t uthread_self(void)
{
	assert(current_tcb);
	return current_tcb->tid;
}

void uthread_exit(int retval)
{
	assert(current_tcb);
	current_tcb->retval = retval;
	current_tcb->state = ZOMBIE;
#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] exit()::uthread_yield() : BEG\n");
#endif
	uthread_yield();
#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] exit()::uthread_yield() : END\n");
#endif
}

int uthread_join(uthread_t tid, int *retval)
{
	if (tid == 0 || tid == uthread_self())
	{
		return -1;
	}
	tcb_t t = NULL;
	if (queue_iterate(candidates, __is_available_tcb_with_tid, (void *)&tid, (void **)&t) != 0 || t == NULL)
	{
		return -1; // return -1 if thread @tid cannot be found
	}
	if (t->joining_tid != USHRT_MAX && t->joining_tid != current_tcb->tid)
	{
		return -1; // return -1 if thread @tid is already being joined
	}

	if (t->state != ZOMBIE)
	{
		t->joining_tid = uthread_self();

		current_tcb->state = BLOCKED; // If T2 is still an active thread, T1 must be blocked until T2 dies.
#ifdef __DEBUG
		fprintf(stderr, "[DEBUG] join()::uthread_yield() : BEG\n");
#endif
		uthread_yield();
#ifdef __DEBUG
		fprintf(stderr, "[DEBUG] join()::uthread_yield() : END\n");
#endif
	}

	// When T2 dies, T1 is unblocked and collects T2.
	// If T2 is already dead, T1 can collect T2 right away.
#ifdef __DEBUG
	fprintf(stderr, "[DEBUG] join()::t[%d,%d]->state=%d\n", tid, t->tid, t->state);
#endif
	if (t->state != ZOMBIE)
	{
		return -1;
	}
	if (retval)
	{
		*retval = t->retval;
	}
	t->joining_tid = USHRT_MAX;

	return 0;
}
