//========= Copyright � 1996-2005, Valve LLC, All rights reserved. ============
//
//=============================================================================
#ifndef TF_PLAYER_H
#define TF_PLAYER_H

#pragma once

#include "basemultiplayerplayer.h"
#include "server_class.h"
#include "tf_playeranimstate.h"
#include "tf_shareddefs.h"
#include "tf_player_shared.h"
#include "tf_playerclass.h"
#include "tf_weapon_medigun.h"
#include "ihasattributes.h"
#include "tf_nav_area.h"
#include <tf_weapon_generator_uber.h>

class CTFPlayer;
class CTFTeam;
class CTFGoal;
class CTFGoalItem;
class CTFItem;
class CTFWeaponBuilder;
class CBaseObject;
class CTFWeaponBase;
class CTriggerAreaCapture;
class CCaptureFlag;
class CTFPlayerEquip;

//=============================================================================
//
// Player State Information
//
class CPlayerStateInfo
{
public:

	int				m_nPlayerState;
	const char		*m_pStateName;

	// Enter/Leave state.
	void ( CTFPlayer::*pfnEnterState )();	
	void ( CTFPlayer::*pfnLeaveState )();

	// Think (called every frame).
	void ( CTFPlayer::*pfnThink )();
};

struct DamagerHistory_t
{
	DamagerHistory_t()
	{
		hDamager = NULL;
		flTimeDamage = 0.0f;
		hWeapon = NULL;
	}
	EHANDLE hDamager;
	float flTimeDamage;
	EHANDLE hWeapon;
};
#define MAX_DAMAGER_HISTORY		2

struct DisguiseWeapon_t
{
	string_t iClassname;
	int iSlot;
	int iSubType;
	CEconItemView *pItem;
};

//=============================================================================
//
// TF Player
//
class CTFPlayer : public CBaseMultiplayerPlayer, public IHasAttributes
{
public:
	DECLARE_CLASS( CTFPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC( CTFPlayer );

	CTFPlayer();
	~CTFPlayer();

	friend class CTFGameMovement;

	// Creation/Destruction.
	static CTFPlayer	*CreatePlayer( const char *className, edict_t *ed );

	virtual void		Spawn();
	virtual int			ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual void		ForceRespawn();
	void				ForceRegenerateAndRespawn();
	void				CheckInstantLoadoutRespawn( void );
	virtual CBaseEntity	*EntSelectSpawnPoint( void );
	virtual void		InitialSpawn();
	virtual void		Precache();
	virtual bool		IsReadyToPlay( void );
	virtual bool		IsReadyToSpawn( void );
	virtual bool		ShouldGainInstantSpawn( void );
	virtual void		ResetPerRoundStats( void );
	virtual void		ResetScores( void );
	void				RemoveNemesisRelationships( bool bTeammatesOnly = false );
	virtual void		PlayerUse( void );

	virtual void		ApplyAbsVelocityImpulse( Vector const &impulse );
	bool				ApplyPunchImpulseX( float flPunch );

	void				CreateViewModel( int iViewModel = 0 );
	CBaseViewModel		*GetOffHandViewModel();
	void				SendOffHandViewModelActivity( Activity activity );

	virtual void		CheatImpulseCommands( int iImpulse );

	virtual void		CommitSuicide( bool bExplode = false, bool bForce = false );
	virtual void		CommitCustomSuicide( bool bExplode = false, bool bForce = false, ETFDmgCustom iCustomDamageType = TF_DMG_CUSTOM_SUICIDE, Vector vecForce = Vector( 0, 0, 0 ), bool bUseForceOverride = false );

	// Combats
	virtual void		TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual int			TakeHealth( float flHealth, int bitsDamageType, CTFPlayer *pHealer = NULL, bool bCritHealLogic = false );
	int					TakeDisguiseHealth( float flHealth, int bitsDamageType );
	virtual	void		Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );
	virtual void		Event_Killed( const CTakeDamageInfo &info );
	void				CleanupOnDeath( bool bDropItems = false );
	virtual bool		Event_Gibbed( const CTakeDamageInfo &info );
	virtual bool		BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	virtual void		PlayerDeathThink( void );
	virtual void		DetermineAssistForKill( const CTakeDamageInfo &info );

	int					GetMaxHealthForBuffing( void );
	virtual int			GetMaxHealth( void );

	virtual int			OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void				ApplyPushFromDamage( const CTakeDamageInfo &info, Vector vecDir );
	void				SetBlastJumpState( int iJumpType, bool bPlaySound = false );
	void				ClearBlastJumpState( void );
	int					GetBlastJumpFlags( void ) { return m_iBlastJumpState; }
	void				AddDamagerToHistory( CBaseEntity *pDamager, CBaseEntity *pWeapon = NULL );
	void				ClearDamagerHistory();
	//DamagerHistory_t	*GetDamagerHistory( int i ) { if ( i >= m_vecDamagerHistory.Count() ) return NULL; return &m_vecDamagerHistory[i]; }
	DamagerHistory_t	*GetRecentDamagerInHistory( CBaseEntity *pIgnore = NULL, float flMaxElapsed = -1.0f );
	virtual void		DamageEffect( float flDamage, int iDamageType );
	void				DamageEffect( CTakeDamageInfo &info );
	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;
	void				PlayDamageResistSound( float flStartDamage, float flModifiedDamage, const CUtlVector<CTFPlayer*>& vecPlayersPlayFullVolume);

	// For the Entrepreneur Achievement
	bool				PlayerHasDiedBefore( void ) { return m_bHasDied; };
	void				ResetPlayerHasDied(void) { m_bHasDied = false; };

	CTFWeaponBase		*GetActiveTFWeapon( void ) const;
	bool				IsActiveTFWeapon( ETFWeaponID iWeaponID ) const;

	CEconItemView		*GetLoadoutItem( int iClass, ETFLoadoutSlot iSlot );
	void				HandleCommand_ItemPreset( int iClass, int iSlotNum, int iPresetNum );

