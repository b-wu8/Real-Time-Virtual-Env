#include "oculus_handlers.h"
#include "oculus_context.h"
#include "oculus_protobuf.h"
#include "oculus_utils.h"
#include "oculus_server_api.h"
#include "oculus_create_response.h"

int handle_message(struct Context* context, struct MessageWrapper* wrapper) {
    char addr_buf[MAX_STRING_LEN];
    format_addr(addr_buf, &wrapper->from_addr);
    printf("handling message [ type=%d digest=%u from=%s ]\n", 
        wrapper->message.type, wrapper->digest, addr_buf);
    printf("key=STATISTICS type=handle server=%d time=%" PRIu64 " ingest=%d hash=%u\n", 
        context->server_id, timestamp(), wrapper->server_id, wrapper->digest);
    switch (wrapper->message.type) {
        case SYN:
            handle_player_syn(context, wrapper);
            break;
        case FIN:
            handle_player_leave(context, wrapper);
            break;
        case CONTINUOUS:
            handle_player_continuous(context, wrapper);
            break;
        case HEARTBEAT:
            handle_player_heartbeat(context, wrapper);
            break;
        case RPC:
            handle_player_rpc(context, wrapper);
            break;
        case SERVER_HEARTBEAT:
            handle_server_heartbeat(context, wrapper);
        default:
            break;
    }
    return 0;
}

int handle_player_heartbeat(struct Context* context, struct MessageWrapper* wrapper) {
    struct Message* message = &wrapper->message;
    struct Session* session;
    int ret = get_player_session(&session, context, message->sender_id);
    if (ret != 0)
        return ret;

    // Find player in lobby, restart their timeout timer, and send the heartbeat
    // message back to them, so they know the server is alive.
    for (int i = 0; i < session->num_players; i++)
        if (session->players[i].id == message->sender_id) {
            gettimeofday(&session->players[i].timestamp, NULL);
            message->sender_id = context->server_id;
            oculus_sendto(context, wrapper, &session->players[i].addr, 0); // UDP send to client
            return 0;
        } 
    return 1;
}

int handle_player_continuous(struct Context* context, struct MessageWrapper* wrapper) {
    struct Message* message = &wrapper->message;
    if (message->type != CONTINUOUS)
        return 4;
    struct ContinuousRequest* request = (struct ContinuousRequest*) message->data;

    struct Session* session;
    int ret = get_player_session(&session, context, message->sender_id);
    if (ret != 0)
        return ret;

    int player_idx = -1;
    for (int i = 0; i < session->num_players; i++) 
        if (session->players[i].id == message->sender_id) {
            player_idx = i;
            break;
        }
    if (player_idx < 0)
        return 5;
    
    struct Player* player = &(session->players[player_idx]);
    player->avatar.head.pos.x = request->data.headset.pos.x;
    player->avatar.head.pos.y = request->data.headset.pos.y;
    player->avatar.head.pos.z = request->data.headset.pos.z;
    player->avatar.head.quat.x = request->data.headset.quat.x;
    player->avatar.head.quat.y = request->data.headset.quat.y;
    player->avatar.head.quat.z = request->data.headset.quat.z;
    player->avatar.head.quat.w = request->data.headset.quat.w;

    player->avatar.left.transform.pos.x = request->data.left_controller.pos.x;
    player->avatar.left.transform.pos.y = request->data.left_controller.pos.y;
    player->avatar.left.transform.pos.z = request->data.left_controller.pos.z;
    player->avatar.left.transform.quat.x = request->data.left_controller.quat.x;
    player->avatar.left.transform.quat.y = request->data.left_controller.quat.y;
    player->avatar.left.transform.quat.z = request->data.left_controller.quat.z;
    player->avatar.left.transform.quat.w = request->data.left_controller.quat.w;

    player->avatar.right.transform.pos.x = request->data.right_controller.pos.x;
    player->avatar.right.transform.pos.y = request->data.right_controller.pos.y;
    player->avatar.right.transform.pos.z = request->data.right_controller.pos.z;
    player->avatar.right.transform.quat.x = request->data.right_controller.quat.x;
    player->avatar.right.transform.quat.y = request->data.right_controller.quat.y;
    player->avatar.right.transform.quat.z = request->data.right_controller.quat.z;
    player->avatar.right.transform.quat.w = request->data.right_controller.quat.w;

    player->avatar.left.joystick.x = request->data.left_joystick.x;
    player->avatar.left.joystick.y = request->data.left_joystick.y;
    player->avatar.right.joystick.x = request->data.right_joystick.x;
    player->avatar.right.joystick.y = request->data.right_joystick.y;
    
    return 0;
}

