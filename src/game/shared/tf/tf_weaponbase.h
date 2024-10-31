//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//	Weapons.
//
//	CTFWeaponBase
//	|
//	|--> CTFWeaponBaseMelee
//	|		|
//	|		|--> CTFWeaponCrowbar
//	|		|--> CTFWeaponKnife
//	|		|--> CTFWeaponMedikit
//	|		|--> CTFWeaponWrench
//	|
//	|--> CTFWeaponBaseGrenade
//	|		|
//	|		|--> CTFWeapon
//	|		|--> CTFWeapon
//	|
//	|--> CTFWeaponBaseGun
//
//=============================================================================
#ifndef TF_WEAPONBASE_H
#define TF_WEAPONBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playeranimstate.h"
#include "tf_weapon_parse.h"
#include "takedamageinfo.h"
#include "npcevent.h"
#include "econ_item_system.h"
#include "tf_wearable.h"

// Client specific.
#if defined( CLIENT_DLL )
#define CTFWeaponBase C_TFWeaponBase
#include "tf_fx_muzzleflash.h"
#endif

#define WEAPON_RANDOM_RANGE 10000
#define MAX_TRACER_NAME		128

#define TF2C_TRANQ_DEPLOY_FACTOR	1.333
#define TF2C_HASTE_RELOAD_FACTOR	0.50f

CTFWeaponInfo *GetTFWeaponInfo( ETFWeaponID iWeapon );
CTFWeaponInfo *GetTFWeaponInfoForItem( CEconItemView *pItem, int iClass );

class CTFPlayer;
class CBaseObject;

void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, ITraceFilter *pFilter );

// Reloading singly.
enum
{
	TF_RELOAD_START = 0,
	TF_RELOADING,
	TF_RELOADING_CONTINUE,
	TF_RELOAD_FINISH
};

// structure to encapsulate state of head bob
struct BobState_t
{
	BobState_t() 
	{ 
		m_flBobTime = 0; 
		m_flLastBobTime = 0;
		m_flLastSpeed = 0;
		m_flVerticalBob = 0;
		m_flLateralBob = 0;
	}

	float m_flBobTime;
	float m_flLastBobTime;
	float m_flLastSpeed;
	float m_flVerticalBob;
	float m_flLateralBob;
};

struct viewmodel_acttable_t
{
	Activity      actBaseAct;
	Activity      actTargetAct;
	ETFWeaponType iWeaponRole;
};

#ifdef CLIENT_DLL
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState );
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState );
#endif

// Interface for weapons that have a charge time
class ITFChargeUpWeapon 
{
public:
	virtual bool CanCharge( void ) { return true; }
	virtual float GetChargeBeginTime( void ) = 0;
	virtual float GetChargeMaxTime( void ) = 0;
	virtual float GetCurrentCharge( void );
	virtual bool ChargeMeterShouldFlash(void) { return false; }

};