	CBaseEntity			*GiveNamedItem( const char *pszName, int iSubType = 0, CEconItemView *pItem = NULL, int iAmmo = TF_GIVEAMMO_NONE, bool bDisguiseWeapon = false );
	CBaseEntity			*GiveEconItem( const char *pszName, int iSubType = 0, int iAmmo = TF_GIVEAMMO_NONE );

	void				SaveMe( void );

	void				NoteWeaponFired( CTFWeaponBase *pWeapon );

	virtual void		OnMyWeaponFired( CBaseCombatWeapon *weapon );

	bool				HasItem( void ) const;					// Currently can have only one item at a time.
	void				SetItem( CTFItem *pItem );
	CTFItem				*GetItem( void ) const;
	CCaptureFlag		*GetTheFlag( void ) const;

	SERVERONLY_EXPORT void				Regenerate( void );
	void				Restock( bool bHealth, bool bAmmo );
	float				GetNextRegenTime( void ){ return m_flNextRegenerateTime; }
	void				SetNextRegenTime( float flTime ){ m_flNextRegenerateTime = flTime; }
	void				SetRegenerating( bool bRegen ) { m_bRegenerating = bRegen; }
	bool				IsRegenerating() const { return m_bRegenerating; }

	float				GetNextChangeClassTime( void ){ return m_flNextChangeClassTime; }
	void				SetNextChangeClassTime( float flTime ){ m_flNextChangeClassTime = flTime; }

	float				GetNextChangeTeamTime( void ){ return m_flNextChangeTeamTime; }
	void				SetNextChangeTeamTime( float flTime ){ m_flNextChangeTeamTime = flTime; }

	virtual	void		RemoveAllItems( bool removeSuit );
	virtual void		RemoveAllWeapons( void );

	void				DropFlag( void );
	void				DropRune( void ) {}

	// Class.
	CTFPlayerClass			*GetPlayerClass( void ) 				{ return &m_PlayerClass; }
	const CTFPlayerClass	*GetPlayerClass( void ) const			{ return &m_PlayerClass; }
	int					GetDesiredPlayerClassIndex( void ) const	{ return m_Shared.m_iDesiredPlayerClass; }
	void				SetDesiredPlayerClassIndex( int iClass )	{ m_Shared.m_iDesiredPlayerClass = iClass; }

	// Team.
	void				ForceChangeTeam( int iTeamNum );
	virtual void		ChangeTeam( int iTeamNum, bool bAutoTeam = false, bool bSilent = false );

	// mp_fadetoblack
	void				HandleFadeToBlack( void );

	// Flashlight controls for SFM - JasonM
	virtual int			FlashlightIsOn( void );
	virtual void		FlashlightTurnOn( void );
	virtual void		FlashlightTurnOff( void );

