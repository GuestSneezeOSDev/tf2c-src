//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Clients CBaseObject
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_BASEOBJECT_H
#define C_BASEOBJECT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseobject_shared.h"
#include <vgui_controls/Panel.h>
#include "particlemgr.h"
#include "particle_prototype.h"
#include "particle_util.h"
#include "c_basecombatcharacter.h"
#include "ihasbuildpoints.h"
#include "tf/tf_colorblind_helper.h"

// Warning levels for buildings in the building hud, in priority order
typedef enum
{
	BUILDING_HUD_ALERT_NONE = 0,
	BUILDING_HUD_ALERT_LOW_AMMO,
	BUILDING_HUD_ALERT_LOW_HEALTH,
	BUILDING_HUD_ALERT_VERY_LOW_AMMO,
	BUILDING_HUD_ALERT_VERY_LOW_HEALTH,
	BUILDING_HUD_ALERT_SAPPER,

	MAX_BUILDING_HUD_ALERT_LEVEL
} BuildingHudAlert_t;

typedef enum
{
	BUILDING_DAMAGE_LEVEL_NONE = 0,		// 100%
	BUILDING_DAMAGE_LEVEL_LIGHT,		// 75% - 99%
	BUILDING_DAMAGE_LEVEL_MEDIUM,		// 50% - 76%
	BUILDING_DAMAGE_LEVEL_HEAVY,		// 25% - 49%	
	BUILDING_DAMAGE_LEVEL_CRITICAL,		// 0% - 24%

	MAX_BUILDING_DAMAGE_LEVEL
} BuildingDamageLevel_t;

class C_TFPlayer;

// Max Length of ID Strings
#define MAX_ID_STRING		256

extern mstudioevent_t *GetEventIndexForSequence( mstudioseqdesc_t &seqdesc );


class C_BaseObject : public C_BaseCombatCharacter, public IHasBuildPoints, public ITargetIDProvidesHint
{
	DECLARE_CLASS( C_BaseObject, C_BaseCombatCharacter );
public:
	DECLARE_CLIENTCLASS();

	C_BaseObject();
	~C_BaseObject( void );

	virtual void	Spawn( void );

	virtual bool	IsBaseObject( void ) const { return true; }
	virtual bool	IsAnUpgrade(void ) const { return false; }

	void			SetType( int iObjectType ) { m_iObjectType = iObjectType; }

	virtual void	AddEntity();

	void			SetActivity( Activity act );
	Activity		GetActivity( ) const;
	void			SetObjectSequence( int sequence );

	virtual void	PreDataUpdate( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );
	
	virtual void	UpdateClientSideGlow();
	virtual void	GetGlowEffectColor( float* r, float* g, float* b );

	virtual int		GetHealth() const            { return m_iHealth; }
	void			SetHealth( int health )      { m_iHealth = health; }
	virtual int		GetMaxHealth() const         { return m_iMaxHealth; }
	int				GetObjectFlags( void ) const { return m_fObjectFlags; }
	void			SetObjectFlags( int flags )  { m_fObjectFlags = flags; }

	void			SetupAttachedVersion( void ) { return; }

	const char		*GetTargetDescription( void ) const { return GetStatusName(); }
	virtual const char	*GetIDString( void );
	virtual bool	IsValidIDTarget( void );

