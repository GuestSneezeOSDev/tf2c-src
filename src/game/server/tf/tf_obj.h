//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Base Object built by a player
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_OBJ_H
#define TF_OBJ_H
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "ihasbuildpoints.h"
#include "baseobject_shared.h"
#include "utlmap.h"
#include "props_shared.h"
#include "tf_team.h"
#include "tf_player.h"
#include "tf_nav_area.h"

class CTFPlayer;
class CTFTeam;
class CRopeKeyframe;
class CVGuiScreen;
class KeyValues;
class CTFWrench;
struct animevent_t;

// Construction
#define OBJECT_CONSTRUCTION_INTERVAL			0.1f
#define OBJECT_CONSTRUCTION_STARTINGHEALTH		0.1f


extern ConVar object_verbose;
extern ConVar tf2c_building_upgrades;
extern ConVar tf2c_building_sharing;

#if defined( _DEBUG )
#define TRACE_OBJECT( str )										\
if ( object_verbose.GetInt() )									\
{																\
	Msg( "%s", str );					\
}																
#else
#define TRACE_OBJECT( string )
#endif

#define SF_OBJ_INVULNERABLE		( 1 << 1 )
#define LAST_SF_OBJ				SF_OBJ_INVULNERABLE

#define ORIGINAL_METAL_RATIO	5.0f
#define GUN_METTLE_METAL_RATIO	3.0f

// ------------------------------------------------------------------------ //
// Resupply object that's built by the player
// ------------------------------------------------------------------------ //
class CBaseObject : public CBaseCombatCharacter, public IHasBuildPoints, public TAutoList<CBaseObject>
{
	DECLARE_CLASS( CBaseObject, CBaseCombatCharacter );
public:
	CBaseObject();

	virtual void	UpdateOnRemove( void );

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual bool	IsBaseObject( void ) const { return true; }

	void			BaseObjectThink( void );
	//virtual void	LostPowerThink( void );

	// Creation
	virtual void	Precache();
	virtual void	Spawn( void );
	virtual void	FirstSpawn( void );
	virtual void	Activate( void );

	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	void			SetBuilder( CTFPlayer *pBuilder );
	int				ObjectType() const;
	void			SetType( int iObjectType ) { m_iObjectType = iObjectType; }

	virtual int		BloodColor( void ) { return BLOOD_COLOR_MECH; }

	int				GetObjectMode( void ) const      { return m_iObjectMode; }
	void			SetObjectMode( int iObjectMode ) { m_iObjectMode = iObjectMode; }

	bool			IsBeingPlaced( void ) const { return m_bPlacing; }
	bool			IsBeingCarried( void ) const { return m_bCarried; }
	bool			IsRedeploying( void ) const  { return m_bCarryDeploy; }

	// Remote PDA
	bool			InRemoteConstruction( void ) const { return m_bRemoteConstruction; }
	void			SetRemoteConstruction( bool bValue );

	// Building
	float			GetTotalTime( void ) const;
	int				GetMaxHealthForCurrentLevel( void ) const;
	int				AdjustHealthForLevel( int iOldMaxHealth );
	virtual void	StartPlacement( CTFPlayer *pPlayer );
	void			StopPlacement( void );
	bool			FindNearestBuildPoint( CBaseEntity *pEntity, CBasePlayer *pBuilder, float &flNearestPoint, Vector &vecNearestBuildPoint, bool bIgnoreLOS = false );
	bool			VerifyCorner( const Vector &vBottomCenter, float xOffset, float yOffset );
	float			GetNearbyObjectCheckRadius( void ) const { return 30.0; }
	bool			UpdatePlacement( void );
	bool			UpdateAttachmentPlacement( CBaseObject *pObject = NULL );
	bool			IsValidPlacement( void ) const { return m_bPlacementOK; }
	bool			EstimateValidBuildPos( void );

	bool			CalculatePlacementPos( void );
	virtual bool	IsPlacementPosValid( void );
	bool			FindSnapToBuildPos( CBaseObject *pObject = NULL );

	void			ReattachChildren( void );

	virtual void	InitializeMapPlacedObject( void );
	
	// I've finished building the specified object on the specified build point
	virtual int		FindObjectOnBuildPoint( CBaseObject *pObject ) const;
	
	virtual bool	StartBuilding( CBaseEntity *pPlayer );
	void			BuildingThink( void );
	void			SetControlPanelsActive( bool bState );
	virtual void	FinishedBuilding( void );
	bool			IsBuilding( void ) const  { return m_bBuilding; };
	bool			IsPlacing( void ) const   { return m_bPlacing; };
	virtual bool	IsUpgrading( void ) const { return false; }
	bool			MustBeBuiltOnAttachmentPoint( void ) const;