	// Think.
	virtual void		PreThink();
	virtual void		PostThink();
	virtual void		PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper );

	virtual void		ItemPostFrame();
	virtual void		HandleAnimEvent( animevent_t *pEvent );
	virtual void		Weapon_FrameUpdate( void );
	virtual void		Weapon_HandleAnimEvent( animevent_t *pEvent );
	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	virtual void		UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual const char	*GetOverrideStepSound( const char *pszBaseStepSoundName );
	virtual void		OnEmitFootstepSound( const CSoundParameters &params, const Vector &vecOrigin, float fVolume );

	// Utility.
	void				UpdateModel( void );
	void				UpdateSkin( int iTeam );

	void				SetCustomModel( char const *pszModel );
	void				SetCustomModelWithClassAnimations( char const *pszModel );
	void				SetCustomModelOffset( Vector const &vecOffs );
	void				SetCustomModelRotation( QAngle const &angRot );
	void				ClearCustomModelRotation( void );
	void				SetCustomModelRotates( bool bSet );
	void				SetCustomModelVisibleToSelf( bool bSet );

	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound = false );
	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound, EAmmoSource ammosource );
	virtual void		RemoveAmmo( int iCount, int iAmmoIndex );
	virtual int			GetAmmoCount( int iAmmoIndex ) const;
	int					GetMaxAmmo( int iAmmoIndex, bool bAddMissingClip = false, int iClassIndex = TF_CLASS_UNDEFINED ) const;
	bool				HasInfiniteAmmo( void ) const;

	bool				CanAttack( void );

	// Hacky and hardcoded, but there's no discernable function used for Live's speed boost multipliers.
	// In future it may be better to use values that are hardcoded to speeds instead of classes, and interpolate between them if needed.
	// This value is multiplied onto the classes' base speed.
	float				GetSpeedBoostMultiplier( void );

	// This passes the event to the client's and server's CPlayerAnimState.
	void				DoAnimationEvent( PlayerAnimEvent_t event, int mData = 0 );

	virtual bool		ClientCommand( const CCommand &args );

	bool				CanPickupBuilding( CBaseObject *pPickupObject );
	bool				TryToPickupBuilding( void );

	int					BuildObservableEntityList( void );
	virtual int			GetNextObserverSearchStartPoint( bool bReverse ); // Where we should start looping the player list in a FindNextObserverTarget call
	virtual CBaseEntity *FindNextObserverTarget( bool bReverse );
	virtual bool		IsValidObserverTarget( CBaseEntity * target ); // true, if player is allowed to see this target
	virtual void		JumptoPosition( const Vector &origin, const QAngle &angles );
	virtual bool		SetObserverTarget( CBaseEntity * target );
	virtual bool		ModeWantsSpectatorGUI( int iMode ) { return ( iMode != OBS_MODE_FREEZECAM && iMode != OBS_MODE_DEATHCAM ); }
	void				FindInitialObserverTarget( void );
	CBaseEntity		    *FindNearestObservableTarget( Vector vecOrigin, float flMaxDist );
	virtual void		ValidateCurrentObserverTarget( void );
	virtual void		CheckObserverSettings();

	virtual unsigned int PlayerSolidMask( bool brushOnly = false ) const;

	void CheckUncoveringSpies( CTFPlayer *pTouchedPlayer );
	void Touch( CBaseEntity *pOther );

	bool CanPlayerMove() const;
	void TeamFortress_SetSpeed();
	void TeamFortress_SetGravity();
	CTFPlayer *TeamFortress_GetDisguiseTarget( int nTeam, int nClass, bool bFindOther );

	void TeamFortress_ClientDisconnected();
	SERVERONLY_EXPORT void RemoveAllOwnedEntitiesFromWorld( bool bSilent = true );
	void RemoveOwnedProjectiles( void );
		
	void SetAnimation( PLAYER_ANIM playerAnim );

	bool IsPlayerClass( int iClass, bool bIgnoreRandomizer = false ) const;

	void PlayFlinch( const CTakeDamageInfo &info );

	void PlayCritReceivedSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );
	void StunSound( CTFPlayer* pAttacker, int iStunFlags, int iOldStunFlags = 0 );

	void SetSeeCrit( bool bAllSeeCrit, bool bMiniCrit, bool bShowDisguisedCrit ) { m_bAllSeeCrit = bAllSeeCrit; m_bMiniCrit = bMiniCrit; m_bShowDisguisedCrit = bShowDisguisedCrit;  }
	void SetAttackBonusEffect( EAttackBonusEffects_t effect ) { m_eBonusAttackEffect = effect; }
	EAttackBonusEffects_t GetAttackBonusEffect( void ) { return m_eBonusAttackEffect; }

	// TF doesn't want the explosion ringing sound
	virtual void			OnDamagedByExplosion( const CTakeDamageInfo &info ) { return; }

	void	OnBurnOther( CTFPlayer *pTFPlayerVictim );
	void	IgnitePlayer();
	void	ExtinguishPlayerBurning();

	void	BleedPlayer( float flDuration );
	void	BleedPlayerEx( float flDuration, int nBleedDamage, bool bPermanent, int );

	int GetBuildResources( void );
	void RemoveBuildResources( int iAmount );
	void AddBuildResources( int iAmount );

	int CanBuild( int iObjectType, int iObjectMode = 0 );

	CBaseObject	*GetObject( int index );
	CBaseObject *GetObjectOfType( int iObjectType, int iObjectMode = 0 );
	int	GetObjectCount( void );
	int GetNumObjects( int iObjectType, int iObjectMode = 0 );
	void RemoveAllObjects( bool bSilent );
	int	StartedBuildingObject( int iObjectType );
	void FinishedObject( CBaseObject *pObject );
	void AddObject( CBaseObject *pObject );
	void OwnedObjectDestroyed( CBaseObject *pObject );
	void RemoveObject( CBaseObject *pObject );
	bool PlayerOwnsObject( CBaseObject *pObject );
	void DetonateOwnedObjectsOfType( int iType, int iMode = 0, bool bIgnoreSapperState = false );
	void StartBuildingObjectOfType( int iType, int iMode = 0 );
	float GetObjectBuildSpeedMultiplier( int iObjectType, bool bIsRedeploy ) const;

	void OnSapperPlaced( CBaseEntity *pSappedObject ) { m_ctRecentSap.Start( 3.0f ); }
	bool IsPlacingSapper( void ) const { return !m_ctRecentSap.IsElapsed(); }
	void OnSapperStarted( float flStartTime );
	void OnSapperFinished( float flStartTime );
	bool IsSapping( void ) const { return m_bSapping; }
	int GetSappingState( void ) const { return m_iSapState; }
	void ResetSappingState( void );

	CTFTeam *GetTFTeam( void ) const;

	void TeleportEffect( int iTeam );
	void RemoveTeleportEffect();
	bool IsAllowedToPickUpFlag( void ) const;
	bool HasTheFlag( void ) const;

	// Death & Ragdolls.
	virtual void CreateRagdollEntity( void );
	void CreateRagdollEntity( int nFlags, float flInvisLevel, ETFDmgCustom iDamageCustom, int bitsDamageType = DMG_GENERIC );
	void DestroyRagdoll( void );
	void StopRagdollDeathAnim( void );
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 
	virtual bool ShouldGib( const CTakeDamageInfo &info );

	virtual void OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea );
	CTFNavArea *GetLastKnownTFArea() const { return static_cast<CTFNavArea *>( GetLastKnownArea() ); }

	// Dropping Ammo
	void DropAmmoPack( void );
	void DropHealthPack( void );

	bool CanDisguise( void );
	bool CanGoInvisible( void );
	void RemoveInvisibility( void );
	void RemoveDisguise( void );

	bool DoClassSpecialSkill( void );

	float GetLastDamageReceivedTime( void ) { return m_flLastDamageTime; }

	void SetClassMenuOpen( bool bIsOpen );
	bool IsClassMenuOpen( void );

	float GetCritMult( void ) { return m_Shared.GetCritMult(); }
	void RecordDamageEvent( const CTakeDamageInfo &info, bool bKill ) { m_Shared.RecordDamageEvent( info, bKill ); }

	bool GetHudClassAutoKill( void ){ return m_bHudClassAutoKill; }
	void SetHudClassAutoKill( bool bAutoKill ){ m_bHudClassAutoKill = bAutoKill; }

	bool GetMedigunAutoHeal( void ){ return m_bMedigunAutoHeal; }
	void SetMedigunAutoHeal( bool bMedigunAutoHeal ){ m_bMedigunAutoHeal = bMedigunAutoHeal; }

	bool ShouldAutoRezoom( void ) { return m_bAutoRezoom; }
	void SetAutoRezoom( bool bAutoRezoom ) { m_bAutoRezoom = bAutoRezoom; }

	bool ShouldAutoReload( void ) { return m_bAutoReload; }
	void SetAutoReload( bool bAutoReload ) { m_bAutoReload = bAutoReload; }

	void SetRememberActiveWeapon( bool bEnabled ) { m_bRememberActiveWeapon = bEnabled; }
	void SetRememberLastWeapon( bool bEnabled ) { m_bRememberLastWeapon = bEnabled; }

	bool ShouldFlipViewModel( void ) { return m_bFlipViewModel; }
	void SetFlipViewModel( bool bFlip ) { m_bFlipViewModel = bFlip; }

	bool ShouldUseProximityVoice( void ) { return m_bProximityVoice; }
	void SetProximityVoice( bool bEnable ) { m_bProximityVoice = bEnable; }

	float GetViewModelFOV( void ) { return m_flViewModelFOV; }
	void SetViewModelFOV( float flVal ) { m_flViewModelFOV = flVal; }

	const Vector &GetViewModelOffset( void ) { return m_vecViewModelOffset; }
	void SetViewModelOffset( const Vector &vecOffset ) { m_vecViewModelOffset = vecOffset; }

	bool ShouldUseMinimizedViewModels( void ) { return m_bMinimizedViewModels; }
	void SetMinimizedViewModels( bool bEnable ) { m_bMinimizedViewModels = bEnable; }

	bool ShouldHoldToZoom( void ) { return m_bHoldZoomSniper; }
	void SetSniperHoldToZoom( bool bEnable ) { m_bHoldZoomSniper = bEnable; }

	bool ShouldHaveInvisibleArms( void ) { return m_bInvisibleArms; }
	void SetShouldHaveInvisibleArms(bool bEnable) { m_bInvisibleArms = bEnable; }

	bool FixedSpreadPreference(void) { return m_bFixedSpreadPreference; }
	void SetFixedSpreadPreference(bool bFixedSpreadPreference) { m_bFixedSpreadPreference = bFixedSpreadPreference; }

	bool AvoidBecomingVIP(void) { return m_bAvoidBecomingVIP; }
	void SetAvoidBecomingVIP(bool bAvoidBecomingVIP) { m_bAvoidBecomingVIP = bAvoidBecomingVIP; }

	bool IsSpywalkInverted(void) { return m_bSpywalkInverted; }
	void SetSpywalkInverted(bool bInverted) { m_bSpywalkInverted = bInverted; }

	bool CenterFirePreference(void) { return m_bCenterFirePreference; }
	void SetCenterFirePreference(bool bCenterFire) { m_bCenterFirePreference = bCenterFire; }

	virtual void	ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet );

	virtual bool CanHearAndReadChatFrom( CBasePlayer *pPlayer );
	virtual bool CanBeAutobalanced( void );

	Vector 	GetClassEyeHeight( void );

	void	UpdateExpression( void );
	void	ClearExpression( void );

	virtual IResponseSystem *GetResponseSystem();
	virtual bool			SpeakConceptIfAllowed( int iConcept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	virtual bool CanSpeakVoiceCommand( void );
	virtual bool ShouldShowVoiceSubtitleToEnemy( void );
	virtual void NoteSpokeVoiceCommand( const char *pszScenePlayed );
	void	SpeakWeaponFire( int iCustomConcept = MP_CONCEPT_NONE );
	void	ClearWeaponFireScene( void );
	void	FiringTalk() { SpeakWeaponFire(); }

	void CreateDisguiseWeaponList( CTFPlayer *pDisguiseTarget );
	void ClearDisguiseWeaponList();

	virtual int DrawDebugTextOverlays( void );

	virtual int	CalculateTeamBalanceScore( void );
	void CalculateTeamScrambleScore( void );
	float GetTeamScrambleScore( void ) { return m_flTeamScrambleScore; }

	bool ShouldAnnouceAchievement( void );
	virtual void OnAchievementEarned( int iAchievement );

	virtual bool IsDeflectable( void ) { return true; }

	int GetNumberOfDominations( void ) { return m_iDominations; }
	void UpdateDominationsCount( void );

	bool GetClientConVarBoolValue( const char *pszValue );
	int GetClientConVarIntValue( const char *pszValue );
	float GetClientConVarFloatValue( const char *pszValue );

	void	UpdateAnimState( void ) { m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] ); }

	bool	IsEnemy( const CBaseEntity *pEntity ) const;

	bool CanBecomeVIP( void );
	void BecomeVIP( void );
	void ReturnFromVIP( void );
	void ResetWasVIP( void ) { m_bWasVIP = false; }
	bool IsVIP( void ) const;

	bool IsAirborne (float flMinHeight = 0.0f);
	bool IsWet (void);

	CTFPlayer *GetObservedPlayer( bool bFirstPerson );
	virtual Vector Weapon_ShootPosition( void );
	void SetEyeAngles( const QAngle &angles );

	float GetSpectatorSwitchTime( void ) { return m_flSpectatorTime; }

	float GetTimeSinceLastThink( void ) const { return ( m_flLastThinkTime >= 0.0f ) ? gpGlobals->curtime - m_flLastThinkTime : -1.0f; }
	float GetRespawnTimeOverride( void ) { return m_flRespawnTimeOverride; }

	bool CanJump() const;
	bool CanDuck() const;
	bool CanAirDash( void ) const;
	virtual bool CanBreatheUnderwater() const;
	bool CanGetWet() const;
	bool IsFireproof() const;

	bool InAirDueToExplosion( void );
	bool InAirDueToKnockback( void );

	// Entity inputs
	void	InputIgnitePlayer( inputdata_t &inputdata );
	void	InputSetCustomModel( inputdata_t &inputdata );
	void	InputSetCustomModelOffset( inputdata_t &inputdata );
	void	InputSetCustomModelRotation( inputdata_t &inputdata );
	void	InputClearCustomModelRotation( inputdata_t &inputdata );
	void	InputSetCustomModelRotates( inputdata_t &inputdata );
	void	InputSetCustomModelVisibleToSelf( inputdata_t &inputdata );
	void	InputExtinguishPlayer( inputdata_t &inputdata );
	void	InputBleedPlayer( inputdata_t &inputdata );
	void	InputSpeakResponseConcept( inputdata_t &inputdata );
	void	InputSetForcedTauntCam( inputdata_t &inputdata );

	virtual CAttributeManager *GetAttributeManager( void ) { return &m_AttributeManager; }
	virtual CAttributeContainer *GetAttributeContainer( void ) { return NULL; }
	virtual CBaseEntity *GetAttributeOwner( void ) { return NULL; }
	virtual CAttributeList *GetAttributeList( void ) { return &m_AttributeList; }
	virtual void ReapplyProvision( void ) { /*Do nothing*/ };

	void	AddCustomAttribute( char const *pszAttribute, float flValue, float flDuration = -1.0f );
	void	RemoveCustomAttribute( char const *pszAttribute );
	void	RemoveAllCustomAttributes();

	float HealthFractionBuffed() const;

