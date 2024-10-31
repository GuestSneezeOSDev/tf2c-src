//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef C_TF_PLAYER_H
#define C_TF_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playeranimstate.h"
#include "c_baseplayer.h"
#include "tf_shareddefs.h"
#include "baseparticleentity.h"
#include "tf_player_shared.h"
#include "c_tf_playerclass.h"
#include "entity_capture_flag.h"
#include "props_shared.h"
#include "hintsystem.h"
#include "c_playerattachedmodel.h"
#include "iinput.h"
#include "tf_weapon_medigun.h"
#include "ihasattributes.h"
#include "c_tf_spymask.h"
#include "tf_colorblind_helper.h"
#include "tf_hud_mediccallers.h"

class C_MuzzleFlashModel;
class C_BaseObject;
class C_TFTeam;
class C_TFRagdoll;
class C_TFProjectile_Arrow;
struct dlight_t;

extern ConVar tf_medigun_autoheal;
extern ConVar cl_autorezoom;
extern ConVar cl_autoreload;
extern ConVar cl_flipviewmodels;
extern ConVar tf2c_zoom_hold_sniper;
extern ConVar tf2c_fixedspread_preference;
extern ConVar tf2c_avoid_becoming_vip;
extern ConVar tf2c_spywalk_inverted;
extern ConVar tf2c_centerfire_preference;

// Allows us to check if an entity wants to glow.
class IModelGlowController
{
public:
	virtual bool ShouldGlow( void ) = 0;

};


class C_TFPlayer : public C_BasePlayer, public IHasAttributes
{
public:

	DECLARE_CLASS( C_TFPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_TFPlayer();
	~C_TFPlayer();

	friend class CTFGameMovement;

	static C_TFPlayer *GetLocalTFPlayer();

	virtual void	UpdateOnRemove( void );

	virtual bool	IsNextBot( void ) const { return m_bIsABot; }
	virtual const QAngle &GetRenderAngles();
	virtual void	UpdateClientSideAnimation();
	virtual void	SetDormant( bool bDormant );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ProcessMuzzleFlashEvent();
	virtual void	ValidateModelIndex( void );
	virtual void	UpdateVisibility( void );

	virtual Vector	GetObserverCamOrigin( void );
	virtual int		DrawModel( int flags );
	virtual bool	OnInternalDrawModel( ClientModelRenderInfo_t *pInfo );
	virtual int		GetBody( void );
	virtual void	BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed );

	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *pCmd );

	virtual bool	IsAllowedToSwitchWeapons( void );

	virtual void	PreThink( void );
	virtual void	PostThink( void );
	virtual void	PhysicsSimulate( void );
	virtual void	ClientThink();

	virtual void	GetToolRecordingState( KeyValues *msg );

	CTFWeaponBase	*GetActiveTFWeapon( void ) const;
	bool			IsActiveTFWeapon( ETFWeaponID iWeaponID ) const;

	virtual void	Simulate( void );
	virtual void	FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	void			LoadInventory( void );
	void			EditInventory( ETFLoadoutSlot iSlot, int iWeapon );

	bool			CanAttack( void );

	C_TFPlayerClass *GetPlayerClass( void )				{ return &m_PlayerClass; }
	const C_TFPlayerClass *GetPlayerClass( void ) const	{ return &m_PlayerClass; }
	bool			IsPlayerClass( int iClass, bool bIgnoreRandomizer = false );
	virtual int		GetMaxHealth( void ) const;

	virtual int		GetRenderTeamNumber( void );

	bool			IsWeaponLowered( void );

	void			AvoidPlayers( CUserCmd *pCmd );

	// Get the ID target entity index. The ID target is the player that is behind our crosshairs,
	// used to display the player's name and get their held weapons to display.
	void			UpdateIDTarget();
	int				GetIDTarget() const;
	void			SetForcedIDTarget( int iTarget );
	void			GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes, bool &bShowingAmmo );

	bool			CanPlayerMove() const;
	bool			CanJump() const;
	bool			CanDuck() const;

	void			SetAnimation( PLAYER_ANIM playerAnim );

	virtual float	GetMinFOV() const;

	virtual const QAngle &EyeAngles();

	int				GetBuildResources( void );

	virtual void	ComputeFxBlend( void );

	// Taunts/VCDs
	virtual bool	StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget );
	virtual void	CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	bool			StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	void			TurnOnTauntCam( void );
	void			TurnOffTauntCam( void );
	void			TauntCamInterpolation( void );
