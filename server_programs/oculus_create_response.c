#include "oculus_create_response.h"
#include "oculus_utils.h"

int create_presence_response(struct MessageWrapper* response_wrapper, struct Session* session) {
    memset(response_wrapper, 0, sizeof(struct MessageWrapper));
    response_wrapper->message.type = PRESENCE;
    struct PresenceResponse* response = (struct PresenceResponse*) response_wrapper->message.data;

    strcpy(response->lobby_name, session->lobby);
    response->num_players = session->num_players;
    for (int i = 0; i < session->num_players; i++) {
        response->player_infos[i].player_id = session->players[i].id;
        strcpy(response->player_infos[i].player_name, session->players[i].name);
        server_id_to_name(response->player_infos[i].datacenter, session->players[i].ingest_server_id);
    }

    return 0;
}

// Small helper function to populate a syn response message
int create_syn_response(struct MessageWrapper* response_wrapper, struct Context* context, struct Player* player, struct Session* session) {
    memset(response_wrapper, 0, sizeof(struct MessageWrapper));
    response_wrapper->message.type = SYN;
    struct SynResponse* response = (struct SynResponse*) response_wrapper->message.data;

    response->client_id = player->id;
    response->ingest_server_id = player->ingest_server_id;
    response->ingest_server_port = context->server_port;
    response->max_players_per_lobby = MAX_NUM_PLAYERS;
    response->server_tick_delay_ms = (int) (context->tick_delay_s * 1000);
    strcpy(response->lobby_name, session->lobby);
    strcpy(response->player_name, player->name);
    strcpy(response->ingest_server_ip, context->server_ip);

    return 0;
}

int create_statistics_response(struct MessageWrapper* response_wrapper, struct Context* context, struct MessageWrapper* request_wrapper) {
    memset(response_wrapper, 0, sizeof(struct MessageWrapper));
    response_wrapper->message.type = STATISTICS;
    struct StatisticsResponse* response = (struct StatisticsResponse*) response_wrapper->message.data;

    server_id_to_name(response->datacenter, context->server_id);
    response->hash = request_wrapper->digest;
    response->recv_us = request_wrapper->recv_time - request_wrapper->unix_time;
    response->handle_us = timestamp() - request_wrapper->unix_time;

    return 0;
}

int create_heartbeat_request(struct MessageWrapper* request_wrapper, struct Context* context) {
    memset(request_wrapper, 0, sizeof(struct MessageWrapper));
    request_wrapper->message.type = SERVER_HEARTBEAT;

    uint64_t stamp = timestamp();
    memcpy(request_wrapper->message.data, &stamp, sizeof(uint64_t));

    return 0;
}

int create_world_response(struct MessageWrapper* response_wrapper, struct Session* session) {
    memset(response_wrapper, 0, sizeof(struct MessageWrapper));
    response_wrapper->message.type = WORLD;
    struct WorldResponse* response = (struct WorldResponse*) response_wrapper->message.data;

    // Add the items to the response
    response->num_items = session->num_world_items;
    for (int i = 0; i < session->num_world_items; i++) {
        strcpy(response->items[i].item_name, session->world_items[i].item_name);
        response->items[i].owner_id = session->world_items[i].owner_id;
        response->items[i].position.x = session->world_items[i].position.x;
        response->items[i].position.y = session->world_items[i].position.y;
        response->items[i].position.z = session->world_items[i].position.z;
    }
    
    // Add the players to the response
    struct WorldAvatar* res_avatar;  // response avatar
    struct Player* ses_player;  // session player
    response->num_avatars = session->num_players;
    for (int i = 0; i < session->num_players; i++) {
        res_avatar = &response->avatars[i];
        ses_player = &session->players[i];

        res_avatar->player_id = ses_player->id;
        
        res_avatar->data.headset.pos.x = ses_player->avatar.head.pos.x;
        res_avatar->data.headset.pos.y = ses_player->avatar.head.pos.y;
        res_avatar->data.headset.pos.z = ses_player->avatar.head.pos.z;
        res_avatar->data.headset.quat.x = ses_player->avatar.head.quat.x;
        res_avatar->data.headset.quat.y = ses_player->avatar.head.quat.y;
        res_avatar->data.headset.quat.z = ses_player->avatar.head.quat.z;
        res_avatar->data.headset.quat.w = ses_player->avatar.head.quat.w;

        res_avatar->data.left_controller.pos.x = ses_player->avatar.left.transform.pos.x;
        res_avatar->data.left_controller.pos.y = ses_player->avatar.left.transform.pos.y;
        res_avatar->data.left_controller.pos.z = ses_player->avatar.left.transform.pos.z;
        res_avatar->data.left_controller.quat.x = ses_player->avatar.left.transform.quat.x;
        res_avatar->data.left_controller.quat.y = ses_player->avatar.left.transform.quat.y;
        res_avatar->data.left_controller.quat.z = ses_player->avatar.left.transform.quat.z;
        res_avatar->data.left_controller.quat.w = ses_player->avatar.left.transform.quat.w;

        res_avatar->data.right_controller.pos.x = ses_player->avatar.right.transform.pos.x;
        res_avatar->data.right_controller.pos.y = ses_player->avatar.right.transform.pos.y;
        res_avatar->data.right_controller.pos.z = ses_player->avatar.right.transform.pos.z;
        res_avatar->data.right_controller.quat.x = ses_player->avatar.right.transform.quat.x;
        res_avatar->data.right_controller.quat.y = ses_player->avatar.right.transform.quat.y;
        res_avatar->data.right_controller.quat.z = ses_player->avatar.right.transform.quat.z;
        res_avatar->data.right_controller.quat.w = ses_player->avatar.right.transform.quat.w;

        res_avatar->data.left_joystick.x = ses_player->avatar.left.joystick.x;
        res_avatar->data.left_joystick.y = ses_player->avatar.left.joystick.y;
        res_avatar->data.right_joystick.x = ses_player->avatar.right.joystick.x;
        res_avatar->data.right_joystick.y = ses_player->avatar.right.joystick.y;

        res_avatar->data.player_offset.x = ses_player->avatar.offset.x;
        res_avatar->data.player_offset.y = ses_player->avatar.offset.y;
        res_avatar->data.player_offset.z = ses_player->avatar.offset.z;
    }

    return 0;
}