public:
	friend class CTFPlayerShared;
	CTFPlayerShared m_Shared;

	float				m_flNextNameChangeTime;
	float				m_flNextTimeCheck;		// Next time the player can execute a "timeleft" command

	int					StateGet( void ) const;

	void				SetOffHandWeapon( CTFWeaponBase *pWeapon );
	void				HolsterOffHandWeapon( void );

	float				GetSpawnTime() { return m_flSpawnTime; }

	virtual bool		Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual void		Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity );

	void				GiveDefaultItems();
	bool				ItemsMatch( CEconItemView *pItem1, CEconItemView *pItem2, CTFWeaponBase *pWeapon );
	bool				ItemIsAllowed( CEconItemView *pItem );
	int					ItemIsExperimental( CEconItemView *pItem );
	void				ValidateWeapons( bool bRegenerate );
	void				ValidateWearables( void );
	void				ManageRegularWeapons( void );
	void				ManageRegularWeaponsLegacy( TFPlayerClassData_t *pData );
	void				ManageRandomWeapons( void );
	void				ManageBuilderWeapons( TFPlayerClassData_t *pData );
	void				RememberLastWeapons( void );
	void				ForgetLastWeapons( void );

	void				PostInventoryApplication( void );

#ifdef ITEM_TAUNTING
	enum
	{
		TAUNT_PARTNER_OK,
		TAUNT_PARTNER_BLOCKED,
		TAUNT_PARTNER_TOO_HIGH,
	};
