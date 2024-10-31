//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#ifndef C_TF_MUSIC_CONTROLLER_H
#define C_TF_MUSIC_CONTROLLER_H

#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "soundenvelope.h"
#include "tf_music_controller_shared.h"

class C_TFMusicController : public C_BaseEntity, public CGameEventListener
{
public:
	DECLARE_CLASS( C_TFMusicController, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_TFMusicController();
	~C_TFMusicController();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual void UpdateOnRemove( void );
	virtual void ClientThink( void );

	virtual void FireGameEvent( IGameEvent *event );

	void ParseMusicData( const char *pszFile );
	void StartMusic( void );
	void StopMusic( bool bPlayEnding = false );
	void RestartMusic( void );
	void ToggleMusicEnabled( bool bEnable );
	bool IsPlayingMusic( void ) { return m_bPlaying; }
	bool IsPlayingEnding( void ) { return m_bPlayingEnding; }
	bool IsInWaiting( void ) { return m_bInWaiting; }
	bool CanPlayMusic( void );
	bool ShouldPlayWaitingMusic( void );

private:
	int m_iTrack;
	bool m_bShouldPlay;

	CUtlMap<int, MusicData_t> m_Tracks;

	CSoundPatch *m_pSound;

	int m_iPlayingTrack;
	bool m_bPlaying;
	bool m_bPlayingEnding;
	bool m_bInWaiting;

	bool m_bOldShouldPlay;
};

extern C_TFMusicController *g_pMusicController;

#endif // C_TF_MUSIC_CONTROLLER_H
