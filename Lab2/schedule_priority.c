#include "stdio.h"
#include "list.h"
#include "cpu.h"
#include "schedulers.h"
#define COMPARABLE priority
#include "priority_queue.h"

struct PriorityQueue pq = {{0}, 0};

Task *pickNextTask() {
    Task newTask = extractMin(&pq);
    if (newTask.burst == -1)
        return NULL;

    Task *ptrNewTask = malloc(sizeof(Task));

    *ptrNewTask = newTask;

    return ptrNewTask;
}

void add(char *name, int priority, int burst) {
    Task newTask;
    newTask.name = name;
    newTask.priority = priority;
    newTask.burst = burst;

    insert_pq(&pq, newTask);
}

void schedule() {
    Task *currTask;
    while ((currTask = pickNextTask())) {
        run(currTask, currTask->burst);
        free(currTask);
    }
}
