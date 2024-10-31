//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include <tier0/icommandline.h>
#include <tier3/tier3.h>
#include <time.h>
#include <netadr.h>
#include "vgui/ILocalize.h"
#include <steam/steam_api.h>
#include "tf_shareddefs.h"
#include "igamesystem.h"
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "tf_mapinfomenu.h"

#include "tf2c_discord_rpc.h"

// ????
// using namespace vgui;

CTFDiscordRPC g_Discord = nullptr;

//-----------------------------------------------------------------------------
// Singleton
//-----------------------------------------------------------------------------
CTFDiscordRPC* GetDiscordRichPresence()
{
	return &g_Discord;
}


//-----------------------------------------------------------------------------
// Purpose: Updates the rich presence when any of these ConVars change.
//-----------------------------------------------------------------------------
void DiscordConVarChangedCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	GetDiscordRichPresence()->UpdatePresence();
}

ConVar tf2c_discord_show_map_info( "tf2c_discord_show_map_info", "1", FCVAR_ARCHIVE, "Shows what map you're playing on. 0 = Disabled, 1 = Enabled", DiscordConVarChangedCallback );
ConVar tf2c_discord_show_location( "tf2c_discord_show_location", "2", FCVAR_ARCHIVE, "Shows what you're currently doing. 0 = Disabled, 1 = Enabled, 2 = Enabled + Show Server Name", DiscordConVarChangedCallback );
ConVar tf2c_discord_show_player_info( "tf2c_discord_show_player_info", "1", FCVAR_ARCHIVE, "Shows what team you're on and what class you currently are. 0 = Disabled, 1 = Enabled", DiscordConVarChangedCallback );
ConVar tf2c_discord_swap_info( "tf2c_discord_swap_info", "0", FCVAR_ARCHIVE, "Swaps the map info and the class info around. 0 = Disabled, 1 = Enabled", DiscordConVarChangedCallback );

//-----------------------------------------------------------------------------
// Purpose: Callback Ready
//-----------------------------------------------------------------------------
static void Discord_Ready(const DiscordUser* user)
{
	*g_Discord.localdiscorduser = *user;

	Color dullyello(255, 136, 78, 255);
	ConColorMsg(dullyello, "Discord: Ready - User %s#%s\n", user->username, user->discriminator);
}

//-----------------------------------------------------------------------------
// Purpose: Callback Disconnect
//-----------------------------------------------------------------------------
static void Discord_Disconnect( int errorCode, const char *message )
{
	Warning( "Discord: Disconnected (%d, %s)\n", errorCode, message );
}

//-----------------------------------------------------------------------------
// Purpose: Callback Error
//-----------------------------------------------------------------------------
static void Discord_Error( int errorCode, const char *message )
{
	Warning( "Discord: Error (%d, %s)\n", errorCode, message );
}

