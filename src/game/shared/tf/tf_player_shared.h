//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: Shared player code.
//
//=============================================================================
#ifndef TF_PLAYER_SHARED_H
#define TF_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"
#include "basegrenade_shared.h"
#include "tf_condition.h"

// Client specific.
#ifdef CLIENT_DLL
class C_TFPlayer;
// Server specific.
#else
class CTFPlayer;
#endif

//=============================================================================
//
// Tables.
//
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_TFPlayerShared );
#else
EXTERN_SEND_TABLE( DT_TFPlayerShared );
#endif

struct stun_struct_t
{
	CHandle<CTFPlayer> hPlayer;
	float flDuration;
	float flExpireTime;
	float flStartFadeTime;
	float flStunAmount;
	int	iStunFlags;
};

//=============================================================================

#define PERMANENT_CONDITION		-1

#define MOVEMENTSTUN_PARITY_BITS	2

// Damage storage for crit multiplier calculation
class CTFDamageEvent
{
public:
	float flDamage;
	float flTime;
	bool bKill;
};

enum
{
	STUN_PHASE_NONE,
	STUN_PHASE_LOOP,
	STUN_PHASE_END,
};

//=============================================================================
//
// Shared player class.
//
class CTFPlayerShared
{
public:

// Client specific.
#ifdef CLIENT_DLL

	friend class C_TFPlayer;
	typedef C_TFPlayer OuterClass;
	DECLARE_PREDICTABLE();

// Server specific.
#else

	friend class CTFPlayer;
	typedef CTFPlayer OuterClass;

#endif
	
	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerShared );

private:
	// Condition Provider tracking.
	CUtlVector<EHANDLE> m_vecProvider;

public:

	// Initialization.
	CTFPlayerShared();
	void				Init( OuterClass *pOuter );
	void				Spawn( void );

	// State (TF_STATE_*).
	int					GetState() const					{ return m_nPlayerState; }
	void				SetState( int nState )				{ m_nPlayerState = nState; }
	bool				InState( int nState ) const			{ return ( m_nPlayerState == nState ); }

	// Condition (TF_COND_*).
	SERVERONLY_EXPORT void	AddCond( ETFCond eCond, float flDuration = PERMANENT_CONDITION, CBaseEntity *pProvider = NULL );
	SERVERONLY_EXPORT void	RemoveCond( ETFCond eCond, bool ignore_duration=false );
	bool				InCond( ETFCond eCond ) const;
	bool				WasInCond( ETFCond eCond ) const;
	void				ForceRecondNextSync( ETFCond eCond );
	void				RemoveAllCond();
	void				OnConditionAdded( ETFCond eCond );
	void				OnConditionRemoved( ETFCond eCond );
	void				ConditionThink( void );
	float				GetConditionDuration( ETFCond eCond ) const;
	void				SetConditionDuration( ETFCond eCond, float flNewDur )
	{
		Assert( eCond < m_flCondExpireTimeLeft.Count() );
		m_flCondExpireTimeLeft.Set( eCond, flNewDur );
	}

	CBaseEntity			*GetConditionProvider( ETFCond eCond ) const;
	CBaseEntity			*GetConditionAssistFromAttacker( void );
	CBaseEntity			*GetConditionAssistFromVictim( void );

	bool				IsCritBoosted( void ) const;
	bool				IsInvulnerable( void ) const;
	bool				IsDisguised( void ) const;
	bool				IsStealthed( void ) const;
	bool				IsImmuneToPushback( void ) const;
	bool				IsMovementLocked( void );
	bool				CanBeDebuffed( void ) const;

	void				ConditionGameRulesThink( void );

	void				InvisibilityThink( void );

	int					GetMaxBuffedHealth( void ) const;
	bool				HealNegativeConds( void );

#ifdef CLIENT_DLL
	// This class only receives calls for these from C_TFPlayer, not
	// natively from the networking system
	virtual void		OnPreDataChanged( void );
	virtual void		OnDataChanged( void );

	// check the newly networked conditions for changes
	void				SyncConditions( int nCond, int nOldCond, int nUnused, int iOffset );
