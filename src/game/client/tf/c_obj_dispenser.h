//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_OBJ_DISPENSER_H
#define C_OBJ_DISPENSER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseobject.h"
#include "ObjectControlPanel.h"
#include "vgui_controls/RotatingProgressBar.h"

class C_ObjectDispenser : public C_BaseObject
{
	DECLARE_CLASS( C_ObjectDispenser, C_BaseObject );

public:
	DECLARE_CLIENTCLASS();

	C_ObjectDispenser();
	~C_ObjectDispenser();

	int GetUpgradeLevel( void ) { return m_iUpgradeLevel; }

	virtual void GetStatusText( wchar_t *pStatus, int iMaxStatusLen );

	int GetMetalAmmoCount() { return m_iAmmoMetal; }

	CUtlVector<CHandle<C_TFPlayer>> m_hHealingTargets;

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );

	virtual void SetDormant( bool bDormant );

	bool ShouldShowHealingEffectForPlayer( C_TFPlayer *pPlayer );
	void UpdateEffects( void );

	virtual void UpdateDamageEffects( BuildingDamageLevel_t damageLevel );

	void UpdateHealingTargets() { m_bUpdateHealingTargets = true; };

	friend class C_ObjectMiniDispenser;
private:
	bool m_bUpdateHealingTargets;

	bool m_bHealingTargetsParity;
	bool m_bOldHealingTargetsParity;

	int m_iAmmoMetal;

	bool m_bPlayingSound;

	struct healingtargeteffects_t
	{
		C_BaseEntity		*pTarget;
		CNewParticleEffect	*pEffect;
	};
	CUtlVector<healingtargeteffects_t> m_hHealingTargetEffects;

	CNewParticleEffect *m_pDamageEffects;

private:
	C_ObjectDispenser( const C_ObjectDispenser & ); // Not defined, not accessible.

};


class CDispenserControlPanel : public CObjectControlPanel
{
	DECLARE_CLASS( CDispenserControlPanel, CObjectControlPanel );

public:
	CDispenserControlPanel( vgui::Panel *parent, const char *panelName );

protected:
	virtual void OnTickActive( C_BaseObject *pObj, C_TFPlayer *pLocalPlayer );

private:
	vgui::RotatingProgressBar *m_pAmmoProgress;

};

class CDispenserControlPanel_Blue : public CDispenserControlPanel
{
	DECLARE_CLASS( CDispenserControlPanel_Blue, CDispenserControlPanel );

public:
	CDispenserControlPanel_Blue( vgui::Panel *parent, const char *panelName ) : CDispenserControlPanel( parent, panelName ) {}

};

class CDispenserControlPanel_Green : public CDispenserControlPanel
{
	DECLARE_CLASS( CDispenserControlPanel_Green, CDispenserControlPanel );

public:
	CDispenserControlPanel_Green( vgui::Panel *parent, const char *panelName) : CDispenserControlPanel(parent, panelName ) {}

};


class CDispenserControlPanel_Yellow : public CDispenserControlPanel
{
	DECLARE_CLASS( CDispenserControlPanel_Yellow, CDispenserControlPanel );

public:
	CDispenserControlPanel_Yellow( vgui::Panel *parent, const char *panelName) : CDispenserControlPanel(parent, panelName ) {}

};

class C_ObjectCartDispenser : public C_ObjectDispenser
{
	DECLARE_CLASS( C_ObjectCartDispenser, C_ObjectDispenser );

public:
	DECLARE_CLIENTCLASS();

};

class C_ObjectMiniDispenser : public C_ObjectDispenser
{
	DECLARE_CLASS(C_ObjectMiniDispenser, C_ObjectDispenser);

public:
	C_ObjectMiniDispenser();
	~C_ObjectMiniDispenser();
	DECLARE_CLIENTCLASS();

};
#endif	//C_OBJ_DISPENSER_H