//=============================================================================
//
// Base TF Weapon Class
//
#ifdef CLIENT_DLL
class CTFWeaponBase : public CBaseCombatWeapon, public IHasOwner, public CGameEventListener
#else
class CTFWeaponBase : public CBaseCombatWeapon, public IHasOwner
#endif
{
	DECLARE_CLASS( CTFWeaponBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ENT_SCRIPTDESC( CTFWeaponBase );

	// Setup.
	CTFWeaponBase();
	virtual ~CTFWeaponBase();

	virtual void	Spawn();
	virtual void	Precache();
	virtual bool	IsPredicted() const	{ return true; }

	// Weapon Data
	CTFWeaponInfo const	&GetTFWpnData() const;
	virtual ETFWeaponID GetWeaponID( void ) const;
	bool			IsWeapon( ETFWeaponID iWeapon ) const;
	virtual float	GetFireRate( void );
	virtual bool	IsBursting( void );
	virtual int		GetDamageType() const { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual ETFDmgCustom GetCustomDamageType() const { return TF_DMG_CUSTOM_NONE; }
	virtual bool	IsMinigun( void ) const { return false; }
	virtual bool	IsSniperRifle( void ) const { return false; }
	virtual int		GetSlot( void );
	virtual int		GetPosition( void );

	// View Model
	virtual void	SetViewModel();
	virtual const char *GetViewModel( int iViewModel = 0 ) const;
	virtual const char *DetermineViewModelType( const char *pszViewModel ) const;
	virtual void	UpdateViewModel( void );
	CBaseViewModel	*GetPlayerViewModel( bool bObserverOK = false ) const;

	// World Model
	virtual const char *GetWorldModel( void ) const;

	virtual bool	CanHolster( void ) const;
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool	IsHolstered( void ) { return ( m_iState != WEAPON_IS_ACTIVE ); }
	virtual bool	Deploy( void );
	virtual void	GiveTo( CBaseEntity *pEntity );
	virtual void	Equip( CBaseCombatCharacter *pOwner );
#ifdef GAME_DLL
	virtual void	UnEquip( void );
#endif
	virtual void	Drop( const Vector &vecVelocity );
	bool			IsViewModelFlipped( void );

	// Additional functions for weapons to take advantage of.
	virtual bool	HealsWhenDropped( void ) { return false; }
	virtual bool	SetupTauntAttack( int &iTauntAttack, float &flTauntAttackTime ) { return false; }
	virtual bool	OwnerCanJump( void ) { return true; }
	virtual float	OwnerMaxSpeedModifier( void ) { return 0.0f; }
	virtual float	OwnerTakeDamage( const CTakeDamageInfo &info ) { return info.GetDamage(); }
	virtual void	OwnerConditionAdded( ETFCond nCond ) { }
	virtual void	OwnerConditionRemoved( ETFCond nCond ) { }

	virtual float	DeployedPoseHoldTime( void )	{ return 0.0f; }

	virtual void	ReapplyProvision( void );
	virtual void	OnActiveStateChanged( int iOldState );
	virtual void	UpdateOnRemove( void );

	virtual void IncrementKillCount( int iNum = 1 ) { m_iKillCount += Min(iNum, 255); }
	virtual int GetKillCount() const { return m_iKillCount; }
	virtual void SetKillCount( int iNum ) { m_iKillCount = Min(iNum, 255); }
	virtual void RemoveKillCount() { m_iKillCount = 0; }

	// Attacks
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual void	CalcIsAttackCritical( void );
	SERVERONLY_EXPORT virtual bool	CalcIsAttackCriticalHelper( void );
	bool			IsCurrentAttackACrit( void ) { return m_bCurrentAttackIsCrit; }
	SERVERONLY_EXPORT virtual bool	CanFireRandomCrit( void );
	virtual float	GetCritChance( void );
	virtual bool	CanHeadshot( void );

	// Ammo
	virtual int		GetMaxClip1( void ) const;
	virtual int		GetDefaultClip1( void ) const;
	virtual int		GetMaxAmmo( void );
	virtual bool	HasPrimaryAmmoToFire( void );

	// Reloads
	virtual bool	Reload( void );
	virtual	void	CheckReload( void );
	virtual void	FinishReload( void );
	virtual void	AbortReload( void );
	virtual bool	DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	virtual bool	CanAutoReload( void );
	virtual bool	ReloadOrSwitchWeapons( void );
	void			IncrementAmmo( void );
	bool			IsReloading( void ) { return m_iReloadMode != TF_RELOAD_START; }

	virtual void	InstantlyReload(int iAmount);

	virtual bool	CanDrop( void ) { return false; }

	// Fire Rate
	float			ApplyFireDelay( float flDelay ) const;

	// Sound
	void			PlayReloadSound( void );
	virtual const char *GetShootSound( int iIndex ) const;

	// Activities
	virtual void	ItemBusyFrame( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemHolsterFrame( void );

	virtual void	SetWeaponVisible( bool visible );
	virtual bool	WeaponShouldBeVisible( void );

	virtual ETFWeaponType GetActivityWeaponRole( void );

	virtual int		TranslateViewmodelHandActivity( int iActivity );
	virtual acttable_t *ActivityList_Custom( int &iActivityCount ) { return NULL; }
	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t s_acttablePrimary[];
	static acttable_t s_acttableSecondary[];
	static acttable_t s_acttableMelee[];
	static acttable_t s_acttableBuilding[];
	static acttable_t s_acttablePDA[];
	static acttable_t s_acttableItem1[];
	static acttable_t s_acttableItem2[];
	static acttable_t s_acttableMeleeAllClass[];
	static acttable_t s_acttableSecondary2[];
	static acttable_t s_acttablePrimary2[];
	static viewmodel_acttable_t s_viewmodelacttable[];

	// Utility.
	CBasePlayer		*GetPlayerOwner() const;
	CTFPlayer		*GetTFPlayerOwner() const;

	virtual CBaseEntity *GetOwnerViaInterface( void ) { return GetOwner(); }

#ifdef CLIENT_DLL
	bool			IsFirstPersonView( void );
	bool			UsingViewModel( void );
	C_BaseAnimating *GetAppropriateWorldOrViewModel();
	C_BaseAnimating	*GetWeaponForEffect();

	virtual void	UpdateExtraWearableVisibility();
#else
	virtual void	UpdateExtraWearable();
	virtual void	RemoveExtraWearable();
#endif

	virtual int		GetViewModelType( void ) { return m_iViewModelType; }
	virtual void	SetViewModelType( int iViewModelType ) { m_iViewModelType = iViewModelType; }

	bool CanAttack( void ) const;

	virtual void	WeaponIdle( void );

	virtual void	WeaponReset( void );
	virtual void	WeaponRegenerate( void );

	// Muzzleflashes
	virtual const char *GetMuzzleFlashModel( void );
	virtual float		GetMuzzleFlashModelLifetime( void );
	virtual float		GetMuzzleFlashModelScale( void );
	virtual const char *GetMuzzleFlashParticleEffect( void );

	virtual const char	*GetTracerType( void );
	virtual void        MakeTracer( const Vector &vecTracerSrc, const trace_t &tr );
	virtual int			GetTracerAttachment( void );

	float			GetLastPrimaryAttackTime( void ) const { return m_flLastPrimaryAttackTime; }
	virtual bool	CanFireAccurateShot( int nBulletsPerShot );

	bool			HideWhileStunned() { return true; }
	void			OnControlStunned( void );

#ifndef CLIENT_DLL
	// Server specific.
	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	// Spawning.
	virtual void	FallInit( void );
	virtual void	CheckRespawn( void );

	// Ammo.
	virtual const Vector &GetBulletSpread( void );

	// On hit effects.
	virtual void	ApplyOnHitAttributes( CBaseEntity *pAttacker, CBaseEntity *pBaseVictim, const CTakeDamageInfo &info );
	virtual void	ApplyPostHitEffects( const CTakeDamageInfo &inputInfo, CTFPlayer *pPlayer );
	virtual void	ApplyOnInjuredAttributes( CTFPlayer *pVictim, CTFPlayer *pAttacker, const CTakeDamageInfo &info ) {}

	// Disguise weapon.
	void			DisguiseWeaponThink( void );

#ifdef ITEM_TAUNTING
	void			UseForTaunt( CEconItemView *pItem );
	bool			IsUsedForTaunt( void ) { return ( m_nTauntModelIndex != -1 ); }
#endif
#else
	// Client specific.
	virtual void	CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt );
	const char*		GetBeamParticleName(const char* strDefaultName, bool bCritical = false);
	virtual int		InternalDrawModel( int flags );
	virtual bool	ShouldDraw( void );
	virtual void	UpdateVisibility();
	virtual ShadowType_t	ShadowCastType( void );
	virtual int		CalcOverrideModelIndex() OVERRIDE;

	virtual bool	ShouldPredict();
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	FireGameEvent( IGameEvent *event );
	virtual int		GetWorldModelIndex( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	Redraw( void );

	virtual const Vector& GetViewmodelOffset();
	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );
	virtual int		GetSkin( void );
	BobState_t		*GetBobState();

	virtual bool	OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual void	EjectBrass( void );
	void			EjectMagazine( const Vector &vecForce );
	void			UnhideMagazine( void );
	virtual CStudioHdr *OnNewModel( void );
	virtual void	ThirdPersonSwitch( bool bThirdperson );

	// ItemEffect Hud defaults
	virtual const char *GetEffectLabelText()	{ return ""; }
	virtual float		GetProgress()			{ return 0; }
	virtual int			GetCount()				{ return -1; }

	// Model muzzleflashes
	CHandle<C_MuzzleFlashModel>		m_hMuzzleFlashModel;
#endif

	// Effect / Regeneration bar handling
	virtual float	GetEffectBarProgress( void );			// Get the current bar state (will return a value from 0.0 to 1.0)
	bool			HasEffectBarRegeneration( void ) { return InternalGetEffectBarRechargeTime() > 0; }	// Check the base, not modified by attribute, because attrib may have reduced it to 0.
	float			GetEffectBarRechargeTime( void );
	void			DecrementBarRegenTime(float flTime, bool bCarryOver = false);

	virtual bool	UpdateBodygroups( CBasePlayer *pOwner, bool bForce );

	void			StartEffectBarRegen( bool bForceReset = true ); // Call this when you want your bar to start recharging (usually when you've deployed your action)

	bool			ShouldShowCritMeter(void);

#ifdef GAME_DLL // TFNavMesh/TFBot stuff
	virtual bool TFNavMesh_ShouldRaiseCombatLevelWhenFired() = 0;
	// Should firing this weapon raise the combat level of the surrounding areas in the nav mesh?
	// - return true in most cases
	// - return false if the weapon isn't really a "weapon" per se
	// - return false for other cases where firing the weapon should NOT increase the nearby perceived danger for TFBots
	// - TFBots use the combat level of nav areas to:
	//   - compute the safest path to their current destination
	//   - decide whether a given control point is safe to capture
	//   - decide when cloaking is necessary (for spies)
	// - ConVars governing this nav mesh combat system:
	//   - tf_nav_combat_build_rate
	//   - tf_nav_combat_decay_rate
	//   - tf_nav_combat_radius
	//   - tf_nav_in_combat_duration
	// - set tf_show_in_combat_areas 1 to visualize the current combat level of areas in the nav mesh

	virtual bool TFBot_IsCombatWeapon() = 0;
	// - if true, TFBots will shoot enemies with it to attack them
	// - if false, TFBots will NOT dodge (under circumstances in which they normally would)
	// - return false only for "weapons" that shouldn't be fired at enemy players (e.g. the medigun)

#if 0
	virtual bool TFBot_IsHitScanWeapon() = 0;
	// - if true, AND in MvM mode AND the bot is not a sniper, then they will attempt to avoid damage falloff by not
	//   firing it at enemies farther away than tf_bot_hitscan_range_limit (1800 HU)
#endif

	// TODO: rename this to better explain its purpose
	virtual bool TFBot_IsExplosiveProjectileWeapon() = 0;
	// - if true, TFBots won't fire it at point-blank range, to avoid taking self-damage

	virtual bool TFBot_IsContinuousFireWeapon() = 0;
	// - if true, TFBots will hold down the attack button continuously for at least tf_bot_fire_weapon_min_time (1 sec)
	// - if false, TFBots will tap-fire, hitting the attack button once for each shot

	virtual bool TFBot_IsBarrageAndReloadWeapon() = 0;
	// - if true, AND the TFBot either has attribute HoldFireUntilFullReload or tf_bot_always_full_reload is enabled,
	//   then when their clip becomes empty, they won't resume firing their weapon until it is COMPLETELY reloaded
	// - if true, AND the TFBot is Hard or Expert, then if the clip is down to 1 or below, they will tactically retreat
	//   behind cover to reload, and won't emerge from cover until COMPLETELY reloaded

	virtual bool TFBot_IsQuietWeapon() = 0;
	// - non-quiet weapons can be heard by TFBots 100% of the time if fired within tf_bot_notice_gunfire_range (3000 HU)
	// - quiet weapons can only be heard by TFBots within a smaller range, tf_bot_notice_quiet_gunfire_range (500 HU)
	// - TFBots also have a less-than-100% chance to notice quiet weapons being fired:
	//   90% (Expert) / 60% (Hard) / 30% (Normal) / 10% (Easy)
	// - whenever a non-quiet weapon is fired within 1000 HU of a TFBot, it "masks" that bot's ability to notice quiet
	//   weapons for the next 3 seconds; in practice, this means that the probability is cut in half:
	//   45% (Expert) / 30% (Hard) / 15% (Normal) / 5% (Easy)

	virtual bool TFBot_ShouldFireAtInvulnerableEnemies() = 0;
	// - if true, TFBots will fire the weapon at ubered enemies (for explosive knockback etc)
	// - if false, TFBots will NOT fire the weapon at ubered enemies (to avoid wasting ammunition)

	// TODO: fix this up so that it works properly with CTFCompoundBow and any other projectile-based headshot weapons
	virtual bool TFBot_ShouldAimForHeadshots() = 0;
	// - if true, the TFBot aim AI will go for headshots, instead of simply aiming for the center of enemies
	//   Hard/Expert: will aim directly for the enemy's head
	//   Normal:      will aim for a point 1/3 of the way down from the enemy's head to their center
	//   Easy:        will aim for the enemy's center

	// TODO: fix this up (or make multiple versions) so that it works properly for things other than just
	// CTFGrenadeLauncher/CTFPipebombLauncher that have arcing trajectories
	virtual bool TFBot_ShouldCompensateAimForGravity() = 0;
	// - if true, the TFBot aim AI will adjust upward to account for gravity's effect on the projectiles' trajectory

	// TODO: rename this to better explain its purpose (maybe)
	virtual bool TFBot_IsSniperRifle() = 0;
	// - if true, the TFBot's preferred attack range will change from 500 HU to infinity
	// - if false, the TFBot AI for the sniper class will be unable to scope and fire the weapon properly

	// TODO: rename this to better explain its purpose
	virtual bool TFBot_IsRocketLauncher() = 0;
	// - if true, AND the TFBot is Hard or Expert, their aim AI will take into account the rocket's travel time, and
	//   will also do intelligent stuff like shooting at enemies' feet in circumstances where it makes sense

	// TODO... other funcs
	// TODO: make sure all weapon classes have appropriate implementations (classgraph...)

#endif

	bool			IsFiringRapidFireStoredCrit(void);

	void			ApplyWeaponDamageBoostDuration(float flDuration);
	bool			IsWeaponDamageBoosted(void);
	float			GetWeaponDamageBoostTime(void) { return m_flDamageBoostTime; }

	bool			IsAmmoInfinite( void );

protected:
	// Reloads.
	void UpdateReloadTimers( bool bStart );
	void SetReloadTimer( float flReloadTime, float flRefillTime = 0.0f );
	bool ReloadSingly( void );
	virtual void OnReloadSinglyUpdate( void ) {}

	// Effect Bar Regeneration
	virtual int		GetEffectBarAmmo( void ) { return m_iPrimaryAmmoType; }
	virtual float	InternalGetEffectBarRechargeTime( void ) { // Time it takes for this regeneration bar to fully recharge from 0 to full.
		if ( !m_bUsesAmmoMeter )
			return 0;

		float flModifiedTime = GetTFWpnData().GetWeaponData( m_iWeaponMode ).m_flTimeReload;
		CALL_ATTRIB_HOOK_FLOAT( flModifiedTime, mult_reload_time );
		CALL_ATTRIB_HOOK_FLOAT( flModifiedTime, mult_reload_time_hidden );
		CALL_ATTRIB_HOOK_FLOAT( flModifiedTime, fast_reload );
		return flModifiedTime;
	}

	void			EffectBarRegenFinished( float flCarryOverTime = 0.0f );
	void			CheckEffectBarRegen( void );

private:
	CNetworkVar( float, m_flEffectBarRegenTime );								// The time Regen is scheduled to complete

protected:
	CNetworkVar( int, m_iWeaponMode );
	CNetworkVar( int, m_iReloadMode );
	//CNetworkVar( bool, m_bUnlimitedReserveAmmo );
	bool			m_bInAttack;
	bool			m_bInAttack2;
	bool			m_bCurrentAttackIsCrit;

	int				m_iReloadStartClipAmount;

	CNetworkVar( int, m_iKillCount );

	CNetworkVar( float, m_flCritTime );
	CNetworkVar( bool, m_bCritTimeIsStoredCrit );
	CNetworkVar( float,	m_flLastCritCheckTime );
	int				m_iLastCritCheckFrame;

	CNetworkVar(float, m_flDamageBoostTime);

	char			m_szTracerName[MAX_TRACER_NAME];

	CNetworkVar( bool, m_bResetParity );

#ifdef CLIENT_DLL
	bool m_bOldResetParity;

	KeyValues *m_pModelKeyValues;
	KeyValues *m_pModelWeaponData;

	bool m_bInitViewmodelOffset;
	Vector m_vecViewmodelOffset;
#endif

public:
	CNetworkVar( int, m_iViewModelType );
	CNetworkVar( float, m_flClipRefillTime );
	CNetworkVar( bool, m_bReloadedThroughAnimEvent );
	CNetworkVar( float, m_flLastPrimaryAttackTime );
	CNetworkVar( bool, m_bDisguiseWeapon );
	CNetworkVar( bool, m_bUsesAmmoMeter );

#ifdef GAME_DLL
	//For explosive bullets
	CBaseEntity		*GetEnemy(void) { return m_hEnemy; }
protected:
	CHandle<CBaseEntity>	m_hEnemy;
public:
#endif

#ifdef ITEM_TAUNTING
	CNetworkVar( int, m_nTauntModelIndex );
#endif

	CNetworkHandle( CTFWearable, m_hExtraWearable );
	CNetworkHandle( CTFWearableVM, m_hExtraWearableViewModel );

private:
	CTFWeaponBase( const CTFWeaponBase & );

};

class CTraceFilterIgnorePlayers : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnorePlayers, CTraceFilterSimple );

	CTraceFilterIgnorePlayers( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );
};