#endif

	SERVERONLY_EXPORT void	Disguise( int nTeam, int nClass );
	void				CompleteDisguise( void );
	SERVERONLY_EXPORT void	RemoveDisguise( void );
	void				RemoveDisguiseWeapon( void );
	void				FindDisguiseTarget( bool bFindOther = false );
	bool				DisguiseFoolsTeam( int iTeam ) const;
	bool				IsFooledByDisguise( int iTeam ) const;
	int					GetDisguiseTeam( void ) const;
	int					GetTrueDisguiseTeam( void ) const { return m_nDisguiseTeam; };
	int					GetDisguiseClass( void ) const 			{ return m_nDisguiseClass; }
	int					GetMaskClass( void ) const				{ return m_nMaskClass; }
	int					GetDesiredDisguiseClass( void ) const	{ return m_nDesiredDisguiseClass; }
	int					GetDesiredDisguiseTeam( void ) const	{ return m_nDesiredDisguiseTeam; }
	bool                GetLastDisguiseWasEnemyTeam( void ) const	{ return m_bLastDisguiseWasEnemyTeam; }
	EHANDLE				GetDisguiseTarget( void ) const;
	int					GetDisguiseTargetIndex( void ) const	{ return m_iDisguiseTargetIndex; }
	int					GetDisguiseHealth( void ) const			{ return m_iDisguiseHealth; }
	void				SetDisguiseHealth( int iDisguiseHealth );
	int					AddDisguiseHealth( int iHealthToAdd, bool bOverheal = false );
	int					GetDisguiseMaxHealth( void ) const		{ return m_iDisguiseMaxHealth; }
	int					GetDisguiseMaxBuffedHealth( void ) const;

	CTFWeaponBase		*GetDisguiseWeapon( void )		{ return m_hDisguiseWeapon; }

	void				DetermineDisguiseWeapon( bool bForcePrimary = false );
#ifdef GAME_DLL
	void				DetermineDisguiseWearables();
	void				RemoveDisguiseWearables();
#endif

	int					GetDisguiseBody( void ) const	{ return m_nDisguiseBody; }
	void				SetDisguiseBody( int iVal )		{ m_nDisguiseBody = iVal; }

	int					GetDisguiseAmmoClip( void ) { return m_iDisguiseClip; }
	void				SetDisguiseAmmoClip( int nValue ) { m_iDisguiseClip = nValue; }

	int					GetDisguiseAmmoCount( void ) { return m_iDisguiseAmmo; }
	void				SetDisguiseAmmoCount( int nValue ) { m_iDisguiseAmmo = nValue; }

#ifdef CLIENT_DLL
	void				OnDisguiseChanged( void );

	void				UpdateLoopingSounds( bool bDormant );
	void				UpdateCritBoostEffect( void );
#endif

#ifdef GAME_DLL
	void				Heal( CTFPlayer *pPlayer, float flAmount, CBaseObject *pDispenser = NULL, bool bRemoveOnMaxHealth = false, bool bCanCritHeal = true );
	void				HealNoOverheal( CTFPlayer *pPlayer, float flAmount, CBaseObject *pDispenser = NULL, bool bRemoveOnMaxHealth = false );
	void				HealTimed( CTFPlayer *pPlayer, float flAmount, float flDuration, bool bRefresh, bool bOverheal = false, CBaseObject *pDispenser = NULL );
	void				StopHealing( CTFPlayer *pPlayer, HealerType type );
	void				StopHealing( int iIndex );
	void				AddBurstHealer( CTFPlayer *pHealer, float flAmount, ETFCond condToApply = ETFCond::TF_COND_INVALID, float flCondApplyTime = 0.0f );
	void				RecalculateChargeEffects( bool bInstantRemove = false );
	int					FindHealerIndex( CTFPlayer *pPlayer, HealerType healType );
	EHANDLE				GetFirstHealer();
	CBaseEntity			*GetHealerByIndex( int iHealerIndex );
	bool				HealerIsDispenser( int iHealerIndex );