#ifdef ITEM_TAUNTING
	void			PlayTauntSoundLoop( const char *pszSound );
	void			StopTauntSoundLoop( void );
#endif
	bool			InTauntCam( void ) { return m_bWasTaunting; }
	bool			InThirdPersonShoulder( void );
	virtual void	ThirdPersonSwitch( bool bThirdperson );

	virtual void	InitPhonemeMappings();

	// Gibs
	void			InitPlayerGibs( void );
	void			CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning = false, int iMaxGibs = -1 );
	void			DropHelmet( breakablepropparams_t &breakParams, Vector &vecBreakVelocity );
	void			DropPartyHat( breakablepropparams_t &breakParams, Vector &vecBreakVelocity );
	void			CreateBoneAttachmentsFromWearables( C_TFRagdoll *pRagdoll );

	int				GetObjectCount( void );
	C_BaseObject	*GetObject( int index );
	C_BaseObject	*GetObjectOfType( int iObjectType, int iObjectMode = 0 );
	int				GetNumObjects( int iObjectType, int iObjectMode = 0 );

	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	float			GetPercentInvisible( void );
	float			GetEffectiveInvisibilityLevel( void );	// Takes viewer into account.

	virtual void	AddDecal( const Vector &rayStart, const Vector &rayEnd,
		const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t &tr, int maxLODToDecal = ADDDECAL_TO_ALL_LODS );

	virtual void	CalcDeathCamView( Vector& eyeOrigin, QAngle& eyeAngles, float &fov );
	virtual Vector GetChaseCamViewOffset( CBaseEntity *target );

	void			ClientPlayerRespawn( void );

	virtual bool	ShouldDraw();

	void			CreateSaveMeEffect( bool bAuto );
	void			CreateOverheadHealthbar();
	
	virtual bool	IsOverridingViewmodel( void );
	virtual int		DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags );

	void			SetHealer( C_TFPlayer *pHealer, float flChargeLevel );
	void			GetHealer( C_TFPlayer **pHealer, float *flChargeLevel ) { *pHealer = m_hHealer; *flChargeLevel = m_flHealerChargeLevel; }
	float			MedicGetChargeLevel( void );
	CBaseEntity		*MedicGetHealTarget( void );

	void			UpdateRecentlyTeleportedEffect(void);
	void			UpdateJumppadTrailEffect(void);
	void			UpdateSpeedBoostTrailEffect(void);

	void			InitializePoseParams( void );
	void			UpdateLookAt( void );

	bool			IsEnemyPlayer( void );
	bool			IsDisguisedEnemy( bool bTeammateDisguise = false );
	void			ShowNemesisIcon( bool bShow );

	CUtlVector<EHANDLE> *GetSpawnedGibs( void ) { return &m_hSpawnedGibs; }

	Vector 			GetClassEyeHeight( void );

	void			ForceUpdateObjectHudState( void );

	virtual bool	AudioStateIsUnderwater( Vector vecMainViewOrigin );

	bool			GetMedigunAutoHeal( void ) { return tf_medigun_autoheal.GetBool(); }
	bool			ShouldAutoRezoom( void ) { return cl_autorezoom.GetBool(); }
	bool			ShouldAutoReload( void ) { return m_bAutoReload; }
	bool			ShouldFlipViewModel( void ) { return m_bFlipViewModel; }
	float			GetViewModelFOV( void ) { return m_flViewModelFOV; }
	const Vector	&GetViewModelOffset( void ) { return m_vecViewModelOffset; }
	bool			ShouldUseMinimizedViewModels( void ) { return m_bMinimizedViewModels; }
	bool			ShouldHoldToZoom( void ) { return tf2c_zoom_hold_sniper.GetBool(); }
	bool			ShouldHaveInvisibleArms( void ) { return m_bInvisibleArms; }
	bool			FixedSpreadPreference(void) { return m_bFixedSpreadPreference; }
	bool			IsSpywalkInverted(void) { return m_bSpywalkInverted; }
	bool			CenterFirePreference(void) { return m_bCenterFirePreference; }

	// Shared Functions
	void			TeamFortress_SetSpeed();
	void			TeamFortress_SetGravity();
	float			GetSpeedBoostMultiplier( void );
	bool			HasItem( void ) const; // Currently can have only one item at a time.
	void			SetItem( C_TFItem *pItem );
	C_TFItem		*GetItem( void ) const;
	bool			IsAllowedToPickUpFlag( void ) const;
	bool			HasTheFlag( void ) const;
	C_CaptureFlag	*GetTheFlag( void ) const;
	float			GetCritMult( void ) { return m_Shared.GetCritMult(); }

	virtual void	ItemPostFrame( void );

	void			SetOffHandWeapon( CTFWeaponBase *pWeapon );
	void			HolsterOffHandWeapon( void );

	virtual int GetSkin();

	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );
	virtual	bool		Weapon_Switch( C_BaseCombatWeapon *pWeapon, int viewmodelindex = 0 );

	bool				CanShowClassMenu( void );

	ITFHealingWeapon	*GetMedigun( void ) const;
	CTFWeaponBase		*GetTFWeapon( int iWeapon ) const { return static_cast<CTFWeaponBase *>( GetWeapon( iWeapon ) ); }
	CTFWeaponBase		*GetTFWeaponBySlot( int iBucket ) const { return static_cast<CTFWeaponBase *>( Weapon_GetSlot( iBucket ) ); }
	CTFWeaponBase		*Weapon_OwnsThisID( ETFWeaponID iWeaponID ) const;
	CTFWeaponBase		*Weapon_GetWeaponByType( ETFWeaponType iType ) const;
	CTFWearable			*Wearable_OwnsThisID( int iWearableID ) const;
	virtual bool		Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon );
	virtual CTFWeaponBase *Weapon_GetSlot( int slot ) const;
	virtual void		UpdateWearables() OVERRIDE;
	C_EconEntity		*GetEntityForLoadoutSlot( ETFLoadoutSlot iSlot ) const;
	C_EconWearable		*GetWearableForLoadoutSlot( ETFLoadoutSlot iSlot ) const;
	CTFWeaponBase		*GetWeaponForLoadoutSlot( ETFLoadoutSlot iSlot ) const;

	virtual void		UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );
	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );
	virtual const char	*GetOverrideStepSound( const char *pszBaseStepSoundName );
	virtual void		OnEmitFootstepSound( const CSoundParameters &params, const Vector &vecOrigin, float fVolume );

	bool				DoClassSpecialSkill( void );
	bool				CanGoInvisible( void );

	C_TFTeam			*GetTFTeam( void ) const;

	virtual void		RemoveAmmo( int iCount, int iAmmoIndex );
	virtual int			GetAmmoCount( int iAmmoIndex ) const;
	int					GetMaxAmmo( int iAmmoIndex, bool bAddMissingClip = false, int iClassIndex = TF_CLASS_UNDEFINED ) const;
	bool				HasInfiniteAmmo( void ) const;
	bool				CanPickupBuilding( C_BaseObject *pPickupObject );

	bool				IsEnemy( const C_BaseEntity *pEntity ) const;

	bool				IsVIP( void ) const;
	void				JumpSound( void );
	CTFPlayer			*GetObservedPlayer( bool bFirstPerson );
	virtual Vector		Weapon_ShootPosition( void );
	void				SetEyeAngles( const QAngle &angles );

	float				HealthFractionBuffed() const;

	virtual CAttributeManager *GetAttributeManager( void ) { return &m_AttributeManager; }
	virtual CAttributeContainer *GetAttributeContainer( void ) { return NULL; }
	virtual CBaseEntity	*GetAttributeOwner( void ) { return NULL; }
	virtual CAttributeList *GetAttributeList( void ) { return &m_AttributeList; }
	virtual void		ReapplyProvision( void ) { /*Do nothing*/ };

	// Ragdolls
	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual IRagdoll		*GetRepresentativeRagdoll() const;
	EHANDLE	m_hRagdoll;
	Vector m_vecRagdollVelocity;

	// Objects
	int CanBuild( int iObjectType, int iObjectMode = 0 );
	CUtlVector<CHandle<C_BaseObject>> m_aObjects;

	virtual CStudioHdr *OnNewModel( void );

	virtual void		OnAchievementAchieved( int iAchievement );

	void				DisplaysHintsForTarget( C_BaseEntity *pTarget );

	virtual int			GetVisionFilterFlags( bool bWeaponsCheck = false ) { return 0x00; }
	bool				HasVisionFilterFlags( int nFlags, bool bWeaponsCheck = false ) { return ( GetVisionFilterFlags( bWeaponsCheck ) & nFlags ) == nFlags; }

	// Shadows
	virtual ShadowType_t ShadowCastType( void );
	virtual void		GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType );
	virtual void		GetRenderBounds( Vector& theMins, Vector& theMaxs );
	virtual bool		GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const;

	CMaterialReference	*GetInvulnMaterialRef( void ) { return &m_InvulnerableMaterial; }
