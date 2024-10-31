#include "cbase.h"
#include "fmtstr.h"

#ifdef GAME_DLL
#include "gameinterface.h"
extern CServerGameDLL g_ServerGameDLL;
#endif

enum ETFCVOverrideFlags
{
	OVERRIDE_SERVER = ( 1 << 0 ),
	OVERRIDE_CLIENT = ( 1 << 1 ),

	OVERRIDE_BOTH = ( OVERRIDE_SERVER | OVERRIDE_CLIENT ),
};

struct CTFConVarDefaultOverrideEntry
{
	ETFCVOverrideFlags nFlags; // flags
	const char *pszName;       // ConVar name
	const char *pszValue;      // new default value
};

static CTFConVarDefaultOverrideEntry s_TFConVarOverrideEntries[] =
{
	/* interpolation */
	{ OVERRIDE_CLIENT, "cl_interp",               "0" },
	{ OVERRIDE_CLIENT, "cl_interp_ratio",       "2.0" },

	/* lag compensation */
	{ OVERRIDE_CLIENT, "cl_lagcompensation",      "1" },

	/* client-side prediction */
	{ OVERRIDE_CLIENT, "cl_pred_optimize",        "2" },
	{ OVERRIDE_CLIENT, "cl_smooth",               "1" },
	{ OVERRIDE_CLIENT, "cl_smoothtime",        "0.05" },

	/* client rates */
	{ OVERRIDE_CLIENT, "rate",                "80000" },
	{ OVERRIDE_CLIENT, "cl_updaterate",          "66" },
	{ OVERRIDE_CLIENT, "cl_cmdrate",             "66" },

	/* server rates */
	{ OVERRIDE_SERVER, "sv_minrate",          "65536" }, // 0.5 mebibits per second
	{ OVERRIDE_SERVER, "sv_maxrate",              "0" }, // Real max is 1048576 == 8 mebibits per second to bytes per second
	{ OVERRIDE_SERVER, "sv_minupdaterate",       "30" },
	{ OVERRIDE_SERVER, "sv_maxupdaterate",       "66" },
	{ OVERRIDE_SERVER, "sv_mincmdrate",          "30" },
	{ OVERRIDE_SERVER, "sv_maxcmdrate",          "66" },
	{ OVERRIDE_SERVER, "sv_client_predict",       "1" }, // Force clients to predict

	/* voice */
	{ OVERRIDE_SERVER, "sv_voicecodec", "steam" },
	//{ OVERRIDE_CLIENT, "voice_maxgain",           "1" },

	/* nav mesh generation */
	{ OVERRIDE_SERVER, "nav_area_max_size",      "10" },

	/* fix maps with large lightmaps crashing on load */
	{ OVERRIDE_BOTH, "r_hunkalloclightmaps", "0" },

	/* optimization for projected textures. broken for sdk2013 */
	{ OVERRIDE_CLIENT, "r_flashlightscissor", "0" },
};


// override some of the more idiotic ConVar defaults with more reasonable values
class CTFConVarDefaultOverride : public CAutoGameSystem
{
public:
	CTFConVarDefaultOverride() : CAutoGameSystem( "TFConVarDefaultOverride" ) {}
	virtual ~CTFConVarDefaultOverride() {}

	virtual bool Init() OVERRIDE;
	virtual void PostInit() OVERRIDE;

private:
	void OverrideDefault( const CTFConVarDefaultOverrideEntry& entry );

#ifdef GAME_DLL
	static const char *DLLName() { return "SERVER"; }
#else
	static const char *DLLName() { return "CLIENT"; }
#endif

};
static CTFConVarDefaultOverride s_TFConVarOverride;

