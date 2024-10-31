#include "cbase.h"
#include "tf_notificationmanager.h"
#include "tf_mainmenu.h"
#include "filesystem.h"
#include "tf_gamerules.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <tier1/fmtstr.h>
#include <tier1/netadr.h>

#include <misc_helpers.h>
#include <sdkCURL.h>


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


const char *CTFNotificationManager::m_aRequestURLs[REQUEST_COUNT] =
{
	"https://api.tf2classic.com/version.vdf",
	"https://api.tf2classic.com/notifications.vdf",
	"https://api.tf2classic.com/serverlist/official.vdf",
	"https://api.ipify.org/" // TODO: Replace this link with our own.
};

MessageNotification::MessageNotification()
{
	timeStamp = 0;
	wszTitle[0] = '\0';
	wszMessage[0] = '\0';
	wszDate[0] = '\0';
	bUnread = true;
	bLocal = false;
}

MessageNotification::MessageNotification( const char *Title, const char *Message, time_t timeVal )
{
	g_pVGuiLocalize->ConvertANSIToUnicode( g_pVGuiLocalize->FindAsUTF8( Title ), wszTitle, sizeof( wszTitle ) );
	g_pVGuiLocalize->ConvertANSIToUnicode( g_pVGuiLocalize->FindAsUTF8( Message ), wszMessage, sizeof( wszMessage ) );
	bUnread = true;
	bLocal = false;
	SetTimeStamp( timeVal );
}

MessageNotification::MessageNotification( const wchar_t *Title, const wchar_t *Message, time_t timeVal )
{
	V_wcscpy_safe( wszTitle, Title );
	V_wcscpy_safe( wszMessage, Message );
	bUnread = true;
	bLocal = false;
	SetTimeStamp( timeVal );
}


void MessageNotification::SetTimeStamp( time_t timeVal )
{
	timeStamp = timeVal;
	
	char szDate[64];
	BGetLocalFormattedDate( timeStamp, szDate, sizeof( szDate ) );

	g_pVGuiLocalize->ConvertANSIToUnicode( szDate, wszDate, sizeof( wszDate ) );
}

static CTFNotificationManager g_TFNotificationManager;
CTFNotificationManager *GetNotificationManager()
{
	return &g_TFNotificationManager;
}

CON_COMMAND_F( tf2c_checkmessages, "Check for any TF2C related updates or messages", FCVAR_NONE )
{
	GetNotificationManager()->CheckVersionAndMessages();
}

CON_COMMAND_F( tf2c_updateserverlist, "Manually update the server list on the main menu", FCVAR_NONE )
{
	GetNotificationManager()->UpdateServerlistInfo();
}

