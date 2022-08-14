/*
 * Server send and receive functions.
 */

#ifndef OCULUS_SERVER_API_H
#define OCULUS_SERVER_API_H

#include "oculus_structs.h"

int oculus_sendto(struct Context* context, struct MessageWrapper* wrapper, struct sockaddr_in* addr, int is_spines);

int oculus_recvfrom(struct Context* context, struct MessageWrapper* wrapper, int is_spines);

#endif