#ifdef TF2C_BETA
	CMaterialReference* GetInvulnMaterialRefPlayerModelPanel(void) { return &m_InvulnerableMaterialPlayerModelPanel; }
#endif
	bool				ShouldShowNemesisIcon();

	virtual	IMaterial	*GetHeadLabelMaterial( void );

	virtual void		FireGameEvent( IGameEvent *event );

	void				UpdateSpyStateChange( void );
	void				UpdateSpyMask( void );

	bool				CanShowSpeechBubbles( void );
	void				UpdateSpeechBubbles( void );
	void				UpdateOverhealEffect( void );
	void				UpdatedMarkedForDeathEffect( bool bFroceStop = false );
	void				UpdatePartyHat( void );

	//static int			ms_nPlayerPatternCounter;
	virtual void		UpdateTeamPatternEffect(void);
	virtual void		DestroyTeamPatternEffect(void);
	bool				m_bColorBlindInitialised;
	CTeamPatternObject	*m_pTeamPatternEffect;
	CTeamPatternObject	*GetTeamPatternObject(void){ return m_pTeamPatternEffect; }

	void				UpdateClientSideGlow( void );
	virtual void		GetGlowEffectColor( float *r, float *g, float *b );

	float				GetDesaturationAmount( void );

	void				CollectVisibleSteamUsers( CUtlVector<CSteamID> &userList );

	static void			ClampPlayerColor( Vector &vecColor );

	bool				AddOverheadEffect( const char *pszEffectName );
	void				RemoveOverheadEffect( const char *pszEffectName, bool bRemoveInstantly );
	void				UpdateOverheadEffects();
	Vector				GetOverheadEffectPosition();

	virtual const Vector &GetRenderOrigin( void );

	void				AddArrow( C_TFProjectile_Arrow *pArrow );
	void				RemoveArrow( C_TFProjectile_Arrow *pArrow );

	bool				ShouldAnnounceAchievement(void) OVERRIDE;