#endif
	int					GetNumHealers( void ) { return m_nNumHealers; }
	int					GetNumHumanHealers( void ) { return m_nNumHumanHealers; }

	SERVERONLY_EXPORT void Burn(CTFPlayer* pAttacker, CTFWeaponBase* pWeapon = NULL);
	void				MakeBleed( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon, float flBleedingTime, int nBleedDmg = TF_BLEEDING_DMG, bool bPermanentBleeding = false );
#ifdef GAME_DLL
	void				StopBleed( CTFPlayer *pPlayer, CTFWeaponBase *pWeapon );
#endif
	void				AirblastPlayer( CTFPlayer *pAttacker, const Vector &vecDir, float flSpeed );
	const Vector		&GetAirblastPosition( void ) { return m_vecAirblastPos.Get(); }

	bool				CheckShieldBash( void );
	void				StopCharge( bool bHitSomething = false );

	void				RecalculatePlayerBodygroups( bool bForce = false );
	void				RestorePlayerBodygroups( void );

	void				GetConditionsBits( CBitVec< TF_COND_LAST >& vbConditions ) const;

	bool				HasDemoShieldEquipped() const { return false; }

	// Weapons.
	CTFWeaponBase		*GetActiveTFWeapon() const;

	// Killstreaks
	virtual void IncrementPlayerKillCount( int iNum = 1 ) { m_iPlayerKillCount += Min(iNum, 255); }
	virtual int GetPlayerKillCount() const { return m_iPlayerKillCount; }
	virtual void SetPlayerKillCount( int iNum ) { m_iPlayerKillCount = Min(iNum, 255); }
	virtual void RemovePlayerKillCount() { m_iPlayerKillCount = 0; }

	// Utility.
	bool				IsLoser( void );

	void				FadeInvis( float flCloakRateScale, bool bNoAttack = false );
	float				GetPercentInvisible( void );
	void				NoteLastDamageTime( int nDamage );
	void				OnSpyTouchedByEnemy( void );
	float				GetLastStealthExposedTime( void ) { return m_flLastStealthExposeTime; }
	void				SetNextStealthTime( float flTime ) { m_flStealthNextChangeTime = flTime; }

	int					GetDesiredPlayerClassIndex( void ) const { return m_iDesiredPlayerClass; }

	int					GetDesiredWeaponIndex( void ) { return m_iDesiredWeaponID; }
	void				SetDesiredWeaponIndex( int iWeaponID ) { m_iDesiredWeaponID = iWeaponID; }

	void				UpdateCloakMeter( void );
	float				GetSpyCloakMeter() const { return m_flCloakMeter; }
	void				SetSpyCloakMeter( float val ) { m_flCloakMeter = val; }
	float				AddSpyCloak( float flAdd )
	{
		flAdd = Min( 100.0f - m_flCloakMeter, flAdd );
		m_flCloakMeter += flAdd;
		if ( m_flCloakMeter < 0.0f ) m_flCloakMeter = 0.0f;
		return flAdd;
	}

	float				GetNextStealthChangeTime() { return m_flStealthNextChangeTime; }
	void				SetNextInvisChangeCompleteTime( float fChangeTime ) { m_flInvisChangeCompleteTime = fChangeTime; }

	bool				IsJumping( void ) { return m_bJumping; }
	void				SetJumping( bool bJumping );
	bool				IsAirDashing( void ) { return m_bAirDash; }
	void				SetAirDash( bool bAirDash );
	int					GetAirDucks( void ) { return m_nAirDucked; }
	void				IncrementAirDucks( void );
	void				ResetAirDucks( void );
	bool				HasParachute() const;
	bool				CanParachute() const;
	bool				IsParachuting() const;

	void				DebugPrintConditions( void );

	float				GetStealthNoAttackExpireTime( void );

	void				SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated );
	bool				IsPlayerDominated( int iPlayerIndex );
	bool				IsPlayerDominatingMe( int iPlayerIndex );
	void				SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated );

	float						GetFlameRemoveTime( void ) { return m_flFlameRemoveTime; }