int handle_player_syn(struct Context* context, struct MessageWrapper* wrapper) {
    struct Message* message = &wrapper->message;
    if (message->type != SYN)
        return 4;
    struct SynRequest* request = (struct SynRequest*) message->data;

    printf("handle_player_syn() [ type=%d sender=%d ingest=%d ]\n", 
        wrapper->message.type, wrapper->message.sender_id, request->ingest_server_id);

    // try to infer message->sender_id if it is 0
    if (message->sender_id == UNKNOWN_ID)  // UNKNOWN_ID = 0
        for (int i = 0; i < context->num_sessions; i++) 
            for (int j = 0; j < context->sessions[i].num_players; j++) 
                if (strcmp(context->sessions[i].players[j].name, request->player_name) == 0 &&
                    strcmp(context->sessions[i].lobby, request->lobby_name) == 0)
                    message->sender_id = context->sessions[i].players[j].id;

    struct MessageWrapper response_wrapper;
    struct Session* session;
    int ret = get_player_session(&session, context, message->sender_id);
    if (ret == 0) {
        // Make sure lobby has name listed in request
        if (strcmp(session->lobby, request->lobby_name) != 0) {
            ret = 2;
        } else {
            // Lobby exists and player is already in that lobby
            // We might still have to update remote address
            ret = 3; // If we don't find player, ret will remain 3
            for (int i = 0; i < session->num_players; i++) 
                if (strcmp(session->players[i].name, request->player_name) == 0) {
                    struct Player* player = &session->players[i];
                    // Update player addr
                    memcpy(&player->addr, &wrapper->from_addr, sizeof(struct sockaddr_in));
                    // create syn response
                    create_syn_response(&response_wrapper, context, player, session);
                    ret = 0;
                    break;
                }
        }        
    } else if (ret == 1) {
        // Check that we have space for another lobby
        if (context->num_sessions == MAX_NUM_SESSIONS) {
            printf("Player \"%s\" cannot create lobby \"%s\". Max number of sessions are already open (%d/%d)\n", 
                request->player_name, request->lobby_name, MAX_NUM_SESSIONS, MAX_NUM_SESSIONS);
            ret = 4;
        } else {
            session = NULL;
            // Check if session already exists
            for (int i = 0; i < context->num_sessions; i++) 
                if (strcmp(context->sessions[i].lobby, request->lobby_name) == 0)
                    session = &context->sessions[i];
            if (!session)
                // Create new session
                session = add_lobby_to_context(context, request);

            // Player is creating new lobby being added to that lobby
            struct Player* player = add_player_to_lobby(session, request);
            memcpy(&player->addr, &wrapper->from_addr, sizeof(struct sockaddr_in));
            create_syn_response(&response_wrapper, context, player, session);
            printf("Heartbeat [ id=%d beat=%ld ]\n", player->id, player->timestamp.tv_sec);
            ret = 0;
        }
    }

    if (ret == 0 && request->ingest_server_id == context->server_id) {
        printf("Responding to client syn request. I am ingest server\n");
        oculus_sendto(context, &response_wrapper, &wrapper->from_addr, 0);  // UDP to client
    } else {
        printf("Not responding to client syn request [ ret=%d ingest=%d server=%d ] \n", 
            ret, request->ingest_server_id, context->server_id);
    }

    return ret;
}