	// Returns information about the various control panels
	virtual void 	GetControlPanelInfo( int nPanelIndex, const char *&pPanelName );
	void			GetControlPanelClassName( int nPanelIndex, const char *&pPanelName );

	// Damage
	void			SetHealth( float flHealth );
	void			TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	bool			PassDamageOntoChildren( const CTakeDamageInfo &info, float *flDamageLeftOver );
	bool			Construct( float flHealth );
	void			BoltHeal( float flAmount, CTFPlayer *pPlayer );

	void			OnConstructionHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos );
	float			GetConstructionMultiplier( void );

	// Destruction
	virtual void	DetonateObject( void );
	bool			ShouldAutoRemove( void ) const { return true; }
	void			Explode( void );
	virtual void	Killed( const CTakeDamageInfo &info );
	bool			IsDying( void ) const { return m_bDying; }
	void 			DestroyScreens( void );

	// Data
	virtual Class_T	Classify( void );
	int				GetType( void ) const     { return m_iObjectType; }
	CTFPlayer		*GetBuilder( void ) const { return m_hBuilder; }
	CTFTeam			*GetTFTeam( void ) const  { return static_cast<CTFTeam *>( GetTeam() ); };
	virtual int		CalculateTeamBalanceScore(void);
	
	// ID functions
	virtual bool	IsAnUpgrade( void ) const		{ return false; }
	virtual bool	IsHostileUpgrade( void ) const	{ return false; }	// Attaches to enemy buildings

	// Inputs
	void			InputSetHealth( inputdata_t &inputdata );
	void			InputAddHealth( inputdata_t &inputdata );
	void			InputRemoveHealth( inputdata_t &inputdata );
	void			InputSetSolidToPlayer( inputdata_t &inputdata );
	void            InputSetBuilder( inputdata_t &inputdata );
	void			InputShow( inputdata_t &inputdata );
	void			InputHide( inputdata_t &inputdata );
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );

	// Wrench hits
	virtual bool	InputWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos );
	virtual bool	OnWrenchHit( CTFPlayer *pPlayer, CTFWrench *pWrench, Vector vecHitPos );
	virtual bool	Command_Repair( CTFPlayer *pActivator, float flRepairMod );

	void			DoWrenchHitEffect( Vector vecHitPos, bool bRepair, bool bUpgrade );

	virtual bool	CheckUpgradeOnHit( CTFPlayer *pPlayer );
	void			AddUpgradeMetal( CTFPlayer *pPlayer, int iMetal );

	virtual void	ChangeTeam( int iTeamNum );			// Assign this entity to a team.

	// Handling object inactive
	bool			ShouldBeActive( void ) const;

	// Sappers
	bool			HasSapper( void ) const { return m_bHasSapper; }
	void			OnAddSapper( void );
	void			OnRemoveSapper( void );

	// Returns the object flags
	int				GetObjectFlags() const { return m_fObjectFlags; }
	void			SetObjectFlags( int flags ) { m_fObjectFlags = flags; }

	void			AttemptToGoActive( void );
	virtual void	OnGoActive( void );
	void			OnGoInactive( void );

	// Disabling
	virtual bool	IsDisabled( void ) const { return m_bDisabled; }
	void			UpdateDisabledState( void );
	void			SetDisabled( bool bDisabled );
	virtual void	OnStartDisabled( void );
	virtual void	OnEndDisabled( void );

	// Animation
	void			PlayStartupAnimation( void );

	Activity		GetActivity( ) const;
	void			SetActivity( Activity act );
	void			SetObjectSequence( int sequence );
	
	// Object points
	void			SpawnObjectPoints( void );

	// Derive to customize an object's attached version
	virtual	void	SetupAttachedVersion( void ) { return; }

	virtual int		DrawDebugTextOverlays( void );

	void			RotateBuildAngles( void );

	bool			ShouldPlayersAvoid( void ) const;

	void			IncrementKills( void )   { m_iKills++; }
	void			IncrementAssists( void ) { m_iAssists++; }
	int				GetKills() const         { return m_iKills; }
	int				GetAssists() const       { return m_iAssists; }

	void			CreateObjectGibs( void );
	virtual void	SetModel( const char *pModel );

	const char		*GetResponseRulesModifier( void );

	// Upgrades
	int				GetUpgradeLevel( void ) const { return m_iUpgradeLevel; }

	// If the players hit us with a wrench, should we upgrade
	virtual bool	CanBeUpgraded( CTFPlayer *pPlayer );
	virtual void	StartUpgrading( void );
	virtual void	FinishUpgrading( void );
	void			UpgradeThink(void);
	virtual int		GetMaxUpgradeLevelFirstSpawn(void);
	virtual int		GetMaxUpgradeLevel(void) const { return m_iHighestUpgradeLevel; }

	virtual const char	*GetPlacementModel( void ) const { return ""; }

	virtual void	MakeCarriedObject( CTFPlayer *pPlayer );
	virtual void	DropCarriedObject( CTFPlayer *pPlayer );

	virtual int		GetBaseHealth( void ) const { return 0; }

	float			GetFloatHealth()       const { return m_flHealth; }
	bool			IsMiniBuilding()       const { return m_bMiniBuilding; }
	bool			IsDisposableBuilding() const { return m_bDisposableBuilding; }

	// Convenience helper functions
	bool			IsDispenser()       const { return ( GetType() == OBJ_DISPENSER || GetType() == OBJ_MINIDISPENSER ); }
	bool			IsTeleporter()      const { return (GetType() == OBJ_TELEPORTER); }
	bool			IsJumppad()			const { return (GetType() == OBJ_JUMPPAD); }
	bool			IsSentry()          const { return ( GetType() == OBJ_SENTRYGUN || GetType() == OBJ_FLAMESENTRY); }
	bool			IsSapper()          const { return ( GetType() == OBJ_ATTACHMENT_SAPPER ); }
	bool			IsNormalDispenser() const { return ( IsDispenser() && ( GetObjectFlags() & OF_IS_CART_OBJECT ) == 0 ); }
	bool			IsCartDispenser()   const { return ( IsDispenser() && ( GetObjectFlags() & OF_IS_CART_OBJECT ) != 0 ); }
	bool			IsTeleEntrance()    const { return ( IsTeleporter() && GetObjectMode() == TELEPORTER_TYPE_ENTRANCE ); }
	bool			IsTeleExit()        const { return ( IsTeleporter() && GetObjectMode() == TELEPORTER_TYPE_EXIT     ); }

	virtual bool	ShouldBlockNav() const { return false; }

	CTFNavArea *GetLastKnownTFArea() const { return static_cast<CTFNavArea *>( GetLastKnownArea() ); }

