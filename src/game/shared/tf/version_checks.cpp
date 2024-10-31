#include <cbase.h>


#include <chksum_file.h>
#include "version_checks.h"
#include <GameEventListener.h>
#include <inetchannel.h>

CVersioning _CVersioning;
CVersioning::CVersioning() : CAutoGameSystem("")
{
}

ConVar versioning_debug("versioning_debug", "0", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_CHEAT, "Debug versioning system, for verifying client/server hash matching.");

bool snapbacktoreality = false;
void versvar_changed(const char* oldval)
{
    if (snapbacktoreality)
    {
        CVersioning::RecalcHashes();
    }
    return;
}

void c_versvar_changed(IConVar* var, const char* pOldValue, float flOldValue)
{
    versvar_changed(pOldValue);
}

void s_versvar_changed(IConVar* var, const char* pOldValue, float flOldValue)
{
    // client version cvars are replicated, so we only care about server here
    #if TF_CLASSIC
        versvar_changed(pOldValue);
    #endif
}

#ifdef TF_CLASSIC_CLIENT
    ConVar c_cli_version("c_cli_version", "", FCVAR_USERINFO | FCVAR_HIDDEN | FCVAR_SPONLY, "", c_versvar_changed);
    ConVar c_srv_version("c_srv_version", "", FCVAR_USERINFO | FCVAR_HIDDEN | FCVAR_SPONLY, "", c_versvar_changed);

    ConVar s_cli_version_win32("s_cli_version_win32", "", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_SPONLY | FCVAR_CHEAT, "");
    ConVar s_cli_version_linux("s_cli_version_linux", "", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_SPONLY | FCVAR_CHEAT, "");
    ConVar s_srv_version_win32("s_srv_version_win32", "", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_SPONLY | FCVAR_CHEAT, "");
    ConVar s_srv_version_linux("s_srv_version_linux", "", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_SPONLY | FCVAR_CHEAT, "");
#endif

#ifdef TF_CLASSIC
    ConVar s_cli_version_win32("s_cli_version_win32", "", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_SPONLY | FCVAR_CHEAT, "", s_versvar_changed);
    ConVar s_cli_version_linux("s_cli_version_linux", "", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_SPONLY | FCVAR_CHEAT, "", s_versvar_changed);
    ConVar s_srv_version_win32("s_srv_version_win32", "", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_SPONLY | FCVAR_CHEAT, "", s_versvar_changed);
    ConVar s_srv_version_linux("s_srv_version_linux", "", FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_SPONLY | FCVAR_CHEAT, "", s_versvar_changed);
#endif

