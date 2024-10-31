//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef TF_POWERUP_H
#define TF_POWERUP_H

#ifdef _WIN32
#pragma once
#endif

#include "items.h"

enum powerupsize_t
{
	POWERUP_SMALL,
	POWERUP_MEDIUM,
	POWERUP_FULL,

	POWERUP_SIZES,
};

extern float PackRatios[POWERUP_SIZES];

//=============================================================================
//
// CTF Powerup class.
//

class CTFPowerup : public CItem
{
public:
	DECLARE_CLASS( CTFPowerup, CItem );
	DECLARE_DATADESC();

	CTFPowerup();

	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual CBaseEntity*	Respawn( void );
	virtual void	Materialize( void );
	virtual void	HideOnPickedUp( void );
	virtual void	UnhideOnRespawn( void );
	virtual void	OnIncomingSpawn( void ) {}
	virtual bool	ValidTouch( CBasePlayer *pPlayer );
	virtual bool	MyTouch( CBasePlayer *pPlayer );
	virtual bool	ItemCanBeTouchedByPlayer( CBasePlayer *pPlayer );

	void			SetRespawnTime( float flDelay );
	void			RespawnThink( void );
	bool			IsDisabled( void );
	void			SetDisabled( bool bDisabled );
	void			FireOutputsOnPickup( CBasePlayer *pPlayer );

	void			DropSingleInstance( const Vector &vecVelocity, CBaseCombatCharacter *pOwner, float flOwnerPickupDelay, float flRestTime, float flRemoveTime = 30.0f );

	// Input handlers
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputEnableWithEffect( inputdata_t &inputdata );
	void			InputDisableWithEffect( inputdata_t &inputdata );
	void			InputToggle( inputdata_t &inputdata );
	void			InputRespawnNow( inputdata_t &inputdata );

	virtual powerupsize_t	GetPowerupSize( void ) { return POWERUP_FULL; }

	virtual const char *GetPowerupModel( void );
	virtual const char *GetDefaultPowerupModel( void ) = 0;

	bool			IsRespawning() const { return m_bRespawning; }

protected:
	bool m_bDropped;

private:
	bool m_bDisabled;
	bool m_bRespawning;
	float m_flRespawnTime;

	float m_flOwnerPickupEnableTime;
	bool m_bFire15SecRemain;

	string_t m_iszModel;

	COutputEvent m_outputOnRespawn;
	COutputEvent m_outputOn15SecBeforeRespawn;
	COutputEvent m_outputOnTeam1Touch;
	COutputEvent m_outputOnTeam2Touch;
	COutputEvent m_outputOnTeam3Touch;
	COutputEvent m_outputOnTeam4Touch;
};

#endif // TF_POWERUP_H


