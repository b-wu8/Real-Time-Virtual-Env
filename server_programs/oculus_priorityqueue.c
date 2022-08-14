#include "oculus_priorityqueue.h"

struct MessageWrapper* pq_get(struct MinPrioQ* pq, int idx) {
    if (idx < 0 || idx > MAX_QUEUE_LEN)
        return NULL;
    if (pq->elements[idx].unix_time == 0)  // considered empty
        return NULL;
    return &pq->elements[idx];
}

int pq_swap(struct MinPrioQ* pq, int i, int j) {
    struct MessageWrapper* elem_i = pq_get(pq, i);
    struct MessageWrapper* elem_j = pq_get(pq, j);
    if (!elem_i || !elem_j)
        return 1;
    struct MessageWrapper temp_elem;
    memcpy(&temp_elem, elem_i, sizeof(struct MessageWrapper));
    memcpy(elem_i, elem_j, sizeof(struct MessageWrapper));
    memcpy(elem_j, &temp_elem, sizeof(struct MessageWrapper));
    return 0;
}

struct MessageWrapper* pq_top(struct MinPrioQ* pq) {
    return pq_get(pq, 0);
}

struct MessageWrapper* pq_flex_min(
    struct MessageWrapper* elem_i, struct MessageWrapper* elem_j, struct MessageWrapper* elem_k
) {
    uint32_t i = elem_i ? elem_i->unix_time : UINT32_MAX;
    uint32_t j = elem_j ? elem_j->unix_time : UINT32_MAX;
    uint32_t k = elem_k ? elem_k->unix_time : UINT32_MAX;
    if (i <= j && i <= k)
        return elem_i;
    else if (j <= i && j <= k)
        return elem_j;
    else
        return elem_k;
}

void pq_print(struct MinPrioQ* pq) {
    return;  // let's stop printing these for now, lol

    if (pq->num_elements == 0) {
        printf("MinPrioQ Empty\n");
        return;
    }
    printf("MinPrioQ Keys: ");
    for (int i = 0; i < pq->num_elements; i++)
        printf("%" PRIu64 " ", pq->elements[i].unix_time);
    printf("\n");
}

int pq_pop(struct MinPrioQ* pq) {
    if (pq->num_elements <= 0) {
        printf("ERROR: priority queue is empty\n");
        return 1;
    }

    // Print removal statement
    printf("Removed message from priority queue [ idx=0 digest=%u type=%d sender_id=%d ]\n", 
        pq->elements[0].digest, pq->elements[0].message.type, pq->elements[0].message.sender_id);

    if (pq->num_elements == 1) {
        memset(&pq->elements[0], 0, sizeof(struct MessageWrapper));
        pq->num_elements--;
        pq_print(pq);
        return 0;
    }
    pq_swap(pq, 0, pq->num_elements - 1);
    memset(&pq->elements[pq->num_elements - 1], 0, sizeof(struct MessageWrapper));
    pq->num_elements--;

    int idx = 0;
    struct MessageWrapper* elem_i;
    struct MessageWrapper* elem_j;
    struct MessageWrapper* elem_k;
    struct MessageWrapper* elem_min;
    while (idx < MAX_QUEUE_LEN) {
        elem_i = pq_get(pq, idx);
        elem_j = pq_get(pq, 2 * idx + 1);
        elem_k = pq_get(pq, 2 * idx + 2);
        elem_min = pq_flex_min(elem_i, elem_j, elem_k);
        if (elem_min == elem_i)  // I am smaller than both children, so we return
            break;
        else if (elem_min == elem_j) {
            pq_swap(pq, idx, 2 * idx + 1);
            idx = 2 * idx + 1;
        } else {  // elem_min == elem_k
            pq_swap(pq, idx, 2 * idx + 2);
            idx = 2 * idx + 2;
        }
    }
    pq_print(pq);
    return 0;
}

int pq_push(struct MinPrioQ* pq, struct MessageWrapper* wrapper) {
    if (pq->num_elements >= MAX_QUEUE_LEN) {
        printf("ERROR: priority queue is full [ max_len=%d ]\n", MAX_QUEUE_LEN);
        return 1;
    }
    if (wrapper->unix_time == 0) {
        printf("ERROR: cannot insert element with unix_time=0 [ digest=%u type=%d sender_id=%d ]\n", 
            wrapper->digest, wrapper->message.type, wrapper->message.sender_id);
        return 2;
    }
    if (pq->num_elements == 0) {
        memcpy(&pq->elements[0], wrapper, sizeof(struct MessageWrapper));
        pq->num_elements++;
        pq_print(pq);
        printf("Added message to priority queue [ idx=0 digest=%u type=%d sender_id=%d ]\n", 
            wrapper->digest, wrapper->message.type, wrapper->message.sender_id);
        return 0;
    }
    
    memcpy(&pq->elements[pq->num_elements], wrapper, sizeof(struct MessageWrapper));
    pq->num_elements++;
    int idx = pq->num_elements - 1;
    struct MessageWrapper* elem_i;
    struct MessageWrapper* elem_j;
    struct MessageWrapper* elem_min;
    while (idx < MAX_QUEUE_LEN) {
        elem_i = pq_get(pq, idx);
        elem_j = pq_get(pq, (idx - 1) / 2);
        elem_min = pq_flex_min(elem_i, elem_j, NULL);
        if (elem_min == elem_j)  // parent is smaller than me, so we return
            break;
        else {  // elem_min == elem_i
            pq_swap(pq, idx, (idx - 1) / 2);
            idx = (idx - 1) / 2;
        }
    }
    
    pq_print(pq);
    printf("Added message to priority queue [ idx=%d digest=%u type=%d sender_id=%d ]\n", 
        idx, wrapper->digest, wrapper->message.type, wrapper->message.sender_id);
    return 0;
}