#include "oculus_context.h"
#include "oculus_handlers.h"
#include "oculus_protobuf.h"
#include "oculus_utils.h"
#include "oculus_server_api.h"
#include "oculus_create_response.h"

struct Player* add_player_to_lobby(struct Session* session, struct SynRequest* request) {
    // Give the player the next index number such that is has no collisions with 
    // other players modulo MAX_NUM_PLAYERS. Thus, the client-side can use pid mod
    // MAX_NUM_PLAYERS to uniquiely select a color for the avatar.
    int player_idx_mod_n[MAX_NUM_PLAYERS];
    memset(player_idx_mod_n, 0, sizeof(player_idx_mod_n));
    for (int i = 0; i < session->num_players; i++)
        player_idx_mod_n[session->players[i].id % MAX_NUM_PLAYERS] = 1;
    int max_tries = MAX_NUM_PLAYERS;
    while (player_idx_mod_n[session->next_player_idx % MAX_NUM_PLAYERS] && max_tries-- > 0)
        session->next_player_idx++;

    struct Player* player = &session->players[session->num_players];
    player->id = session->next_player_idx++;
    player->ingest_server_id = request->ingest_server_id;
    strcpy(player->name, request->player_name);
    player->avatar.offset.y = 1.1176;  // FIXME: Make everyone the same height ?
    gettimeofday(&player->timestamp, NULL);  // Start timer for player timeout
    session->num_players++;
    session->has_changed = 1; // Ensures updates are sent out in next server tick
    printf("Player \"%s\" (id=%d ingest=%d) joined lobby \"%s\" (%d/%d)\n", 
        request->player_name, player->id, player->ingest_server_id, 
        request->lobby_name, session->num_players, MAX_NUM_PLAYERS);
    return player;
}

struct Session* add_lobby_to_context(struct Context* context, struct SynRequest* request) {
    struct Session* session = &(context->sessions[context->num_sessions]);
    strcpy(session->lobby, request->lobby_name);
    // move_sphere(&session->sphere, 0.001);
    int id_offset = context->num_sessions_created++ * MAX_NUM_PLAYERS * 1024;
    session->next_player_idx = MIN_PLAYER_ID + id_offset;
    session->tick_delay_s = context->tick_delay_s;
    session->has_changed = 1; // Ensures updates are sent out in next server tick
    session->timeout_sec = DEFAULT_TIMEOUT_SEC;
    strcpy(session->world_items[0].item_name, INTERACTABLE_REVOLVING_SPHERE);
    session->world_items[0].position.y = -1;  // start sphere below ground
    strcpy(session->world_items[1].item_name, INTERACTABLE_CLAIM_SPHERE);
    session->world_items[1].position.y = 5;  // start calimable sphere in air
    strcpy(session->world_items[2].item_name, INTERACTABLE_HAPTICS_CYLINDER);
    session->world_items[2].position.y = 1;  // start cylinder in center
    session->num_world_items = 3;
    printf("Lobby \"%s\" (idx=%d) has been created (0/%d)\n", 
        request->lobby_name, context->num_sessions, MAX_NUM_PLAYERS);
    context->num_sessions++;
    return session;
}

int remove_players_if_timeout(struct Context* context) {    
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    // removal_wrappers will store artificial requests
    // that we make to remove players. This way we can use
    // the handle_player_leave logic.
    struct MessageWrapper removal_wrappers[MAX_TOTAL_PLAYERS];
    memset(removal_wrappers, 0, sizeof(struct MessageWrapper));
    int num_timeouts = 0;
    struct Session* session;

    for (int i = 0; i < context->num_sessions; i++) {
        session = &context->sessions[i];
        for (int j = 0; j < session->num_players; j++) 
            if (session->players[j].ingest_server_id == context->server_id &&
                current_time.tv_sec - session->players[j].timestamp.tv_sec > session->timeout_sec
            ) {
                // This server is ingest server for player j and
                // Player j in session i has timed out
                printf("Player %d has timed out [ now=%ld last=%ld ]\n", 
                    session->players[j].id, current_time.tv_sec, session->players[j].timestamp.tv_sec);
                removal_wrappers[num_timeouts].message.type = FIN;
                removal_wrappers[num_timeouts].message.sender_id = session->players[j].id;
                num_timeouts++;
            }
    }
    
    if (num_timeouts > 0)
        printf("%d players timed out\n", num_timeouts);
    for (int i = 0; i < num_timeouts; i++) {
        removal_wrappers[i].unix_time = timestamp();
        oculus_sendto(context, &removal_wrappers[i], &context->spines_mcast_addr, 1); // spines send
        // oculus_sendto(context, &removal_wrappers[i], )
        // spines_sendto(context->spines_sk, &removal_wrappers[i], sizeof(struct MessageWrapper), 
        //    0, (struct sockaddr *) &context->spines_mcast_addr, sizeof(struct sockaddr));
    }
    
    return num_timeouts;
}

