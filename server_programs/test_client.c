/*
 * Program simulating an Oculus client.
 */

#include "oculus_utils.h"

void Usage(int argc, char *argv[], 
    char *lobby, int *ingest_server, int *port, char *location) {

    char ip_str[MAX_STRING_LEN];
    int i1, i2, i3, i4;

    if (argc == 1) {
        printf("To test the client, please specify the following parameters:\n");
        printf("\t[-n <lobby name>]\n");
        printf("\t[-s <ingest server IP address>]\n");
        printf("\t[-p <port number>]\n");
        printf("\t[-l <location (eg. LAX, DFW, DEN, etc.)>]\n");
        exit(-1);
    }

    while (--argc > 0) {
        argv++;

        if (!strncmp(*argv, "-n", 2)) { 
            sscanf(argv[1], "%s", lobby);
            argc--;
            argv++;
        } else if (!strncmp(*argv, "-s", 2)) { 
            sscanf(argv[1], "%s", ip_str);
            argc--;
            sscanf(ip_str, "%d.%d.%d.%d", &i1, &i2, &i3, &i4);
            *ingest_server = ((i1 << 24) | (i2 << 16) | (i3 << 8) | i4);
            argv++;
        } else if (!strncmp(*argv, "-p", 2)) {
            sscanf(argv[1], "%d", port);
            argc--;
            argv++;
        } else if (!strncmp(*argv, "-l", 2)) {
            sscanf(argv[1], "%s", location);
            argc--;
            argv++;
        } else {
            printf("To test the client, please specify the following parameters:\n");
            printf("\t[-n <lobby name>]\n");
            printf("\t[-s <ingest server IP address>]\n");
            printf("\t[-p <port number>]\n");
            printf("\t[-l <location (eg. LAX, DFW, DEN, etc.)>]\n");
            exit(-1);
        }
    }
}

int main(int argc, char *argv[]) 
{
    int s, buffer_len, ret;
    struct sockaddr_in host, from_addr;
    char lobby[MAX_STRING_LEN];
    int ingest_server;
    int port;
    char location[MAX_STRING_LEN];
    char buffer[MAX_MESSAGE_SIZE];
    fd_set mask;
    socklen_t dummy_len = sizeof(struct sockaddr);
    
    Usage(argc, argv, lobby, &ingest_server, &port, location);
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("Sending Socket creation error");
        exit(1);
    }

    memset(&from_addr, 0, sizeof(from_addr));
    from_addr.sin_family = AF_INET;
    from_addr.sin_port = htons(port + 2);
    from_addr.sin_addr.s_addr = INADDR_ANY;
    
    char buf[MAX_MESSAGE_SIZE];
    format_addr(buf, &from_addr);
    printf("IP address: %s\n", buf);
    
    memset(&host, 0, sizeof(host));
    host.sin_family = AF_INET;
    host.sin_port = htons(port);
    host.sin_addr.s_addr = htonl(ingest_server);

    if (bind(s, (struct sockaddr *) &from_addr, sizeof(from_addr)) < 0) {
        perror("Bind error");
        exit(1);
    }

    // allocate memory 
    ADS__SynRequest test_syn = ADS__SYN_REQUEST__INIT; 
    test_syn.player_name = (char*) calloc(MAX_STRING_LEN, sizeof(char)); 
    test_syn.server_name = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    test_syn.lobby_name = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    // test_syn->ingest_server_id = primitive, already allocated by protobuf

    // populate fields
    strcpy(test_syn.player_name, "Tester Client");
    strcpy(test_syn.server_name, location);
    strcpy(test_syn.lobby_name, lobby);
    test_syn.ingest_server_id = 0;

    // temp message sent to pass NAT
    ADS__Message temp = ADS__MESSAGE__INIT;
    temp.type = 0;
    buffer_len = ads__message__pack(&temp, (uint8_t*) buffer);
    if (buffer_len < 0) {
        perror("Problem packing data into buffer");
        exit(1);
    }

    sendto(s, buffer, buffer_len, 0, (struct sockaddr *) &host, sizeof(host));
    host.sin_addr.s_addr = htonl(ingest_server);
    
    // make message to send
    ADS__Message ads_message = ADS__MESSAGE__INIT;
    ads_message.sender_id = 0;
    ads_message.type = SYN;
    ads_message.data.data = (uint8_t*) calloc(MAX_DATA_SIZE, sizeof(char));

    // pack message
    ads_message.data.len = ads__syn_request__pack(&test_syn, ads_message.data.data);
    buffer_len = ads__message__pack(&ads_message, (uint8_t*) buffer);
    if (buffer_len < 0) {
        perror("Problem packing data into buffer");
        exit(1);
    }

    sendto(s, buffer, buffer_len, 0, (struct sockaddr *) &host, sizeof(host));
    printf("Tester SYN sent to %s\n", location);

    // receive messages
    printf("Awaiting messages...\n");
    
    for (;;) {
        // Initialize file descriptor set
        FD_ZERO(&mask);
        FD_SET(s, &mask);

        ret = select(FD_SETSIZE, &mask, NULL, NULL, NULL);
        if (ret > 0) {
            if (FD_ISSET(s, &mask)) {
                buffer_len = recvfrom(s, buffer, MAX_MESSAGE_SIZE, 0, 
                    (struct sockaddr *) &from_addr, &dummy_len);
                if (buffer_len <= 0) 
                    continue;
                uint32_t digest = hash(buffer, buffer_len);
                ADS__Message *rec_mess = ads__message__unpack(NULL, buffer_len, (uint8_t*) buffer);
                printf("RECEIVED message from sender %d, type: %d, digest: %d\n", 
                    rec_mess->sender_id, rec_mess->type, digest);
                if ((enum Type) rec_mess->type == WORLD) {
                    ADS__WorldResponse *resp = ads__world_response__unpack(NULL, rec_mess->data.len, rec_mess->data.data);
                    printf("Item names: %s, %s, %s\n", resp->items[0]->item_name, resp->items[1]->item_name, resp->items[2]->item_name);
                }
                ads__message__free_unpacked(rec_mess, NULL);
            }
        }
    }
    close(s);
    return 0;
}
