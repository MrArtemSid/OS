/**
 * Various list operations
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "list.h"

// add a new task to the list of tasks
void insert(struct node **head, task *newTask) {
    // add the new task to the list
    struct node *newNode = malloc(sizeof(struct node));

    newNode->t = newTask;
    newNode->next = *head;
    *head = newNode;
}

// delete the selected task from the list
void delete(struct node **head, task *task) {
    struct node *temp;
    struct node *prev;

    temp = *head;
    // special case - beginning of list
    if (temp->t == task) {
        *head = (*head)->next;
    }
    else {
        // interior or last element in the list
        prev = *head;
        temp = temp->next;
        while (temp->t != task) {
            prev = temp;
            temp = temp->next;
        }

        prev->next = temp->next;
    }
}