const char* badbins_dc_msg = "Your game binaries do not match this server's. Try updating your game";
void CVersioning::FireGameEvent(IGameEvent* event)
{
    int userid = event->GetInt("userid");
    CBasePlayer* eventBasePlayer = static_cast<CBasePlayer*>(UTIL_PlayerByUserId(userid));
    if (!eventBasePlayer)
    {
        return;
    }

    if (UTIL_IsFakePlayer(eventBasePlayer))
    {
        return;
    }

    #ifdef TF_CLASSIC_CLIENT
        if (versioning_debug.GetBool())
        {
            Warning
            (
                "[versdbg]       \n"
                "!! TF_CLASSIC_CLI !!\n"
                "c_cli_version          = %s\n"
                "s_cli_version_win32    = %s\n"
                "s_cli_version_linux    = %s\n"
                "s_srv_version_win32    = %s\n"
                "s_srv_version_linux    = %s\n",
                c_cli_version.GetString(),
                s_cli_version_win32.GetString(),
                s_cli_version_linux.GetString(),
                s_srv_version_win32.GetString(),
                s_srv_version_linux.GetString()
            );
        }

        INetChannel* netchan = static_cast<INetChannel*>(engine->GetNetChannelInfo());
        // just in case?
        if (!netchan)
        {
            // Error("NO NETCHAN??");
            Warning("NO NETCHAN?\n");
            engine->ExecuteClientCmd("disconnect\n");
        }

        #ifdef _WIN32
            // win cli != server copy of win cli
            if ( V_stricmp( c_cli_version.GetString(), s_cli_version_win32.GetString() ) != 0 )
            {
                netchan->Shutdown(badbins_dc_msg);
                return;
            }
        #else
            // linux cli != server copy of linux cli
            if ( V_stricmp(c_cli_version.GetString(), s_cli_version_linux.GetString() ) != 0 )
            {
                netchan->Shutdown(badbins_dc_msg);
                return;
            }
        #endif

        bool clisrv_matches_w32 = V_stricmp(c_srv_version.GetString(), s_cli_version_win32.GetString()) != 0;
        bool clisrv_matches_lin = V_stricmp(c_srv_version.GetString(), s_cli_version_linux.GetString()) != 0;
        // local srv copy doesnt match EITHER copy [mismatched], or it matches BOTH copies [this is impossible]
        if (clisrv_matches_w32 != clisrv_matches_lin)
        {
            netchan->Shutdown(badbins_dc_msg);
            return;
        }


        return;

    #endif
    #ifdef TF_CLASSIC
        int idx = eventBasePlayer->entindex();
        if (idx <= 0)
        {
            return;
        }

        const char* c_clivers = engine->GetClientConVarValue(idx, "c_cli_version");
        const char* c_srvvers = engine->GetClientConVarValue(idx, "c_srv_version");

        const char* s_clivers_w32 = s_cli_version_win32.GetString();
        const char* s_clivers_lin = s_cli_version_linux.GetString();

        const char* s_srvvers_w32 = s_srv_version_win32.GetString();
        const char* s_srvvers_lin = s_srv_version_linux.GetString();

        if (versioning_debug.GetBool())
        {
            Warning
            (
                "[versdbg]   \n"
                "!! TF_CLASSIC !!\n"
                "c_clivers       = %s\n"
                "s_clivers_w32   = %s\n"
                "s_clivers_lin   = %s\n"
                "c_srvvers       = %s\n"
                "s_srvvers_w32   = %s\n"
                "s_srvvers_lin   = %s\n",
                c_clivers,
                s_clivers_w32,
                s_clivers_lin,
                c_srvvers,
                s_srvvers_w32,
                s_srvvers_lin
            );
        }

        // they can't BOTH be wrong [that means NO files match]
        // they can't BOTH be right [that means BOTH files match, which is impossible (this is a sanity check!) ]
        // IF they win == linux, one of these things is implied, so we kick the client for mismatching
        // funny boolean logic go brr
        bool w32_match = V_stricmp(c_clivers, s_clivers_w32) != 0;
        bool lin_match = V_stricmp(c_clivers, s_clivers_lin) != 0;
        if (w32_match == lin_match)
        {
            const char* kickcmd = UTIL_VarArgs("kickid %i %s;", userid, badbins_dc_msg);
            engine->ServerCommand(kickcmd);
            return;
        }
        w32_match = V_stricmp(c_srvvers, s_srvvers_w32) != 0;
        lin_match = V_stricmp(c_srvvers, s_srvvers_lin) != 0;
        if (w32_match == lin_match)
        {
            const char* kickcmd = UTIL_VarArgs("kickid %i %s;", userid, badbins_dc_msg);
            engine->ServerCommand(kickcmd);
            return;
        }

        return;
    #endif
}

bool CVersioning::Init()
{
    // SHARED ACROSS TF_CLASSIC && TF_CLASSIC_CLIENT
    ListenForGameEvent("player_activate");

    return true;
}

void CVersioning::PostInit()
{
    Warning("Recalculating bin hashes...\n");
    RecalcHashes();

    snapbacktoreality = true;
}

void CVersioning::RecalcHashes()
{
    #ifdef TF_CLASSIC
        Warning("RecalcHashes - TF_CLASSIC ->\n");
    #endif
    #ifdef TF_CLASSIC_CLIENT
        Warning("RecalcHashes - TF_CLASSIC_CLI ->\n");
    #endif

    char clihash_WIN32[XXH3_128_charsize] = {};
    char clihash_LINUX[XXH3_128_charsize] = {};

    char srvhash_WIN32[XXH3_128_charsize] = {};
    char srvhash_LINUX[XXH3_128_charsize] = {};

    CChecksum::XXH3_128__File("bin/client.dll", clihash_WIN32);
    CChecksum::XXH3_128__File("bin/client.so",  clihash_LINUX);
    

    CChecksum::XXH3_128__File("bin/server.dll", srvhash_WIN32);
    CChecksum::XXH3_128__File("bin/server.so",  srvhash_LINUX);
    // char enghash[XXH3_128_charsize] = {};
    // CChecksum::XXH3_128__File("bin/engine.dll", enghash, "BASE_PATH");
    // Warning("engine -> %s\n", enghash);
    #ifdef TF_CLASSIC_CLIENT

        snapbacktoreality = false;
        #ifdef _WIN32
            c_cli_version.SetValue(clihash_WIN32);
            c_srv_version.SetValue(srvhash_WIN32);
        #else
            c_cli_version.SetValue(clihash_LINUX);
            c_srv_version.SetValue(srvhash_LINUX);
        #endif
        snapbacktoreality = true;

    #endif

    #ifdef TF_CLASSIC

        snapbacktoreality = false;
        s_cli_version_win32.SetValue(clihash_WIN32);
        s_cli_version_linux.SetValue(clihash_LINUX);
        s_srv_version_win32.SetValue(srvhash_WIN32);
        s_srv_version_linux.SetValue(srvhash_LINUX);
        snapbacktoreality = true;

    #endif
}