#endif

	// Taunts.
	bool				IsAllowedToTaunt( void );
	void				Taunt( void );
	void				StopTaunt( void );
#ifdef ITEM_TAUNTING
	void				StopTauntSoundLoop( void );
#endif
	void				PlayTauntScene( const char *pszScene );
#ifdef ITEM_TAUNTING
	void				PlayTauntSceneFromItem( CEconItemView *pItem );
#endif
	void				OnTauntSucceeded( const char *pszScene, bool bHoldTaunt = false, bool bParnerTaunt = false );
	float				GetTauntRemoveTime( void ) const { return m_Shared.m_flTauntRemoveTime; }
#ifdef ITEM_TAUNTING
	void				EndLongTaunt( void );
	bool				IsPartnerTauntReady( void ) { return m_bIsReadyToHighFive; }
	void				SetPartnerTauntReady( bool bReady ) { m_bIsReadyToHighFive = bReady; }
#endif
	bool				IsTaunting( void ) { return m_Shared.InCond( TF_COND_TAUNTING ); }
#ifdef ITEM_TAUNTING
	CTFPlayer			*FindPartnerTauntInitiator( void );
	int					GetTauntPartnerPosition( CEconItemDefinition *pItemDef, Vector &vecPos, QAngle &vecAngles, unsigned int nMask, CTFPlayer *pPartner = NULL );
	void				AcceptTauntWithPartner( CTFPlayer *pPlayer );
	CEconItemView		*GetTauntItem( void ) { return m_pTauntItem; }
#endif
	void				DoTauntAttack( void );
	void				ClearTauntAttack( void );
	int					GetTauntAttackType( void ) { return m_iTauntAttack; }
	void				SetupTauntAttack( int iType, float flTime ) { m_iTauntAttack = iType; m_flTauntAttackTime = gpGlobals->curtime + flTime; }
#ifdef ITEM_TAUNTING
	void				SetTauntPartner( CTFPlayer *pPlayer ) { m_hTauntPartner = pPlayer; }
