//=============================================================================
//
// Purpose:
//
//=============================================================================
#ifndef TF_TRIGGERS_H
#define TF_TRIGGERS_H

#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

class CTriggerAddTFPlayerCondition : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerAddTFPlayerCondition, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

private:
	ETFCond m_nCond;
	float m_flDuration;
};

class CTriggerRemoveTFPlayerCondition : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerRemoveTFPlayerCondition, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *pOther );

private:
	ETFCond m_nCond;
};

class CFuncJumpPad : public CBaseTrigger
{
public:
	DECLARE_CLASS( CFuncJumpPad, CBaseTrigger );
	DECLARE_DATADESC();

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void StartTouch( CBaseEntity *pOther );

private:
	QAngle m_angPush;
	Vector m_vecPushDir;
	float m_flPushForce;
	string_t m_iszJumpSound;

	COutputEvent m_OnJump;
};

//-----------------------------------------------------------------------------
// Purpose: Puts anything that touches the trigger into loser state.
//-----------------------------------------------------------------------------
class CTriggerStun : public CBaseTrigger
{
public:
	CTriggerStun()
	{
	}

	DECLARE_CLASS( CTriggerStun, CBaseTrigger );

	
	void Spawn( void );
	void StunThink( void );
	void Touch( CBaseEntity *pOther );
	void EndTouch( CBaseEntity *pOther );
	bool StunEntity( CBaseEntity *pOther );
	int StunAllTouchers( float dt );

	DECLARE_DATADESC();

	float	m_flTriggerDelay;
	float	m_flStunDuration;
	float	m_flMoveSpeedReduction;
	int		m_iStunType;
	bool	m_bStunEffects;

	// Outputs
	COutputEvent m_OnStunPlayer;

	float	m_flLastStunTime;
	CUtlVector<EHANDLE>	m_stunEntities;
	
};

#endif // TF_TRIGGERS_H