#ifdef GAME_DLL
	CHandle<CTFWeaponBase>		GetBurnWeapon(void) { return m_hBurnWeapon; }
	CHandle<CTFPlayer>			GetBurnAttacker(void) { return m_hBurnAttacker; }
#endif

	bool				IsCarryingObject( void ) { return m_bCarryingObject; }

#ifdef GAME_DLL
	void				SetCarriedObject( CBaseObject *pObj );
	CBaseObject			*GetCarriedObject( void );
#endif

	static int			CalculateObjectCost( CTFPlayer *pBuilder, int iObjectType );

	// Stuns
	stun_struct_t		*GetActiveStunInfo( void ) const;
#ifdef GAME_DLL
	void				StunPlayer( float flTime, float flReductionAmount, int iStunFlags = TF_STUN_MOVEMENT, CTFPlayer* pAttacker = NULL );
#endif // GAME_DLL
	float				GetAmountStunned( int iStunFlags );
	bool				IsLoserStateStunned( void ) const;
	bool				IsControlStunned( void );
	bool				IsSnared( void );
	CTFPlayer			*GetStunner( void );
	void				ControlStunFading( void );
	int					GetStunFlags( void ) const		{ return GetActiveStunInfo() ? GetActiveStunInfo()->iStunFlags : 0; }
	float				GetStunExpireTime( void ) const { return GetActiveStunInfo() ? GetActiveStunInfo()->flExpireTime : 0; }
	void				SetStunExpireTime( float flTime );
	void				UpdateLegacyStunSystem( void );

	CTFPlayer			*GetAssist( void ) const			{ return m_hAssist; }
	void				SetAssist( CTFPlayer *newAssist )	{ m_hAssist = newAssist; }

	void				SetCloakConsumeRate( float flCloakConsumeRate ) { m_flCloakConsumeRate = flCloakConsumeRate; }
	void				SetCloakRegenRate( float flCloakRegenRate ) { m_flCloakRegenRate = flCloakRegenRate; }

	int					GetTeleporterEffectColor( void ) { return m_nTeamTeleporterUsed; }
	void				SetTeleporterEffectColor( int iTeam ) { m_nTeamTeleporterUsed = iTeam; }
#ifdef CLIENT_DLL
	bool				ShouldShowRecentlyTeleported( void );
#endif

	int					GetSequenceForDeath( CBaseAnimating *pAnim, int iDamageCustom );

	void				AddAttributeToPlayer( char const *szName, float flValue );
	void				RemoveAttributeFromPlayer( char const *szName );

#ifndef CLIENT_DLL
	void				UpdateVIPBuff( void );
	void				PulseVIPBuff( CTFPlayer *pPulser );
	void				ResetVIPBuffPulse( void ) { m_flNextCivilianBuffCheckTime = 0.0f; }

	// Used for the TF_TRANQ_TORTURE achievement
	CUtlVector<CHandle<CTFPlayer>> m_aTranqAttackers;
#endif

	int					GetStoredCrits();
	int					GetStoredCritsCapacity();
	void				RemoveStoredCrits(int iAmount, bool bMissed = false);
	void				ResetStoredCrits();
	void				GainStoredCrits(int iAmount);
	
	// Projectile headshot changes
	static float		HeadshotFractionForClass(int iClassIndex);
	// Projectile bodyshot changes
	static float		ProjetileHurtCylinderForClass(int iClassIndex);