#endif
	void				SetForcedTauntCam( int nForce ) { m_nForceTauntCam = nForce; }
	float				GetCurrentTauntMoveSpeed() const { return 0.0f; }
	void				SetCurrentTauntMoveSpeed( float flSpeed ) {}

	virtual float		PlayScene( const char *pszScene, float flDelay = 0.0f, AI_Response *response = NULL, IRecipientFilter *filter = NULL );
	virtual bool		StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	virtual	bool		ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	void				SetDeathFlags( int iDeathFlags ) { m_iDeathFlags = iDeathFlags; }
	int					GetDeathFlags() { return m_iDeathFlags; }
	void				SetMaxSentryKills( int iMaxSentryKills ) { m_iMaxSentryKills = iMaxSentryKills; }
	int					GetMaxSentryKills() { return m_iMaxSentryKills; }
	
	void				CheckForIdle( void );
	void				PickWelcomeObserverPoint();

	void				StopRandomExpressions( void ) { m_flNextRandomExpressionTime = -1; }
	void				StartRandomExpressions( void ) { m_flNextRandomExpressionTime = gpGlobals->curtime; }

	virtual bool		WantsLagCompensationOnEntity( const CBasePlayer	*pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;

	bool				CanShowClassMenu( void );
	float				MedicGetChargeLevel( void );
	CBaseEntity			*MedicGetHealTarget( void );

	ITFHealingWeapon	*GetMedigun( void ) const;
	CTFWeaponBase		*GetTFWeapon( int iWeapon ) const { return static_cast<CTFWeaponBase *>( GetWeapon( iWeapon ) ); }
	CTFWeaponBase		*GetTFWeaponBySlot( int iBucket ) const { return static_cast<CTFWeaponBase *>( Weapon_GetSlot( iBucket ) ); }
	CTFWeaponBase		*Weapon_OwnsThisID( ETFWeaponID iWeaponID ) const;
	CTFWeaponBase		*Weapon_GetWeaponByType( ETFWeaponType iType ) const;
	CTFWearable			*Wearable_OwnsThisID( int iWearableID ) const;
	CEconEntity			*GetEntityForLoadoutSlot( ETFLoadoutSlot iSlot ) const;
	CEconWearable		*GetWearableForLoadoutSlot( ETFLoadoutSlot iSlot ) const;
	CTFWeaponBase		*GetWeaponForLoadoutSlot( ETFLoadoutSlot iSlot ) const;

	bool				CalculateAmmoPackPositionAndAngles( CTFWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles );

	CTriggerAreaCapture	*GetControlPointStandingOn( void );
	bool				IsCapturingPoint( void );

	float				GetTimeSinceLastInjuryByAnyEnemyTeam() const;

	bool				IsArenaSpectator() const { return m_bArenaSpectator; }
	float				GetArenaBalanceTime() const { return m_flArenaBalanceTime; }
	void				SetArenaBalanceTime() { m_flArenaBalanceTime = gpGlobals->curtime; }

	int					GetLastTeamNumber() const { return m_iLastTeamNum; }

	int					GetCurrency() const { return m_nCurrency; }
	void				SetCurrency( int iCurrency ) { m_nCurrency = iCurrency; }
	void				AddCurrency( int iAmount ) { m_nCurrency += iAmount; }
	void				RemoveCurrency( int iAmount ) { m_nCurrency -= iAmount; }

	bool				IsMiniBoss() const { return m_bIsMiniBoss; }
	void				SetIsMiniBoss( bool bSet ) { m_bIsMiniBoss = bSet; }

	int					GetTFBotSkill() const    { return m_nBotSkill;  }
	void				SetTFBotSkill( int skill ) { m_nBotSkill = skill; }

	bool				IsBasicBot() const  { return m_bIsBasicBot; }
	void				SetAsBasicBot()     { m_bIsBasicBot = true; }

	IntervalTimer		&GetMedicCallTimer() { return m_itMedicCall; }
	bool				IsCallingForMedic() const { return m_itMedicCall.HasStarted() && m_itMedicCall.IsLessThan( 5.0 ); }
	float				GetTimeSinceCalledForMedic() const { return m_itMedicCall.GetElapsedTime(); }

	CountdownTimer		&GetRecentSapTimer() { return m_ctRecentSap; }

	void				LockViewAngles( void ) { m_iLockViewanglesTickNumber = gpGlobals->tickcount; m_qangLockViewangles = pl.v_angle; }

	bool				IsNextAttackMinicrit(){ return m_bNextAttackIsMinicrit; }
	void				SetNextAttackMinicrit( bool bNextAttackIsMinicrit ){  m_bNextAttackIsMinicrit = bNextAttackIsMinicrit; }
	
	void				UpdateJumppadTrailEffect(void);

	void				PlayTickTockSound(void); // For Beacon

	// ----------------------------------------------------------------------------
	// VScript accessors
	// ----------------------------------------------------------------------------
	HSCRIPT ScriptGetActiveWeapon();
	HSCRIPT ScriptGetHealTarget();

	void ScriptAddCond( int cond );
	void ScriptAddCondEx( int cond, float flDuration, HSCRIPT hProvider );
	void ScriptRemoveCond( int cond );
	void ScriptRemoveCondEx( int cond, bool bIgnoreDuration );
	bool ScriptInCond( int cond );
	bool ScriptWasInCond( int cond );
	void ScriptRemoveAllCond();
	float ScriptGetCondDuration( int cond );
	void ScriptSetCondDuration( int cond, float flDuration );

	HSCRIPT ScriptGetDisguiseTarget();
	int ScriptGetDisguiseAmmoCount();
	void ScriptSetDisguiseAmmoCount( int nCount );
	int ScriptGetDisguiseTeam();

	bool ScriptIsCarryingRune();
	bool ScriptIsCritBoosted();
	bool ScriptIsInvulnerable();
	bool ScriptIsStealthed();
	bool ScriptCanBeDebuffed();
	bool ScriptIsImmuneToPushback();

	bool ScriptIsFullyInvisible();
	float ScriptGetSpyCloakMeter();
	void ScriptSetSpyCloakMeter( float flMeter );

	bool ScriptIsRageDraining();
	float ScriptGetRageMeter();
	void ScriptSetRageMeter( float flMeter );

	bool ScriptIsHypeBuffed();
	float ScriptGetScoutHypeMeter();
	void ScriptSetScoutHypeMeter( float flMeter );

	bool ScriptIsJumping();
	bool ScriptIsAirDashing();
	bool ScriptIsControlStunned();
	bool ScriptIsSnared();

	int ScriptGetCaptures();
	int ScriptGetDefenses();
	int ScriptGetDominations();
	int ScriptGetRevenge();
	int ScriptGetBuildingsDestroyed();
	int ScriptGetHeadshots();
	int ScriptGetBackstabs();
	int ScriptGetHealPoints();
	int ScriptGetInvulns();
	int ScriptGetTeleports();
	int ScriptGetResupplyPoints();
	int ScriptGetKillAssists();
	int ScriptGetBonusPoints();
	void ScriptResetScores();

	bool ScriptIsParachuteEquipped();

	int ScriptGetPlayerClass();
	void ScriptSetPlayerClass( int nClass );

	void ScriptRemoveAllItems( bool removeSuit );

	Vector ScriptWeapon_ShootPosition();
	bool ScriptWeapon_CanUse( HSCRIPT hWeapon );
	void ScriptWeapon_Equip( HSCRIPT hWeapon );
	void ScriptWeapon_Drop( HSCRIPT hWeapon );
	void ScriptWeapon_DropEx( HSCRIPT hWeapon, Vector vecDir, Vector vecVelocity );
	void ScriptWeapon_Switch( HSCRIPT hWeapon );
	void ScriptWeapon_SetLast( HSCRIPT hWeapon );
	HSCRIPT ScriptGetLastWeapon();

	void ScriptEquipWearableViewModel( HSCRIPT hViewModel );
	
	bool ScriptIsFakeClient();
	int ScriptGetBotType();
	bool ScriptIsBotOfType( int botType );

	// Stubs
	HSCRIPT GetGrapplingHookTarget() { return NULL; }
	void SetGrapplingHookTarget( HSCRIPT hTarget, bool a2 ) {}
	void SetUseBossHealthBar( bool bSet ) {}
	void GrantOrRemoveAllUpgrades() {}
	bool IsViewingCYOAPDA() const { return false; }
	bool IsUsingActionSlot() const { return false; }
	bool IsInspecting() const { return false; }
	void RollRareSpell() {}
	void ClearSpells() {}

private:
	// Creation/Destruction.
	void				InitClass( void );
	bool				SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot );
	void				PrecachePlayerModels( void );
	bool				CanInstantlyRespawn( void );

	// Think.
	void				TFPlayerThink();
	void				UpdateTimers( void );

	void				RegenThink();
	float				m_flTotalHealthRegen;
	float				m_flTotalDisguiseHealthRegen;
	float				m_flLastRegenTime;

	// Taunt.
	EHANDLE				m_hTauntScene;
	bool				m_bInitTaunt;
