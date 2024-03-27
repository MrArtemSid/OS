#include "stdio.h"
#include "list.h"
#include "cpu.h"
#include "schedulers.h"
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
    newTask.start_time = 0;
    newTask.end_time = 0;
    cntTasks += 1;

    insert_pq(&pq, newTask);
}

void schedule() {
    Task *currTask;
    while ((currTask = pickNextTask())) {
        currTask->end_time = currTime + currTask->burst;
        run(currTask, currTask->burst);
        free(currTask);
    }
    show_times();
}