int handle_player_leave(struct Context* context, struct MessageWrapper* wrapper) {
    struct Message* message = &wrapper->message;
    if (message->type != FIN)
        return 4;
    struct Session* session;
    int ret = get_player_session(&session, context, message->sender_id);
    if (ret != 0) {
        printf("ret=%d so we couldn't find player in lobby\n", ret);
        return ret;
    }

    // Unclaim all items currently claimed by player
    for (int i = 0; i < session->num_world_items; i++)
        if (session->world_items[i].owner_id == message->sender_id)
            session->world_items[i].owner_id = UNKNOWN_ID;  // No one owns this item anymore  
        
    // Remove player from lobby
    int player_id = message->sender_id;
    for (int i = 0; i < session->num_players; i++)
        // found the player to remove
        if (session->players[i].id == player_id) {
            // Must print player left message before we delete player
            printf("Player \"%s\" left lobby \"%s\" (%d/%d)\n", 
                session->players[i].name, session->lobby, session->num_players, MAX_NUM_PLAYERS);
            // Shift all subsequent player up one spot in the player array
            for (int j = i; j < session->num_players - 1; j++)
                memcpy(&session->players[j], &session->players[j+1], sizeof(struct Player));
        }
    
    // Clear the last player slot
    memset(&session->players[session->num_players - 1], 0, sizeof(struct Player));
    session->num_players--;
    session->has_changed = 1;

    // If this was the last player, close the session
    if (session->num_players == 0) {
        printf("Lobby \"%s\" has been closed\n", session->lobby);  // must print before we clear the session
        for (int i = 0; i < context->num_sessions; i++)
            if (strcmp(context->sessions[i].lobby, session->lobby) == 0) {
                for (int j = i; j < context->num_sessions - 1; j++)
                    memcpy(&context->sessions[j], &context->sessions[j+1], sizeof(struct Session));
            }
        memset(&context->sessions[context->num_sessions - 1], 0, sizeof(struct Session));
        context->num_sessions--;
    } else {
        /*
        struct MessageWrapper response_wrapper;
        struct Message* response_message = &response_wrapper.message;
        response_message->type = PRESENCE;
        response_message->sender_id = context->server_id;
        struct PresenceResponse* response = (struct PresenceResponse*) response_message->data;

        strcpy(response->lobby_name, session->lobby);
        response->num_players = session->num_players;
        for (int i = 0; i < session->num_players; i++) {
            response->player_infos[i].player_id = session->players[i].id;
            strcpy(response->player_infos[i].player_name, session->players[i].name);
            server_id_to_name(response->player_infos[i].datacenter, session->players[i].ingest_server_id);
        }
        */
        struct MessageWrapper response_wrapper;
        create_presence_response(&response_wrapper, session);
        for (int i = 0; i < session->num_players; i++) 
            oculus_sendto(context, &response_wrapper, &session->players[i].addr, 0);  // UDP to client
    }

    return 0;
}

