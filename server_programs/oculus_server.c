#include "oculus_structs.h"
#include "oculus_handlers.h"
#include "oculus_context.h"
#include "oculus_utils.h"
#include "oculus_priorityqueue.h"
#include "oculus_protobuf.h"
#include "oculus_server_api.h"
#include "oculus_create_response.h"

int main(int argc, char *argv[])
{
    // Setup global context
    struct Context context;
    if (setup_context(&context, argc, argv))
        exit(1);

    // Server forloop variables
    fd_set mask;
    struct timeval loop_timeout;
    struct MessageWrapper wrapper;
    memset(&wrapper, 0, sizeof(struct MessageWrapper));
    int num, len;            
    uint64_t last_server_heartbeat = timestamp();
    uint64_t last_server_worldview = timestamp();
    uint64_t last_presence_sent = timestamp();
    struct timeval ONE_MS_TIMEOUT = (struct timeval){0};
    ONE_MS_TIMEOUT.tv_usec = 1000;
    
    // Create temporary buffers for print outs
    printf("Awaiting messages from Oculus...\n");

    // Tbh, I still don't know how masks work
    memcpy(&loop_timeout, &context.timeout, sizeof(struct timeval));
    for (;;)
    { 
        // Initialize file descriptor set
        FD_ZERO(&mask);
        FD_SET(context.client_sk, &mask);
        FD_SET(context.spines_sk, &mask);

        // FIXME: explicitly use gettimeofday() to update loop_timeout
        num = select(FD_SETSIZE, &mask, NULL, NULL, &loop_timeout);
        if (num > 0) {  // Received messages
            if (FD_ISSET(context.client_sk, &mask)) {
                len = oculus_recvfrom(&context, &wrapper, 0); // UDP receive
                if (len < 0)
                    exit(1);
                
                if (wrapper.message.type == SYN)
                    assign_ingest_server(&wrapper.message, &context);

                if (wrapper.message.type == HEARTBEAT) {
                    handle_message(&context, &wrapper);
                } else {
                    printf("sent message to spines [ digest=%u type=%d sender_id=%d server=%d ]\n", 
                        wrapper.digest, wrapper.message.type, wrapper.message.sender_id, wrapper.server_id);
                    oculus_sendto(&context, &wrapper, &context.spines_mcast_addr, 1);  // Spines send
                }
            } else if (FD_ISSET(context.spines_sk, &mask)) {
                len = oculus_recvfrom(&context, &wrapper, 1);
                if (len < 0)  // spines receive
                    exit(1);
                if (wrapper.message.type == SERVER_HEARTBEAT) {
                    struct sockaddr_in from_addr;
                    memcpy(&wrapper.from_addr, &from_addr, sizeof(struct sockaddr_in));
                    handle_message(&context, &wrapper);
                }
                else
                    pq_push(&context.queue, &wrapper);
            } else {
                printf("Error: Unexpected file descriptor is set...\n");
            }
        } else { // timeout
            struct MessageWrapper* top_wrapper;
            uint64_t now = timestamp();
            while ((top_wrapper = pq_top(&context.queue)) && now - top_wrapper->unix_time > context.sync_delay_us) {
                handle_message(&context, top_wrapper);
                pq_pop(&context.queue);
            }
            if (now - last_server_heartbeat > context.server_heartbeat_time_us)  {  // 60 mill microseconds = 1 minute
                create_heartbeat_request(&wrapper, &context);
                oculus_sendto(&context, &wrapper, &context.spines_mcast_addr, 1);
                last_server_heartbeat = now;
            }
            if (now - last_presence_sent > 1000000)  {  // 1 mill microseconds = 1 sec
                for (int i = 0; i < context.num_sessions; i++) {
                    create_presence_response(&wrapper, &context.sessions[i]);
                    for (int j = 0; j < context.sessions[i].num_players; j++)
                        oculus_sendto(&context, &wrapper, &context.sessions[i].players[j].addr, 0);
                }
                last_presence_sent = now;
            }
            if(now - last_server_worldview > context.tick_delay_s * (double)1000000) {
                send_world_state(&context);
                last_server_worldview = now;
            }

            memcpy(&loop_timeout, &ONE_MS_TIMEOUT, sizeof(struct timeval));  // Reset loop timeout
        }
    }
}
