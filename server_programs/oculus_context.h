#ifndef OCULUS_CONTEXT_H
#define OCULUS_CONTEXT_H

#include "oculus_structs.h"

/**
 * @brief Given a request, populate a pointer to the session that the 
 * player is in.
 * 
 * @param session pointer to session pointer. This will be populated inside 
 * the function if ret = 0 or 1. The session will be the session that
 * the request->lobby specifies
 * @param context global context
 * @param player_id request from client
 * @return ** int : 0 if lobby exists and player is in lobby, 1 if lobby exists
 * and player is not in lobby. 2 if lobby doesn't exist
 */
int get_player_session(struct Session** session, struct Context* context, int player_id);

/**
 * @brief Remove any players in any sessions who have timed out
 * 
 * @param context global context
 * @return ** int : number of players removed
 */
int remove_players_if_timeout(struct Context* context);

// TO BE DEPRECATED in lieu of LocomotionDriver on client-side
int apply_movement_step(struct Session* session);

/**
 * @brief Add a player to a session/lobby
 * 
 * @param session session to add player to
 * @param request contains information about player name
 * @return ** struct Player* : pointer to player that was just added
 */
struct Player* add_player_to_lobby(struct Session* session, struct SynRequest* request);

/**
 * @brief Add a session to the global context
 * 
 * @param context global context
 * @param request contains information about lobby name
 * @return ** struct Session* : pointer to session that was just created
 */
struct Session* add_lobby_to_context(struct Context* context, struct SynRequest* request);

int send_world_state(struct Context* context);

int setup_context(struct Context* context, int argc, char* argv[]);

#endif