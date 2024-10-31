//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_MINIGUN_H
#define TF_WEAPON_MINIGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#include "particles_new.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFMinigun C_TFMinigun
#endif

enum MinigunState_t
{
	// Firing states.
	AC_STATE_IDLE = 0,
	AC_STATE_STARTFIRING,
	AC_STATE_FIRING,
	AC_STATE_SPINNING,
	AC_STATE_DRYFIRE
};

//=============================================================================
//
// TF Weapon Minigun
//
class CTFMinigun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFMinigun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFMinigun();
	~CTFMinigun();

	virtual ETFWeaponID	GetWeaponID( void ) const { return TF_WEAPON_MINIGUN; }
	virtual bool	IsMinigun( void ) const { return true; }

	virtual void	Precache( void );
	virtual void	ItemPreFrame( void );
	virtual void	ItemPostFrame( void );
	virtual void	SharedAttack();
	virtual bool	SendWeaponAnim( int iActivity );
	virtual bool	Deploy( void );
	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual bool	Lower( void );
	virtual void	HandleFireOnEmpty( void );
	virtual void	WeaponReset( void );

	virtual float	OwnerMaxSpeedModifier( void );
	virtual bool	OwnerCanJump( void );

#ifdef GAME_DLL
	virtual int		UpdateTransmitState( void );
#endif

	virtual ETFDmgCustom	GetCustomDamageType() const { return TF_DMG_CUSTOM_MINIGUN; }

	float			GetFiringTime( void ) { return m_flStartedFiringAt >= 0.0f ? gpGlobals->curtime - m_flStartedFiringAt : 0.0f; }
	float			GetSpunupTime( void ) { return m_flStartedSpinupAt >= 0.0f ? gpGlobals->curtime - m_flStartedSpinupAt : 0.0f; }
	void			UpdateBarrelMovement( void );

	bool			HasSpinSounds( void ) const { int iMode = 0; CALL_ATTRIB_HOOK_INT( iMode, minigun_no_spin_sounds ); return iMode != 1; };

	float			GetWindUpTime( bool bIgnoreTranq = false );
	float			GetWindDownTime( bool bIgnoreTranq = false );

	virtual float	GetWeaponSpread( void );
	virtual float	GetProjectileDamage( void );

#ifdef CLIENT_DLL
	float			GetBarrelRotation();
	virtual void	GetWeaponCrosshairScale( float& flScale );
#endif

protected:
#ifdef CLIENT_DLL
	// Barrel spinning
	virtual CStudioHdr	*OnNewModel( void );
	virtual void		StandardBlendingRules( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask );
	virtual void		ViewModelAttachmentBlending( CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask, C_ViewmodelAttachmentModel *pAttachment );

	virtual void		UpdateOnRemove( void );

	virtual void		OnDataChanged( DataUpdateType_t type );
	virtual void		Simulate( void );

	// Firing sound
	virtual void		WeaponSoundUpdate( void );
	void				PlayStopFiringSound( void );

	virtual void		ThirdPersonSwitch( bool bThirdperson );

	virtual void		SetDormant( bool bDormant );
#endif

	void WindUp( void );
	virtual void WindDown( void );

private:
	CTFMinigun( const CTFMinigun & ) {}

protected:
	CNetworkVar( MinigunState_t, m_iWeaponState );
	CNetworkVar( float, m_flStateUpdateTime );

private:
	virtual void PlayWeaponShootSound( void ) {}	// override base class call to play shoot sound; we handle that ourselves separately
	
	CNetworkVar( bool, m_bCritShot );
protected:
	CNetworkVar( float, m_flBarrelCurrentVelocity );
	CNetworkVar(float, m_flBarrelTargetVelocity);
private:

	float			m_flStartedFiringAt;
	float			m_flStartedSpinupAt;
	float			m_flLeftGroundAt;

#ifdef GAME_DLL
	float			m_flNextFiringSpeech;
#else
	void StartBrassEffect();
	void StopBrassEffect();
	void HandleBrassEffect();

	EHANDLE				m_hBrassEffectHost;
	CNewParticleEffect *m_pEjectBrassEffect;

	void StartMuzzleEffect();
	void StopMuzzleEffect();
	void HandleMuzzleEffect();

	EHANDLE				m_hMuzzleEffectHost;
	CNewParticleEffect *m_pMuzzleEffect;

	int				m_iBarrelBone;
	float			m_flBarrelAngle;
	float			m_flEjectBrassTime;

protected:
	CSoundPatch		*m_pSoundCur;				// the weapon sound currently being played
	int				m_iMinigunSoundCur;			// the enum value of the weapon sound currently being played
	MinigunState_t	m_iPrevMinigunState;
	float			m_flSpinningSoundTime;
#endif

public:
	bool			GetCritShot(void) { return m_bCritShot; }
	void			SetCritShot(bool bCrit) { m_bCritShot = bCrit; }
	float			GetFireStartTime(void) { return m_flStartedFiringAt; }
	void			SetFireStartTime(float flTime) { m_flStartedFiringAt = flTime; }
#ifdef GAME_DLL
	float			GetNextFiringSpeech(void) { return m_flNextFiringSpeech; }
	void			SetNextFiringSpeech(float flTime) { m_flNextFiringSpeech = flTime; }
#endif
};

#endif // TF_WEAPON_MINIGUN_H