protected:
	void				ResetFlexWeights( CStudioHdr *pStudioHdr );

private:
	void				HandleTaunting( void );

	void				OnPlayerClassChange( void );

	bool				CanLightCigarette( void );

	void				InitInvulnerableMaterial( void );
#ifdef TF2C_BETA
	void				InitInvulnerableMaterialPlayerModelPanel(void);
#endif
	bool				m_bWasTaunting;
	float				m_flTauntOffTime;
	CSoundPatch			*m_pTauntSound;

	QAngle				m_angTauntPredViewAngles;
	QAngle				m_angTauntEngViewAngles;

	CUtlMap<const char *, HPARTICLEFFECT> m_mapOverheadEffects;
	float				m_flOverheadEffectStartTime;

private:
	C_TFPlayerClass		m_PlayerClass;

	// ID Target
	int					m_iIDEntIndex;
	int					m_iForcedIDTarget;

	CNewParticleEffect	*m_pTeleporterEffect;
	bool				m_bToolRecordingVisibility;

	int					m_iOldSpawnCounter;

	int					m_iOldObserverMode;
	EHANDLE				m_hOldObserverTarget;
	int					m_iOldObserverTeam;

	// Healer
	CHandle<C_TFPlayer>	m_hHealer;
	float				m_flHealerChargeLevel;
	int					m_iOldHealth;

	// Look At
	/*
	int m_headYawPoseParam;
	int m_headPitchPoseParam;
	float m_headYawMin;
	float m_headYawMax;
	float m_headPitchMin;
	float m_headPitchMax;
	float m_flLastBodyYaw;
	float m_flCurrentHeadYaw;
	float m_flCurrentHeadPitch;
	*/

	// Spy cigarette smoke
	CNewParticleEffect	*m_pCigaretteSmoke;
	dlight_t			*m_pCigaretteLight;

	// Medic Callout Particle
	CNewParticleEffect	*m_pSaveMeEffect;

	bool				m_bUpdateObjectHudState;

	Vector				m_vecCustomModelOrigin;
	bool				m_bOldCustomModelVisible;

	float				m_flChangeClassTime;

	int					m_iOldStoredCrits;

public:
	friend class CTFPlayerShared;
	CTFPlayerShared m_Shared;

