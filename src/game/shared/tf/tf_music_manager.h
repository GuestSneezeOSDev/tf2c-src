//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef TF_MUSIC_MANAGER_H
#define TF_MUSIC_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"

enum
{
	TF_MUSIC_INVALID = -1,
	TF_MUSIC_ROUND_START,
	TF_MUSIC_SETUP,

	TF_MUSIC_COUNT
};

class CTFMusicManager : public CAutoGameSystem
{
public:
	CTFMusicManager( const char *pszName );

	virtual bool Init( void );
	virtual void LevelInitPreEntity( void );

	const char *GetMusicForMessage( int iMessage );
	
#ifdef GAME_DLL
	void PlayMusic( IRecipientFilter &filter, int iMessage );
	void PlayMusic( int iMessage );
	void PlayMusic( CBasePlayer *pPlayer, int iMessage );
	void PlayMusic( int iTeam, int iMessage );
#else
	void PlayMusic( int iMessage );
#endif

	enum
	{
		TF_MUSICTHEME_DEFAULT = 0,
		TF_MUSICTHEME_COUNT,
	};

	static const char *m_aMusicManagerSounds[TF_MUSICTHEME_COUNT][TF_MUSIC_COUNT];
};

extern CTFMusicManager g_TFMusicManager;

#endif // TF_MUSIC_MANAGER_H
