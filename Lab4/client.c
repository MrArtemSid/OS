/**
 * Example client program that uses thread pool.
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "threadpool.h"

struct data
{
    int a;
    int b;
};

void add(void *param)
{
    struct data *temp;
    temp = (struct data*)param;

    printf("I add two values %d and %d result = %d\n",temp->a, temp->b, temp->a + temp->b);
}

int main(void)
{
    srand(time(NULL));   // Initialization, should only be called once.

    struct data works[10];
    for (int i = 0; i < sizeof (works) / sizeof (struct data); ++i) {
        works[i].a = rand() % 100;
        works[i].b = rand() % 100;
    }

    // initialize the thread pool
    pool_init();

    for (int i = 0; i < sizeof (works) / sizeof (struct data); ++i) {
        pool_submit(&add,&works[i]);
    }
    // submit the work to the queue

    // may be helpful 
    //sleep(3);

    pool_shutdown();

    return 0;
}
