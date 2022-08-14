// oculus structs and constants

#ifndef OCULUS_STRUCTS_H
#define OCULUS_STRUCTS_H

#include <inttypes.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include "spines_lib.h"
#include "spu_events.h"
#include "spu_alarm.h"
#include "math.h"
#include "protoc_generated/game.pb-c.h"

#define SPINES_PORT 8100
#define SPINES_MCAST_ADDR ((225 << 24) | (0 << 16) | (1 << 8) | (1))  // (225.0.1.1)
#define SPINES_RECV_PORT 8400 
#define MAX_NUM_PLAYERS 8  // 12 players (one for each color)
#define MAX_NUM_SESSIONS 16
#define MAX_NUM_ITEMS 3
#define MAX_STRING_LEN 64 
#define MAX_TOTAL_PLAYERS (MAX_NUM_PLAYERS + MAX_NUM_SESSIONS)
#define DEFAULT_TIMEOUT_SEC 10  // 10 seconds until player timeout
#define MAX_DATA_SIZE 1200
#define MAX_MESSAGE_SIZE 1300
#define MAX_QUEUE_LEN 1024
#define UNKNOWN_ID 0
#define MIN_PLAYER_ID 1200

#define SPEED_MOVEMENT 1
#define SPEED_PULL 1

#define RPC_HAPTICS "HAPTICS"
#define INTERACTABLE_REVOLVING_SPHERE "revolving_sphere"
#define INTERACTABLE_CLAIM_SPHERE "claim_sphere"
#define INTERACTABLE_HAPTICS_CYLINDER "haptics_cylinder"

// FIXME: redo this comment
// Controller joystick position represented by (x.y) vector
// The magnitude of this vector implies how far the stick is moved
struct Vector2 {
    float x;
    float y;
};

// Represents position of unity object as (x,y,z) vector 
struct Vector3 {
    float x;
    float y;
    float z;
};

// Represents rotation of unity object as (x,y,z,w) quaternion
// MAKE SURE QUATERNION FORMAT IS (X,Y,Z,W), NOT (W,X,Y,Z)
struct Quaternion {
    float x;
    float y;
    float z;
    float w;
};

// Transform is defined as a pair of position and rotation
struct Transform {
    struct Vector3 pos;
    struct Quaternion quat;
};

// Controller is currently a transform and a joystick. 
// More features will be added to the joystick
struct Controller {
    struct Transform transform;
    struct Vector2 joystick;
};

// Avatar objects are rendered on each of the clients. One for each
// player. The head, left, and right are all used to construct the 
// avatar on the client side. The offset gives the avatars coarse-grained 
// position in the game. 
struct Avatar {
    struct Vector3 offset;
    struct Transform head;
    struct Controller left;
    struct Controller right;
};

// This is a logical player struct that represents everything we 
// know about a given player
struct Player {
    int id;  // unique id assigned to player
    int avatar_id;  // avatar id ensures colors consistency
    int ingest_server_id;
    char name[MAX_STRING_LEN];  // player_name
    struct Avatar avatar;  // avatar to be constructed on client-side
    struct sockaddr_in addr;  // address to send messages to
    struct timeval timestamp;  // time of last heartbeat message (used for timeouts)
};

struct ProtoMemory {
    ADS__Message mess;
    ADS__WorldResponse world_res;
    ADS__SynResponse syn_res;
    ADS__PresenceResponse presence_res;
    ADS__RpcRequest rpc_req;
    ADS__StatisticsResponse stats_res;
};

// Types of messages that can be sent or received.
enum Type { // request type
    UNKNOWN = ADS__TYPE__UNKNOWN, // default unprocessed type
    TEST = ADS__TYPE__TEST,
    SYN = ADS__TYPE__SYN,
    FIN = ADS__TYPE__FIN,
    CONTINUOUS = ADS__TYPE__CONTINUOUS,
    RPC = ADS__TYPE__RPC,
    HEARTBEAT = ADS__TYPE__HEARTBEAT,
    WORLD = ADS__TYPE__WORLD, // all gameobjects, sent from server
    PRESENCE = ADS__TYPE__PRESENCE, // which players are in the session
    STATISTICS = ADS__TYPE__STATISTICS,
    SERVER_HEARTBEAT = 100
};

struct Message {
    int sender_id; // for both servers and clients
    enum Type type;
    char data[MAX_DATA_SIZE]; // encoded reponse or request
};

