//====== Copyright © 1996-2016, Valve Corporation, All rights reserved. =======
//
// Purpose: Jungle Inferno Crocodiles
//
//=============================================================================

#ifndef FUNC_CROC_H
#define FUNC_CROC_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"
#include "tf_player.h"

//=============================================================================
//
// Func Crocodile
//
class CFuncCroc : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFuncCroc, CBaseTrigger );
	DECLARE_DATADESC();

	CFuncCroc();

	void	Spawn( void );
	void	Precache( void );

	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	virtual void	StartTouch( CBaseEntity *pOther );

	void			FireOutputs( CTFPlayer *pActivator );

private:
	COutputEvent	m_OnEat;
	COutputEvent	m_OnEatBlue;
	COutputEvent	m_OnEatRed;
	COutputEvent	m_OnEatGreen;
	COutputEvent	m_OnEatYellow;

};

//=============================================================================
//
// Entity Crocodile
//
class CEntityCroc : public CBaseAnimating
{
public:
	DECLARE_CLASS( CEntityCroc, CBaseAnimating );

	void	InitCroc( void );
	void	Think ( void );
	void	CrocAttack ( void );

	DECLARE_DATADESC();
};

#endif // FUNC_CROC_H



