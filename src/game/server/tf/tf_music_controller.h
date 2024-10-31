//=============================================================================//
//
// Purpose:
//
//=============================================================================//
#ifndef TF_MUSIC_CONTROLLER_H
#define TF_MUSIC_CONTROLLER_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_music_controller_shared.h"

#define SF_MUSIC_STARTSILENT ( 1 << 0 )

class CTFMusicController : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFMusicController, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFMusicController();
	~CTFMusicController();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual int UpdateTransmitState( void );

	void InputSetTrack( inputdata_t &inputdata );
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

	void ParseMusicData( const char *pszFile );
	void PrecacheTrack( int iTrack );

private:
	CNetworkVar( int, m_iTrack );
	CNetworkVar( bool, m_bShouldPlay );

	CUtlMap<int, MusicData_t> m_Tracks;
};

#endif // TF_MUSIC_CONTROLLER_H
