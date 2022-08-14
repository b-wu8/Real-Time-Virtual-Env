#include "oculus_protobuf.h"
#include "oculus_utils.h"

int parse_message(struct MessageWrapper* wrapper, char* buffer, int len) {
    ADS__Message* ads_message =
        ads__message__unpack(NULL, len, (uint8_t*) buffer);
    if (ads_message == NULL) {
        return 1;
    } else {
        wrapper->message.type = ads_message->type;
        wrapper->message.sender_id = ads_message->sender_id;
    }
    
    switch (wrapper->message.type) {
        case SYN: {
            ADS__SynRequest* ads_req =
                ads__syn_request__unpack(NULL, ads_message->data.len, (uint8_t*) ads_message->data.data);
            if (ads_req == NULL) {
                ads__message__free_unpacked(ads_message, NULL);
                return 1;
            }
            struct SynRequest req;
            strcpy(req.lobby_name, ads_req->lobby_name);
            strcpy(req.player_name, ads_req->player_name);
            strcpy(req.server_name, ads_req->server_name);
            req.ingest_server_id = ads_req->ingest_server_id;
            
            memcpy(wrapper->message.data, &req, sizeof(struct SynRequest));
            ads__syn_request__free_unpacked(ads_req, NULL);
            break;
        }
        case CONTINUOUS: {
            ADS__ContinuousRequest* ads_req =
                ads__continuous_request__unpack(NULL, ads_message->data.len, (uint8_t*) ads_message->data.data);
            if (ads_req == NULL) {
                ads__message__free_unpacked(ads_message, NULL);
                return 1;
            }
            struct ContinuousRequest req;

            req.data.headset.pos.x = ads_req->data->headset->position->x;
            req.data.headset.pos.y = ads_req->data->headset->position->y;
            req.data.headset.pos.z = ads_req->data->headset->position->z;
            req.data.headset.quat.x = ads_req->data->headset->rotation->x;
            req.data.headset.quat.y = ads_req->data->headset->rotation->y;
            req.data.headset.quat.z = ads_req->data->headset->rotation->z;
            req.data.headset.quat.w = ads_req->data->headset->rotation->w;
        
            req.data.left_controller.pos.x = ads_req->data->left_controller->position->x;
            req.data.left_controller.pos.y = ads_req->data->left_controller->position->y;
            req.data.left_controller.pos.z = ads_req->data->left_controller->position->z;
            req.data.left_controller.quat.x = ads_req->data->left_controller->rotation->x;
            req.data.left_controller.quat.y = ads_req->data->left_controller->rotation->y;
            req.data.left_controller.quat.z = ads_req->data->left_controller->rotation->z;
            req.data.left_controller.quat.w = ads_req->data->left_controller->rotation->w;
            
            req.data.right_controller.pos.x = ads_req->data->right_controller->position->x;
            req.data.right_controller.pos.y = ads_req->data->right_controller->position->y;
            req.data.right_controller.pos.z = ads_req->data->right_controller->position->z;
            req.data.right_controller.quat.x = ads_req->data->right_controller->rotation->x;
            req.data.right_controller.quat.y = ads_req->data->right_controller->rotation->y;
            req.data.right_controller.quat.z = ads_req->data->right_controller->rotation->z;
            req.data.right_controller.quat.w = ads_req->data->right_controller->rotation->w;

            req.data.left_joystick.x = ads_req->data->left_joystick->x;
            req.data.left_joystick.y = ads_req->data->left_joystick->y;
            
            req.data.right_joystick.x = ads_req->data->right_joystick->x;
            req.data.right_joystick.y = ads_req->data->right_joystick->y;
            
            req.data.player_offset.x = ads_req->data->player_offset->x;
            req.data.player_offset.y = ads_req->data->player_offset->y;
            req.data.player_offset.z = ads_req->data->player_offset->z;

            memcpy(wrapper->message.data, &req, sizeof(struct ContinuousRequest));
            ads__continuous_request__free_unpacked(ads_req, NULL);
            break;
        }
        case RPC: {
            ADS__RpcRequest* ads_req =
                ads__rpc_request__unpack(NULL, ads_message->data.len, (uint8_t*) ads_message->data.data);
            if (ads_req == NULL) {
                ads__message__free_unpacked(ads_message, NULL);
                return 1;
            }
            struct RpcRequest req;
            req.initiator_id = ads_req->initiator_id;
            strcpy(req.rpc, ads_req->rpc);

            memcpy(wrapper->message.data, &req, sizeof(struct RpcRequest));
            ads__rpc_request__free_unpacked(ads_req, NULL);
            break;
        }
        default:
            break;
    }
    
    ads__message__free_unpacked(ads_message, NULL);
    return 0;
}