// There is a buffer overflow involving sv_downloadurl.
// We need to clamp it to 128 chars or less.
// This will get hardened and moved to sdk gigalib eventually.
// -sappho
void sv_downloadurl_changed(IConVar* var, const char* pOldValue, float flOldValue)
{
	ConVar* sv_downloadurl = cvar->FindVar("sv_downloadurl");
	const char* sv_downloadurl_cstr = sv_downloadurl->GetString();
	if (strlen(sv_downloadurl_cstr) > 127)
	{
		Warning("sv_downloadurl too long, malformed, or otherwise bad! Kicking clients...\n");

#ifdef CLIENT_DLL
		engine->ExecuteClientCmd("disconnect");
#endif
		sv_downloadurl->SetValue("https://0.0.0.0/sv_download_url_is_bad");
	}
}

#ifdef CLIENT_DLL
void runDLUrlWrangling()
{
	ConVarRef refDLUrl("sv_downloadurl");
	if (!refDLUrl.IsValid())
	{
		return;
	}
	ConVar* DLUrlPtr = static_cast<ConVar*>(refDLUrl.GetLinkedConVar());

	DLUrlPtr->InstallChangeCallback(sv_downloadurl_changed);
}
#include <fakeconvar.h>
#include <memy/memytools.h>
static FnChangeCallback_t engineCallback = nullptr;
void hdr_changed(IConVar* var, const char* pOldValue, float flOldValue)
{
	static_cast<ConVar*>(var)->SetValue(2);
	engineCallback(var, pOldValue, flOldValue);
}

#include <misc_helpers.h>

// we can't do this the "normal" way since we want it to be untouchable, so we have to do this jank,
// where we set the flags by member var manually (ugh),
// force change the default (ugh),
// and pseudo-detour the original engine callback for mat_hdr_level by grabbing its address and comparing it
// to make sure it's inside of the engine bin. then, if it is, we store it in a static var, 
// null out the old callback, set our *own* callback, run it once (InstallChangeCallback does this by itself),
// and whenever mat_hdr_level gets changed (which it shouldn't, but just in case it does), it'll set the value to 2,
// and then call the original engine callback that we saved accordingly
// This will spew "Not playing a local game." in console, because it's trying to run `save` for some stupid reason that i don't care about
void runHdrWrangling()
{
	ConVarRef hdrRef("mat_hdr_level");
	if (hdrRef.IsValid())
	{
		ConVar* hdrPtr = static_cast<ConVar*>(hdrRef.GetLinkedConVar());
		if (hdrPtr)
		{
			hdrPtr->AddFlags(FCVAR_DEVELOPMENTONLY);
			hdrPtr->SetDefault("2");
			hdrPtr->SetValue("2");

			uintptr_t cbAddr = reinterpret_cast<uintptr_t>(hdrPtr->m_fnChangeCallback);
			if (memy::IsAddrInsideBin(engine_bin, cbAddr))
			{
				engineCallback = hdrPtr->m_fnChangeCallback;
				hdrPtr->m_fnChangeCallback = nullptr;
				hdrPtr->InstallChangeCallback(hdr_changed);
			}
		}
	}

	// To disable the effects of HDR, requires cheat flag removal
	ConVarRef mat_dynamic_tonemapping("mat_dynamic_tonemapping");
	if (mat_dynamic_tonemapping.IsValid())
	{
		ConVar* pConvar = mat_dynamic_tonemapping.GetLinkedConVarPtr();
		if (pConvar)
		{
            ((FakeConVar*)pConvar)->SetFlags(FCVAR_ARCHIVE);
		}
	}

	// To disable the effects of HDR, requires cheat flag removal
	ConVarRef mat_disable_bloom("mat_disable_bloom");
	if (mat_disable_bloom.IsValid())
	{
		ConVar* pConvar = mat_disable_bloom.GetLinkedConVarPtr();
		if (pConvar)
		{
			((FakeConVar*)pConvar)->SetFlags(FCVAR_ARCHIVE);
		}
	}

	// People with FPS configs are getting black screens due to forced HDR, so force these convars
	ConVarRef mat_autoexposure_min("mat_autoexposure_min");
	if (mat_autoexposure_min.IsValid())
	{
		ConVar* pConvar = mat_autoexposure_min.GetLinkedConVarPtr();
		if (pConvar)
		{
			pConvar->SetValue(0.8f);
            ((FakeConVar*)pConvar)->SetFlags(FCVAR_CHEAT);
		}
	}

	ConVarRef mat_autoexposure_max("mat_autoexposure_max");
	if (mat_autoexposure_max.IsValid())
	{
		ConVar* pConvar = mat_autoexposure_max.GetLinkedConVarPtr();
		if (pConvar)
		{
			pConvar->SetValue(1.2f);
            ((FakeConVar*)pConvar)->SetFlags(FCVAR_CHEAT);
		}
	}
}