private:
	void				ImpactWaterTrace( trace_t &trace, const Vector &vecStart );

	void				OnAddStealthed( void );
	void				OnAddInvulnerable( void );
	void				OnAddTeleported(void);
	void				OnAddLaunched(void);
	void				OnAddBurning( void );
	void				OnAddShieldCharge( void );
	void				OnAddDisguising( void );
	void				OnAddDisguised( void );
	void				OnAddTaunting( void );
	void				OnAddStunned( void );
	void				OnAddBleeding( void );
	void				OnAddMarkedForDeath( void );
	void				OnAddMarkedForDeathSilent( void );
	void				OnAddHalloweenGiant( void );
	void				OnAddHalloweenTiny( void );
	void				OnAddSlowed( void );
	void				OnAddMeleeOnly( void );
	void				OnAddResistanceBuff( void );
	void				OnAddCivResistanceBoost( void );
	void				OnAddCivSpeedBoost( void );

	void				OnRemoveZoomed( void );
	void				OnRemoveBurning( void );
	void				OnRemoveShieldCharge( void );
	void				OnRemoveStealthed( void );
	void				OnRemoveDisguised( void );
	void				OnRemoveDisguising( void );
	void				OnRemoveInvulnerable( void );
	void				OnRemoveTeleported(void);
	void				OnRemoveLaunched(void);
	void				OnRemoveTaunting( void );
	void				OnRemoveStunned( void );
	void				OnRemoveBleeding( void );
	void				OnRemoveMarkedForDeath( void );
	void				OnRemoveMarkedForDeathSilent( void );
	void				OnRemoveHalloweenGiant( void );
	void				OnRemoveHalloweenTiny( void );
	void				OnRemoveSlowed( void );
	void				OnRemoveMeleeOnly( void );
	void				OnRemoveResistanceBuff( void );
	void				OnRemoveCivResistanceBoost( void );
	void				OnRemoveCivSpeedBoost( void );

	float				GetCritMult( void );

#ifdef GAME_DLL
	void				UpdateCritMult( void );
	void				RecordDamageEvent( const CTakeDamageInfo &info, bool bKill );
	void				ClearDamageEvents( void ) { m_DamageEvents.Purge(); }
	int					GetNumKillsInTime( float flTime );

	// Invulnerable.
	medigun_charge_types GetChargeEffectBeingProvided( CTFPlayer *pPlayer, bool bAutoSelfCheck = false );
	void				SetChargeEffect( medigun_charge_types chargeType, bool bShouldCharge, bool bInstantRemove, const MedigunEffects_t &chargeEffect, float flRemoveTime, CTFPlayer *pProvider );
	void				TestAndExpireChargeEffect( medigun_charge_types chargeType );
#endif

private:

	friend class CTFCondition_CritBoost;

	// Vars that are networked.
	CNetworkVar( int, m_nPlayerState );			// Player state.
	CNetworkVar( int, m_nPlayerCond );			// Player condition flags.
	// Ugh...
	CNetworkVar( int, m_nPlayerCondEx );
	CNetworkVar( int, m_nPlayerCondEx2 );
	CNetworkVar( int, m_nPlayerCondEx3 );
	CNetworkVar( int, m_nPlayerCondEx4 );

	CNetworkVarEmbedded( CTFConditionList, m_ConditionList );
	CNetworkArray( float, m_flCondExpireTimeLeft, TF_COND_LAST ); // Time until each condition expires
	CNetworkArray( int, m_nPreventedDamageFromCondition, TF_COND_LAST );
	CNetworkArray( bool, m_bPrevActive, TF_COND_LAST );

	CNetworkVar( int, m_nDisguiseTeam );		// Team spy is disguised as.
	CNetworkVar( int, m_nDisguiseClass );		// Class spy is disguised as.
	CNetworkVar( int, m_nMaskClass );			// Fake disguise class.
#ifdef GAME_DLL
	EHANDLE m_hDisguiseTarget;					// Playing the Spy is using for name disguise.
#endif // GAME_DLL
	CNetworkVar( int, m_iDisguiseTargetIndex );				// Player the spy is using for name disguise.
	CNetworkVar( int, m_iDisguiseHealth );		// Health to show our enemies in player id
	CNetworkVar( int, m_iDisguiseMaxHealth );
	CNetworkVar( float, m_flDisguiseChargeLevel );
	CNetworkVar( int, m_nDesiredDisguiseClass );
	CNetworkVar( int, m_nDesiredDisguiseTeam );
	bool m_bLastDisguiseWasEnemyTeam;

	CNetworkVar( float, m_flInvisibility );
	CNetworkVar( float, m_flInvisChangeCompleteTime );		// when uncloaking, must be done by this time
	float m_flLastStealthExposeTime;
	float m_flCloakConsumeRate;
	float m_flCloakRegenRate;

	CNetworkVar(int, m_iStoredCrits);

	CNetworkVar( int, m_nNumHealers );
	CNetworkVar( int, m_nNumHumanHealers );

	friend class CTFGameMovement;
	CNetworkVar( float, m_flChargeTooSlowTime );

	// Vars that are not networked.
	OuterClass			*m_pOuter;					// C_TFPlayer or CTFPlayer (client/server).