int format_response(char* buffer, struct ProtoMemory* proto_mem, struct Message* message) {
    // printf("format_response() type=%d\n", message->type);

    // Initialize message to be sent
    ADS__Message* ads_message = &proto_mem->mess;
    ads_message->type = message->type;
    ads_message->sender_id = message->sender_id;

    int buffer_len = -1;
    switch (message->type) {
        case SYN: {
            ADS__SynResponse* ads_response = &proto_mem->syn_res;
            struct SynResponse* response = (struct SynResponse*) message->data;

            strcpy(ads_response->lobby_name, response->lobby_name);
            strcpy(ads_response->player_name, response->player_name);
            strcpy(ads_response->ingest_server_ip, response->ingest_server_ip);
            ads_response->client_id = response->client_id;
            ads_response->server_tick_delay_ms = response->server_tick_delay_ms;
            ads_response->ingest_server_id = response->ingest_server_id;
            ads_response->ingest_server_port = response->ingest_server_port;
            ads_response->max_players_per_lobby = response->max_players_per_lobby;
            
           
            ads_message->data.len = ads__syn_response__pack(ads_response, ads_message->data.data);
            buffer_len = ads__message__pack(ads_message, (uint8_t*) buffer);
            break;
        }
        case HEARTBEAT: {            
            ADS__Message* ads_message = &proto_mem->mess;
            ads_message->type = message->type;
            ads_message->sender_id = message->sender_id;
            buffer_len = ads__message__pack(ads_message, (uint8_t*) buffer);
            break;
        }
        case WORLD: {
            ADS__WorldResponse* ads_response = &proto_mem->world_res;
            struct WorldResponse* response = (struct WorldResponse*) message->data;

            ads_response->n_items = response->num_items;
            for (int i = 0; i < response->num_items; i++) {
                strcpy(ads_response->items[i]->item_name, response->items[i].item_name);
                ads_response->items[i]->owner_id = response->items[i].owner_id;
                ads_response->items[i]->position->x = response->items[i].position.x;
                ads_response->items[i]->position->y = response->items[i].position.y;
                ads_response->items[i]->position->z = response->items[i].position.z;
            }

            ADS__WorldResponse__Avatar* ads_avatar;
            struct WorldAvatar* avatar;
            ads_response->n_avatars = response->num_avatars;
            for (int i = 0; i < response->num_avatars; i++) {
                ads_avatar = ads_response->avatars[i];
                avatar = &response->avatars[i];

                ads_avatar->player_id = avatar->player_id;

                ads_avatar->data->headset->position->x = avatar->data.headset.pos.x;
                ads_avatar->data->headset->position->y = avatar->data.headset.pos.y;
                ads_avatar->data->headset->position->z = avatar->data.headset.pos.z;
                ads_avatar->data->headset->rotation->x = avatar->data.headset.quat.x;
                ads_avatar->data->headset->rotation->y = avatar->data.headset.quat.y;
                ads_avatar->data->headset->rotation->z = avatar->data.headset.quat.z;
                ads_avatar->data->headset->rotation->w = avatar->data.headset.quat.w;

                ads_avatar->data->left_controller->position->x = avatar->data.left_controller.pos.x;
                ads_avatar->data->left_controller->position->y = avatar->data.left_controller.pos.y;
                ads_avatar->data->left_controller->position->z = avatar->data.left_controller.pos.z;
                ads_avatar->data->left_controller->rotation->x = avatar->data.left_controller.quat.x;
                ads_avatar->data->left_controller->rotation->y = avatar->data.left_controller.quat.y;
                ads_avatar->data->left_controller->rotation->z = avatar->data.left_controller.quat.z;
                ads_avatar->data->left_controller->rotation->w = avatar->data.left_controller.quat.w;
                
                ads_avatar->data->right_controller->position->x = avatar->data.right_controller.pos.x;
                ads_avatar->data->right_controller->position->y = avatar->data.right_controller.pos.y;
                ads_avatar->data->right_controller->position->z = avatar->data.right_controller.pos.z;
                ads_avatar->data->right_controller->rotation->x = avatar->data.right_controller.quat.x;
                ads_avatar->data->right_controller->rotation->y = avatar->data.right_controller.quat.y;
                ads_avatar->data->right_controller->rotation->z = avatar->data.right_controller.quat.z;
                ads_avatar->data->right_controller->rotation->w = avatar->data.right_controller.quat.w;

                ads_avatar->data->left_joystick->x = avatar->data.left_joystick.x;
                ads_avatar->data->left_joystick->y = avatar->data.left_joystick.y;
                
                ads_avatar->data->right_joystick->x = avatar->data.right_joystick.x;
                ads_avatar->data->right_joystick->y = avatar->data.right_joystick.y;
                
                ads_avatar->data->player_offset->x = avatar->data.player_offset.x;
                ads_avatar->data->player_offset->y = avatar->data.player_offset.y;
                ads_avatar->data->player_offset->z = avatar->data.player_offset.z;
            }

            ads_message->data.len = ads__world_response__pack(ads_response, ads_message->data.data);
            buffer_len = ads__message__pack(ads_message, (uint8_t*) buffer);
            break;
        }
        case PRESENCE: {
            ADS__PresenceResponse* ads_response = &proto_mem->presence_res;
            struct PresenceResponse* response = (struct PresenceResponse*) message->data;
            ads_response->n_player_infos = response->num_players;
            strcpy(ads_response->lobby_name, response->lobby_name);

            ADS__PresenceResponse__Player* ads_player_info;
            for (int i = 0; i < response->num_players; i++) {
                ads_player_info = ads_response->player_infos[i];
                strcpy(ads_player_info->player_name, response->player_infos[i].player_name);
                strcpy(ads_player_info->datacenter, response->player_infos[i].datacenter);
                ads_player_info->player_id = response->player_infos[i].player_id;
            }

            ads_message->data.len = ads__presence_response__pack(ads_response, ads_message->data.data);
            buffer_len = ads__message__pack(ads_message, (uint8_t*) buffer);
            break;
        }
        case RPC: {
            // RpcResponse is an RpcRequest
            ADS__RpcRequest* ads_req = &proto_mem->rpc_req;
            struct RpcRequest* response = (struct RpcRequest*) message->data;
            strcpy(ads_req->rpc, response->rpc);
            ads_req->initiator_id = response->initiator_id;

            ads_message->data.len = ads__rpc_request__pack(ads_req, ads_message->data.data);
            buffer_len = ads__message__pack(ads_message, (uint8_t*) buffer);
            break;
        }
        case STATISTICS: {
            ADS__StatisticsResponse* ads_res = &proto_mem->stats_res;
            struct StatisticsResponse* response = (struct StatisticsResponse*) message->data;
            strcpy(ads_res->datacenter, response->datacenter);
            ads_res->hash = response->hash;
            ads_res->recv_us = response->recv_us;
            ads_res->handle_us = response->handle_us;

            ads_message->data.len = ads__statistics_response__pack(ads_res, ads_message->data.data);
            buffer_len = ads__message__pack(ads_message, (uint8_t*) buffer);
            break;
        }
        default: 
            break;
    }
        
    return buffer_len;
}

