//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Sniper Rifle
//
//=============================================================================//
#ifndef TF_WEAPON_SNIPERRIFLE_H
#define TF_WEAPON_SNIPERRIFLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "Sprite.h"

#define TF_WEAPON_SNIPERRIFLE_CHARGE_PER_SEC	50.0f
#define TF_WEAPON_SNIPERRIFLE_UNCHARGE_PER_SEC	75.0f
#define	TF_WEAPON_SNIPERRIFLE_DAMAGE_MIN		50
#define TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX		150
#define TF_WEAPON_SNIPERRIFLE_ZOOM_TIME			0.3f

#define TF_WEAPON_SNIPERRIFLE_NO_CRIT_AFTER_ZOOM_TIME	0.2f

#if defined( CLIENT_DLL )
#define CTFSniperRifle C_TFSniperRifle
#define CSniperDot C_SniperDot
#endif

//=============================================================================
//
// Sniper Rifle Laser Dot class.
//
class CSniperDot : public CBaseEntity
{
public:
	DECLARE_CLASS( CSniperDot, CBaseEntity );
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	// Creation/Destruction.
	CSniperDot( void );
	~CSniperDot( void );

	static CSniperDot *Create( const Vector &origin, CBaseEntity *pOwner = NULL, bool bVisibleDot = true );
	void		ResetChargeTime( void ) { m_flChargeStartTime = gpGlobals->curtime; }
	void		SetChargeSpeed( float flSpeed ) { m_flChargeSpeed = flSpeed; }

	// Attributes.
	int			ObjectCaps()							{ return ( BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_DONT_SAVE; }

	// Targeting.
	void        Update( CBaseEntity *pTarget, const Vector &vecOrigin, const Vector &vecNormal );
	CBaseEntity	*GetTargetEntity( void )				{ return m_hTargetEnt; }

// Client specific.
#ifdef CLIENT_DLL
	// Rendering.
	virtual bool			IsTransparent( void )		{ return true; }
	virtual RenderGroup_t	GetRenderGroup( void )		{ return RENDER_GROUP_TRANSLUCENT_ENTITY; }
	virtual int				DrawModel( int flags );
	virtual bool			ShouldDraw( void );
	virtual void			OnDataChanged( DataUpdateType_t updateType );

	CMaterialReference		m_hSpriteMaterial;
#endif

protected:
	Vector					m_vecSurfaceNormal;
	EHANDLE					m_hTargetEnt;

	CNetworkVar( float, m_flChargeStartTime );
	CNetworkVar( float, m_flChargeSpeed );
};

//=============================================================================
//
// Sniper Rifle class.
//
class CTFSniperRifle : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFSniperRifle, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFSniperRifle();
	~CTFSniperRifle();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_SNIPERRIFLE; }
	virtual bool IsSniperRifle( void ) const { return true; }

	virtual void Precache();
	void		 ResetTimers( void );

	virtual bool CanHolster( void ) const;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo );

	void		 HandleZooms( void );
	virtual void ItemPostFrame( void );
	virtual bool Lower( void );
	virtual float GetProjectileDamage( void );
	virtual float GetWeaponSpread( void );
	virtual float GetChargeSpeed( void );

	virtual void WeaponReset( void );

	virtual bool OwnerCanJump( void );
	virtual float OwnerMaxSpeedModifier( void );

	virtual float	DeployedPoseHoldTime( void ) { return 2.0f; }

	virtual bool CanAutoReload( void ) { return !IsZoomed(); }
	virtual bool CanFireRandomCrit( void );
	virtual bool CanHeadshot( void );

	bool		IsFullyCharged() const { return m_flChargedDamage >= 150.0f; }

	virtual void DoFireEffects( void );
#ifdef CLIENT_DLL
	virtual CStudioHdr	*OnNewModel( void );
	float GetHUDDamagePerc( void );
	virtual bool ShouldDrawCrosshair( void );
#endif

	bool IsZoomed( void );

#ifdef GAME_DLL
	virtual bool TFBot_IsContinuousFireWeapon() OVERRIDE { return false; }
	virtual bool TFBot_ShouldAimForHeadshots()  OVERRIDE { return true;  }
	virtual bool TFBot_IsSniperRifle()          OVERRIDE { return true;  }
#endif

	void		UpdateCylinder( void );

private:

	virtual void CreateSniperDot( void );
	virtual void DestroySniperDot( void );
	virtual void UpdateSniperDot( void );

private:
	// Auto-rezooming handling
	void SetRezoom( bool bRezoom, float flDelay );

	void Zoom( void );
	void ZoomOutIn( void );
	void ZoomIn( void );
	void ZoomOut( void );
	void Fire( CTFPlayer *pPlayer );

protected:

	CNetworkVar( float,	m_flChargedDamage );

	// Handles rezooming after the post-fire unzoom
	CNetworkVar( float, m_flUnzoomTime );
	CNetworkVar( float, m_flRezoomTime );
	CNetworkVar( bool, m_bRezoomAfterShot );

	CNetworkVar( bool, m_bHasNoScope );

	CNetworkVar( int, m_iShotsFired );

#ifdef CLIENT_DLL
	int m_iCylinderPoseParameter;
#endif

#ifdef GAME_DLL
	CHandle<CSniperDot>		m_hSniperDot;
#else
	bool m_bDinged;
#endif

	CTFSniperRifle( const CTFSniperRifle & );
public:
	bool IsFullyCharged() { return m_flChargedDamage >= TF_WEAPON_SNIPERRIFLE_DAMAGE_MAX; }
};

#endif // TF_WEAPON_SNIPERRIFLE_H
