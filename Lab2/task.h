/**
 * Representation of a task in the system.
 */

#ifndef TASK_H
#define TASK_H

extern int currTime;
extern int cntTasks;
extern int responseTime;
extern int turnaroundTime;
extern int waitingTime;
extern int cntTasks;
void show_times();


// representation of a task
typedef struct task {
    char *name;
    int tid;
    int priority;
    int burst;

    int start_time;
    int end_time;
} Task;

#endif