ConVar tf2c_messages_updatefrequency( "tf2c_messages_updatefrequency", "900", FCVAR_DEVELOPMENTONLY, "Messages check frequency (seconds)" );
ConVar tf2c_serverlist_updatefrequency( "tf2c_serverlist_updatefrequency", "10", FCVAR_ARCHIVE, "Updatelist update frequency (seconds)" );
ConVar tf2c_latest_notification( "tf2c_latest_notification", "0", FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CTFNotificationManager::CTFNotificationManager() : CAutoGameSystemPerFrame( "CTFNotificationManager" )
{
	SetDefLessFunc( m_Servers );
	m_flLastCheck       = -1.0f;
	m_flUpdateLastCheck = -1.0f;
	m_bOutdated         = false;
	m_bPlayedSound      = false;
	m_bHasExternalIP    = false;

    m_hServersRequest   = {};
}

CTFNotificationManager::~CTFNotificationManager()
{
	if ( steamapicontext->SteamMatchmakingServers() )
	{
		steamapicontext->SteamMatchmakingServers()->ReleaseRequest( m_hServersRequest );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initializer
//-----------------------------------------------------------------------------
bool CTFNotificationManager::Init()
{
#ifdef STEAM_GROUP_CHECKPOINT
	// DRM check.
	QAngle angles;
	engine->GetViewAngles( angles );
	if ( angles == vec3_angle && angles.z != ( 90 * 2 ) )
	{
		//Error( "This account is not authorized to play TF2C Beta." );
		return false;
	}

#elif NO
	if ( steamapicontext && !steamapicontext->SteamApps()->BIsAppInstalled( 440 ) )
	{
		Error( "Team Fortress 2 is not currently installed.\nPlease verify that it is before trying again." );
		return false;
	}
#endif


	ParseVersionFile();

    // we can't inline init this since the steamapi header uses strncpy and i don't want to remove
    // the strncpy footgun prevention in client_base.vpc
    MatchMakingKeyValuePair_t filter = {};
	V_snprintf( filter.m_szKey,     sizeof(filter.m_szKey),     "%s", "gamedir" );
	V_snprintf( filter.m_szValue,   sizeof(filter.m_szValue),   "%s", "tf2classic" );
	m_ServerFilters.AddToTail( filter );


	// Do it only once.
	AddRequest( REQUEST_SERVERLIST );

    return true;
}


void CTFNotificationManager::Update( float frametime )
{
    if (!guiroot)
    {
        return;
    }

	float flCurTime = engine->Time();
	if ( m_flLastCheck < 0.0f || ( flCurTime - m_flLastCheck ) > tf2c_messages_updatefrequency.GetFloat() )
	{
		m_flLastCheck = flCurTime;
		CheckVersionAndMessages();
	}

    // if we can see the cursor we're doing something that isn't shooting our gun,
    // so we can afford to hitch a tiny bit
    bool cursorVisible = vgui::surface()->IsCursorVisible();
    if 
    (
        cursorVisible
        &&
        (
            m_flUpdateLastCheck < 0.0f
            ||
            (flCurTime - m_flUpdateLastCheck) > tf2c_serverlist_updatefrequency.GetFloat()
        )
    )
    {
        DevMsg("Updating server list...\n");
		m_bGatheringServerList = true;
		m_flUpdateLastCheck = flCurTime;
		UpdateServerlistInfo();
	}
}


void CTFNotificationManager::CheckVersionAndMessages( void )
{
	AddRequest( REQUEST_VERSION );
	AddRequest( REQUEST_MESSAGE );
	m_bPlayedSound = false;
}

void curlRequestDone_THUNK(const curlResponse* response)
{
    g_TFNotificationManager.curlRequestDone(response);
}

void CTFNotificationManager::AddRequest( RequestType type )
{
	switch ( type )
	{
		case REQUEST_SERVERLIST:
			m_bGatheringServerList = true;
			break;
		case REQUEST_EXTERNALIP:
			m_bGatheringExternalIP = true;
			break;
	}


    std::string url = m_aRequestURLs[type];
    
    g_sdkCURL->CURLGet(url, curlRequestDone_THUNK);
}

void CTFNotificationManager::curlRequestDone(const curlResponse* response)
{
    if (response->respCode != 200)
    {
        return;
    }
    auto& url    = response->originalURL;
    auto& body   = response->body;

    if ( url == m_aRequestURLs[REQUEST_VERSION] )
    {
        OnVersionCheckCompleted(body.c_str(), url.c_str());
    }
    else if ( url == m_aRequestURLs[REQUEST_MESSAGE] )
    {
        OnMessageCheckCompleted(body.c_str(), url.c_str());
    }
    else if ( url == m_aRequestURLs[REQUEST_SERVERLIST] )
    {
        OnServerlistCheckCompleted(body.c_str(), url.c_str());
    }
    else if ( url == m_aRequestURLs[REQUEST_EXTERNALIP] )
    {
        OnExternalIPCheckCompleted(body.c_str(), url.c_str());
    }
    else
    {
        Assert(false);
    }
}


void CTFNotificationManager::OnVersionCheckCompleted( const char *pszPage, const char *pszSource )
{
	// Check header to make sure we loaded the correct page.
	static int iHeaderLen = V_strlen( "\"tf2classic_version\"" );
	if ( V_strncmp( pszPage, "\"tf2classic_version\"", iHeaderLen ) != 0 )
    {
        return;
    }

	KeyValues *pVersionKeys = new KeyValues( "tf2classic_version" );
	pVersionKeys->LoadFromBuffer( pszSource, pszPage );

	// Get the timestamp of the latest version.
	time_t timeLatest = V_atoi64( pVersionKeys->GetString( "time", "0" ) );
	if ( m_VersionTime < timeLatest )
	{
		if ( !m_bOutdated )
		{
			m_bOutdated = true;

			MessageNotification Notification;
			const wchar_t *pszLocalizedTitle = g_pVGuiLocalize->Find( "#TF_GameOutdatedTitle" );
			if ( pszLocalizedTitle )
			{
				V_wcscpy_safe( Notification.wszTitle, pszLocalizedTitle );
			}
			else
			{
				V_wcscpy_safe( Notification.wszTitle, L"#TF_GameOutdatedTitle" );
			}

			wchar_t wszVersion[16];
			g_pVGuiLocalize->ConvertANSIToUnicode( pVersionKeys->GetString( "name", "" ), wszVersion, sizeof( wszVersion ) );

			char szDate[64];
			wchar_t wszDate[64];
			BGetLocalFormattedDate( timeLatest, szDate, sizeof( szDate ) );
			g_pVGuiLocalize->ConvertANSIToUnicode( szDate, wszDate, sizeof( wszDate ) );

			g_pVGuiLocalize->ConstructString( Notification.wszMessage, sizeof( Notification.wszMessage ), g_pVGuiLocalize->Find( "#TF_GameOutdated" ), 2, wszVersion, wszDate );

			// Urgent - set time to now.
			Notification.SetTimeStamp( time( NULL ) );
			Notification.bLocal = true;

			SendNotification( Notification );
		}
	}
	else
	{
		m_bOutdated = false;
	}

	pVersionKeys->deleteThis();
}


void CTFNotificationManager::OnMessageCheckCompleted( const char *pszPage, const char *pszSource )
{
	// Check header to make sure we loaded the correct page.
	static int iHeaderLen = V_strlen( "\"tf2classic_messages\"" );
	if ( V_strncmp( pszPage, "\"tf2classic_messages\"", iHeaderLen ) != 0 )
    {
        return;
    }

	KeyValues *pNotifications = new KeyValues( "tf2classic_messages" );
	pNotifications->UsesEscapeSequences( true );
	pNotifications->LoadFromBuffer( pszSource, pszPage );

	for ( KeyValues *pData = pNotifications->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		// Get the timestamp of this notification.
		// ConVar does not support int64 so we have to work around it.
		time_t timePrevious = V_atoi64( tf2c_latest_notification.GetString() );
		time_t timeNew = V_atoi64( pData->GetName() );

		if ( timeNew <= timePrevious ) // Already viewed this one.
        {
            continue;
        }

		tf2c_latest_notification.SetValue( pData->GetName() );

		const char *pszTitle = pData->GetString( "title", "" );
		const char *pszMessage = pData->GetString( "message", "" );

		MessageNotification Notification( pszTitle, pszMessage, timeNew );
		SendNotification( Notification );
	}

	pNotifications->deleteThis();
}


void CTFNotificationManager::OnServerlistCheckCompleted( const char *pszPage, const char *pszSource )
{
	// Check header to make sure we loaded the correct page.
	static int iHeaderLen = V_strlen( "\"tf2classic_servers\"" );
    if (V_strncmp(pszPage, "\"tf2classic_servers\"", iHeaderLen) != 0)
    {
        return;
    }

	KeyValues *pServers = new KeyValues( "tf2classic_servers" );
	pServers->LoadFromBuffer( pszSource, pszPage );

	// Clear the list and re-fill it.
	m_OfficialServers.RemoveAll();

	for ( KeyValues *pData = pServers->GetFirstSubKey(); pData != NULL; pData = pData->GetNextKey() )
	{
		const char *pszAddress = pData->GetString( "full_address", "" );
        if (pszAddress[0] == '\0')
        {
            continue;
        }

		netadr_t netaddr;
		netaddr.SetFromString( pszAddress, true );
		if ( !netaddr.GetPort() )
		{
			// Use the default port since it was not entered.
			netaddr.SetPort( 27015 );
		}

		if ( !netaddr.IsValid() )
		{
			Warning( "Offical server list contains invalid address %s!\n", pszAddress );
			continue;
		}

		gameserveritem_t server;
		server.m_NetAdr.Init( netaddr.GetIPHostByteOrder(), netaddr.GetPort(), netaddr.GetPort() );
		DevMsg( "Added official server: %s\n", server.m_NetAdr.GetConnectionAddressString() );

		m_OfficialServers.AddToTail( server );
	}

	pServers->deleteThis();

	UpdateServerlistInfo();
}


void CTFNotificationManager::UpdateServerlistInfo()
{
	ISteamMatchmakingServers *pMatchmaking = steamapicontext->SteamMatchmakingServers();
    if (!pMatchmaking || pMatchmaking->IsRefreshing(m_hServersRequest))
    {
        return;
    }

	MatchMakingKeyValuePair_t *pFilters;
	uint32 nFilters = GetServerFilters( &pFilters );

	if (m_hServersRequest)
	{
		pMatchmaking->ReleaseRequest(m_hServersRequest);
		m_hServersRequest = nullptr;
	}
	m_hServersRequest = pMatchmaking->RequestInternetServerList( engine->GetAppID(), &pFilters, nFilters, this );
}


gameserveritem_t *CTFNotificationManager::GetServerInfo( int index )
{
	return &m_Servers[index];
};


bool CTFNotificationManager::IsOfficialServer( int index )
{
	for ( int i = 0, c = m_OfficialServers.Count(); i < c; i++ )
	{
		if
        (
            m_OfficialServers[i].m_NetAdr.GetIP() == m_Servers[index].m_NetAdr.GetIP()
            &&
            m_OfficialServers[i].m_NetAdr.GetConnectionPort() == m_Servers[index].m_NetAdr.GetConnectionPort()
        )
        {
            return true;
        }
	}

	return false;
};


void CTFNotificationManager::ServerResponded( HServerListRequest hRequest, int iServer )
{
	gameserveritem_t *pServerItem = steamapicontext->SteamMatchmakingServers()->GetServerDetails( hRequest, iServer );
	m_Servers.InsertOrReplace( iServer , *pServerItem );
}


// BUGBUG: What if the user loses internet connection? Wouldn't this just remove all the servers?
void CTFNotificationManager::ServerFailedToRespond( HServerListRequest hRequest, int iServer )
{
	gameserveritem_t *pServerItem = steamapicontext->SteamMatchmakingServers()->GetServerDetails( hRequest, iServer );
	int iServerIndex = m_Servers.Find( iServer );
	if ( iServerIndex != m_Servers.InvalidIndex() )
	{
		DevMsg( "%i SERVER: %s (%s), PING: %i, PLAYERS: %i/%i, MAP: %s - failed to respond, removing...\n", iServer, pServerItem->GetName(), pServerItem->m_NetAdr.GetQueryAddressString(),
		pServerItem->m_nPing, pServerItem->m_nPlayers, pServerItem->m_nMaxPlayers, pServerItem->m_szMap );
		m_Servers.Remove( iServerIndex );
	}
}



void CTFNotificationManager::RefreshComplete( HServerListRequest hRequest, EMatchMakingServerResponse response )
{
	if ( guiroot )
	{
		guiroot->SetServerlistSize( m_Servers.Count() );
		guiroot->OnServerInfoUpdate();
	}

	if ( response == eServerResponded )
	{
		m_bGatheringServerList = false;
	}
}


uint32 CTFNotificationManager::GetServerFilters( MatchMakingKeyValuePair_t **pFilters )
{
	*pFilters = m_ServerFilters.Base();
	return m_ServerFilters.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Sort notifications by timestamp (latest first)
//-----------------------------------------------------------------------------
int NotificationsSort( const MessageNotification *pItem1, const MessageNotification *pItem2 )
{
	return ( pItem2->timeStamp - pItem1->timeStamp );
}


void CTFNotificationManager::SendNotification( MessageNotification &pMessage, const char *pszSoundscript /*= "#ui/notification_alert.wav"*/ )
{
	m_Notifications.AddToTail( pMessage );
	m_Notifications.Sort( NotificationsSort );

	guiroot->OnNotificationUpdate();

	// Only play sound once per notification.
	if ( !m_bPlayedSound )
	{
		vgui::surface()->PlaySound( pszSoundscript );
		m_bPlayedSound = true;
	}
}


void CTFNotificationManager::RemoveNotification( int iIndex )
{
	m_Notifications.Remove( iIndex );
	m_Notifications.Sort( NotificationsSort );

	guiroot->OnNotificationUpdate();
}


int CTFNotificationManager::GetUnreadNotificationsCount()
{
	int iCount = 0;
	for ( int i = 0; i < m_Notifications.Count(); i++ )
	{
        if (m_Notifications[i].bUnread)
        {
            iCount++;
        }
	}
	return iCount;
}

#ifdef _DEBUG
CON_COMMAND_F(ParseVersionFile, "", FCVAR_NONE)
{
    g_TFNotificationManager.ParseVersionFile();
}
#endif

void CTFNotificationManager::ParseVersionFile( void )
{
    std::stringstream versionTxt = {};

    // we really gotta add stdlib wrappers to valve fs someday...
    // -sappho
	if ( filesystem->FileExists( "version.txt", "MOD" ) )
	{
        char* psFileBuffer = nullptr;
		FileHandle_t fh = filesystem->Open( "version.txt", "r", "MOD" );
		unsigned int iFileLen = filesystem->Size( fh );
        psFileBuffer = new char[iFileLen] {};
		filesystem->Read( psFileBuffer, iFileLen, fh );
		psFileBuffer[iFileLen - 1] = '\0'; // Gotta put a zero terminator at the end.
		filesystem->Close( fh );
        versionTxt << psFileBuffer;
        delete[] psFileBuffer;
	}
	else
	{
		Warning( "version.txt is missing!\n" );
		return;
	}
    std::vector<std::pair<std::string, std::string>> pairVec = {};

    // we don't need all these temp variables on the stack so im having them fall out of scope
    {
        std::string temp = {};
        std::vector<std::string> tempVec = {};
        // loop thru file line by line
        while (std::getline(versionTxt, temp))
        {
            const char* sepErr = "Malformed version.txt, too many seperators!";

            // split into vector of strings seperated by equals sign
            tempVec = UTIL_SplitSTDString(temp, "=");
            AssertMsg(tempVec.size() == 2, sepErr);
            if (tempVec.size() != 2)
            {
                Warning("%s\n", sepErr);
                return;
            }

            // push that into our actual pair
            pairVec.push_back( { tempVec.at(0), tempVec.at(1) } );
        }
    }

    AssertMsg(pairVec.size() == 2, "Too many lines in version.txt?!");

    for (auto& pair : pairVec)
    {
        auto& key = pair.first;
        auto& val = pair.second;
        if (key == "VersionName")
        {
            V_snprintf(m_szVersionName, sizeof(m_szVersionName), val.c_str());
        }
        else if (key == "VersionTime")
        {
            m_VersionTime = V_atoi64(val.c_str());
        }
        else
        {
            AssertMsg(0 == 1, "Malformed version.txt!");
            Warning("Malformed version.txt!\n");
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: Get our external IP.
//-----------------------------------------------------------------------------
bool CTFNotificationManager::HasExternalIP()
{
	if ( !m_bHasExternalIP )
	{
		if ( m_flLastIPCheck < gpGlobals->curtime )
		{
			AddRequest( REQUEST_EXTERNALIP );
			m_flLastIPCheck = gpGlobals->curtime + 25.0f;
		}

		return false;
	}

	return true;
}


void CTFNotificationManager::OnExternalIPCheckCompleted( const char *pszPage, const char *pszSource )
{
	m_bGatheringExternalIP = false;

	if ( pszPage[0] == '\0' )
    {
        return;
    }

    V_snprintf( m_szExternalIP, sizeof(m_szExternalIP), "%s", pszPage );
	m_bHasExternalIP = true;
}
