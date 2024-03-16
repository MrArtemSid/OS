#include <stdio.h>
#include <stdlib.h>
#include "task.h"
#include "list.h"
#include "cpu.h"
#include "schedulers.h"

struct node *tasks = NULL;

Task *pickNextTask() {
    if (!tasks)
        return NULL;

    Task *newTask = tasks->task;
    delete(&tasks, newTask);

    return tasks->task;
}

void add(char *name, int priority, int burst) {
    Task *newTask = malloc(sizeof(Task));
    newTask->name = name;
    newTask->priority = priority;
    newTask->burst = burst;
    insert(&tasks, newTask);
}

void schedule() {
    Task *currTask;
    while ((currTask = pickNextTask())) {
        run(currTask, currTask->burst);
    }
}