#ifdef GAME_DLL

	// Healer handling
	struct healers_t
	{
		EHANDLE				pPlayer;
		EHANDLE				pDispenser;

		float				flAmount;
		bool				bRemoveOnMaxHealth;

		float				iRecentAmount;
		float				flNextNofityTime;

		bool				bRemoveAfterOneFrame = false;
		bool				bRemoveAfterExpiry = false;
		float				flAutoRemoveTime;

		bool				bCanOverheal = false;
		bool				bCritHeals = false;

		HealerType			healerType = HEALER_TYPE_BEAM;

		// Optional condition to apply when this healer is removed.
		ETFCond				condApplyOnRemove = TF_COND_INVALID;
		float				condApplyTime = 0.0f;
	};
	CUtlVector<healers_t>	m_vecHealers;	
	float					m_flHealFraction;	// Store fractional health amounts
	float					m_flDisguiseHealFraction;	// Same for disguised healing

	float					m_flChargeOffTime[TF_CHARGE_COUNT];
	bool					m_bChargeSounds[TF_CHARGE_COUNT];
#endif

	// Burn handling
	CHandle<CTFPlayer>		m_hBurnAttacker;
	CHandle<CTFWeaponBase>	m_hBurnWeapon;
	float					m_flFlameBurnTime;
	CNetworkVar( float, m_flFlameRemoveTime );
	float					m_flTauntRemoveTime;

	// Bleeding
#ifdef GAME_DLL
	struct bleed_struct_t
	{
		CHandle<CTFPlayer>		hBleedingAttacker;
		CHandle<CTFWeaponBase>  hBleedingWeapon;
		float					flBleedingTime;
		float					flBleedingRemoveTime;
		int						nBleedDmg;
		bool					bPermanentBleeding;
	};
	CUtlVector <bleed_struct_t> m_PlayerBleeds;
#endif // GAME_DLL

	// Used by Baleful Beacon for storing remaining afterburn off killed players to heal off them
	// This is not a metal band name
#ifdef GAME_DLL
	struct deathburn_struct_t
	{
		float					flBurningNextTick;
		float					flBurningRemoveTime;
	};
	CUtlVector <deathburn_struct_t> m_PlayerDeathBurns;
public:
	void StoreNewRemainingAfterburn(float flRemoveTime, float flNextTick);
	void RemoveAllRemainingAfterburn();
private:
#endif // GAME_DLL

	float					m_flDisguiseCompleteTime;

	CNetworkVar( int, m_iDesiredPlayerClass );
	CNetworkVar( int, m_iDesiredWeaponID );

	float m_flNextBurningSound;

	CNetworkVar( int, m_nDisguiseBody );
	CNetworkVar( float, m_flCloakMeter );	// [0,100]
	CNetworkHandle( CTFWeaponBase, m_hDisguiseWeapon );
	CNetworkVar( int, m_iDisguiseClip );
	CNetworkVar( int, m_iDisguiseAmmo );

	CNetworkVar( bool, m_bJumping );
	CNetworkVar( bool, m_bAirDash );
	CNetworkVar( int, m_nAirDucked );

	CNetworkVar( bool, m_bScaredEffects ); // RE-DO

	CNetworkVar( float, m_flStealthNoAttackExpire );
	CNetworkVar( float, m_flStealthNextChangeTime );

	CNetworkVar( int, m_iCritMult );

	CNetworkArray( bool, m_bPlayerDominated, MAX_PLAYERS+1 );		// array of state per other player whether player is dominating other players
	CNetworkArray( bool, m_bPlayerDominatingMe, MAX_PLAYERS+1 );	// array of state per other player whether other players are dominating this player

	CNetworkVector( m_vecAirblastPos );

	CNetworkVar( float, m_flMovementStunTime );
	CNetworkVar( int, m_iMovementStunAmount );
	CNetworkVar( unsigned char, m_iMovementStunParity );
	CNetworkHandle( CTFPlayer, m_hStunner );
	CNetworkVar( int, m_iStunFlags );
	CNetworkVar( int, m_iStunIndex );
	
	CNetworkHandle( CBaseObject, m_hCarriedObject );
	CNetworkVar( bool, m_bCarryingObject );

	CHandle<CTFPlayer> m_hAssist;

	CNetworkVar( int, m_nTeamTeleporterUsed );

	CNetworkVar( float, m_flNextCivilianBuffCheckTime );

	CNetworkVar( int, m_iPlayerKillCount );

	// Was networked, but messed with the check on the client.
	// CNetworkVar( bool, m_bEmittedResistanceBuff );
	bool m_bEmittedResistanceBuff;

	CNetworkVar( int, m_nRestoreBody );
	CNetworkVar( int, m_nRestoreDisguiseBody );

