#include "oculus_utils.h"
#include "oculus_server_api.h"

int setup_spines_socket(int* spines_sk, struct sockaddr_in* spines_mcast_addr) {
    if (spines_init(NULL) < 0)
    {
        printf("oculus_server: spines socket error\n");
        exit(1);
    }

    *spines_sk = spines_socket(PF_SPINES, SOCK_DGRAM, SOFT_REALTIME_LINKS, NULL);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(SPINES_RECV_PORT);
    if (spines_bind(*spines_sk, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("oculus_server: spines bind error");
        exit(1);
    }

    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    mreq.imr_multiaddr.s_addr = htonl(SPINES_MCAST_ADDR);
    if (spines_setsockopt(*spines_sk, IPPROTO_IP, SPINES_ADD_MEMBERSHIP, &mreq, sizeof(struct ip_mreq)) < 0)
    {
        perror("oculus_server: spines could join mcast group");
        exit(1);
    }

    spines_mcast_addr->sin_family = AF_INET;
    spines_mcast_addr->sin_port = htons(SPINES_RECV_PORT);
    spines_mcast_addr->sin_addr.s_addr = htonl(SPINES_MCAST_ADDR);

    return 0;
}

int setup_client_socket(int* client_sk, char* ip, char* port) {
    int ret;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    // hints.ai_flags = AI_PASSIVE; // use my IP

    // Get address info
    if ((ret = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((*client_sk = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("oculus_server: socket");
            continue;
        }
        if (bind(*client_sk, p->ai_addr, p->ai_addrlen) == -1) {
            close(*client_sk);
            perror("oculus_server: bind for recv socket failed\n");
            continue;
        } else {
            printf("Receiving on %s:%s\n", ip, port);
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "oculus_server: couldn't open recv socket\n");
        return 2;
    }
    freeaddrinfo(servinfo);

    return 0;
}

void format_addr(char *buf, struct sockaddr_in* addr)
{
    int ip = ntohl(addr->sin_addr.s_addr);
    int port = ntohs(addr->sin_port);
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    sprintf(buf, "%d.%d.%d.%d:%d", bytes[3], bytes[2], bytes[1], bytes[0], port);
}

int server_name_to_info(char* server_name, int* server_id, char* server_ip) {
    if (strcmp(server_name, "server__SJC") == 0) {
        *server_id = 1;
        strcpy(server_ip, "rain1.cnds.jhu.edu");
    } else if (strcmp(server_name, "server__DEN") == 0) {
        *server_id = 3;
        strcpy(server_ip, "rain3.cnds.jhu.edu");
    } else if (strcmp(server_name, "server__LAX") == 0) {
        *server_id = 11;
        strcpy(server_ip, "rain11.cnds.jhu.edu");
    } else if (strcmp(server_name, "server__WAS") == 0) {
        *server_id = 12;
        strcpy(server_ip, "rain12.cnds.jhu.edu");
    } else if (strcmp(server_name, "server__CHI") == 0) {
        *server_id = 13;
        strcpy(server_ip, "rain13.cnds.jhu.edu");
    } else if (strcmp(server_name, "server__DFW") == 0) {
        *server_id = 14;
        strcpy(server_ip, "rain14.cnds.jhu.edu");
    } else if (strcmp(server_name, "server__ATL") == 0) {
        *server_id = 15;
        strcpy(server_ip, "rain15.cnds.jhu.edu");
    } else if (strcmp(server_name, "server__NYC") == 0) {
        *server_id = 16;
        strcpy(server_ip, "rain16.cnds.jhu.edu");
    } else {
        *server_id = 0;
        strcpy(server_ip, "");
        return 1;
    }
    return 0;
}

int server_id_to_name(char* server_name, int server_id) {
    switch(server_id) {
        case 1:
            strcpy(server_name, "SJC");
            break;
        case 3:
            strcpy(server_name, "DEN");
            break;
        case 11:
            strcpy(server_name, "LAX");
            break;
        case 12:
            strcpy(server_name, "WAS");
            break;
        case 13:
            strcpy(server_name, "CHI");
            break;
        case 14:
            strcpy(server_name, "DFW");
            break;
        case 15:
            strcpy(server_name, "ATL");
            break;
        case 16:
            strcpy(server_name, "NYC"); 
            break;      
        default:
            strcpy(server_name, "UNKNOWN");
    }
    // return 0 if we found the server, 1 otherwise
    return strcmp(server_name, "UNKNOWN") == 0 ? 1 : 0;
}

int assign_ingest_server(struct Message* message, struct Context* context) {
    if (message->type != SYN)
        return 1;

    struct SynRequest* request = (struct SynRequest*) message->data;
    char dummy_ip[MAX_STRING_LEN];
    char server_name[128];
    sprintf(server_name, "server__%s", (char*) request->server_name);
    int ret = server_name_to_info(server_name, &request->ingest_server_id, dummy_ip);
    if (ret == 1) {
        printf("Couldn't resolve server name \"%s\", settings ignest server as \"%s\"\n", 
            request->server_name, context->server_name);
        request->ingest_server_id = context->server_id;
        return 1;
    }
    
    return 0;
}

double vector3_distance(struct Vector3* a, struct Vector3* b) {
    return sqrt(pow(a->x - b->x, 2) + pow(a->y - b->y, 2) + pow(a->z - b->z, 2));
}

struct Vector3 vector3_add(struct Vector3* a, struct Vector3* b) {
    struct Vector3 result;
    result.x = a->x + b->x;
    result.y = a->y + b->y;
    result.z = a->z + b->z;
    return result;
}

struct Vector3 vector3_interpolate(struct Vector3* src, struct Vector3* dst, double t) {
    struct Vector3 result;
    result.x = (1 - t) * src->x + t * dst->x;
    result.y = (1 - t) * src->y + t * dst->y;
    result.z = (1 - t) * src->z + t * dst->z;
    return result;
}

// DBJ2
uint32_t hash(char* str, int len) {
    uint8_t* data = (uint8_t*) str;
    unsigned int hash = 5381;
    for (int i = 0; i < len; i++)
        hash = ((hash << 5) + hash) + data[i];  /* hash * 33 + data[i] */
    return hash;
}

uint64_t timestamp() {
    // Returns microseconds since epoch
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return ((uint64_t) tv.tv_sec) * 1000000 + ((uint64_t) tv.tv_usec);
}
