/* NextBotPlayer
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotPlayerBody.h"


ConVar NextBotPlayerStop  ("nb_player_stop",   "0", FCVAR_CHEAT, "Stop all NextBotPlayers from updating");
ConVar NextBotPlayerWalk  ("nb_player_walk",   "0", FCVAR_CHEAT, "Force bots to walk");
ConVar NextBotPlayerCrouch("nb_player_crouch", "0", FCVAR_CHEAT, "Force bots to crouch");
ConVar NextBotPlayerMove  ("nb_player_move",   "1", FCVAR_CHEAT, "Prevents bots from moving");
