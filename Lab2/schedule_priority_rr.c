#include "stdio.h"
#include "list.h"
#include "cpu.h"
#include "schedulers.h"
#define COMPARABLE priority
#include "priority_queue.h"

int used[MAX_PRIORITY + 1];
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
    ++used[priority];
    insert_pq(&pq, newTask);
}

void schedule_helper_cycle() {
    Task *currTask;
    while ((currTask = pickNextTask_cycle())) {
        int delta = min(QUANTUM, currTask->burst);
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
    while ((currTask = pickNextTask())) {
        if (used[currTask->priority] > 1) {
            while (used[currTask->priority] > 0) {
                insert(&tasks, currTask);
                currTask = pickNextTask();
                --used[currTask->priority];
            }

            while (tasks) {
                schedule_helper_cycle();
                prev = tasks;
            }
        }

        run(currTask, currTask->burst);
        free(currTask);
    }
}
