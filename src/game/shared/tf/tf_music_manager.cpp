//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tf_music_manager.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#include <engine/IEngineSound.h>
#include "hud_macros.h"

extern ConVar tf2c_music_cues;
#endif

const char *CTFMusicManager::m_aMusicManagerSounds[TF_MUSICTHEME_COUNT][TF_MUSIC_COUNT] =
{
	// Default
	{
		"music.round_start",
		"music.setup"
	},
};

CTFMusicManager g_TFMusicManager( "Music Manager" );

#ifdef CLIENT_DLL
static void __MsgFunc_PlayMusic( bf_read &msg )
{
	int iMessage = msg.ReadShort();
	g_TFMusicManager.PlayMusic( iMessage );
}
#endif

CTFMusicManager::CTFMusicManager( const char *pszName ) : CAutoGameSystem( pszName )
{
}


bool CTFMusicManager::Init( void )
{
#ifdef CLIENT_DLL
	HOOK_MESSAGE( PlayMusic );
#endif

	return true;
}


void CTFMusicManager::LevelInitPreEntity( void )
{
#ifdef GAME_DLL
	for ( int i = 0; i < TF_MUSICTHEME_COUNT; i++ )
	{
		for ( int j = 0; j < TF_MUSIC_COUNT; j++ )
		{
			const char *pszSound = m_aMusicManagerSounds[i][j];
			if ( !pszSound || !pszSound[0] )
				continue;

			CBaseEntity::PrecacheScriptSound( pszSound );
		}
	}
#endif
}


const char *CTFMusicManager::GetMusicForMessage( int iMessage )
{
	int iType = TF_MUSICTHEME_DEFAULT;

	return m_aMusicManagerSounds[iType][iMessage];
}

#ifdef GAME_DLL


void CTFMusicManager::PlayMusic( IRecipientFilter &filter, int iMessage )
{
	UserMessageBegin( filter, "AnnouncerSpeak" );
	WRITE_SHORT( iMessage );
	MessageEnd();
}


void CTFMusicManager::PlayMusic( int iMessage )
{
	CReliableBroadcastRecipientFilter filter;
	PlayMusic( filter, iMessage );
}


void CTFMusicManager::PlayMusic( CBasePlayer *pPlayer, int iMessage )
{
	CSingleUserRecipientFilter filter( pPlayer );
	filter.MakeReliable();
	PlayMusic( filter, iMessage );
}


void CTFMusicManager::PlayMusic( int iTeam, int iMessage )
{
	CTeamRecipientFilter filter( iTeam, true );
	PlayMusic( filter, iMessage );
}

#else


void CTFMusicManager::PlayMusic( int iMessage )
{
	if ( tf2c_music_cues.GetBool() )
	{
		const char *pszSound = GetMusicForMessage( iMessage );
		if ( !pszSound || !pszSound[0] )
			return;

		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSound );
	}
}

#endif
