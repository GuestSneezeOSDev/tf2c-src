/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_sniper_attack.h"


static ConVar tf_bot_sniper_flee_range ("tf_bot_sniper_flee_range",  "400", FCVAR_CHEAT, "If threat is closer than this, retreat");
       ConVar tf_bot_sniper_melee_range("tf_bot_sniper_melee_range", "200", FCVAR_CHEAT, "If threat is closer than this, attack with melee weapon");
static ConVar tf_bot_sniper_linger_time("tf_bot_sniper_linger_time",   "5", FCVAR_CHEAT, "How long Sniper will wait around after losing his target before giving up");


// TODO