	virtual void	GetTargetIDString( wchar_t *sIDString, int iMaxLenInBytes );
	virtual void	GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes );

	void			AttemptToGoActive( void );
	bool			ShouldBeActive( void ) const;
	void			OnGoActive( void );
	void			OnGoInactive( void );

	virtual void	UpdateOnRemove( void );

	C_TFPlayer		*GetBuilder( void ) const     { return m_hBuilder; }
	int				GetBuilderIndex( void ) const { return m_hBuilder.IsValid() ? m_hBuilder.GetEntryIndex() : 0; }

	virtual void	SetDormant( bool bDormant );

	void			SendClientCommand( const char *pCmd );

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );

	// Colourblind Effect
	virtual void		UpdateTeamPatternEffect(void);
	virtual void		DestroyTeamPatternEffect(void);
	bool				m_bColorBlindInitialised;
	CTeamPatternObject	*m_pTeamPatternEffect;
	CTeamPatternObject	*GetTeamPatternObject(void){ return m_pTeamPatternEffect; }

	// Builder preview...
	void			ActivateYawPreview( bool enable ) { m_YawPreviewState = enable ? YAW_PREVIEW_ON : YAW_PREVIEW_WAITING_FOR_UPDATE; }
	void			PreviewYaw( float yaw )           { m_fYawPreview = yaw; }
	bool			IsPreviewingYaw() const           { return m_YawPreviewState != YAW_PREVIEW_OFF; }
	
	void			RecalculateIDString( void );

	int				GetType() const       { return m_iObjectType; }
	int				GetObjectMode() const { return m_iObjectMode; }
	bool			IsOwnedByLocalPlayer() const;

	virtual void	Simulate();

	virtual int		DrawModel( int flags );

	float			GetPercentageConstructed( void ) const { return m_flPercentageConstructed; }

	bool			IsPlacing( void ) const     { return m_bPlacing; }
	bool			IsBuilding( void ) const    { return m_bBuilding; }
	virtual bool	IsUpgrading( void ) const   { return false; }

	bool			IsBeingCarried( void ) const  { return m_bCarried; }
	bool			IsRedeploying( void ) const { return m_bCarryDeploy; }

	// Remote PDA
	bool			InRemoteConstruction( void ) const { return m_bRemoteConstruction; }
	void			SetRemoteConstruction( bool bValue ) { m_bRemoteConstruction = bValue; }

	void			FinishedBuilding( void ) { return; }

	const char*		GetStatusName() const { return GetObjectInfo( GetType() )->m_pStatusName; }
	virtual void	GetStatusText( wchar_t *pStatus, int iMaxStatusLen );

	// Object Previews
	void			HighlightBuildPoints( int flags );

	bool			HasSapper( void ) const { return m_bHasSapper; }

	void			OnStartDisabled( void );
	void			OnEndDisabled( void );

	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;
	bool			ShouldPlayersAvoid( void ) const;

	bool			MustBeBuiltOnAttachmentPoint( void ) const;

	virtual bool	IsHostileUpgrade( void ) const { return false; }

	// For ordering in hud building status
	int				GetDisplayPriority( void ) const { return GetObjectInfo( GetType() )->m_iDisplayPriority; }

	virtual const char *GetHudStatusIcon( void ) const { return GetObjectInfo( GetType() )->m_pHudStatusIcon; }

	virtual BuildingHudAlert_t GetBuildingAlertLevel( void ) const;

	// Upgrades
	int GetUpgradeLevel( void ) const    { return m_iUpgradeLevel; }
	int GetUpgradeMetal( void ) const    { return m_iUpgradeMetal; }
	int GetMaxUpgradeLevel( void ) const { return m_iHighestUpgradeLevel; }
	int GetUpgradeMetalRequired( void ) const { return m_iUpgradeMetalRequired; }

private:
	void StopAnimGeneratedSounds( void );

public:
	// Client/Server shared build point code
	void				CreateBuildPoints( void );
	void				AddAndParseBuildPoint( int iAttachmentNumber, KeyValues *pkvBuildPoint );
	int					AddBuildPoint( int iAttachmentNum );
	void				AddValidObjectToBuildPoint( int iPoint, int iObjectType );
	CBaseObject			*GetBuildPointObject( int iPoint ) const;
	bool				IsBuiltOnAttachment( void ) const { return m_hBuiltOnEntity.IsValid(); }
	void				AttachObjectToObject( CBaseEntity *pEntity, int iPoint, Vector &vecOrigin );
	CBaseObject			*GetParentObject( void );
	void				SetBuildPointPassenger( int iPoint, int iPassenger );

	// Build points
	CUtlVector<BuildPoint_t>	m_BuildPoints;

	bool				IsDisabled( void ) const { return m_bDisabled; }

	// Shared placement
	bool 				VerifyCorner( const Vector &vBottomCenter, float xOffset, float yOffset );
	bool				CalculatePlacementPos( void );
	virtual bool		IsPlacementPosValid( void );
	float				GetNearbyObjectCheckRadius( void ) const { return 30.0; }

	virtual void		OnPlacementStateChanged( bool bValidPlacement );

	// allow server to trump our placement state
	bool				ServerValidPlacement( void ) { return m_bServerOverridePlacement; }

	bool				WasLastPlacementPosValid( void );	// query if we're in a valid place, when we last tried to calculate it

