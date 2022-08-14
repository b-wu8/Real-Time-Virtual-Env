#ifndef OCULUS_HANDLERS_H
#define OCULUS_HANDLERS_H

#include "oculus_structs.h"

// The following request handlers handle messages sent from the client
int handle_message(struct Context* context, struct MessageWrapper* wrapper);

int handle_player_syn(struct Context* context, struct MessageWrapper* wrapper);

int handle_player_continuous(struct Context* context, struct MessageWrapper* wrapper);

int handle_player_heartbeat(struct Context* context, struct MessageWrapper* wrapper);

int handle_player_leave(struct Context* context, struct MessageWrapper* wrapper);

int handle_player_rpc(struct Context* context, struct MessageWrapper* wrapper);

int handle_server_heartbeat(struct Context* context, struct MessageWrapper* wrapper);

#endif