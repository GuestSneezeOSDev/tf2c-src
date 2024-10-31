//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Jumppad
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_JUMPPAD_H
#define TF_OBJ_JUMPPAD_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"

class CTFPlayer;

// ------------------------------------------------------------------------ //
// Base Teleporter object
// ------------------------------------------------------------------------ //
class CObjectJumppad : public CBaseObject
{
	DECLARE_CLASS(CObjectJumppad, CBaseObject);

public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CObjectJumppad();
	~CObjectJumppad();

	virtual void	Spawn();
	virtual void	FirstSpawn();
	virtual void	Precache();
	virtual bool	StartBuilding( CBaseEntity *pBuilder );
	virtual void	OnGoActive( void );
	virtual int		DrawDebugTextOverlays(void) ;
	virtual void	SetModel( const char *pModel );

	virtual void	FinishedBuilding( void );

	void SetState( int state );
	virtual void	DeterminePlaybackRate( void );

	void JumppadThink( void );
	
	void CheckAndLaunch(void);
	bool CheckAndLaunchPlayer(CTFPlayer *pPlayer);
	void LaunchPlayer( CTFPlayer *pPlayer );

	bool IsReady( void );

	int GetState( void ) { return m_iState; }	// state of the object ( building, charging, ready etc )

	virtual int GetBaseHealth( void ) const;
	virtual const char *GetPlacementModel( void ) const;

	virtual void	MakeCarriedObject( CTFPlayer *pPlayer );

	virtual bool	IsUnderwater();

	void GetJumppadBounds( Vector &vecPos, Vector &vecMins, Vector &vecMaxs );
protected:
	CNetworkVar( int, m_iState );
	CNetworkVar( int, m_iTimesUsed );

	float m_flLastStateChangeTime;

	float m_flMyNextThink;

	int m_iBlurBodygroup;
private:
	// Only used by hammer placed entities
	int m_iJumppadType;
};

#endif // TF_OBJ_JUMPPAD_H
