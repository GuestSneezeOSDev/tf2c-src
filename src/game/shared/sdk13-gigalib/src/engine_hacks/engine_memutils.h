#pragma once

#include <signal.h>
#include <csignal>
bool initEngineSpew();
char* Engine_GetSpew();

#ifdef GAME_DLL
int UTIL_GetSignonState(CBasePlayer* basePlayer);
int UTIL_GetSignonState(INetChannelInfo* info);
bool UTIL_IsFullySignedOn(CBasePlayer* basePlayer);
#endif