public:		

	virtual bool TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

public:
	// Client/Server shared build point code
	void				CreateBuildPoints( void );
	void				AddAndParseBuildPoint( int iAttachmentNumber, KeyValues *pkvBuildPoint );
	int					AddBuildPoint( int iAttachmentNum );
	void				AddValidObjectToBuildPoint( int iPoint, int iObjectType );
	CBaseObject			*GetBuildPointObject( int iPoint ) const;
	bool				IsBuiltOnAttachment( void ) const { return (m_hBuiltOnEntity.Get() != NULL); }
	void				AttachObjectToObject( CBaseEntity *pEntity, int iPoint, Vector &vecOrigin );
	virtual void		DetachObjectFromObject( void );
	CBaseObject			*GetParentObject( void );

// IHasBuildPoints
public:
	virtual int			GetNumBuildPoints( void ) const;
	virtual bool		GetBuildPoint( int iPoint, Vector &vecOrigin, QAngle &vecAngles );
	virtual int			GetBuildPointAttachmentIndex( int iPoint ) const;
	virtual bool		CanBuildObjectOnBuildPoint( int iPoint, int iObjectType ) const;
	virtual void		SetObjectOnBuildPoint( int iPoint, CBaseObject *pObject );
	virtual float		GetMaxSnapDistance( int iBuildPoint ) const;
	virtual bool		ShouldCheckForMovement( void ) const { return true; }
	virtual int			GetNumObjectsOnMe() const;
	virtual CBaseEntity	*GetFirstFriendlyObjectOnMe( void ) const;
	virtual CBaseObject *GetObjectOfTypeOnMe( int iObjectType ) const;
	virtual void		RemoveAllObjects( void );
	virtual void		OutlineForEnemy( float flDuration );


// IServerNetworkable.
public:

	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual void	SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );


protected:
	// Show/hide vgui screens.
	bool ShowVGUIScreen( int panelIndex, bool bShow );

	// Spawns the various control panels
	void SpawnControlPanels();

	void			DetermineAnimation( void );
	virtual void	DeterminePlaybackRate( void );

	void UpdateDesiredBuildRotation( float flFrameTime );

private:
	// Purpose: Spawn any objects specified inside the mdl
	void SpawnEntityOnBuildPoint( const char *pEntityName, int iAttachmentNumber );

	bool TestAgainstRespawnRoomVisualizer( CTFPlayer *pPlayer, const Vector &vecEnd );

	//bool TestPositionForPlayerBlock( Vector vecBuildOrigin, CBasePlayer *pPlayer );
	//void RecursiveTestBuildSpace( int iNode, bool *bNodeClear, bool *bNodeVisited );