struct MessageWrapper {
    int server_id;  // for both servers and clients
    struct sockaddr_in from_addr;
    uint32_t digest;
    uint64_t unix_time;  // microseconds since epoch
    uint64_t recv_time;  // time when this server received the message
    struct Message message;
};

struct SynRequest {
    char player_name[MAX_STRING_LEN];  // Proposed player name
    char server_name[MAX_STRING_LEN];  // Server name (WAS, ATL, LAX, etc)
    char lobby_name[MAX_STRING_LEN];  // Lobby for player to join
    int ingest_server_id;  // 0 if unknown / non-zero if known
};

struct SynResponse {
    char player_name[MAX_STRING_LEN];  // echoing player name (altered if naming conflict)
    char lobby_name[MAX_STRING_LEN];  // empty if player is not currently in a lobby
    int client_id;  // Assigning client_id
    int server_tick_delay_ms;  // Assigning ingest_server_id
    int ingest_server_id;  // Assigning ingest_server_id
    char ingest_server_ip[MAX_STRING_LEN];  // ip of ingest server
    int ingest_server_port;  // port of ingest server
    int max_players_per_lobby;  // Modulus lets colors be set
};

struct ContinuousData {
    struct Transform headset;
    struct Transform left_controller;
    struct Transform right_controller;
    struct Vector2 left_joystick;
    struct Vector2 right_joystick;
    struct Vector3 player_offset;
};

struct ContinuousRequest {
    struct ContinuousData data;
};

struct RpcRequest {
    int initiator_id;
    char rpc[MAX_STRING_LEN];   // remote procedure call
};

struct WorldAvatar {
    int player_id;
    struct ContinuousData data;
};

struct WorldOwnedVec3 {
    int owner_id;
    char item_name[MAX_STRING_LEN];
    struct Vector3 position;
};

struct WorldMetaData {
    struct Vector3 int_sphere_offset;
};

struct WorldResponse {
    struct WorldAvatar avatars[MAX_NUM_PLAYERS];
    struct WorldOwnedVec3 items[MAX_NUM_ITEMS];
    int num_avatars;
    int num_items;
};

struct PresencePlayer {
    char player_name[MAX_STRING_LEN];
    int player_id;
    char datacenter[MAX_STRING_LEN];
};

struct PresenceResponse {
    char lobby_name[MAX_STRING_LEN];
    struct PresencePlayer player_infos[MAX_NUM_PLAYERS]; 
    int num_players;
};

struct StatisticsResponse {
    uint32_t hash;
    char datacenter[MAX_STRING_LEN];
    uint64_t recv_us;
    uint64_t handle_us;
};

struct MinPrioQ {
    int num_elements;
    struct MessageWrapper elements[MAX_QUEUE_LEN];
};

// This manages the room that players are in. Session and lobby are used
// interchangeably
struct Session {
    char lobby[MAX_STRING_LEN];  // name of lobby/session
    int next_player_idx;
    int has_changed;  // boolean to note if server state has changed. if true, we will send update to clients
    int timeout_sec;  // number of seconds before we kick player for timeout
    double tick_delay_s;  // tick_delay_s (in seconds) for the lobby
    struct Player players[MAX_NUM_PLAYERS];  // Dynamically allocate memory later
    int num_players; 
    struct WorldOwnedVec3 world_items[MAX_NUM_ITEMS];
    struct WorldMetaData world_meta;
    int num_world_items; 
};

// Variable that holds all information for server
struct Context {
    int server_id;  // id number of server
    char server_name[MAX_STRING_LEN];  // name of server
    char server_ip[MAX_STRING_LEN];  // ip of server (DNS resovlable)
    int server_port;  // port of server
    struct sockaddr_in spines_mcast_addr;  // socket to send to servers on spines
    struct timeval timeout;  // tick timeout timeval
    int num_sessions;  
    int num_sessions_created;
    double tick_delay_s;  // Default tick_delay_s to be given to newly created lobbies
    uint32_t sync_delay_us;  // Default tick_delay_s to be given to newly created lobbies
    uint32_t server_heartbeat_time_us;
    int client_sk;  // socket used to send data
    int spines_sk;
    struct Session sessions[MAX_NUM_SESSIONS];  // list of lobbies/sessions
    struct MinPrioQ queue;
    struct ProtoMemory proto_mem;
};

#endif