//-----------------------------------------------------------------------------
// Purpose: Callback JoinGame
//-----------------------------------------------------------------------------
static void Discord_JoinGame( const char *joinSecret )
{
	engine->ClientCmd_Unrestricted( VarArgs( "connect %s", joinSecret ) );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFDiscordRPC::CTFDiscordRPC( const char *pszName ) : CAutoGameSystemPerFrame( pszName )
{
	m_iConnectionTime = 0;
	m_szConnectionString[0] = '\0';
	m_szSteamID[0] = '\0';
	m_iNumPlayers = 0;
	m_bInGame = false;

	m_bIsSaneServerToConnectTo = false;

	m_szHostName[0] = '\0';

	localdiscorduser = new DiscordUser{};
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFDiscordRPC::~CTFDiscordRPC()
{
	delete [] localdiscorduser;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDiscordRPC::Init( void )
{
#ifndef TF2C_BETA
	if ( !CommandLine()->CheckParm( "-nodiscordpresence" ) )
	{
#else
	if( 0 ) // stub rpc in beta
	{
#endif
		m_bDisabledViaCommandLine = false;

		// Init Discord.
		// IMPORTANT: Client ID will need to be updated if we re-create Discord app.

		// TF2c ID: 378278981297766400
		// Agrimars ID: 459640917033353226
		const char *szClientID = "459640917033353226";

		DiscordEventHandlers handlers;
		memset( &handlers, 0, sizeof( handlers ) );
		handlers.ready = Discord_Ready;
		handlers.disconnected = Discord_Disconnect;
		handlers.errored = Discord_Error;
		handlers.joinGame = Discord_JoinGame;
		//Discord_Initialize( "378278981297766400", &handlers, true, VarArgs( "%d", engine->GetAppID() ) );
		Discord_Initialize( szClientID, &handlers, false, NULL );

		// HACK: Shitty way of getting the launch command until we get on Steam. This needs to go away later.
		char szCommand[512];
		V_sprintf_safe( szCommand, "%s -game \"%s\" -steam", CommandLine()->GetParm( 0 ), CommandLine()->ParmValue( "-game", CommandLine()->ParmValue( "-defaultgamedir", "hl2" ) ) );
		Discord_Register( szClientID, szCommand );

		UpdatePresence();

		ListenForGameEvent( "server_spawn" );
		ListenForGameEvent( "localplayer_changeteam" );
		ListenForGameEvent( "localplayer_changeclass" );
	}
	else
	{
		m_bDisabledViaCommandLine = true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordRPC::Shutdown( void )
{
	if ( !m_bDisabledViaCommandLine )
	{
		Discord_Shutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordRPC::LevelInitPostEntity( void )
{
	if ( !m_bDisabledViaCommandLine )
	{
		m_iConnectionTime = time( 0 );
		m_bInGame = true;
		CountPlayers();
		UpdatePresence();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordRPC::LevelShutdownPostEntity( void )
{
	if ( !m_bDisabledViaCommandLine )
	{
		m_bInGame = false;
		m_iNumPlayers = 0;
		UpdatePresence();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordRPC::Update( float frametime )
{
	if ( !m_bDisabledViaCommandLine )
	{
		if ( m_bInGame )
		{
			// Update state if player count changes.
			if ( CountPlayers() )
			{
				UpdatePresence();
			}
		}

		Discord_RunCallbacks();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordRPC::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( "localplayer_changeteam", event->GetName() ) || FStrEq( "localplayer_changeclass", event->GetName() ) )
	{
		UpdatePresence();
	}
	else if ( FStrEq( "server_spawn", event->GetName() ) )
	{
		V_strcpy_safe( m_szHostName, event->GetString( "hostname" ) );
		V_strcpy_safe( m_szSteamID,  event->GetString( "steamid" ) );

		// Check that our server isn't like. On lan or a local server, lol
		// -sappho
		char addr[24] = {};
		char port[8]  = {};
		V_strcpy_safe( addr, event->GetString("address") );
		V_strcpy_safe( port, event->GetString("port") );

		V_snprintf( m_szConnectionString, sizeof(m_szConnectionString), "%s:%s", addr, port );

		bool isdedi = event->GetBool( "dedicated" );
		bool ispwd  = event->GetBool( "password" );

		netadr_s netadr = {};
		netadr.SetFromString( m_szConnectionString );

		//Warning("isdedi %i ispwd = %i islocal = %i isreserved = %i isloopback = %i isvalid %i\n", isdedi, ispwd, netadr.IsLocalhost(), netadr.IsReservedAdr(), netadr.IsLoopback(), netadr.IsValid());

		if ( !isdedi || !ispwd || !netadr.IsValid() || netadr.IsLocalhost() || netadr.IsReservedAdr() || netadr.IsLoopback() )
		{
			//Warning("NOT SANE SERVER\n");
			m_bIsSaneServerToConnectTo = false;
		}
		else
		{
			//Warning("IS SANE SERVER\n");
			m_bIsSaneServerToConnectTo = true;
		}
		//Warning("-> SERVER SPAWN = %s\n", addr);
		//Warning("-> SERVER SPAWN = %s\n", port);
		//Warning("-> SERVER SPAWN = %s\n", m_szConnectionString);
		//Warning("-> SERVER SPAWN = %s\n", m_szHostName);
		//Warning("-> SERVER SPAWN = %s\n", m_szSteamID);

		UpdatePresence();
	}
}

extern s_MapTypeInfo s_MapTypes[MAX_MAP_TYPES];
extern const char *GetMapDisplayName( const char *mapName );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFDiscordRPC::UpdatePresence( void )
{
	if ( !m_bDisabledViaCommandLine )
	{
		// Update Discord Rich Presence.
		DiscordRichPresence discordPresence;
		memset( &discordPresence, 0, sizeof( discordPresence ) );

		bool bShowMapInfo = tf2c_discord_show_map_info.GetBool();
		bool bShowLocation = tf2c_discord_show_location.GetBool();
		bool bShowPlayerInfo = tf2c_discord_show_player_info.GetBool();
		bool bSwapInfo = tf2c_discord_swap_info.GetBool();
		if ( bShowMapInfo || bShowLocation || bShowPlayerInfo )
		{
			const char *mainInfoKey = "";
			const char *mainInfoText = "";
			// ----------------
			const char *sideInfoKey = "";
			const char *sideInfoText = "";

			static wchar_t wszLocalized[256];
			static char szLocalized[256];

			if ( m_bInGame )
			{
				if ( bShowMapInfo )
				{
					static char szDetails[256];
					static char szMapName[MAX_MAP_NAME];
					V_FileBase( engine->GetLevelName(), szMapName, sizeof( szMapName ) );
					const char *pszMapDisplayName = GetMapDisplayName( szMapName );
					const char *pszGameTypeName = GetGameTypeName();

					static wchar_t wszMapDisplayName[MAX_MAP_NAME];
					mbstowcs( wszMapDisplayName, pszMapDisplayName, MAX_MAP_NAME );

					g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( TFGameRules()->IsValveMap() ? "#Discord_InOfficialMap" : "#Discord_InCommunityMap" ), 1, wszMapDisplayName );
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szDetails, sizeof( szDetails ) );

					discordPresence.details = szDetails;
					discordPresence.startTimestamp = m_iConnectionTime;

					mainInfoKey = "hidden";
					mainInfoText = pszGameTypeName;

					for ( int i = 0, c = ARRAYSIZE( s_MapTypes ); i < c; ++i )
					{
						if ( !Q_strncmp( szMapName, s_MapTypes[i].pDiskPrefix, s_MapTypes[i].iLength ) )
						{
							mainInfoKey = s_MapTypes[i].pDiskPrefix;
							break;
						}
					}
				}
				else
				{
					mainInfoKey = "hidden";
				}

				C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pPlayer && bShowPlayerInfo )
				{
					int iTeam = pPlayer->GetTeamNumber();
					if ( iTeam < FIRST_GAME_TEAM )
					{
						sideInfoKey = "class_spectator";
						sideInfoText = "Spectator";
					}
					else
					{
						int iClass = pPlayer->GetPlayerClass()->GetClassIndex();
						static char szClassImage[128];
						const char *pszClassName = g_aPlayerClassNames_NonLocalized[iClass];
						const char *pszTeamName = g_aTeamNames[iTeam];
						V_sprintf_safe( szClassImage, "class_%s_%s", pszClassName, pszTeamName );
						V_strlower( szClassImage );

						if ( iClass != TF_CLASS_UNDEFINED )
						{
							sideInfoKey = szClassImage;
							sideInfoText = pszClassName;
						}
					}
				}

				if ( bSwapInfo )
				{
					if ( Q_strcmp( sideInfoKey, "" ) )
					{
						discordPresence.largeImageKey = sideInfoKey;
						discordPresence.largeImageText = sideInfoText;
						// ----------------
						discordPresence.smallImageKey = mainInfoKey;
						discordPresence.smallImageText = mainInfoText;
					}
					else
					{
						discordPresence.largeImageKey = mainInfoKey;
						discordPresence.largeImageText = mainInfoText;
					}
				}
				else
				{
					if ( Q_strcmp( mainInfoKey, "" ) )
					{
						discordPresence.largeImageKey = mainInfoKey;
						discordPresence.largeImageText = mainInfoText;
						// ----------------
						discordPresence.smallImageKey = sideInfoKey;
						discordPresence.smallImageText = sideInfoText;
					}
					else
					{
						discordPresence.largeImageKey = sideInfoKey;
						discordPresence.largeImageText = sideInfoText;
					}
				}
			
				if ( bShowLocation )
				{
					// Assume if they have any string here that it's a valid steamid
					if ( m_szSteamID[0] != '\0')
					{
						if
						(
							  m_szHostName[0] == '\0'                  // blank hostname
							|| tf2c_discord_show_location.GetInt() < 2 // client doesnt want to show location
							|| !m_bIsSaneServerToConnectTo             // local/lan/invalid ip/etc
						)
						{
							// Due to some strange issue with FindAsUTF8(), I have to go with this two-liner.
							g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->FindAsUTF8( "#Discord_InServer" ), 0 );
							g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

							discordPresence.state = szLocalized;
						}
						else
						{
							static wchar_t wszHostName[512] = {};
							mbstowcs( wszHostName, m_szHostName, sizeof(m_szHostName) );

							g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( "#Discord_InServer_Info" ), 1, wszHostName );
							g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

							discordPresence.state = szLocalized;
							discordPresence.partyId = m_szSteamID;
							discordPresence.partySize = m_iNumPlayers;
							discordPresence.partyMax = gpGlobals->maxClients;
							discordPresence.joinSecret = m_szConnectionString;
						}
					}
					// No steamid? Wacky!
					else
					{
						g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->FindAsUTF8( "#Discord_InServer_NoSteam" ), 0 );
						g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

						discordPresence.state = szLocalized;

						discordPresence.largeImageText = g_pVGuiLocalize->FindAsUTF8( "#Discord_InServer_NoSteam_ToolTip" );
						discordPresence.largeImageKey = "no_steam";
					}
				}
			}
			else if ( bShowLocation )
			{
				if ( engine->IsDrawingLoadingImage() )
				{
					g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->FindAsUTF8( "#Discord_IsLoading" ), 0 );
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

					discordPresence.state = szLocalized;

					discordPresence.largeImageText = g_pVGuiLocalize->FindAsUTF8( "#Discord_IsLoading_ToolTip" );
					discordPresence.largeImageKey = "is_loading";
				}
				else
				{
					g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->FindAsUTF8( "#Discord_InMenu" ), 0 );
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalized, szLocalized, sizeof( szLocalized ) );

					discordPresence.state = szLocalized;

					discordPresence.largeImageText = g_pVGuiLocalize->FindAsUTF8( "#Discord_InMenu_ToolTip" );
					discordPresence.largeImageKey = "in_menu";
				}
			}
		}

		Discord_UpdatePresence( &discordPresence );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFDiscordRPC::CountPlayers( void )
{
	// Count players.
	int iNumPlayers = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		player_info_t pi;
		if ( engine->GetPlayerInfo( i, &pi ) )
		{
			iNumPlayers++;
		}
	}

	if ( iNumPlayers != m_iNumPlayers )
	{
		m_iNumPlayers = iNumPlayers;
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFDiscordRPC::GetGameTypeName( void )
{
	// We cannot use localized names since we want to always show game mode name in English.
	if ( TFGameRules()->IsFourTeamGame() )
	{
		switch ( TFGameRules()->GetGameType() )
		{
			case TF_GAMETYPE_CTF:
				if ( TFGameRules()->IsInSpecialDeliveryMode() )
					return "Special Delivery (Four Team)";

				return "Capture the Flag (Four Team)";

			case TF_GAMETYPE_CP:
				if ( TFGameRules()->IsInKothMode() )
					return "King of the Hill (Four Team)";
		
				if ( TFGameRules()->IsInDominationMode() )
					return "Domination (Four Team)";

				return "Control Points (Four Team)";

			case TF_GAMETYPE_ESCORT:
				if ( TFGameRules()->HasMultipleTrains() )
					return "Payload Race (Four Team)";

				return "Payload (Four Team)";

			case TF_GAMETYPE_ARENA:
				return "Arena (Four Team)";

			case TF_GAMETYPE_VIP:
				return "VIP (Four Team)";

			default:
				return NULL;
		}
	}
	else
	{
		switch ( TFGameRules()->GetGameType() )
		{
			case TF_GAMETYPE_CTF:
				if ( TFGameRules()->IsInSpecialDeliveryMode() )
					return "Special Delivery";

				return "Capture the Flag";

			case TF_GAMETYPE_CP:
				if ( TFGameRules()->IsInKothMode() )
					return "King of the Hill";
		
				if ( TFGameRules()->IsInDominationMode() )
					return "Domination";

				return "Control Points";

			case TF_GAMETYPE_ESCORT:
				if ( TFGameRules()->HasMultipleTrains() )
					return "Payload Race";

				return "Payload";

			case TF_GAMETYPE_ARENA:
				return "Arena";

			case TF_GAMETYPE_VIP:
				return "VIP";

			default:
				return NULL;
		}
	}
}
