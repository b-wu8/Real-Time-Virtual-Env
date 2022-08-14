#ifndef OCULUS_UTILS_H
#define OCULUS_UTILS_H

#include "oculus_structs.h"

// Parse command line arguments to setup receive socket from which 
// the server will receive information from oculus clients
// client_sk -> pointer to socket id which will be populated
// ip -> string representation of IP Address
// port -> string representation of port number
// return 0 if socket is properly opened, 1 if address can't be found,
//  2 if socket can't be accessed.
int setup_client_socket(int* client_sk, char* ip, char* port);

int setup_spines_socket(int* spines_sk, struct sockaddr_in* spines_mcast_addr);

// Helper function used to format sockaddr_in structs
// format is like <ip>:<port> (e.x. "12.34.56.78:1234")
void format_addr(char *buf, struct sockaddr_in* addr);

double vector3_distance(struct Vector3* a, struct Vector3* b);

struct Vector3 vector3_add(struct Vector3* a, struct Vector3* b);

struct Vector3 vector3_interpolate(struct Vector3* src, struct Vector3* dst, double t);

int server_id_to_name(char* server_name, int server_id);

int server_name_to_info(char* server_name, int* server_id, char* server_ip);

int assign_ingest_server(struct Message* message, struct Context* context);

uint32_t hash(char* str, int len); 
 
uint64_t timestamp();  

#endif