class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	DECLARE_CLASS(CTraceFilterIgnoreTeammates, CTraceFilterSimple);

	CTraceFilterIgnoreTeammates(const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam)
		: CTraceFilterSimple(passentity, collisionGroup), m_iIgnoreTeam(iIgnoreTeam)
	{
	}

	virtual bool ShouldHitEntity(IHandleEntity *pServerEntity, int contentsMask);

private:
	int m_iIgnoreTeam;
};

class CTraceFilterIgnoreTeammatesAndWorld : public CTraceFilterIgnoreTeammates
{
public:
	DECLARE_CLASS(CTraceFilterIgnoreTeammatesAndWorld, CTraceFilterIgnoreTeammates);

	CTraceFilterIgnoreTeammatesAndWorld(const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam)
		: CTraceFilterIgnoreTeammates(passentity, collisionGroup, iIgnoreTeam)
	{
	}

	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}

};

// used by sentries to still damage builder
// passentity takes priority over pExceptEntity
class CTraceFilterIgnoreTeammatesExceptEntity : public CTraceFilterSimple
{
public:
	DECLARE_CLASS(CTraceFilterIgnoreTeammatesExceptEntity, CTraceFilterSimple);

	CTraceFilterIgnoreTeammatesExceptEntity(const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam, const IHandleEntity *pExceptEntity)
		: CTraceFilterSimple(passentity, collisionGroup), m_iIgnoreTeam(iIgnoreTeam), m_pExceptEntity(pExceptEntity)
	{
	}

