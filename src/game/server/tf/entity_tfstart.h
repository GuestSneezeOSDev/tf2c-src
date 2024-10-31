//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF Spawn Point.
//
//=============================================================================//
#ifndef ENTITY_TFSTART_H
#define ENTITY_TFSTART_H

#ifdef _WIN32
#pragma once
#endif

class CTeamControlPoint;
class CTeamControlPointRound;

//-----------------------------------------------------------------------------
// Spawnflags
//-----------------------------------------------------------------------------
#define	SF_TEAMSPAWN_SCOUT			1
#define SF_TEAMSPAWN_SNIPER			2
#define SF_TEAMSPAWN_SOLDIER		4
#define SF_TEAMSPAWN_DEMOMAN		8
#define	SF_TEAMSPAWN_MEDIC			16
#define SF_TEAMSPAWN_HEAVY			32
#define SF_TEAMSPAWN_PYRO			64
#define SF_TEAMSPAWN_SPY			128
#define SF_TEAMSPAWN_ENGINEER		128
#define SF_TEAMSPAWN_CIVILIAN		512

//=============================================================================
//
// TF team spawning entity.
//

class CTFTeamSpawn : public CLogicalEntity, public TAutoList<CTFTeamSpawn>
{
public:
	DECLARE_CLASS( CTFTeamSpawn, CLogicalEntity );

	CTFTeamSpawn();

	virtual void Activate( void );

	bool IsDisabled( void ) { return m_bDisabled || m_nMatchSummaryType != 0; }
	void SetDisabled( bool bDisabled ) { m_bDisabled = bDisabled; }

	bool SpawnsAllTeams( void ) { return m_bAllTeams; }
	bool SpawnsLimitedClasses( void ) { return m_bRestrictClasses; }

	// Inputs/Outputs.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputRoundSpawn( inputdata_t &inputdata );

	int DrawDebugTextOverlays( void );

	CHandle<CTeamControlPoint> GetControlPoint( void ) { return m_hControlPoint; }
	CHandle<CTeamControlPointRound> GetRoundBlueSpawn( void ) { return m_hRoundBlueSpawn; }
	CHandle<CTeamControlPointRound> GetRoundRedSpawn( void ) { return m_hRoundRedSpawn; }
	CHandle<CTeamControlPointRound> GetRoundGreenSpawn( void ) { return m_hRoundGreenSpawn; }
	CHandle<CTeamControlPointRound> GetRoundYellowSpawn( void ) { return m_hRoundYellowSpawn; }

private:
	bool	m_bDisabled;		// Enabled/Disabled?
	int		m_nMatchSummaryType; // For live TF2 compatibility

	bool	m_bAllTeams;		// Allows any team to spawn here
	bool	m_bRestrictClasses;		// Use spawnflags to limit which TF2 classes can spawn here

	string_t						m_iszControlPointName;
	string_t						m_iszRoundBlueSpawn;
	string_t						m_iszRoundRedSpawn;
	string_t						m_iszRoundGreenSpawn;
	string_t						m_iszRoundYellowSpawn;

	CHandle<CTeamControlPoint>		m_hControlPoint;
	CHandle<CTeamControlPointRound>	m_hRoundBlueSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundRedSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundGreenSpawn;
	CHandle<CTeamControlPointRound>	m_hRoundYellowSpawn;

	DECLARE_DATADESC();
};

#endif // ENTITY_TFSTART_H


