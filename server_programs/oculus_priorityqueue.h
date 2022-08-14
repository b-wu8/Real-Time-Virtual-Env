#ifndef OCULUS_PRIORITYQUEUE_H
#define OCULUS_PRIORITYQUEUE_H

#include "oculus_structs.h"

struct MessageWrapper* pq_get(struct MinPrioQ* pq, int idx);
int pq_swap(struct MinPrioQ* pq, int i, int j);
struct MessageWrapper* pq_top(struct MinPrioQ* pq);
int pq_pop(struct MinPrioQ* pq);
int pq_push(struct MinPrioQ* pq, struct MessageWrapper* mess);

#endif