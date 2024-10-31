//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#ifndef TF_WEAPON_PAINTBALLRIFLE_H
#define TF_WEAPON_PAINTBALLRIFLE_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_medigun.h"

#if defined( CLIENT_DLL )
#define CTFPaintballRifle C_TFPaintballRifle
#endif

#define PAINTBALL_BACKPACK_RANGE_SQ 450*450

//=============================================================================
//
// Paintball Rifle class.
//
class CTFPaintballRifle : public CTFWeaponBaseGun, public ITFHealingWeapon
{
public:
	DECLARE_CLASS( CTFPaintballRifle, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFPaintballRifle();
	~CTFPaintballRifle();
	virtual bool	Deploy( void ) override;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo ) override;
	virtual bool	Lower( void ) override;
	virtual bool	CanHolster( void ) const;
	virtual void	WeaponReset( void ) override;
	virtual void	ItemHolsterFrame( void ) override;
	virtual void	ItemPostFrame( void ) override;
	virtual void	UpdateOnRemove( void ) override;
	virtual float	GetChargeLevel( void ) const override { return m_flChargeLevel; }
	virtual float	GetMinChargeAmount() const override { return 1.00f; }
	bool			IsReleasingCharge( void ) const override { return m_bChargeRelease; }
	medigun_charge_types GetChargeType( void ) override;
	CTFWeaponBase		*GetWeapon( void ) override { return this; }
	int					GetMedigunType( void ) override { return TF_MEDIGUN_STOCK; }
	bool				AutoChargeOwner( void ) override { return false; }
	virtual int			GetUberRateBonusStacks( void ) const override { return 0; }
	virtual float		GetUberRateBonus( void ) const override { return 0.0f; }
	virtual void		BuildUberForTarget( CBaseEntity *pTarget, bool bMultiTarget = false ) override;
	virtual void		OnHealedPlayer( CTFPlayer *pPatient, float flAmount, HealerType tType ) override {}; // Already managed by rifle, no need for any code here
	virtual void		DrainCharge( void ) override;
	virtual void		AddCharge( float flAmount ) override;

	void			HandleZooms( void );
	bool			IsZoomed( void );

	virtual void	TertiaryAttack() override;

	virtual float OwnerMaxSpeedModifier( void ) override;
	virtual float DeployedPoseHoldTime( void ) override	{ return 0.6f; }

	virtual ETFWeaponID	GetWeaponID( void ) const override		{ return TF_WEAPON_PAINTBALLRIFLE; }

	CUtlVector<EHANDLE>		m_vAutoHealTargets;

	CNetworkVar( int, m_iMainPatientHealthLast );
	CNetworkVar( bool, m_bMainTargetParity );
	CNetworkVar( bool, m_bBackpackTargetsParity );
	CNetworkHandle( CBaseEntity, m_hMainPatient );

	CNetworkVar( float, m_flChargeLevel );
	CNetworkVar( bool, m_bChargeRelease );
	bool m_bBuiltChargeThisFrame;

#ifdef GAME_DLL
	virtual void		HitAlly( CBaseEntity *pEntity ) override;
	virtual void		OnDirectHit( CTFPlayer *pPatient );
	virtual void		AddBackpackPatient( CTFPlayer *pPlayer ) override {};
	virtual bool		IsBackpackPatient( CTFPlayer *pPlayer ) override { return false; }
	virtual void		RemoveBackpackPatient( int iIndex ) override {};
	virtual void		UpdateBackpackMaxTargets( void ) override {};

	CUtlVector<float>		m_vTargetRemoveTime;
	CUtlVector<bool>		m_vTargetsActive;
	bool			m_bMainPatientFlaggedForRemoval;

	virtual bool	 TFBot_IsCombatWeapon() OVERRIDE{ return false; }

	virtual void	AddHealTarget( CTFPlayer *pPlayer, float flDuration );
	virtual void	RemoveHealTarget( CTFPlayer *pPlayer );
	virtual void	RemoveAllHealTargets( void );
	virtual void	ApplyBackpackHealing( void );
	virtual float	GetHealRate( void );
	void			UpdateBackpackTargets() { m_bBackpackTargetsParity = !m_bBackpackTargetsParity; };

	virtual void	AddUberRateBonusStack( float flBonus, int nStack = 1 ) override {};
#else
	void			UpdateEffects( void );
	bool			ShouldShowEffectForPlayer( C_TFPlayer *pPlayer );
	void			UpdateBackpackTargets() { m_bUpdateBackpackTargets = true; };
	void			OnDataChanged( DataUpdateType_t updateType ) override;
	void			OnPreDataChanged( DataUpdateType_t updateType ) override;
#ifdef TF2C_BETA
	void			UpdateRecentPatientHealthbar( C_TFPlayer *pPatient );
	virtual void	FireGameEvent( IGameEvent * event ) override;
#endif
	struct healingtargeteffects_t
	{
		EHANDLE				hOwner;
		C_BaseEntity		*pTarget;
		CNewParticleEffect	*pEffect;
	};

	CUtlVector<healingtargeteffects_t> m_hBackpackTargetEffects;
	CUtlVector<healingtargeteffects_t> m_hTargetOverheadEffects;

	bool					m_bUpdateBackpackTargets;
	bool					m_bOldBackpackTargetsParity;
	bool					m_bMainTargetParityOld;
	float					m_flNextBuzzTime;

	CTFHealSoundManager		*m_pHealSoundManager;
#endif
protected:
	CNetworkVar( float, m_flUnzoomTime );
	CNetworkVar( float, m_flRezoomTime );
	CNetworkVar( bool, m_bRezoomAfterShot );
#ifdef GAME_DLL
	virtual void		CheckAndExpireStacks( void ) override {};

	CTFPlayer		*m_pUberTarget;
#endif
		
private:
	CTFPaintballRifle( const CTFPaintballRifle & ) {}

	// Auto-rezooming handling
	void SetRezoom( bool bRezoom, float flDelay );

	void Zoom( void );
	void ZoomOutIn( void );
	void ZoomIn( void );
	void ZoomOut( void );

	void Fire( CTFPlayer *pPlayer );

//#ifdef CLIENT_DLL
//	void		ManageChargeEffect( void );
//#endif

};

#endif // TF_WEAPON_PAINTBALLRIFLE_H