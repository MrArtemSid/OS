/**
 * "Virtual" CPU that also maintains track of system time.
 */

#include <stdio.h>
#include "task.h"
int currTime = 0;
int cntTasks = 0;
int responseTime = 0;
int turnaroundTime = 0;
int waitingTime = 0;

// run this task for the specified time slice
void run(Task *task, int slice) {

    if (task->start_time == 0) {
        task->start_time = currTime;
        responseTime += task->start_time;
    }

    if (task->end_time != 0) {
        turnaroundTime += task->end_time;
        waitingTime += task->end_time - task->start_time;
    }

    printf("Running task = [%s] [%d] [%d] for %d units.\n",task->name, task->priority, task->burst, slice);
    waitingTime -= slice;
    currTime += slice;
}

void show_times() {
    printf("\nTasks amount: %d\n", cntTasks);
    printf("Current time: %d\n", currTime);
    printf("Average turnaround time: %f\n", (double)turnaroundTime / cntTasks);
    printf("Average response time: %f\n", (double)responseTime / cntTasks);
    printf("Average waiting time: %f\n", (double)waitingTime / cntTasks);
}