int handle_player_rpc(struct Context* context, struct MessageWrapper* wrapper) {
    struct Message* message = &wrapper->message;

    if (message->type != RPC)
        return 4;

    struct RpcRequest* request = (struct RpcRequest*) message->data;
    printf("handle_player_rpc() [ type=%d sender=%d rpc=%s ]\n", 
        wrapper->message.type, wrapper->message.sender_id, request->rpc);
    struct Session* session;
    int ret = get_player_session(&session, context, message->sender_id);
    if (ret != 0)
        return ret;

    if (strcmp(request->rpc, RPC_HAPTICS) == 0) {
        for (int i = 0; i < session->num_world_items; i++) 
            if (strcmp(session->world_items[i].item_name, INTERACTABLE_HAPTICS_CYLINDER) == 0)
                session->world_items[i].owner_id = request->initiator_id;
        message->sender_id = context->server_id;
        printf("RPC received by player %d and sends it to lobby %s\n", request->initiator_id, session->lobby);
        for (int i = 0; i < session->num_players; i++)
            oculus_sendto(context, wrapper, &session->players[i].addr, 0); // UDP to client
        return 0;
    }

    struct Player* player = NULL;
    for (int i = 0; i < session->num_players; i++)
        if (session->players[i].id == request->initiator_id)
            player = &session->players[i];
    if (player == NULL) {
        printf("ERROR: Couldn't find player in lobby\n");
        return 1;
    }
    
    char rpc[128];
    sprintf(rpc, "claim__%s", INTERACTABLE_CLAIM_SPHERE);
    if (strcmp(request->rpc, rpc) == 0)
        for (int i = 0; i < session->num_world_items; i++) 
            if (strcmp(session->world_items[i].item_name, INTERACTABLE_CLAIM_SPHERE) == 0) {
                int old_owner = session->world_items[i].owner_id;
                if (session->world_items[i].owner_id == 0 || session->world_items[i].owner_id == request->initiator_id) {
                    struct WorldOwnedVec3* item = &session->world_items[i];
                    item->owner_id = request->initiator_id;  // someone claims unclaimed ball
                    // Set fixed offset between interactable sphere and player right hand
                    session->world_meta.int_sphere_offset.x = item->position.x - (player->avatar.right.transform.pos.x + player->avatar.offset.x);
                    session->world_meta.int_sphere_offset.y = item->position.y - (player->avatar.right.transform.pos.y + player->avatar.offset.y);
                    session->world_meta.int_sphere_offset.z = item->position.z - (player->avatar.right.transform.pos.z + player->avatar.offset.z);
                }
                printf("Handled iteractable rpc [ rpc=\"%s\" old_owner=%d new_owner=%d initator=%d ]\n", 
                    rpc, old_owner, session->world_items[i].owner_id, request->initiator_id);
            }
    
    sprintf(rpc, "unclaim__%s", INTERACTABLE_CLAIM_SPHERE);
    if (strcmp(request->rpc, rpc) == 0)
        for (int i = 0; i < session->num_world_items; i++) 
            if (strcmp(session->world_items[i].item_name, INTERACTABLE_CLAIM_SPHERE) == 0) {
                int old_owner = session->world_items[i].owner_id;
                if (session->world_items[i].owner_id == request->initiator_id) 
                    session->world_items[i].owner_id = UNKNOWN_ID;  // owner reliquishes ownership
                printf("Handled iteractable rpc [ rpc=\"%s\" old_owner=%d new_owner=%d initator=%d ]\n", 
                    rpc, old_owner, session->world_items[i].owner_id, request->initiator_id);
            }

    sprintf(rpc, "rand__");
    if (strncmp(request->rpc, rpc, strlen(rpc)) == 0) {
        if (player->ingest_server_id == context->server_id) {
            oculus_sendto(context, wrapper, &player->addr, 0);
        }

        // Send statistics message so that client can display them on screen
        struct MessageWrapper response_wrapper;
        create_statistics_response(&response_wrapper, context, wrapper);
        oculus_sendto(context, &response_wrapper, &player->addr, 0);
    }

    sprintf(rpc, "stats__");
    if (strncmp(request->rpc, rpc, strlen(rpc)) == 0) {
        uint64_t start_unix_us;
        uint64_t end_unix_us;
        uint32_t h;
        sscanf(request->rpc, "stats__%" SCNu64 "_%" SCNu64 "_%u", &start_unix_us, &end_unix_us, &h);
        printf("key=STATISTICS type=stats start=%" PRIu64 " end=%" PRIu64 " hash=%u player=%s\n", start_unix_us, end_unix_us, h, player->name);
    }

    sprintf(rpc, "ping__");
    if (strncmp(request->rpc, rpc, strlen(rpc)) == 0) {
        uint64_t ping_us;
        sscanf(request->rpc, "ping__%" SCNu64 "", &ping_us);
        printf("key=STATISTICS type=ping value=%" PRIu64 " player=%s\n", ping_us, player->name);
    }

    return 0;
}

int handle_server_heartbeat(struct Context* context, struct MessageWrapper* wrapper) {
    if (wrapper->message.sender_id == context->server_id)
        return 0;
    uint64_t new_stamp = timestamp();
    uint64_t old_stamp;
    char temp[MAX_STRING_LEN];
    format_addr(temp, &wrapper->from_addr);
    memcpy(&old_stamp, wrapper->message.data, sizeof(uint64_t));
    double delta = (((int)new_stamp) - ((int)old_stamp)) / 1000.0;
    printf("server heartbeat = [ sender=%s delta=%.1fms ]\n", temp, delta);
    return 0;
}
