#include <cbase.h>


#include <engine_memutils.h>
#include <memytools.h>
#include <atomic>
#include <csignal>

volatile sig_atomic_t GetSpewPtr = NULL;
bool initEngineSpew()
{
	    /*                              v-- unique string -------------v
        sub_10246FC0(Destination, "\nConsole History (reversed)\n\n", 95000, -1);
        v16 = strlen(Destination);
        if (v16 < 0x17318)
        {
            result = GetSpew_sub_101FC3A0(&Destination[v16], 94999 - v16);
            Destination[94999] = 0;
        }
        return result;
    */
#ifdef _WIN32
    //Signature for GetSpew_sub_101FC3A0:
    //55 8B EC 53 FF 15 ?  ?  ?  ?  8B D0 BB  ?  ?  ?  ? 3B 15 ? ? ? ?     74 ? 8B CA 33 C0 F0 0F B1 0B 85 C0 74 ? F3 90 6A 00 52 8B CB FF 15 ? ? ? ? EB ? FF 05 ? ? ? ? 0F B7 05 ? ? ? ?
    static constexpr const char*    pattern     = "\x55\x8B\xEC\x53\xFF\x15\x2A\x2A\x2A\x2A\x8B\xD0\xBB\x2A\x2A\x2A\x2A\x3B\x15\x2A\x2A\x2A\x2A\x74\x2A\x8B\xCA\x33\xC0\xF0\x0F\xB1\x0B\x85\xC0\x74\x2A\xF3\x90\x6A\x00\x52\x8B\xCB\xFF\x15\x2A\x2A\x2A\x2A\xEB\x2A\xFF\x05\x2A\x2A\x2A\x2A\x0F\xB7\x05\x2A\x2A\x2A\x2A";
    static constexpr size_t         patternSize = 65;
#else // LINUX

    // for now these sigs are the same
    // -sappho
//#ifdef GAME_DLL
//  // Signature for _Z7GetSpewPcj:
//  // 55 89 E5 57 56 53 83 EC 2C 8B 7D 08 8B 75 0C E8 ? ? ? ?
//  static constexpr const char*    pattern     = "\x55\x89\xE5\x57\x56\x53\x83\xEC\x2C\x8B\x7D\x08\x8B\x75\x0C\xE8\x2A\x2A\x2A\x2A";
//  static constexpr size_t         patternSize = 20;
//#else
    // Signature for sub_5742E0:
    // 55 89 E5 57 56 53 83 EC 2C 8B 7D 08 8B 75 0C E8 ? ? ? ?
    static constexpr const char*    pattern     = "\x55\x89\xE5\x57\x56\x53\x83\xEC\x2C\x8B\x7D\x08\x8B\x75\x0C\xE8\x2A\x2A\x2A\x2A";
    static constexpr size_t         patternSize = 20;       
#endif

    GetSpewPtr = memy::FindPattern(engine_bin, pattern, patternSize, 0);
    ConVarRef sys_minidumpspewlines("sys_minidumpspewlines", false);
    if (sys_minidumpspewlines.IsValid())
    {
        // will never be null if valid
        ConVar* spewCvar = static_cast<ConVar*>(sys_minidumpspewlines.GetLinkedConVar());
        // up defaults from 512 to 2048
        spewCvar->SetDefault("2048");
        spewCvar->SetValue(2048);
        return true;
    }
    return false;
}

char SpewOut[256000] = {};
// Do not free the return value of this function ever - it is a global char[256000]
char* Engine_GetSpew()
{
	typedef void GetSpew(char* buffer, unsigned int length);
	GetSpew* SpewFunc = (GetSpew*)GetSpewPtr;
    SpewFunc( (char*)SpewOut, 256000 );
	return SpewOut;
}

#ifdef GAME_DLL
#include <iclient.h>
#include <inetchannel.h>
#include <inetchannelinfo.h>

// uniquish string demorestart in ExecStringCmd, that function + 3, see here
// https://github.com/sapphonie/StAC-tf2/blob/a834bb1c2b4a0431de9b89998504684e3fc7af5f/gamedata/stac.txt#L86-L130
/*
 dd offset _ZN11CBaseClient20ExecuteStringCommandEPKc ; CBaseClient::ExecuteStringCommand(char const*)
 dd offset _ZN11CBaseClient10SendNetMsgER11INetMessageb ; CBaseClient::SendNetMsg(INetMessage &,bool)
 dd offset _ZN11CBaseClient12ClientPrintfEPKcz ; CBaseClient::ClientPrintf(char const*,...)
 dd offset _ZNK11CBaseClient11IsConnectedEv ; CBaseClient::IsConnected(void)
 dd offset _ZNK11CBaseClient9IsSpawnedEv ; CBaseClient::IsSpawned(void)
 dd offset _ZNK11CBaseClient8IsActiveEv ; CBaseClient::IsActive(void)
*/

/*
	sub_1009B4E0
	sub_1009B4E0
	sub_1009B4E0
	sub_1009B4E0          ; BOOL __thiscall sub_1009B4E0(_DWORD *this)
	sub_1009B4E0          sub_1009B4E0 proc near
	sub_1009B4E0      000 xor     eax, eax        ; Logical Exclusive OR
	sub_1009B4E0+2    000 cmp     dword ptr [ecx+0C4h], 6 ; Compare Two Operands (you want the 0xC4 here)
	sub_1009B4E0+9    000 setz    al              ; Set Byte if Zero (ZF=1)
	sub_1009B4E0+C    000 retn                    ; Return Near from Procedure
	sub_1009B4E0+C        sub_1009B4E0 endp
	sub_1009B4E0+C
*/

int GetSignOnStateFromNetChanInfo(INetChannelInfo* info)
{
	if (!info)
	{
		return -1;
	}
	INetChannel* netchan = static_cast<INetChannel*>(info);
	IClient* iclient = static_cast<IClient*>(netchan->GetMsgHandler());
	if (!iclient)
	{
		return -1;
	}
	uintptr_t iclient_addr = (uintptr_t)iclient;

#ifdef _WIN32
	uintptr_t offs = 0xC4;
#else
	uintptr_t offs = 0xC8;
#endif

#ifdef _WIN32
	uint8 signonstate_byte = *(reinterpret_cast<uint8*>( iclient_addr + offs));
#else
	uint8 signonstate_byte = *(reinterpret_cast<uint8*>( (iclient_addr - 0x4) + offs));
#endif
	int signonstate = signonstate_byte;

	return signonstate;
}


int UTIL_GetSignonState(CBasePlayer* basePlayer)
{
	if (!basePlayer)
	{
		Warning("Null baseplayer in GetClientSignonstate\n");
		return -1;
	}
	INetChannelInfo* info = engine->GetPlayerNetInfo(basePlayer->entindex());
	return GetSignOnStateFromNetChanInfo(info);
}


int UTIL_GetSignonState(INetChannelInfo* info)
{
	return GetSignOnStateFromNetChanInfo(info);
}


bool UTIL_IsFullySignedOn(CBasePlayer* basePlayer)
{
	int signonstate = UTIL_GetSignonState(basePlayer);
	if (signonstate != 6 && signonstate != 7)
	{
		return false;
	}
	return true;
}

#endif

/*
	if ( !UTIL_IsFullySignedOn(pPlayer) )
	{
		return TICK_INTERVAL;
	}
*/
