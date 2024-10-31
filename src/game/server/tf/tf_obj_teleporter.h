//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Teleporter
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_TELEPORTER_H
#define TF_OBJ_TELEPORTER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"

class CTFPlayer;

// ------------------------------------------------------------------------ //
// Base Teleporter object
// ------------------------------------------------------------------------ //
class CObjectTeleporter : public CBaseObject
{
	DECLARE_CLASS( CObjectTeleporter, CBaseObject );

public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CObjectTeleporter();
	~CObjectTeleporter();

	virtual void	Spawn();
	virtual void	FirstSpawn();
	virtual void	Precache();
	virtual bool	StartBuilding( CBaseEntity *pBuilder );
	virtual void	OnGoActive( void );
	virtual int		DrawDebugTextOverlays(void) ;
	virtual bool	IsPlacementPosValid( void );
	virtual void	SetModel( const char *pModel );

	virtual void	StartUpgrading( void );
	virtual void	FinishUpgrading( void );
	virtual bool	IsUpgrading( void ) const;

	virtual void	FinishedBuilding( void );

	void SetState( int state );
	virtual void	DeterminePlaybackRate( void );

	void TeleporterThink( void );
	void TeleporterTouch( CBaseEntity *pOther );
	void StartTouch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );

	void TeleporterReceive( CTFPlayer *pPlayer, float flDelay );
	void TeleporterSend( CTFPlayer *pPlayer );

	void FactoryReset( void );

	void CopyUpgradeStateToMatch( CObjectTeleporter *pMatch, bool bCopyFrom );

	CObjectTeleporter *GetMatchingTeleporter( void );
	CObjectTeleporter *FindMatch( void );	// Find the teleport partner to this object
	void FindMatchAndCopyStaticStats( void );

	virtual bool InputWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos );

	virtual int OnTakeDamage( const CTakeDamageInfo& info ) OVERRIDE;

	virtual bool Command_Repair( CTFPlayer *pActivator, float flRepairMod );

	virtual bool CheckUpgradeOnHit( CTFPlayer *pPlayer );

	virtual void InitializeMapPlacedObject( void );

	bool IsReady( void );
	bool IsMatchingTeleporterReady( void );

	SERVERONLY_EXPORT bool PlayerCanBeTeleported(CTFPlayer * pSender);

	bool IsSendingPlayer( CTFPlayer *pSender );

	int GetState( void ) { return m_iState; }	// state of the object ( building, charging, ready etc )

	void SetTeleportingPlayer( CTFPlayer *pPlayer )
	{
		m_hTeleportingPlayer = pPlayer;
	}

	virtual int GetBaseHealth( void ) const;
	virtual const char *GetPlacementModel( void ) const;

	virtual void	MakeCarriedObject( CTFPlayer *pPlayer );

	void GetTeleportBounds( Vector &vecPos, Vector &vecMins, Vector &vecMaxs );

	virtual int CalculateTeamBalanceScore(void);

	void AddPlayerToQueue( CTFPlayer *pPlayer, bool bAddToHead = false );
	void RemovePlayerFromQueue( CTFPlayer *pPlayer );
	bool IsFirstInQueue( CTFPlayer *pPlayer );
	void CalcWaitTimeForPlayer( CTFPlayer *pPlayer, bool bAlreadyInQueue );
	float GetLastCalculatedWaitTime( void ) { return m_flPotentialWaitTime; }

	struct TeleporterQueueEntry_t
	{
		CTFPlayer *pPlayer;
		float flJoinTime;
	};

protected:
	CNetworkVar( int, m_iState );
	CNetworkVar( float, m_flRechargeTime );
	CNetworkVar( int, m_iTimesUsed );
	CNetworkVar( float, m_flYawToExit );

	CHandle<CObjectTeleporter> m_hMatchingTeleporter;

	bool m_bMatchingTeleporterDown;

	float m_flLastStateChangeTime;

	float m_flMyNextThink;	// replace me

	CHandle<CTFPlayer> m_hTeleportingPlayer;

	float m_flNextEnemyTouchHint;

	// Direction Arrow, shows roughly what direction the exit is from the entrance
	void ShowDirectionArrow( bool bShow );

	bool m_bShowDirectionArrow;
	int m_iDirectionBodygroup;
	int m_iBlurBodygroup;

	CUtlVector<TeleporterQueueEntry_t> m_PlayerQueue;
	float m_flPotentialWaitTime;

private:
	// Only used by hammer placed entities
	int m_iTeleporterType;
	string_t m_szMatchingTeleporterName;
};

#endif // TF_OBJ_TELEPORTER_H