#ifdef GAME_DLL
	float				m_flTranqStartTime;

	float				m_flNextCritUpdate;
	CUtlVector<CTFDamageEvent> m_DamageEvents;
#else
	// All of these CNewParticleEffects could be particle effect handlers
	// this is all very unsafe - Kay
	CNewParticleEffect	*m_pBurningEffect;
	CSoundPatch			*m_pBurningSound;

	CNewParticleEffect	*m_pDisguisingEffect;

	CNewParticleEffect	*m_pCritEffect;
	EHANDLE				m_hCritEffectHost;
	CSoundPatch			*m_pCritSound;

	CNewParticleEffect *m_pTranqEffect;
	CSoundPatch			*m_pTranqSound;

	CNewParticleEffect *m_pScaredEffect;

	CNewParticleEffect *m_pResistanceBuffEffect;
	CNewParticleEffect *m_pCivResistanceBoostEffect;

	int					m_nOldDisguiseClass;
	int					m_nOldDisguiseTeam;

	bool				m_bSyncingConditions;

	bool				m_bWasCritBoosted;
	float				m_flOldFlameRemoveTime;

	unsigned char		m_iOldMovementStunParity;
#endif

	int					m_nOldConditions;
	int					m_nOldConditionsEx;
	int					m_nOldConditionsEx2;
	int					m_nOldConditionsEx3;
	int					m_nOldConditionsEx4;

	int					m_nForceConditions;
	int					m_nForceConditionsEx;
	int					m_nForceConditionsEx2;
	int					m_nForceConditionsEx3;
	int					m_nForceConditionsEx4;

public:
	float				m_flStunFade;
	float				m_flStunEnd;
	float				m_flStunMid;
	int					m_iStunAnimState;
	int					m_iPhaseDamage;
	
	// Movement stun state.
	bool				m_bStunNeedsFadeOut;
	float				m_flStunLerpTarget;
	float				m_flLastMovementStunChange;
#ifdef GAME_DLL
	CUtlVector<stun_struct_t> m_PlayerStuns;
#else
	stun_struct_t		m_ActiveStunInfo;
#endif // CLIENT_DLL

};

#define CONTROL_STUN_ANIM_TIME	1.5f
#define STUN_ANIM_NONE	0
#define STUN_ANIM_LOOP	1
#define STUN_ANIM_END	2

extern const char *g_pszBDayGibs[22];

//-----------------------------------------------------------------------------
// This filter is used for player movement checks.
//-----------------------------------------------------------------------------
class CTraceFilterObject : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterObject, CTraceFilterSimple );

	CTraceFilterObject( const IHandleEntity *passentity, int collisionGroup, const IHandleEntity *pIgnoreOther = NULL ) :
		BaseClass( passentity, collisionGroup ),
		m_pIgnoreOther( pIgnoreOther )
	{
	}
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:
	const IHandleEntity *m_pIgnoreOther;
};

#endif // TF_PLAYER_SHARED_H
