//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========
//
// Purpose: Generator objective in the Power Siege game mode.
//
//=============================================================================
#ifndef TF_GENERATOR_H
#define TF_GENERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_gamerules.h"
#ifdef GAME_DLL
#include "tf_fx.h"
#include "te_effect_dispatch.h"
#include "tf_team.h"
#endif

#ifdef TF_GENERATOR

#ifdef CLIENT_DLL
#define CTeamShield C_TeamShield
#endif

class CTeamShield : public CBaseAnimating, public TAutoList<CTeamShield>
{
public:
	DECLARE_CLASS( CTeamShield, CBaseAnimating );
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif
	DECLARE_NETWORKCLASS();
	CNetworkVar(float, m_flHealthPercentage);
#ifdef GAME_DLL
	CTeamShield();

	void Spawn( void );
	void Precache( void );
	void ChangeTeam( int iTeamNum );
	void AdjustDamage( CTakeDamageInfo& info );
	virtual int	OnTakeDamage( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );

	void Enable( void );
	void Disable( void );
	virtual int		DrawDebugTextOverlays(void);
	int UpdateTransmitState();
private:
	bool m_bEnabled;
#endif
};


#ifdef CLIENT_DLL
#define CTeamGenerator C_TeamGenerator
#endif

class CTeamGenerator : public CBaseAnimating, public TAutoList<CTeamGenerator>
{
public:
	DECLARE_CLASS( CTeamGenerator, CBaseAnimating );
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif
	DECLARE_NETWORKCLASS();
	CNetworkVar(float, m_flHealthPercentage);

#ifdef GAME_DLL
	CTeamGenerator();

	void Spawn( void );
	void Precache( void );
	void ChangeTeam( int iTeamNum );
	void AdjustDamage( CTakeDamageInfo& info );
	virtual int	OnTakeDamage( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	virtual int		DrawDebugTextOverlays(void);
	void InputShieldDisable( inputdata_t& inputData );
	void InputShieldEnable(inputdata_t& inputData);
	int UpdateTransmitState();
#endif
	EHANDLE GetShield() { return m_hShield; }
private:
	CNetworkVar(EHANDLE, m_hShield);
#ifdef GAME_DLL
	string_t	m_szModel;
	COutputEvent	m_OnDestroyed;
	COutputEvent	m_OnHealthBelow90Percent;
	COutputEvent	m_OnHealthBelow80Percent;
	COutputEvent	m_OnHealthBelow70Percent;
	COutputEvent	m_OnHealthBelow60Percent;
	COutputEvent	m_OnHealthBelow50Percent;
	COutputEvent	m_OnHealthBelow40Percent;
	COutputEvent	m_OnHealthBelow30Percent;
	COutputEvent	m_OnHealthBelow20Percent;
	COutputEvent	m_OnHealthBelow10Percent;
public:
	COutputEvent	m_OnShieldRespawned;
	COutputEvent	m_OnShieldDestroyed;
#endif
};
#endif

#ifdef GAME_DLL
class CBonusPack : public CBaseAnimating
{
public:
	DECLARE_CLASS( CBonusPack, CBaseAnimating );
	DECLARE_DATADESC();

	CBonusPack();

	void Spawn( void );
	void Precache( void );
	void ChangeTeam( int iTeamNum );
	void OnTouch( CBaseEntity *pOther );

	string_t	m_szModel;
	string_t	m_szPickupSoundPos;
	string_t	m_szPickupSoundNeg;
	COutputEvent	m_OnSpawn;
	COutputEvent	m_OnPickup;
	COutputEvent	m_OnPickupTeam1;
	COutputEvent	m_OnPickupTeam2;
	COutputEvent	m_OnPickupTeam3;
	COutputEvent	m_OnPickupTeam4;

};
#endif

#endif // TF_GENERATOR_H
