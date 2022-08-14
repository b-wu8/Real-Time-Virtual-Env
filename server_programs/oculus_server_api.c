#include "oculus_server_api.h"
#include "oculus_protobuf.h"
#include "oculus_utils.h"

int oculus_sendto(struct Context* context, struct MessageWrapper* wrapper, struct sockaddr_in* addr, int is_spines) {
    
    int len;
    char buffer[MAX_MESSAGE_SIZE];
    char addr_str_buff[MAX_STRING_LEN];
    wrapper->server_id = context->server_id;

    if (is_spines) {
        len = spines_sendto(context->spines_sk, wrapper, sizeof(struct MessageWrapper), 
            0, (struct sockaddr*) addr, sizeof(struct sockaddr));
    } else {
        len = format_response(buffer, &context->proto_mem, &wrapper->message);
        len = sendto(context->client_sk, buffer, len, 0, (struct sockaddr*) addr, sizeof(struct sockaddr));
    }

    format_addr(addr_str_buff, addr);
    printf("send message [ digest=%u type=%d size=%dB recv_addr=%s is_spines=%d ]\n",
        wrapper->digest, wrapper->message.type, len, addr_str_buff, is_spines);
    
    return len;
}

int oculus_recvfrom(struct Context* context, struct MessageWrapper* wrapper, int is_spines) {

    int len;
    char buffer[MAX_MESSAGE_SIZE];
    char addr_str_buff[MAX_STRING_LEN];
    struct sockaddr_in from_addr;

    socklen_t sockaddr_len;
    sockaddr_len = sizeof(struct sockaddr);

    if (is_spines) {
        len = spines_recvfrom(context->spines_sk, wrapper, sizeof(struct MessageWrapper), 
            0, (struct sockaddr *) &from_addr, &sockaddr_len);
        if (len < 0) {
            printf("FATAL: Spines daemon has disconnected\nTerminating Server\n");
            return -1;
        }
        if (wrapper->server_id == context->server_id)
            wrapper->recv_time = wrapper->unix_time;
        else 
            wrapper->recv_time = timestamp();
    } else {
        len = recvfrom(context->client_sk, buffer, MAX_MESSAGE_SIZE, 
            0, (struct sockaddr *) &from_addr, &sockaddr_len);
        wrapper->digest = hash(buffer, len);
        if (parse_message(wrapper, buffer, len) != 0) {
            format_addr(addr_str_buff, &from_addr);
            printf("received unparsable message [ size=%dB sender=%s digest=%u ]\n", 
                len, addr_str_buff, wrapper->digest);
            return -2;
        }
        wrapper->server_id = context->server_id;
        wrapper->unix_time = timestamp();  // timestamp the message
        memcpy(&wrapper->from_addr, &from_addr, sizeof(struct sockaddr_in));
    }

    printf("key=STATISTICS type=receive server=%d time=%" PRIu64 " hash=%u\n", 
            context->server_id, timestamp(), wrapper->digest);

    format_addr(addr_str_buff, &from_addr);
    printf("recv message [ digest=%u type=%d size=%dB sender_id=%d send_addr=%s server=%d is_spines=%d ]\n", 
        wrapper->digest, wrapper->message.type, len, wrapper->message.sender_id, addr_str_buff, wrapper->server_id, is_spines);

    return len;
}