#ifdef ITEM_TAUNTING
	bool				m_bIsReadyToHighFive;
	bool				m_bHoldingTaunt;
	CEconItemView		*m_pTauntItem;
	EHANDLE				m_hTauntProp;
	CHandle<CTFPlayer>	m_hTauntPartner;
	bool				m_bPlayingTauntSound;
#endif

	// Client commands.
public:
	int					GetAutoTeam( void );
	
	void				HandleCommand_JoinTeam( const char *pTeamName );
	void				HandleCommand_JoinTeam_Arena( const char *pTeamName );
	void				HandleCommand_JoinClass( const char *pClassName );
	void				HandleCommand_JoinTeam_NoMenus( const char *pTeamName );
	void				HandleCommand_JoinTeam_NoKill( const char *pTeamName );
	void				SelectInitialClass( void );
	void				ShowTeamMenu( bool bShow = true );
	void				ShowClassMenu( bool bShow = true );

	// Bots.
	friend void			Bot_Think( CTFPlayer *pBot );

private:
	// Physics.
	void				PhysObjectSleep();
	void				PhysObjectWake();

	// Ammo pack.
	void				AmmoPackCleanUp( void );

	// State.
	CPlayerStateInfo	*StateLookupInfo( int nState );
	void				StateEnter( int nState );
	void				StateLeave( void );
	void				StateTransition( int nState );
	void				StateEnterWELCOME( void );
	void				StateThinkWELCOME( void );
	void				StateEnterPICKINGTEAM( void );
	void				StateEnterACTIVE( void );
	void				StateEnterOBSERVER( void );
	void				StateThinkOBSERVER( void );
	void				StateEnterDYING( void );
	void				StateThinkDYING( void );

	virtual bool		SetObserverMode(int mode);
	virtual void		AttemptToExitFreezeCam( void );

	bool				PlayGesture( const char *pGestureName );
	bool				PlaySpecificSequence( const char *pSequenceName );
	bool				PlayDeathAnimation( const CTakeDamageInfo &info, CTakeDamageInfo &info_modified );

	bool				GetResponseSceneFromConcept( int iConcept, char *chSceneBuffer, int numSceneBufferBytes );

public:
	void				SetWaterExitTime( float flTime ){ m_flWaterExitTime = flTime; }
	float				GetWaterExitTime( void ){ return m_flWaterExitTime; }

	bool				m_bSelfKnockback;
private:
	// Networked.
	CNetworkVar( bool, m_bSaveMeParity );
	CNetworkQAngle( m_angEyeAngles );					// Copied from EyeAngles() so we can send it to the client.
	CNetworkVar( int, m_iSpawnCounter );
	CNetworkVar( float, m_flLastDamageTime );
	CNetworkVar( bool, m_bArenaSpectator );

	// Items.
	CNetworkHandle( CTFItem, m_hItem );

	// Combat.
	CNetworkHandle( CTFWeaponBase, m_hOffHandWeapon );
	CNetworkVar( int, m_nActiveWpnClip );
	CNetworkVar( int, m_nActiveWpnAmmo );

	CNetworkVar( int, m_nForceTauntCam );
#ifdef ITEM_TAUNTING
	CNetworkVar( bool, m_bAllowMoveDuringTaunt );
	CNetworkVar( bool, m_bTauntForceForward );
	CNetworkVar( float, m_flTauntSpeed );
	CNetworkVar( float, m_flTauntTurnSpeed );
