//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Dispenser
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_DISPENSER_H
#define TF_OBJ_DISPENSER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_obj.h"

class CTFPlayer;

enum
{
	DISPENSER_LEVEL_1 = 0,
	DISPENSER_LEVEL_2,
	DISPENSER_LEVEL_3,
};

#define SF_IGNORE_LOS					(SF_OBJ_INVULNERABLE<<1)
#define SF_NO_DISGUISED_SPY_HEALING		(SF_OBJ_INVULNERABLE<<2)

// ------------------------------------------------------------------------ //
// Resupply object that's built by the player
// ------------------------------------------------------------------------ //
class CObjectDispenser : public CBaseObject, public TAutoList<CObjectDispenser>
{
	DECLARE_CLASS( CObjectDispenser, CBaseObject );

public:
	DECLARE_SERVERCLASS();

	// We have both CBaseObject::AutoList and CObjectDispenser::AutoList.
	// This 'using' statement resolves the ambiguity such that:
	// CBaseObject::AutoList      --> TAutoList<CBaseObject>::AutoList
	// CObjectDispenser::AutoList --> TAutoList<CObjectDispenser>::AutoList
	using TAutoList<CObjectDispenser>::AutoList;

	CObjectDispenser();
	~CObjectDispenser();

	static CObjectDispenser *Create( const Vector &vOrigin, const QAngle &vAngles );

	virtual void	Spawn();
	virtual void	FirstSpawn();
	virtual void	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	virtual void	Precache();

	virtual void	DetonateObject( void );
	virtual void	OnGoActive( void );	
	virtual bool	StartBuilding( CBaseEntity *pBuilder );
	virtual void	InitializeMapPlacedObject( void );
	virtual int		DrawDebugTextOverlays( void );
	virtual void	SetModel( const char *pModel );

	void RefillThink( void );
	void DispenseThink( void );

	virtual float GetDispenserRadius( void );
	virtual float GetHealRate( void );

	virtual void StartTriggerTouch( CBaseEntity *pOther );
	virtual void Touch( CBaseEntity *pOther );
	virtual void EndTriggerTouch( CBaseEntity *pOther );

	virtual int GetBaseHealth( void ) const;

	virtual bool DispenseAmmo( CTFPlayer *pPlayer );

	void StartHealing( CBaseEntity *pOther );
	void StopHealing( CBaseEntity *pOther );

	void AddHealingTarget( CBaseEntity *pOther );
	void RemoveHealingTarget( CBaseEntity *pOther );
	bool IsHealingTarget( CBaseEntity *pTarget );

	void ResetHealingTargets( void );

	bool CouldHealTarget( CBaseEntity *pTarget );

	Vector GetHealOrigin( void );

	CUtlVector<EHANDLE>	m_hHealingTargets;

	virtual bool	OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos );

	virtual bool	IsUpgrading( void ) const;
	virtual const char	*GetPlacementModel( void ) const;
	virtual const char	*GetDispenserUpgradeModelForLevel(int level) const;
	virtual const char	*GetDispenserModelForLevel(int level) const;

	virtual void	MakeCarriedObject( CTFPlayer *pPlayer );
	virtual void	DropCarriedObject( CTFPlayer *pPlayer );

	int GetAvailableMetal() const { return m_iAmmoMetal; }

	void UpdateHealingTargets() { m_bHealingTargetsParity = !m_bHealingTargetsParity; };
	virtual float GetRefillDelay() { return 6.0f; }

	virtual Vector GetMins() { return Vector(-20, -20, 0); };
	virtual Vector GetMaxs() { return Vector(20, 20, 55); };

	friend class CObjectMiniDispenser;
private:
	void StartUpgrading( void );
	void FinishUpgrading( void );

	//CNetworkArray( EHANDLE, m_hHealingTargets, MAX_DISPENSER_HEALING_TARGETS );

	// Entities currently being touched by this trigger.
	CUtlVector<EHANDLE> m_hTouchingEntities;

	CNetworkVar( bool, m_bHealingTargetsParity );

	CNetworkVar( int, m_iAmmoMetal );

	// Time when the upgrade animation will complete.
	float m_flUpgradeCompleteTime;

	float m_flNextAmmoDispense;

	bool m_bIsUpgrading;

	EHANDLE m_hTouchTrigger;
	string_t m_szTriggerName;
	float m_flRadius;

	DECLARE_DATADESC();

};

class CObjectCartDispenser : public CObjectDispenser
{
	DECLARE_CLASS( CObjectCartDispenser, CObjectDispenser );
	DECLARE_DATADESC();

public:
	DECLARE_SERVERCLASS();

	virtual int		GetMaxUpgradeLevel( void ) const { return 1; }
	virtual void	Spawn( void );
	virtual bool	CanBeUpgraded( CTFPlayer *pPlayer ) { return false; }
	virtual void	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName ) { return; }
	virtual void	SetModel( const char *pModel );
	virtual void	OnGoActive( void );

	void			InputSetDispenserLevel( inputdata_t &inputdata );
	void			InputEnable( inputdata_t &input );
	void			InputDisable( inputdata_t &input );

private:

	CNetworkVar( int, m_iAmmoMetal );

};

// ------------------------------------------------------------------------ //
// Resupply object that's built by the player
// ------------------------------------------------------------------------ //
class CObjectMiniDispenser : public CObjectDispenser
{
	DECLARE_CLASS(CObjectMiniDispenser, CObjectDispenser);

public:
	DECLARE_SERVERCLASS();

	// We have both CBaseObject::AutoList and CObjectDispenser::AutoList.
	// This 'using' statement resolves the ambiguity such that:
	// CBaseObject::AutoList      --> TAutoList<CBaseObject>::AutoList
	// CObjectDispenser::AutoList --> TAutoList<CObjectDispenser>::AutoList
	using TAutoList<CObjectDispenser>::AutoList;

	CObjectMiniDispenser();
	~CObjectMiniDispenser();

	static CObjectMiniDispenser* Create(const Vector& vOrigin, const QAngle& vAngles);

	virtual float GetRefillDelay() { return -1.0f; }

	float GetDispenserRadius(void);
	float GetHealRate(void);

	virtual int GetBaseHealth(void) const;

	void OnGoActive();
	bool DispenseAmmo(CTFPlayer* pPlayer);

	virtual const char* GetPlacementModel(void) const;
	virtual const char* GetDispenserUpgradeModelForLevel(int level) const;
	virtual const char* GetDispenserModelForLevel(int level) const;

	virtual Vector GetMins() { return Vector(-15, -15, 0); };
	virtual Vector GetMaxs() { return Vector(15, 15, 25); };

	virtual void	MakeCarriedObject(CTFPlayer* pPlayer);
private:
	//CNetworkArray( EHANDLE, m_hHealingTargets, MAX_DISPENSER_HEALING_TARGETS );

	// Entities currently being touched by this trigger.
	DECLARE_DATADESC();

};
#endif // TF_OBJ_DISPENSER_H