// IHasBuildPoints
public:
	virtual int			GetNumBuildPoints( void ) const;
	virtual bool		GetBuildPoint( int iPoint, Vector &vecOrigin, QAngle &vecAngles );
	virtual int			GetBuildPointAttachmentIndex( int iPoint ) const;
	virtual bool		CanBuildObjectOnBuildPoint( int iPoint, int iObjectType ) const;
	virtual void		SetObjectOnBuildPoint( int iPoint, CBaseObject *pObject );
	virtual float		GetMaxSnapDistance( int iBuildPoint ) const;
	virtual bool		ShouldCheckForMovement( void ) const { return true; }
	virtual int			GetNumObjectsOnMe( void ) const;
	virtual CBaseObject *GetObjectOfTypeOnMe( int iObjectType ) const;
	virtual void		RemoveAllObjects( void );
	virtual int			FindObjectOnBuildPoint( CBaseObject *pObject ) const;

	virtual bool TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr );

// ITargetIDProvidesHint
public:
	virtual void		DisplayHintTo( C_BasePlayer *pPlayer );

protected:
	virtual void		UpdateDamageEffects( BuildingDamageLevel_t damageLevel ) {}	// default is no effects

	void				UpdateDesiredBuildRotation( float flFrameTime );

protected:

	BuildingDamageLevel_t CalculateDamageLevel( void );

	char			m_szIDString[ MAX_ID_STRING ];

	BuildingDamageLevel_t m_damageLevel;

	Vector m_vecBuildOrigin;
	Vector m_vecBuildCenterOfMass;

private:
	enum
	{
		YAW_PREVIEW_OFF	= 0,
		YAW_PREVIEW_ON,
		YAW_PREVIEW_WAITING_FOR_UPDATE
	};

	Activity		m_Activity;

	int				m_fObjectFlags;
	float			m_fYawPreview;
	char			m_YawPreviewState;
	CHandle< C_TFPlayer > m_hBuilder;
	bool			m_bWasActive;
	int				m_iOldHealth;
	bool			m_bHasSapper;
	bool			m_bOldSapper;
	int				m_iObjectType;
	int				m_iHealth;
	int				m_iMaxHealth;
	bool			m_bWasBuilding;
	bool			m_bBuilding;
	bool			m_bWasPlacing;
	bool			m_bPlacing;
	bool			m_bCarried;
	bool			m_bWasCarried;
	bool			m_bCarryDeploy;
	bool			m_bRemoteConstruction;
	bool			m_bMiniBuilding;
	bool			m_bDisabled;
	bool			m_bOldDisabled;
	float			m_flPercentageConstructed;
	EHANDLE			m_hBuiltOnEntity;

	CNetworkVector( m_vecBuildMaxs );
	CNetworkVector( m_vecBuildMins );

	CNetworkVar( int, m_iDesiredBuildRotations );
	float m_flCurrentBuildRotation;

	int m_iLastPlacementPosValid;	// -1 - init, 0 - invalid, 1 - valid

	CNetworkVar( bool, m_bServerOverridePlacement );

	// Outlining by tagging weapons
	CNetworkVar( bool, m_bOutlined );

	int m_nObjectOldSequence;

	// Used when calculating the placement position
	Vector	m_vecBuildForward;
	float	m_flBuildDistance;

protected:

	int m_iUpgradeLevel;
	int	m_iOldUpgradeLevel;
	int m_iUpgradeMetal;
	int m_iUpgradeMetalRequired;
	int m_iHighestUpgradeLevel;
	int m_iObjectMode;
	bool m_bDisposableBuilding;
	bool m_bWasMapPlaced;

private:
	C_BaseObject( const C_BaseObject & ); // not defined, not accessible
};

#endif // C_BASEOBJECT_H
