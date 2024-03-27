#include <stdio.h>
#include "list.h"
#include "cpu.h"
#include "schedulers.h"
#define COMPARABLE priority
#include "priority_queue.h"

int used[MAX_SIZE];
struct PriorityQueue pq = {{0}, 0};

struct node *tasks = NULL;
struct node *prev = NULL;

int min(int a, int b) {
    return a <= b ? a : b;
}

Task *pickNextTask_cycle() {
    if (!prev)
        return NULL;

    Task *nextTask = prev->task;
    prev = prev->next;

    return nextTask;
}

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
    ++used[priority];
    insert_pq(&pq, newTask);
}

void schedule_helper_cycle() {
    Task *currTask;
    while ((currTask = pickNextTask_cycle())) {
        int delta = min(QUANTUM, currTask->burst);
        if (!(currTask->burst - delta)) {
            currTask->end_time = currTime + delta;
        }
        run(currTask, delta);
        currTask->burst -= delta;
        if (!currTask->burst) {
            delete(&tasks, currTask);
            free(currTask);
            return;
        }
    }
}

void schedule() {
    Task *currTask;
    currTask = pickNextTask();
    while (currTask != NULL) {
        int cntInSamePriority = used[currTask->priority];
        if (cntInSamePriority > 1) {
            used[currTask->priority] = 0;
            while (cntInSamePriority) {
                insert(&tasks, currTask);
                currTask = pickNextTask();
                --cntInSamePriority;
            }

            while (tasks) {
                schedule_helper_cycle();
                prev = tasks;
            }
            continue;
        }

        if (!currTask->start_time)
            currTask->end_time = currTime + currTask->burst;

        run(currTask, currTask->burst);
        free(currTask);
        currTask = pickNextTask();
    }
    show_times();
}
