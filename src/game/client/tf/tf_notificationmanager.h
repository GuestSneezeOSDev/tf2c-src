#ifndef TF_NOTIFICATIONMANAGER_H
#define TF_NOTIFICATIONMANAGER_H

#ifdef _WIN32
#pragma once
#endif


#include "tf_shareddefs.h"
#include "igamesystem.h"
#include <steam/steam_api.h>
#include <ctime>

#define TF_NOTIFICATION_TITLE_SIZE 64
#define TF_NOTIFICATION_MESSAGE_SIZE 1024

class CTFNotificationManager;

struct MessageNotification
{
	MessageNotification();
	MessageNotification( const char *Title, const char *Message, time_t timeVal );
	MessageNotification( const wchar_t *Title, const wchar_t *Message, time_t timeVal );

	void SetTimeStamp( time_t timeVal );

	time_t timeStamp;
	wchar_t wszTitle[TF_NOTIFICATION_TITLE_SIZE];
	wchar_t wszDate[64];
	wchar_t wszMessage[TF_NOTIFICATION_MESSAGE_SIZE];
	bool bUnread;
	bool bLocal;
};

#include <sdkCURL.h>
class CTFNotificationManager : public CAutoGameSystemPerFrame, public ISteamMatchmakingServerListResponse
{
public:
	CTFNotificationManager();
	~CTFNotificationManager();

	enum RequestType
	{
		REQUEST_IDLE = -1,
		REQUEST_VERSION = 0,
		REQUEST_MESSAGE,
		REQUEST_SERVERLIST,
		REQUEST_EXTERNALIP,

		REQUEST_COUNT
	};

	// Methods of IGameSystem
	virtual bool Init();
	virtual char const *Name() { return "CTFNotificationManager"; }
	// Gets called each frame
	virtual void Update( float frametime );

	void CheckVersionAndMessages( void );

	virtual void AddRequest( RequestType type );

    void curlRequestDone(const curlResponse* response);


	virtual void SendNotification( MessageNotification &pMessage, const char *pszSoundscript = "#ui/notification_alert.wav" );
	virtual MessageNotification *GetNotification( int iIndex ) { return &m_Notifications[iIndex]; };
	virtual int GetNotificationsCount() { return m_Notifications.Count(); };
	virtual int GetUnreadNotificationsCount();
	virtual void RemoveNotification( int iIndex );
	virtual bool IsOutdated() { return m_bOutdated; };
	const char *GetVersionName( void ) { return m_szVersionName; }
	time_t GetVersionTimeStamp( void ) { return m_VersionTime; }
	void ParseVersionFile( void );

	uint32 GetServerFilters( MatchMakingKeyValuePair_t **pFilters );

	// Methods of ISteamMatchmakingServerListResponse
	// Server has responded ok with updated data
	virtual void ServerResponded( HServerListRequest hRequest, int iServer );
	// Server has failed to respond
	virtual void ServerFailedToRespond( HServerListRequest hRequest, int iServer );
	// A list refresh you had initiated is now 100% completed
	virtual void RefreshComplete( HServerListRequest hRequest, EMatchMakingServerResponse response );

	virtual bool GatheringServerList() { return m_bGatheringServerList; }
	void UpdateServerlistInfo();
	gameserveritem_t *GetServerInfo( int index );
	bool IsOfficialServer( int index );

	virtual bool GatheringExternalIP() { return m_bGatheringExternalIP; };
	virtual bool HasExternalIP();
	virtual char *GetExternalIP() { return m_szExternalIP; };
	virtual void ResetExternalIP( void ) { m_bHasExternalIP = false; m_flLastIPCheck = gpGlobals->curtime; };

private:
	static const char	*m_aRequestURLs[REQUEST_COUNT];

	CUtlVector<MessageNotification>	m_Notifications;

	bool				m_bOutdated             = {};
	bool				m_bPlayedSound          = {};

	float				m_flLastCheck           = {};
	float				m_flUpdateLastCheck     = {};

	void				OnMessageCheckCompleted( const char *pszPage, const char *pszSource );
	void				OnVersionCheckCompleted( const char *pszPage, const char *pszSource );
	void				OnServerlistCheckCompleted( const char *pszPage, const char *pszSource );
	void				OnExternalIPCheckCompleted( const char *pszPage, const char *pszSource );

    float				m_flLastIPCheck         = {};
    bool				m_bGatheringExternalIP  = {};
    bool				m_bHasExternalIP        = {};
    char				m_szExternalIP[64]      = {};

	CCallResult<CTFNotificationManager, HTTPRequestCompleted_t> m_CallResults[REQUEST_COUNT];

	void				OnHTTPRequestCompleted( HTTPRequestCompleted_t *CallResult, bool iofailure );

	bool				m_bGatheringServerList  = {};
	HServerListRequest	m_hServersRequest       = {};
	CUtlVector<gameserveritem_t>			m_OfficialServers;
	CUtlMap<int, gameserveritem_t>			m_Servers;
	CUtlVector<MatchMakingKeyValuePair_t>	m_ServerFilters;

	time_t				m_VersionTime       = {};
    char				m_szVersionName[32] = {};
};

CTFNotificationManager *GetNotificationManager();

#endif // TF_NOTIFICATIONMANAGER_H
