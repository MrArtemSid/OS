#include <stdio.h>
#include <stdlib.h>
#include "task.h"
#include "list.h"
#include "cpu.h"
#include "schedulers.h"

struct node *tasks = NULL;
struct node *prev = NULL;

int min(int a, int b) {
    return a <= b ? a : b;
}

Task *pickNextTask() {
    if (!prev)
        return NULL;

    Task *nextTask = prev->task;
    prev = prev->next;

    return nextTask;
}

void add(char *name, int priority, int burst) {
    Task *newTask = malloc(sizeof(Task));
    newTask->name = name;
    newTask->priority = priority;
    newTask->burst = burst;
    newTask->start_time = -1;
    newTask->end_time = -1;
    cntTasks += 1;
    insert(&tasks, newTask);
}

void schedule_helper_cycle() {
    Task *currTask;
    while ((currTask = pickNextTask())) {
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
    while (tasks) {
        schedule_helper_cycle();
        prev = tasks;
    }
    show_times();
}


/*
void schedule_helper(Task *currTask) {
    if (!currTask) {
        return;
    }

    Task *nextTask = pickNextTask();
    schedule_helper(nextTask);
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

void schedule_rec() {
    while (tasks) {
        Task *currTask = pickNextTask();
        schedule_helper(currTask);
        prev = tasks;
    }
    show_times();
}
*/
