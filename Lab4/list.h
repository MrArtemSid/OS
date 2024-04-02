/* this represents work that has to be
   completed by a thread in the pool */
typedef struct
{
    void (*function)(void *p);
    void *data;
}
task;

/**
 * list data structure containing the tasks in the system
 */

struct node {
    task *t;
    struct node *next;
};

// insert and delete operations.
void insert(struct node **head, task *t);
void delete(struct node **head, task *t);
void traverse(struct node *head);
