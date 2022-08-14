#ifndef OCULUS_PROTOBUF_H
#define OCULUS_PROTOBUF_H

#include "oculus_structs.h"

// When a message is received, the contents is stored in request.data. 
// This function parses the request.data field to populate the Request struct
// request -> request received from client
// returns 0 (always)
int parse_message(struct MessageWrapper* wrapper, char* buffer, int len);

// Format a response buffer to be sent to the entire session. This response
// buffer contains the state of all players whihc needs to be updated
// response -> response byte stream to send to clients in session
// session -> lobby that response is being sent to
// returns 0 (always)
int format_response(char* response_buffer, struct ProtoMemory* proto_mem, struct Message* response_message);

// Allocate memory for protobuf
int allocate_protobuf_memory(struct ProtoMemory* proto_mem);

#endif