int allocate_protobuf_memory(struct ProtoMemory* proto_mem) {
    ADS__Message mess = ADS__MESSAGE__INIT;
    mess.data.data = (uint8_t*) calloc(MAX_DATA_SIZE, sizeof(char));
    memcpy(&proto_mem->mess, &mess, sizeof(ADS__Message));

    ADS__SynResponse syn_res = ADS__SYN_RESPONSE__INIT;
    syn_res.lobby_name = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    syn_res.player_name = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    syn_res.ingest_server_ip = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    memcpy(&proto_mem->syn_res, &syn_res, sizeof(ADS__SynResponse));

    ADS__WorldResponse world_res = ADS__WORLD_RESPONSE__INIT;
    world_res.items = (ADS__WorldResponse__OwnedVec3**) calloc(MAX_NUM_ITEMS, sizeof(ADS__WorldResponse__OwnedVec3*));
    for (int j = 0; j < MAX_NUM_ITEMS; j++) {
        world_res.items[j] = (ADS__WorldResponse__OwnedVec3*) calloc(sizeof(ADS__WorldResponse__OwnedVec3), sizeof(char));
        ads__world_response__owned_vec3__init(world_res.items[j]);
        world_res.items[j]->item_name = (char*) calloc(MAX_STRING_LEN, sizeof(char));
        world_res.items[j]->position = (ADS__Vector3*) calloc(sizeof(ADS__Vector3), sizeof(char));
        ads__vector3__init(world_res.items[j]->position);
    }    
    world_res.avatars = (ADS__WorldResponse__Avatar**) calloc(MAX_NUM_PLAYERS, sizeof(ADS__WorldResponse__Avatar*));
    for (int j = 0; j < MAX_NUM_PLAYERS; j++) {
        world_res.avatars[j] = (ADS__WorldResponse__Avatar*) calloc(sizeof(ADS__WorldResponse__Avatar), sizeof(char));
        ads__world_response__avatar__init(world_res.avatars[j]);
        world_res.avatars[j]->data = (ADS__ContinuousData*) calloc(sizeof(ADS__ContinuousData), sizeof(char));
        ads__continuous_data__init(world_res.avatars[j]->data);
        world_res.avatars[j]->data->headset = (ADS__Transform*) calloc(sizeof(ADS__Transform), sizeof(char));
        ads__transform__init(world_res.avatars[j]->data->headset);
        world_res.avatars[j]->data->headset->position = (ADS__Vector3*) calloc(sizeof(ADS__Vector3), sizeof(char));
        ads__vector3__init(world_res.avatars[j]->data->headset->position);
        world_res.avatars[j]->data->headset->rotation = (ADS__Quaternion*) calloc(sizeof(ADS__Quaternion), sizeof(char));
        ads__quaternion__init(world_res.avatars[j]->data->headset->rotation);
        world_res.avatars[j]->data->left_controller = (ADS__Transform*) calloc(sizeof(ADS__Transform), sizeof(char));
        ads__transform__init(world_res.avatars[j]->data->left_controller);
        world_res.avatars[j]->data->left_controller->position = (ADS__Vector3*) calloc(sizeof(ADS__Vector3), sizeof(char));
        ads__vector3__init(world_res.avatars[j]->data->left_controller->position);
        world_res.avatars[j]->data->left_controller->rotation = (ADS__Quaternion*) calloc(sizeof(ADS__Quaternion), sizeof(char));
        ads__quaternion__init(world_res.avatars[j]->data->left_controller->rotation);
        world_res.avatars[j]->data->right_controller = (ADS__Transform*) calloc(sizeof(ADS__Transform), sizeof(char));
        ads__transform__init(world_res.avatars[j]->data->right_controller);
        world_res.avatars[j]->data->right_controller->position = (ADS__Vector3*) calloc(sizeof(ADS__Vector3), sizeof(char));
        ads__vector3__init(world_res.avatars[j]->data->right_controller->position);
        world_res.avatars[j]->data->right_controller->rotation = (ADS__Quaternion*) calloc(sizeof(ADS__Quaternion), sizeof(char));
        ads__quaternion__init(world_res.avatars[j]->data->right_controller->rotation);
        world_res.avatars[j]->data->player_offset = (ADS__Vector3*) calloc(sizeof(ADS__Vector3), sizeof(char));
        ads__vector3__init(world_res.avatars[j]->data->player_offset);
        world_res.avatars[j]->data->left_joystick = (ADS__Vector2*) calloc(sizeof(ADS__Vector2), sizeof(char));
        ads__vector2__init(world_res.avatars[j]->data->left_joystick);
        world_res.avatars[j]->data->right_joystick = (ADS__Vector2*) calloc(sizeof(ADS__Vector2), sizeof(char));
        ads__vector2__init(world_res.avatars[j]->data->right_joystick);
    }
    memcpy(&proto_mem->world_res, &world_res, sizeof(ADS__WorldResponse));

    ADS__PresenceResponse presence_res = ADS__PRESENCE_RESPONSE__INIT;
    presence_res.lobby_name = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    presence_res.player_infos = (ADS__PresenceResponse__Player**) calloc(MAX_NUM_PLAYERS, sizeof(ADS__PresenceResponse__Player*));
    for (int j = 0; j < MAX_NUM_PLAYERS; j++) {
        presence_res.player_infos[j] = (ADS__PresenceResponse__Player*) calloc(sizeof(ADS__PresenceResponse__Player), sizeof(char));
        ads__presence_response__player__init(presence_res.player_infos[j]);
        presence_res.player_infos[j]->player_name = (char*) calloc(MAX_STRING_LEN, sizeof(char));
        presence_res.player_infos[j]->datacenter = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    }
    memcpy(&proto_mem->presence_res, &presence_res, sizeof(ADS__PresenceResponse));

    ADS__RpcRequest ads_rpc_request = ADS__RPC_REQUEST__INIT;
    ads_rpc_request.rpc = (char*) calloc(MAX_STRING_LEN, sizeof (char)); 
    memcpy(&proto_mem->rpc_req, &ads_rpc_request, sizeof(ADS__RpcRequest));

    ADS__StatisticsResponse ads_stats_res = ADS__STATISTICS_RESPONSE__INIT;
    ads_stats_res.datacenter = (char*) calloc(MAX_STRING_LEN, sizeof(char));
    memcpy(&proto_mem->stats_res, &ads_stats_res, sizeof(ADS__StatisticsResponse));

    return 0;
}