void runDx8Wrangling(void)
{
	// Don't allow DXLevel to be changed in game, only from -dxlevel launch parameter
	// This essentially removes DirectX 8 from existence
	ConVarRef mat_dxlevel("mat_dxlevel");
	if (mat_dxlevel.IsValid())
	{
		ConVar* pConvar = mat_dxlevel.GetLinkedConVarPtr();
		if (pConvar)
		{
			if (pConvar->GetInt() > 50 && pConvar->GetInt() < 90)
				Error("Team Fortress 2 Classic cannot be ran in DirectX 8 or lower.\nPut -dxlevel 95 into the launch parameters of your game, start the game once and then remove this launch parameter afterwards, so it's permanently saved.");

			pConvar->AddFlags(FCVAR_DEVELOPMENTONLY);
		}
	}
}
#endif

void CTFConVarDefaultOverride::PostInit()
{
#ifdef CLIENT_DLL
	runDLUrlWrangling();
	runHdrWrangling();
	runDx8Wrangling();
#endif
}

bool CTFConVarDefaultOverride::Init()
{
	for ( const auto& entry : s_TFConVarOverrideEntries )
	{
		OverrideDefault( entry );
	}

#ifdef GAME_DLL
	/* ensure that server-side clock correction is ACTUALLY limited to 2 tick-intervals */
	static CFmtStrN<16> s_Buf( "%.0f", 2 * ( g_ServerGameDLL.GetTickInterval() * 1000.0f ) );
	OverrideDefault( { OVERRIDE_SERVER, "sv_clockcorrection_msecs", s_Buf } );
#endif
	return true;
}



void CTFConVarDefaultOverride::OverrideDefault( const CTFConVarDefaultOverrideEntry& entry )
{
#ifdef GAME_DLL
	if ( !( entry.nFlags & OVERRIDE_SERVER ) )
		return;
#else
	if ( !( entry.nFlags & OVERRIDE_CLIENT ) )
		return;
#endif

	ConVarRef ref( entry.pszName, true );
	if ( !ref.IsValid() )
	{
		DevWarning( "[%s] CTFConVarDefaultOverride: can't get a valid ConVarRef for \"%s\"\n", DLLName(), entry.pszName );
		return;
	}

	auto pConVar = dynamic_cast<ConVar *>( ref.GetLinkedConVar() );
	if ( pConVar == nullptr )
	{
		DevWarning( "[%s] CTFConVarDefaultOverride: can't get a ConVar ptr for \"%s\"\n", DLLName(), entry.pszName );
		return;
	}

	CUtlString strOldDefault( pConVar->GetDefault() );
	CUtlString strOldValue  ( pConVar->GetString()  );

	/* override the convar's default, and if it was at the default, keep it there */
	bool bWasDefault = FStrEq( pConVar->GetString(), pConVar->GetDefault() );
	pConVar->SetDefault( entry.pszValue );
	if ( bWasDefault )
		pConVar->Revert();

	DevMsg( "[%s] CTFConVarDefaultOverride: \"%s\" was \"%s\"/\"%s\", now \"%s\"/\"%s\"\n", DLLName(), entry.pszName,
		strOldDefault.Get(), strOldValue.Get(), pConVar->GetDefault(), pConVar->GetString() );
}

