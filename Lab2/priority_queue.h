#ifndef LAB2_PRIORITY_QUEUE_H
#define LAB2_PRIORITY_QUEUE_H

#define MAX_SIZE 100

struct PriorityQueue {
    Task heap[MAX_SIZE];
    int size;
};

void swap(Task *a, Task *b) {
    Task temp = *a;
    *a = *b;
    *b = temp;
}

void heapify(struct PriorityQueue *pq, int i) {
    int smallest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < pq->size && pq->heap[left].COMPARABLE < pq->heap[smallest].COMPARABLE) {
        smallest = left;
    }

    if (right < pq->size && pq->heap[right].COMPARABLE < pq->heap[smallest].COMPARABLE) {
        smallest = right;
    }

    if (smallest != i) {
        swap(&pq->heap[i], &pq->heap[smallest]);
        heapify(pq, smallest);
    }
}

void insert_pq(struct PriorityQueue *pq, Task task) {
    if (pq->size == MAX_SIZE) {
        return;
    }

    int i = pq->size;
    pq->heap[i] = task;
    pq->size++;

    while (i > 0 && pq->heap[(i-1)/2].COMPARABLE > pq->heap[i].COMPARABLE) {
        swap(&pq->heap[i], &pq->heap[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

Task extractMin(struct PriorityQueue *pq) {
    Task empty_task = {"", -1, -1, -1};

    if (pq->size <= 0) {
        return empty_task;
    }

    if (pq->size == 1) {
        pq->size--;
        return pq->heap[0];
    }

    Task root = pq->heap[0];
    pq->heap[0] = pq->heap[pq->size - 1];
    pq->size--;
    heapify(pq, 0);

    return root;
}

#endif //LAB2_PRIORITY_QUEUE_H