	virtual bool ShouldHitEntity(IHandleEntity *pServerEntity, int contentsMask);

private:
	int m_iIgnoreTeam;
	const IHandleEntity* m_pExceptEntity;
};

class CTraceFilterIgnoreEnemies : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreEnemies, CTraceFilterSimple );

	CTraceFilterIgnoreEnemies( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

private:
	int m_iIgnoreTeam;
};

class CTraceFilterIgnoreEnemiesExceptSpies : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreEnemies, CTraceFilterSimple );
	
	CTraceFilterIgnoreEnemiesExceptSpies( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam )
	: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam )
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );
	
private:
	int m_iIgnoreTeam;
};

class CTraceFilterIgnoreTeammatesAndProjectiles : public CTraceFilterIgnoreTeammates
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreTeammatesAndProjectiles, CTraceFilterIgnoreTeammates );

	CTraceFilterIgnoreTeammatesAndProjectiles( const IHandleEntity* passentity, int collisionGroup, int iIgnoreTeam )
		: CTraceFilterIgnoreTeammates( passentity, collisionGroup, iIgnoreTeam )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity* pServerEntity, int contentsMask )
	{
		CBaseEntity* pEntity = EntityFromEntityHandle( pServerEntity );
		if ( pEntity )
		{
			if ( pEntity->GetCollisionGroup() == TF_COLLISIONGROUP_GRENADES )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKETS_NOTSOLID )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_RESPAWNROOMS )
				return false;
		}

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

#define CREATE_SIMPLE_WEAPON_TABLE( WpnName, entityname )			\
																	\
	IMPLEMENT_NETWORKCLASS_ALIASED( WpnName, DT_##WpnName )	\
															\
	BEGIN_NETWORK_TABLE( C##WpnName, DT_##WpnName )			\
	END_NETWORK_TABLE()										\
															\
	BEGIN_PREDICTION_DATA( C##WpnName )						\
	END_PREDICTION_DATA()									\
															\
	LINK_ENTITY_TO_CLASS( entityname, C##WpnName );			\
	PRECACHE_WEAPON_REGISTER( entityname );
#endif // TF_WEAPONBASE_H