#endif
	CNetworkVar( bool, m_bTyping );

	CNetworkVar( bool, m_bHasDied );

	float m_flLastThinkTime;
	CNetworkVar( float, m_flRespawnTimeOverride );

	CNetworkVarEmbedded( CAttributeContainerPlayer, m_AttributeManager );
	CUtlMap<CUtlString, float> m_mapCustomAttributes;

	CNetworkVar( int, m_nCurrency );
	// TFBot stuff (mostly MvM related, basically just stubs)
	CNetworkVar( bool, m_bIsMiniBoss );
	CNetworkVar( bool, m_bIsABot );
	CNetworkVar( int, m_nBotSkill );

	bool				m_bAllSeeCrit;
	bool				m_bMiniCrit;
	bool				m_bShowDisguisedCrit;
	EAttackBonusEffects_t	m_eBonusAttackEffect;

	// Non-networked.
	bool				m_bAbortFreezeCam;
	bool				m_bSeenRoundInfo;
	bool				m_bRegenerating;

	float				m_flNextRegenerateTime;
	float				m_flNextChangeClassTime;
	float				m_flNextChangeTeamTime;

	// Disguises are removed before the attack check so, we have to do this Terribleness - Kay
	bool				m_bNextAttackIsMinicrit;

	// Ragdolls.
	Vector				m_vecTotalBulletForce;

	// State.
	CPlayerStateInfo	*m_pStateInfo;

	CTFPlayerClass		m_PlayerClass;
	int					m_iPreviousClassIndex;
	bool				m_bWasVIP;
	int					m_iItemPresets[TF_CLASS_COUNT_ALL][TF_LOADOUT_SLOT_COUNT];

	CTFPlayerAnimState	*m_PlayerAnimState;

	float				m_flNextPainSoundTime;
	int					m_LastDamageType;
	int					m_iDeathFlags;				// TF_DEATH_* flags with additional death info
	int					m_iMaxSentryKills;			// most kills by a single sentry

	bool				m_bPlayedFreezeCamSound;

	CUtlVector<EHANDLE>	m_aObjects;			// List of player objects

	bool				m_bIsClassMenuOpen;

	float				m_flSpawnTime;

	float				m_flLastAction;
	bool				m_bIsIdle;

	CUtlVector<EHANDLE>	m_hObservableEntities;
	CUtlVector<DamagerHistory_t> m_vecDamagerHistory; // History of who has damaged this player.
	//CUtlVector<float>	m_aBurnOtherTimes; // Vector of times this player has burned others.

	// Background expressions
	string_t			m_iszExpressionScene;
	EHANDLE				m_hExpressionSceneEnt;
	float				m_flNextRandomExpressionTime;
	EHANDLE				m_hWeaponFireSceneEnt;
	float				m_flNextVoiceCommandTime;
	float				m_flNextSpeakWeaponFire;

	bool				m_bSpeakingConceptAsDisguisedSpy;

	// ConVar settingss.
	bool				m_bHudClassAutoKill;
	bool 				m_bMedigunAutoHeal;
	bool				m_bAutoRezoom;	// does the player want to re-zoom after each shot for sniper rifles
	CNetworkVar( bool,	m_bAutoReload );
	bool				m_bRememberActiveWeapon;
	bool				m_bRememberLastWeapon;
	CNetworkVar( bool,	m_bFlipViewModel );
	bool				m_bProximityVoice;
	CNetworkVar( float, m_flViewModelFOV );
	CNetworkVector(		m_vecViewModelOffset );
	CNetworkVar( bool,	m_bMinimizedViewModels );
	bool				m_bHoldZoomSniper;
	CNetworkVar( bool,	m_bInvisibleArms );
	CNetworkVar( bool,  m_bFixedSpreadPreference );
	bool				m_bAvoidBecomingVIP;
	CNetworkVar( bool,	m_bSpywalkInverted );
	CNetworkVar( bool,  m_bCenterFirePreference );

	int					m_iLastLifeActiveWeapon;
	int					m_iLastLifeLastWeapon; // Durrrr

	// Copy disguise target weapons to this list.
	CUtlVector<DisguiseWeapon_t> m_hDisguiseWeaponList;

	float				m_flTauntAttackTime;
	float				m_flTauntAttackEndTime;
	int					m_iTauntAttack;

	float				m_flNextCarryTalkTime;

	int					m_iBlastJumpState;
	bool				m_bBlastLaunched;
	bool				m_bCreatedRocketJumpParticles;

	int					m_iDominations;

	float				m_flWaterEntryTime;
	float				m_flWaterExitTime;

	float				m_flArenaBalanceTime;
	int					m_iLastTeamNum;

	CountdownTimer		m_ctNavCombatUpdate;

	// TFBot stuff
	bool				m_bIsBasicBot; // True if this player is a bot (a basic bot, not a TFBot).
	IntervalTimer		m_itMedicCall;

	CountdownTimer		m_ctRecentSap;
	bool				m_bSapping;
	int					m_iSapState;
	float				m_flSapTime;

	CEconItemView		m_RandomItems[TF_LOADOUT_SLOT_COUNT];
	bool				m_bPreserveRandomLoadout;

	float				m_flTeamScrambleScore;

	float				m_flSpectatorTime;

	COutputEvent		m_OnDeath;

	// New stuff.
public:
	int					m_iOldStunFlags;

private:
	float				m_flLastDamageResistSoundTime;
	int					m_iLastWeaponFireUsercmd; // The last usercmd we shot a bullet on.

	bool				bTickTockOrder;

public:
	bool				SetPowerplayEnabled( bool bOn );
	bool				PlayerIsDeveloper(void);
	bool				PlayerIsCredited(void);
	void				PowerplayThink( void );

	float				m_flPowerPlayTime;

	CSoundPatch*		m_pJumpPadSoundLoop;
	void				DropTeamHealthPack( int iTeamOfKiller );
	EHANDLE				m_pOwnedUberGenerator;

	
	bool				m_pVIPInstantlyRespawned;
};

//-----------------------------------------------------------------------------
// Purpose: Utility function to convert an entity into a tf player.
//   Input: pEntity - the entity to convert into a player
//-----------------------------------------------------------------------------
inline CTFPlayer *ToTFPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return assert_cast<CTFPlayer *>( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: Const-qualified version of ToTFPlayer.
//-----------------------------------------------------------------------------
inline const CTFPlayer *ToTFPlayer( const CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return assert_cast<const CTFPlayer *>( pEntity );
}

inline int CTFPlayer::StateGet( void ) const
{
	return m_Shared.m_nPlayerState;
}

#endif	// TF_PLAYER_H