int apply_movement_step(struct Session* session) {
    // Move Revolving Sphere (1/15 rev per second)
    for (int i = 0; i < session->num_world_items; i++) {
        struct WorldOwnedVec3* item = &session->world_items[i];
        if (strcmp(item->item_name, INTERACTABLE_REVOLVING_SPHERE) == 0) {
            double t = 2 * M_PI / 15 * timestamp() / 1000000.0;
            item->position.x = 4 * cos(t);
            item->position.y = 2 * sin(t / 4 + 1) + 4;
            item->position.z = 4 * sin(t);
        }
    }

    // Joystick values read between -1 and 1. If the magnitude
    // is less than 0.2, then don't move in that direction. 
    // Otherwise, move proportional to the magnitude. Currently,
    // the velocity is implicit as one "unit" per second. Unity units are
    // technically in meters. 
    struct Avatar* avatar;
    for (int i = 0; i < session->num_players; i++) {
        avatar = &session->players[i].avatar;     
        // Use left joystick to maneuver 
        if (fabs(avatar->left.joystick.y) > 0.2)
            avatar->offset.z += SPEED_MOVEMENT * avatar->left.joystick.y * session->tick_delay_s;
        if (fabs(avatar->left.joystick.x) > 0.2) 
            avatar->offset.x += SPEED_MOVEMENT * avatar->left.joystick.x * session->tick_delay_s;

        // Move interactable sphere
        for (int j = 0; j < session->num_world_items; j++)
            if (strcmp(session->world_items[j].item_name, INTERACTABLE_CLAIM_SPHERE) == 0 &&
                session->world_items[j].owner_id == session->players[i].id
            ) {
                // Use right joystick to move interactable sphere
                struct WorldOwnedVec3* item = &session->world_items[j];
                item->position.x = session->world_meta.int_sphere_offset.x + (avatar->right.transform.pos.x + avatar->offset.x);
                item->position.y = session->world_meta.int_sphere_offset.y + (avatar->right.transform.pos.y + avatar->offset.y);
                item->position.z = session->world_meta.int_sphere_offset.z + (avatar->right.transform.pos.z + avatar->offset.z);
                if (fabs(avatar->right.joystick.y) > 0.2) {
                    struct Vector3 source = vector3_add(&avatar->right.transform.pos, &avatar->offset);
                    double dist = vector3_distance(&item->position, &source);
                    if (dist == 0) {
                        printf("Player has grabbed an item which he is directly on top of [ dist=0 player=\"%s\" ]\n", session->players[i].name);
                        continue;
                    }
                    double new_dist = dist + SPEED_PULL * avatar->right.joystick.y * session->tick_delay_s;
                    if (new_dist < 0.5) 
                        new_dist = 0.5;  // Don't pull any objects closer than 0.1 meters
                    else if (new_dist > 8)
                        new_dist = 8;
                    double t = new_dist / dist;
                    struct Vector3 new_item_pos = vector3_interpolate(&source, &item->position, t);
                    session->world_meta.int_sphere_offset.x = new_item_pos.x - (avatar->right.transform.pos.x + avatar->offset.x);
                    session->world_meta.int_sphere_offset.y = new_item_pos.y - (avatar->right.transform.pos.y + avatar->offset.y);
                    session->world_meta.int_sphere_offset.z = new_item_pos.z - (avatar->right.transform.pos.z + avatar->offset.z);
                    item->position.x = new_item_pos.x;
                    item->position.y = new_item_pos.y;
                    item->position.z = new_item_pos.z;
                }
            }
    }
    return 0;
}

