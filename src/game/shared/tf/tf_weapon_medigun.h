//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_MEDIGUN_H
#define TF_WEAPON_MEDIGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "utlqueue.h"
#include "tf_player_shared.h"

#if defined( CLIENT_DLL )
#define CWeaponMedigun C_WeaponMedigun
#endif

#define MAX_HEALING_TARGETS			1	//6

#define CLEAR_ALL_TARGETS			-1

#define BP_MAX_LEN 16
#define BP_PARTICLE_ACTIVE "_universal_backpacktarget_marker_enabled"
#define BP_PARTICLE_INACTIVE "_universal_backpacktarget_marker_disabled"
#define BP_SOUND_HEALON "Building_Dispenser.Heal"
#define BP_SOUND_HEALOFF "WeaponMedigun.Healing.Detach"
#define BP_REMOVE_TIME 9

#ifdef CLIENT_DLL
void RecvProxy_HealingTarget( const CRecvProxyData *pData, void *pStruct, void *pOut );
#endif


#ifdef CLIENT_DLL

class CTFHealSoundManager {
public:
	CTFHealSoundManager( CTFPlayer *pHealer = NULL, CTFWeaponBase *pMedigun = NULL);
	~CTFHealSoundManager();
	// Must be called externally via a think function to update the sound envelope.
	void SoundManagerThink();
	void SetPatient( CTFPlayer *pPatient );
	
	// Forces the ratio for pitch to a certain value. Use this with the pre-healing value for burst healers.
	void SetLastPatientHealth( int iLastHealth );

	float m_flRemoveTime = 0.8f;
	float m_flFadeoutTime = 0.3f;
	float m_flBaseVolume = 1.0f;

	void RemoveSound();

	bool ParseCollageSchema( const char *pszFile );
	void ParseCollage( KeyValues *pKeyValuesData );

	bool IsSetupCorrectly();

	void EnvelopeSounds( float flBaseVolume, float flHealthRatio );

	bool TrySelectSoundCollage( const char *pszCollageName );

	CTFWeaponBase *GetMedigun() { return m_pMedigun;  }

	// A sound definition to be included in a collage
	struct layeredenvelope_t
	{
		CSoundPatch		*m_pSound       = nullptr;
		// X,Y = Ratio input min and max, Z,W = output remaps
		// Ratio input is clamped between X and Y and then remapped from X-Y to Z-W before being passed to volume/pitch equations
        Vector4D		m_vVolumeBounds = {};		// Volume at min ratio and max ratio, lerps and clamps.
		Vector4D		m_vPitchBounds  = {};			// The pitch (as proportion) at min ratio and max ratio
		const char		*szSoundName    = {};
	};

	class SoundCollage {
	public:
		SoundCollage( KeyValues *pKeyValuesData );
		~SoundCollage();
		void EnvelopeSounds( float flBaseVolume, float flHealthRatio );
		void Create( CTFWeaponBase *pMedigun );
		void Remove();
		void CleanupRemove();
		void Precache( CBaseEntity *pParent );
		const char *GetName() { return m_pszName; }
		const char * GetOverhealThresholdSound() { return m_pszMaxHPSound; }
		const char * GetOverhealThresholdMaxSound() { return m_pszMaxOverhealSound; }
	protected:
		void EnvelopeSound( layeredenvelope_t *pLayeredSound, float flBaseRatio, float flBaseVolume );
		const char						*m_pszName;
		CUtlVector<layeredenvelope_t>	m_vEnvelopedSounds;

		const char *m_pszStartSound;
		const char *m_pszFinishSound;

		const char *m_pszMaxHPSound;
		const char *m_pszMaxOverhealSound;

		CTFWeaponBase	*m_pMedigun;
	};

	void Precache( CBaseEntity *pParent );

protected:
	
	void CreateSound();

	// This value slides over time as the health value changes, but is forced by the SetLastPatientHealth function
	float			m_flRatioForPitch;
	// This value is updated constantly as the health changes, as is moved toward by the above value.
	float			m_flCurrentRatio;

	// Used for detecting the crossing of important health thresholds (max HP, max overheal)
	int				m_iLastHealthValue;

	
	CUtlVector<SoundCollage*>			m_vCollages;
	SoundCollage			*m_pActiveCollage;

