/**
 * Implementation of thread pool.
 */

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include "list.h"
#include "threadpool.h"

#define QUEUE_SIZE 10
#define NUMBER_OF_THREADS 3

#define TRUE 1

// the work queue
task worktodo;

// the worker bees
pthread_t bees[NUMBER_OF_THREADS];
pthread_mutex_t queue_lock = PTHREAD_MUTEX_INITIALIZER;
sem_t sem;

struct node *queue = NULL;

// insert a task into the queue
// returns 0 if successful or 1 otherwise,
int enqueue(task t)
{
    int ret = 0;

    task *newTask = malloc(sizeof(task));
    if (!newTask) {
        ret = 1;
        return ret;
    }

    newTask->function = t.function;
    newTask->data = t.data;

    pthread_mutex_lock(&queue_lock);
    insert(&queue, newTask);
    pthread_mutex_unlock(&queue_lock);

    return ret;
}

// remove a task from the queue
task dequeue()
{
    pthread_mutex_lock(&queue_lock);
    delete(&queue, queue->t);
    pthread_mutex_unlock(&queue_lock);
    return worktodo;
}

// the worker thread in the thread pool
void *worker(void *param)
{
    while (queue != NULL) {
        sem_wait(&sem);
        execute(queue->t->function, queue->t->data);
        dequeue();
    }
}

/**
 * Executes the task provided to the thread pool
 */
void execute(void (*somefunction)(void *p), void *p)
{
    (*somefunction)(p);
}

/**
 * Submits work to the pool.
 */
int pool_submit(void (*somefunction)(void *p), void *p)
{
    int ret = 0;

    worktodo.function = somefunction;
    worktodo.data = p;
    ret = enqueue(worktodo);

    if (!ret)
        sem_post(&sem);

    return ret;
}

// initialize the thread pool
void pool_init(void)
{
    pthread_mutex_init(&queue_lock, NULL);
    sem_init(&sem, 0, 0);
    for (int th = 0; th < NUMBER_OF_THREADS; ++th) {
        pthread_create(&bees[th],NULL,worker,NULL);
    }
}

// shutdown the thread pool
void pool_shutdown(void)
{
    pthread_mutex_destroy(&queue_lock);
    sem_destroy(&sem);
    for (int th = 0; th < NUMBER_OF_THREADS; ++th) {
        pthread_cancel(bees[th]);
        pthread_join(bees[th],NULL);
    }
}