int get_player_session(struct Session** session, struct Context* context, int player_id) {
    // Find lobby in context. Then find player in lobby. We use strcmp on the lobby 
    // name and player name. This is probably not good, but it works for now.
    for (int i = 0; i < context->num_sessions; i++) {
        for (int j = 0; j < context->sessions[i].num_players; j++) {
            if (context->sessions[i].players[j].id == player_id) {
                *session = &(context->sessions[i]);
                printf("Found player \"%s\" (id=%d) in lobby \"%s\" (%d/%d)\n", 
                    (*session)->players[j].name, player_id, (*session)->lobby, (*session)->num_players, MAX_NUM_PLAYERS);
                return 0;
            }
        }
    }
    printf("Couldn't find player with id %d\n", player_id);
    return 1;
}

int send_world_state(struct Context* context) {
    remove_players_if_timeout(context);
    
    struct Session* session;
    struct MessageWrapper wrapper;

    for (int i = 0; i < context->num_sessions; i++) {
        session = &context->sessions[i];
        apply_movement_step(session);  // move players based on joystick data (to be removed)
        if (session->has_changed) {
            create_world_response(&wrapper, session);
            for (int j = 0; j < session->num_players; j++)
                if (session->players[j].ingest_server_id == context->server_id)
                    oculus_sendto(context, &wrapper, &session->players[j].addr, 0); 
        }
    }
    return 0;
}

int setup_context(struct Context* context, int argc, char* argv[]) {
    memset(context, 0, sizeof(struct Context));

    if (sizeof(struct SynRequest) > MAX_DATA_SIZE ||
        sizeof(struct SynResponse) > MAX_DATA_SIZE ||
        sizeof(struct ContinuousRequest) > MAX_DATA_SIZE ||
        sizeof(struct RpcRequest) > MAX_DATA_SIZE ||
        sizeof(struct WorldResponse) > MAX_DATA_SIZE ||
        sizeof(struct PresenceResponse) > MAX_DATA_SIZE
    ) {
        printf("FATAL: MAX_DATA_SIZE=%dB is insufficient\n", MAX_DATA_SIZE);
        exit(1);
    }

    // Verify command line invocation
    int tick_delay_ms = 33;
    int sync_delay_ms = 65;
    if (argc < 3 || argc > 5){
        printf("Usage: ./oculus_server <server_name> <port> <tick_delay_ms (%dms)> <sync_delay_ms (%dms)>\n",
            tick_delay_ms, sync_delay_ms);
        printf("\tServer Options:\n");
        printf("\tSJC  ---  rain1.cnds.jhu.edu\n");
        printf("\tDEN  ---  rain3.cnds.jhu.edu\n");
        printf("\tLAX  ---  rain11.cnds.jhu.edu\n");
        printf("\tWAS  ---  rain12.cnds.jhu.edu\n");
        printf("\tCHI  ---  rain13.cnds.jhu.edu\n");
        printf("\tDFW  ---  rain14.cnds.jhu.edu\n");
        printf("\tATL  ---  rain15.cnds.jhu.edu\n");
        printf("\tNYC  ---  rain16.cnds.jhu.edu\n");
        exit(1);
    }
    if (argc >= 4)
        tick_delay_ms = atoi(argv[3]);
    if (argc >= 5) 
        sync_delay_ms = atoi(argv[4]);

    char server_name[MAX_STRING_LEN];
    sprintf(server_name, "server__%s", argv[1]);
    server_name_to_info(server_name, &context->server_id, context->server_ip);
    context->server_port = atoi(argv[2]);
    strcpy(context->server_name, server_name);
    printf("Setting up server \"%s\" [ id=%d ip=%s port=%d ]\n", 
        context->server_name, context->server_id, context->server_ip, context->server_port);

    if (setup_client_socket(&context->client_sk, context->server_ip, argv[2]))
        exit(1);  // Error is printed within setup_client_socket()
    
    if (setup_spines_socket(&context->spines_sk, &context->spines_mcast_addr))
        exit(1);  // Error is printed within setup_spines_socket()

    // Set context timeout timeval
    if (tick_delay_ms <= 0) {
        printf("time_step of %d ms is invalid\n", tick_delay_ms);
        exit(1);
    }
    context->timeout.tv_sec = tick_delay_ms / 1000;
    context->timeout.tv_usec = (tick_delay_ms * 1000) % 1000000;
    
    // Setup delay variables
    context->tick_delay_s = ((double) tick_delay_ms) / 1000;
    context->sync_delay_us = ((uint32_t) sync_delay_ms) * 1000;
    context->server_heartbeat_time_us = 10 * 1000 * 1000; // 10 sec

    allocate_protobuf_memory(&context->proto_mem);

    return 0;
}
