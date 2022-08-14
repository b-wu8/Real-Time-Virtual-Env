#ifndef OCULUS_CREATE_RESPONSE_H
#define OCULUS_CREATE_RESPONSE_H

#include "oculus_structs.h"

int create_presence_response(struct MessageWrapper* response_wrapper, struct Session* session);

int create_syn_response(struct MessageWrapper* response_wrapper, struct Context* context, struct Player* player, struct Session* session);

int create_statistics_response(struct MessageWrapper* response_wrapper, struct Context* context, struct MessageWrapper* request_wrapper);

int create_heartbeat_request(struct MessageWrapper* request_wrapper, struct Context* context);

int create_world_response(struct MessageWrapper* response_wrapper, struct Session* session);

#endif