	CTFPlayer		*m_pPatient;
	CTFPlayer		*m_pHealer;
	CTFWeaponBase	*m_pMedigun;

	bool			m_bHealSoundPlaying;

	// Are we currently healing this patient?
	bool			m_bHealingPatient;

	// When should we automatically stop playing the audio after stopping healing the patient?
	float			m_flStopPlayingTime;

	bool			m_bSchemaLoaded;
};


static CTFHealSoundManager *g_pHealSoundManager;

inline CTFHealSoundManager *UTIL_GetHSM( CTFWeaponBase *pWeapon = NULL )
{
	if ( pWeapon != NULL )
	{
		if ( !g_pHealSoundManager )
			g_pHealSoundManager = new CTFHealSoundManager( pWeapon->GetTFPlayerOwner(), pWeapon );
		else if ( g_pHealSoundManager->GetMedigun() != pWeapon || !g_pHealSoundManager->IsSetupCorrectly() )
		{
			g_pHealSoundManager->RemoveSound();
			delete g_pHealSoundManager;
			g_pHealSoundManager = new CTFHealSoundManager( pWeapon->GetTFPlayerOwner(), pWeapon );
		}
	}

	return g_pHealSoundManager;
}

inline void UTIL_HSMSetPlayerThink( CTFPlayer *pPlayer )
{
	if ( UTIL_GetHSM() ) {
		g_pHealSoundManager->SetPatient( pPlayer );
		g_pHealSoundManager->SoundManagerThink();
	}
}

#endif

//=========================================================
// Interface for any weapon that can heal and store
// Uber in itself. Used for situations where the game
// needs info about a Medic's healing weapons (mediguns)
// without know the exact class.
// Do not use this for primary/melee weapons that can heal -
// only weapons that can actually apply Uber themselves.
//=========================================================

#define TF2C_HASTE_HEALING_FACTOR	1.33f
#define TF2C_HASTE_UBER_FACTOR		1.33f

class ITFHealingWeapon
{
public:
	ITFHealingWeapon(){}
	virtual ~ITFHealingWeapon(){}
	virtual float			GetChargeLevel( void ) const = 0;
	virtual void			AddCharge( float flAmount ) = 0;
	virtual void			DrainCharge( void ) = 0;
	virtual bool			IsReleasingCharge( void ) const = 0;
	virtual medigun_charge_types GetChargeType( void ) = 0;
	virtual bool			AutoChargeOwner( void ) = 0;
	virtual CTFWeaponBase	*GetWeapon( void ) = 0;
	virtual int				GetMedigunType( void ) = 0;
	virtual void			OnHealedPlayer( CTFPlayer *pPatient, float flAmount, HealerType tType = HEALER_TYPE_BEAM ) = 0;
	virtual int				GetUberRateBonusStacks( void ) const = 0;
	virtual float			GetUberRateBonus( void ) const = 0;
	virtual float			GetMinChargeAmount() const { return 1.00f; }
	virtual void			BuildUberForTarget( CBaseEntity *pTarget, bool bMultiTarget = false ) = 0;
	CUtlVector<EHANDLE>		m_vBackpackTargets;
	CUtlVector<float>		m_vBackpackTargetRemoveTime;
	CUtlVector<float>		m_vBackpackTargetAddedTime;

#ifdef GAME_DLL
	virtual void					AddBackpackPatient( CTFPlayer *pPlayer ) = 0;
	virtual void					RemoveBackpackPatient( int iIndex ) = 0;
	//virtual CUtlVector<EHANDLE>		GetBackpackPatients( void ){ return m_vBackpackTargets; }
	//virtual CUtlVector<float>		GetBackpackPatientRemoveTimes( void ) { return m_vBackpackTargetRemoveTime; }
	virtual bool					IsBackpackPatient( CTFPlayer *pPlayer ) = 0;
	virtual void					UpdateBackpackMaxTargets( void ) = 0;


	virtual void			AddUberRateBonusStack( float flBonus, int nStack = 1 ) = 0;

protected:
	virtual void			CheckAndExpireStacks( void ) = 0;
	float					m_flStacksRemoveTime = 0;		// The time when all stacks should be removed	
#endif
};