protected:
	enum OBJSOLIDTYPE
	{
		SOLID_TO_PLAYER_USE_DEFAULT = 0,
		SOLID_TO_PLAYER_YES,
		SOLID_TO_PLAYER_NO,
	};

	bool		IsSolidToPlayers( void ) const;

	// object flags....
	CNetworkVar( int, m_fObjectFlags );
	CNetworkHandle( CTFPlayer,	m_hBuilder );

	// Placement
	Vector			m_vecBuildOrigin;
	Vector			m_vecBuildCenterOfMass;
	CNetworkVector( m_vecBuildMaxs );
	CNetworkVector( m_vecBuildMins );
	CNetworkHandle( CBaseEntity, m_hBuiltOnEntity );
	int				m_iBuiltOnPoint;

	// Upgrade specific
	// Upgrade Level ( 1, 2, 3 )
	CNetworkVar( int, m_iUpgradeLevel );
	CNetworkVar( int, m_iHighestUpgradeLevel );
	CNetworkVar( int, m_iUpgradeMetal );
	int		m_iTargetUpgradeLevel;		// Used when re-deploying
	int		m_iDefaultUpgrade;			// Used for map-placed buildings
	int		m_iTargetHealth;			// Used when re-deploying

	bool	m_bDying;

	// Time when the upgrade animation will complete
	float m_flUpgradeCompleteTime;

	// Outputs
	COutputEvent m_OnDestroyed;
	COutputEvent m_OnDamaged;
	COutputEvent m_OnRepaired;

	COutputEvent m_OnBecomingDisabled;
	COutputEvent m_OnBecomingReenabled;

	COutputFloat m_OnObjectHealthChanged;

	// Control panel
	typedef CHandle<CVGuiScreen>	ScreenHandle_t;
	CUtlVector<ScreenHandle_t>	m_hScreens;

	// Upgrades.
	CNetworkVar( int, m_iUpgradeMetalRequired );
	CNetworkVar( bool, m_bCarried );		
	CNetworkVar( bool, m_bCarryDeploy );
	CNetworkVar( bool, m_bMiniBuilding );

	CNetworkVar( bool, m_bDisposableBuilding );
	CNetworkVar( bool, m_bWasMapPlaced );

	// Remote PDA
	CNetworkVar( bool, m_bRemoteConstruction );

	// Outlining by tagging weapons
	CNetworkVar( bool, m_bOutlined );
	float		m_flOutlineTime;

private:
	// Make sure we pick up changes to these.
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_iHealth );
	IMPLEMENT_NETWORK_VAR_FOR_DERIVED( m_takedamage );

	Activity	m_Activity;

	CNetworkVar( int, m_iObjectType );
	CNetworkVar( int, m_iObjectMode );


	// True if players shouldn't do collision avoidance, but should just collide exactly with the object.
	OBJSOLIDTYPE	m_SolidToPlayers;
	void		SetSolidToPlayers( OBJSOLIDTYPE stp, bool force = false );

	// Disabled
	CNetworkVar( bool, m_bDisabled );

	// Building
	CNetworkVar( bool, m_bBuilding );				// True while the object's still constructing itself
	CNetworkVar( bool, m_bPlacing );				// True while the object's being placed
	float	m_flConstructionTimeLeft;	// Current time left in construction
	float	m_flTotalConstructionTime;	// Total construction time (the value of GetTotalTime() at the time construction 
										// started, ie, incase you teleport out of a construction yard)

	CNetworkVar( float, m_flPercentageConstructed );	// Used to send to client
	float	m_flHealth;					// Health during construction. Needed a float due to small increases in health.

	// Sapper on me
	CNetworkVar( bool, m_bHasSapper );

	// Build points
	CUtlVector<BuildPoint_t>	m_BuildPoints;

	// Maps player ent index to repair expire time
	struct constructor_t
	{
		float flHitTime; // Time this constructor last hit me
		float flValue;	 // Speed value of this constructor. Defaults to 1.0, but some constructors are worth more.
	};
	CUtlMap<int, constructor_t>	m_ConstructorList;

	// Result of last placement test
	bool		m_bPlacementOK;				// last placement state

	CNetworkVar( int, m_iKills );
	CNetworkVar( int, m_iAssists );

	CNetworkVar( int, m_iDesiredBuildRotations );		// Number of times we've rotated, used to calc final rotation
	float m_flCurrentBuildRotation;

	// Gibs.
	CUtlVector<breakmodel_t>	m_aGibs;

	CNetworkVar( bool, m_bServerOverridePlacement );

	// Used when calculating the placement position
	Vector	m_vecBuildForward;
	float	m_flBuildDistance;
};

extern short g_sModelIndexFireball;		// holds the index for the fireball

#endif // TF_OBJ_H