// Called by shared code.
public:
	float				GetClassChangeTime() const { return m_flChangeClassTime; }

	bool				HasSaveMeEffect() const { return m_pSaveMeEffect != null; }

	void				DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	int					GetNumActivePipebombs( void );
	float				GetRespawnTimeOverride( void ) { return m_flRespawnTimeOverride; };

	CTFPlayerAnimState	*m_PlayerAnimState;

	QAngle				m_angEyeAngles;
	CInterpolatedVar<QAngle> m_iv_angEyeAngles;

	CNetworkHandle( C_TFItem, m_hItem );

	CNetworkHandle( C_TFWeaponBase, m_hOffHandWeapon );
	int					m_nActiveWpnClip;
	int					m_nActiveWpnAmmo;

	int					m_iSpawnCounter;

	bool				m_bSaveMeParity;
	bool				m_bOldSaveMeParity;

	int					m_nOldWaterLevel;
	float				m_flWaterEntryTime;
	bool				m_bWaterExitEffectActive;

	CMaterialReference	m_InvulnerableMaterial;
#ifdef TF2C_BETA
	CMaterialReference	m_InvulnerableMaterialPlayerModelPanel;
#endif
	// Burning
	float				m_flBurnEffectStartTime;
	float				m_flBurnRenewTime;

	EHANDLE				m_hFirstGib;
	CUtlVector<EHANDLE>	m_hSpawnedGibs;

	int					m_iOldTeam;
	int					m_iOldPlayerClass;
	int					m_iOldDisguiseTeam;
	int					m_iOldDisguiseClass;
	bool				m_bIsDisplayingNemesisIcon;

	bool				m_bDisguised;
	int					m_iPreviousMetal;

	EHANDLE				m_hOldActiveWeapon;

	bool				m_bUpdatePartyHat;
	CHandle<C_PlayerAttachedModel> m_hPartyHat;

	int					m_nForceTauntCam;
	bool				m_bArenaSpectator;
#ifdef ITEM_TAUNTING
	bool				m_bAllowMoveDuringTaunt;
	bool				m_bTauntForceForward;
	float				m_flTauntSpeed;
	float				m_flTauntTurnSpeed;
#endif
	float				m_flLastDamageTime;

	bool				m_bAutoReload;
	bool				m_bFlipViewModel;
	float				m_flViewModelFOV;
	Vector				m_vecViewModelOffset;
	bool				m_bMinimizedViewModels;
	bool				m_bInvisibleArms;
	bool				m_bFixedSpreadPreference;
	bool				m_bSpywalkInverted;
	bool				m_bCenterFirePreference;

	float				m_flRespawnTimeOverride;

	bool				m_bUpdateAttachedModels;
	CHandle<C_TFSpyMask> m_hSpyMask;

	bool				m_bTyping;
	CNewParticleEffect	*m_pTypingEffect;
	CNewParticleEffect	*m_pVoiceEffect;

	CNewParticleEffect	*m_pOverhealEffect;

	CAttributeContainerPlayer m_AttributeManager;

	CountdownTimer		m_blinkTimer;

	HPARTICLEFFECT		m_pStunnedEffect;

	// Blast Jump Whistle
	CSoundPatch			*m_pBlastJumpLoop;
	float				m_flBlastJumpLaunchTime;


	CNewParticleEffect	*m_pJumppadJumpEffect;
	CNewParticleEffect* m_pSpeedEffect;

	// HL1 view bob, roll and idle camera effects.
	
	virtual void CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );
	virtual void CalcViewRoll( QAngle& eyeAngles );
	virtual void CalcViewBob( Vector& eyeOrigin );
	virtual void CalcViewIdle( QAngle& eyeAngles );

	// Networking
	virtual void		NotifyShouldTransmit( ShouldTransmitState_t state );

	float ViewBob;
	double BobTime;
	float BobLastTime;
	float IdleScale;

	// Box debugging
	void DrawBBoxVisualizations(void) override;

private:
	// TFBot stuff (mostly MvM related, therefore unused)
	bool				m_bIsABot;
	//bool				m_bIsMiniBoss;
	//bool				m_nBotSkill;

	// Gibs
	CUtlVector<breakmodel_t> m_aGibs;

	// Arrows
	CUtlVector<CHandle<C_TFProjectile_Arrow>> m_aArrows;

	C_TFPlayer( const C_TFPlayer & );
};

inline C_TFPlayer *ToTFPlayer( C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return assert_cast<C_TFPlayer *>( pEntity );
}

inline const C_TFPlayer *ToTFPlayer( const C_BaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return assert_cast<const C_TFPlayer *>( pEntity );
}

C_TFPlayer *GetLocalObservedPlayer( bool bFirstPerson );

#endif // C_TF_PLAYER_H