#ifdef GAME_DLL
#endif

//=========================================================
// Beam healing gun
//=========================================================
class CWeaponMedigun : public CTFWeaponBaseGun, public ITFHealingWeapon
{
public:
	DECLARE_CLASS( CWeaponMedigun, CTFWeaponBaseGun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponMedigun( void );
	~CWeaponMedigun( void );

	virtual void	Precache();

	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	UpdateOnRemove( void );
	virtual void	ItemHolsterFrame( void );
	virtual void	ItemPostFrame( void );
	virtual bool	Lower( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual void	WeaponIdle( void );
	void			DrainCharge( void ) override;
	void			AddCharge( float flAmount ) override;
	virtual void	WeaponReset( void );
	virtual void	AddUberRateBonusStack( float flBonus, int nStack = 1 );
	virtual int		GetUberRateBonusStacks( void )const;
	virtual float	GetUberRateBonus( void ) const;
	virtual void	BuildUberForTarget( CBaseEntity *pTarget, bool bMultiTarget = false ) override;
#ifdef GAME_DLL
	virtual void	UpdateBackpackMaxTargets( void );
	virtual void	AddBackpackPatient( CTFPlayer *pPlayer );
	virtual void	RemoveBackpackPatient( int iIndex );
	virtual void	CheckAndExpireStacks( void );
	void			UpdateBackpackTargets() { m_bBackpackTargetsParity = !m_bBackpackTargetsParity; };
	virtual void	ApplyBackpackHealing( void );
	virtual bool	IsBackpackPatient( CTFPlayer *pPlayer );
#endif

	virtual float	GetTargetRange( bool bBackpack = false );
	virtual float	GetStickRange( bool bBackpack = false );
	virtual float	GetHealRate( bool bMultiheal = false );
	virtual bool	AppliesModifier( void ) { return true; }
	int				GetMedigunType( void ) override;

	virtual ETFWeaponID	GetWeaponID( void ) const			{ return TF_WEAPON_MEDIGUN; }

	bool			IsReleasingCharge( void ) const override { return ( m_bChargeRelease && !m_bHolstered ); }
	medigun_charge_types GetChargeType( void );
	bool			AutoChargeOwner( void ) { return true; }

	CBaseEntity		*GetHealTarget( void ) { return m_hHealingTarget.Get(); }

	const char		*GetHealSound( void );

	// This method's only for non-mediguns really, so just leave it blank.
	void			OnHealedPlayer(CTFPlayer *pPatient, float flAmount, HealerType tType) override { }

#if defined( CLIENT_DLL )
	// Stop all sounds being output.
	void			StopHealSound( bool bStopHealingSound = true, bool bStopNoTargetSound = true );

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink();
	virtual void	ThirdPersonSwitch( bool bThirdperson );

	void			UpdateEffects( void );
	void			ForceHealingTargetUpdate( void ) { m_bUpdateHealingTargets = true; }

	void			ManageChargeEffect( void );

	void			UpdateMedicAutoCallers( void );

	void			UpdateBackpackTargets() { m_bUpdateBackpackTargets = true; };
	bool			ShouldShowEffectForPlayer( C_TFPlayer *pPlayer );
#else
	void			HealTargetThink( void );
#endif

	float			GetChargeLevel( void ) const override { return m_flChargeLevel; }

	virtual float	GetMinChargeAmount() const override { return 1.00f; } // live TF2: used for Vaccinator uber charge chunks

	CTFWeaponBase *GetWeapon( void ) override { return this; }

#ifdef GAME_DLL
	virtual bool TFNavMesh_ShouldRaiseCombatLevelWhenFired() OVERRIDE { return false; }
	virtual bool TFBot_IsCombatWeapon()                      OVERRIDE { return false; }
	virtual bool TFBot_IsQuietWeapon()                       OVERRIDE { return true; }
#endif

private:
	bool					FindAndHealTargets( void );
	virtual bool			HealingTarget( CBaseEntity *pTarget );
	bool					CouldHealTarget( CBaseEntity *pTarget );
	bool					AllowedToHealTarget( CBaseEntity *pTarget );

protected:
	virtual void			RemoveHealingTarget( bool bStopHealingSelf = false );
	virtual void			MaintainTargetInSlot();
	virtual void			FindNewTargetForSlot();


public:
	CNetworkHandle( CBaseEntity, m_hHealingTarget );
	//CUtlVector<EHANDLE> m_hBackpackTargets; // filled by m_qBackpackPatients when the latter is updated

protected:
	// Networked data.
	CNetworkVar( bool,		m_bHealing );
	CNetworkVar( bool,		m_bAttacking );

	CNetworkVar( bool,		m_bHolstered );
	CNetworkVar( bool,		m_bChargeRelease );
	CNetworkVar( float,		m_flChargeLevel );

	CNetworkVar( float,		m_flNextTargetCheckTime );
	CNetworkVar( bool,		m_bCanChangeTarget ); // Used to track the PrimaryAttack key being released for AutoHeal mode.

	CNetworkVar( int, m_nUberRateBonusStacks );			// How many stacks of increased uber build rate are allowed
	CNetworkVar( float, m_flUberRateBonus );			// The total uber build rate bonus

	CNetworkVar( bool, m_bBackpackTargetsParity );
	CNetworkVar( int, m_iMaxBackpackTargets );

	CNetworkArray( bool, m_bIsBPTargetActive, BP_MAX_LEN );

	struct targetdetachtimes_t
	{
		float	flTime;
		EHANDLE	hTarget;
	};
	CUtlVector<targetdetachtimes_t>		m_DetachedTargets; // Tracks times we last applied charge to a target.


#ifdef GAME_DLL
	CDamageModifier			m_DamageModifier;		// This attaches to whoever we're healing.
	bool					m_bHealingSelf;
	int						m_iPlayersHealedOld;
#endif

#ifdef CLIENT_DLL
	float					m_flNextBuzzTime;

	bool					m_bPlayingSound;
	bool					m_bUpdateHealingTargets;
	struct healingtargeteffects_t
	{
		EHANDLE				hOwner;
		C_BaseEntity		*pTarget;
		CNewParticleEffect	*pEffect;
	};
	struct overheadtargeteffects_t
	{
		C_BaseEntity		*pTarget;
		CNewParticleEffect	*pEffect;
		bool				bActiveEffect;
	};
	healingtargeteffects_t m_hHealingTargetEffect;

	float					m_flFlashCharge;
	bool					m_bOldChargeRelease;

	CNewParticleEffect		*m_pChargeEffect;
	EHANDLE					m_hChargeEffectHost;
	CSoundPatch				*m_pChargedSound;

	int						m_bPlayerHurt[MAX_PLAYERS + 1];

	bool					m_bUpdateBackpackTargets;
	bool					m_bOldBackpackTargetsParity;

	CUtlVector<healingtargeteffects_t> m_hBackpackTargetEffects;
	CUtlVector<overheadtargeteffects_t> m_hTargetOverheadEffects;
#endif

private:														
	CWeaponMedigun( const CWeaponMedigun & );
#ifdef GAME_DLL
public:
	const char* s_pszMedigunHealTargetThink = "MedigunHealTargetThink";
#endif
};

// Now make sure there isn't something other than team players in the way.
class CMedigunFilter : public CTraceFilterSimple
{
public:
	CMedigunFilter(CBaseEntity *pShooter) : CTraceFilterSimple(pShooter, COLLISION_GROUP_WEAPON)
	{
		m_pShooter = pShooter;
	}

	virtual bool ShouldHitEntity(IHandleEntity *pHandleEntity, int contentsMask)
	{
		// If it hit an edict the isn't the target and is on our team, then the ray is blocked.
		CBaseEntity *pEnt = static_cast<CBaseEntity*>(pHandleEntity);

		// Ignore collisions with the shooter
		if (pEnt == m_pShooter)
			return false;

		if (pEnt->GetTeam() == m_pShooter->GetTeam())
			return false;

		return CTraceFilterSimple::ShouldHitEntity(pHandleEntity, contentsMask);
	}

	CBaseEntity	*m_pShooter;
};

#endif // TF_WEAPON_MEDIGUN_H
