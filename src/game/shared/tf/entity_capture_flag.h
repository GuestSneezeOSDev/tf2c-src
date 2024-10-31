//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Flag.
//
//=============================================================================//
#ifndef ENTITY_CAPTURE_FLAG_H
#define ENTITY_CAPTURE_FLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_item.h"
#include "SpriteTrail.h"
#include "GameEventListener.h"

#ifdef GAME_DLL
class CTFBot;
#endif

#ifdef CLIENT_DLL
#define CCaptureFlag C_CaptureFlag
#endif

//=============================================================================
//
// CTF Flag defines.
//

#define TF_CTF_FLAGSPAWN			"CaptureFlag.FlagSpawn"

#define TF_CTF_CAPTURED_TEAM_FRAGS	1

#define TF_CTF_RESET_TIME			60.0f

#define TF2C_CTF_MINIMUM_RESET_TIME	8.0f

//=============================================================================
//
// Attack/Defend Flag defines.
//

#define TF_AD_CAPTURED_SOUND		"AttackDefend.Captured"

#define TF_AD_CAPTURED_FRAGS		30
#define TF_AD_RESET_TIME			60.0f

//=============================================================================
//
// Invade Flag defines.
//

#define TF_INVADE_CAPTURED_FRAGS		10
#define TF_INVADE_CAPTURED_TEAM_FRAGS	1

#define TF_INVADE_RESET_TIME			60.0f
#define TF_INVADE_NEUTRAL_TIME			30.0f

//=============================================================================
//
// Special Delivery defines.
//

#define TF_SD_FLAGSPAWN				"Resource.FlagSpawn"

#ifdef CLIENT_DLL
	#define CCaptureFlagReturnIcon C_CaptureFlagReturnIcon
	#define CBaseAnimating C_BaseAnimating
#endif

#ifdef CLIENT_DLL

typedef struct
{
	float maxProgress;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} progress_segment_t;

extern progress_segment_t Segments[8];

#endif

class CCaptureFlagReturnIcon: public CBaseAnimating
{
public:
	DECLARE_CLASS( CCaptureFlagReturnIcon, CBaseEntity );
	DECLARE_NETWORKCLASS();

	CCaptureFlagReturnIcon();

#ifdef CLIENT_DLL

	virtual int		DrawModel( int flags );
	void			DrawReturnProgressBar( void );

	virtual RenderGroup_t GetRenderGroup( void );
	virtual bool	ShouldDraw( void ) { return true; }

	virtual void GetRenderBounds( Vector& theMins, Vector& theMaxs );

private:

	IMaterial	*m_pReturnProgressMaterial_Empty;		// For labels above players' heads.
	IMaterial	*m_pReturnProgressMaterial_Full;

#else
public:
	virtual void Spawn( void );
	virtual int UpdateTransmitState( void );

#endif

};

//=============================================================================
//
// CTF Flag class.
//
class CCaptureFlag : public CTFItem, public TAutoList<CCaptureFlag>, public CGameEventListener
{
public:

	DECLARE_CLASS( CCaptureFlag, CTFItem );
	DECLARE_NETWORKCLASS();

	CCaptureFlag();
	~CCaptureFlag();

	enum
	{
		TF_FLAGEFFECTS_NONE = 0,
		TF_FLAGEFFECTS_ALL,
		TF_FLAGEFFECTS_PAPERTRAIL_ONLY,
		TF_FLAGEFFECTS_COLORTRAIL_ONLY,
	};

	enum
	{
		TF_FLAGNEUTRAL_NEVER = 0,
		TF_FLAGNEUTRAL_DEFAULT,
		TF_FLAGNEUTRAL_HALFRETURN,
	};

	enum
	{
		TF_FLAGSCORING_SCORE = 0,
		TF_FLAGSCORING_CAPS,
	};

	unsigned int	GetItemID( void ) { return TF_ITEM_CAPTURE_FLAG; }

	CBaseEntity		*GetPrevOwner( void ) { return m_hPrevOwner.Get(); }

	ETFFlagType		GetGameType( void ) { return m_nGameType; }

	bool			IsDropped( void ) { return ( m_nFlagStatus == TF_FLAGINFO_DROPPED ); }
	bool			IsHome( void ) { return ( m_nFlagStatus == TF_FLAGINFO_NONE ); }
	bool			IsStolen( void ) { return ( m_nFlagStatus == TF_FLAGINFO_STOLEN ); }
	bool			IsDisabled( void ) { return m_bDisabled; }

	bool			IsVisibleWhenDisabled( void ) { return m_bVisibleWhenDisabled; }
	int				GetScoringType( void ) { return m_nScoringType; }

	virtual void	FireGameEvent( IGameEvent *event );

// Game DLL Functions
#ifdef GAME_DLL

	virtual bool	KeyValue( const char *szKeyName, const char *szValue );
	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	Activate( void );

	virtual void	Reset( void );
	void			ResetMessage( void );
	void			ResetExplode( void );

	bool			CanTouchThisFlagType( const CBaseEntity *pOther );
	bool			TeamCanTouchThisFlag( const CTFPlayer *pPlayer );
	void			FlagTouch( CBaseEntity *pOther );

	void			Capture( CTFPlayer *pPlayer, int nCapturePoint );
	virtual void	PickUp( CTFPlayer *pPlayer, bool bInvisible );
	virtual void	Drop( CTFPlayer *pPlayer, bool bVisible, bool bThrown = false, bool bMessage = true );

	void			SetDisabled( bool bDisabled );

	// Input handlers
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputRoundActivate( inputdata_t &inputdata );
	void			InputForceDrop( inputdata_t &inputdata );
	void			InputForceReset( inputdata_t &inputdata );
	void			InputForceResetSilent( inputdata_t &inputdata );
	void			InputForceResetAndDisableSilent( inputdata_t &inputdata );
	void			InputSetReturnTime( inputdata_t &inputdata );
	void			InputShowTimer( inputdata_t &inputdata );
	void			InputForceGlowDisabled( inputdata_t &inputdata );
	void			InputSetLocked( inputdata_t &inputdata );
	void			InputSetUnlockTime( inputdata_t &inputdata );
	void			InputSetTeamCanPickup( inputdata_t& inputdata );

	void			Think( void );
	virtual void	ThinkDropped( void );
	
	void			SetFlagStatus( int iStatus );
	int				GetFlagStatus( void ) { return m_nFlagStatus; };
	void			ResetFlagReturnTime( void ) { m_flResetTime = 0; }
	void			SetFlagReturnIn( float flTime )
	{
		m_flResetTime = gpGlobals->curtime + flTime;
	}

	void			ResetFlagNeutralTime( void ) { m_flNeutralTime = 0; }
	void			SetFlagNeutralIn( float flTime )
	{ 
		m_flNeutralTime = gpGlobals->curtime + flTime;
	}
	bool			IsCaptured( void ) { return m_bCaptured; }

	int				UpdateTransmitState( void );

	void			GetTrailEffect( int iTeamNum, char *pszBuf, int iBufSize );
	void			ManageSpriteTrail( void );

	void			UpdateReturnIcon( void );

	void			AddFollower( CTFBot *pBot );
	void			RemoveFollower( CTFBot *pBot );

	void			SetLocked( bool bLocked );

	Vector			GetResetPos( void ) { return m_vecResetPos; }
	int				GetLimitToClass( void ) { return m_iLimitToClass; }
	void			SetTeamCanPickup( int iTeam, bool bCanPickup );

#else // CLIENT DLL Functions

	virtual const char	*GetIDString(void) { return "entity_capture_flag"; };

	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual void	OnDataChanged( DataUpdateType_t updateType );

	virtual int		GetSkin( void );
	virtual void	Simulate( void );
	void			ManageTrailEffects( void );
	void			UpdateFlagVisibility( void );

	float			GetResetDelay() { return m_flResetDelay; }
	float			GetReturnProgress( void );

	void			UpdateGlowEffect( void );

	void			GetHudIcon( int iTeamNum, char *pszBuf, int buflen );
	bool			GetMyTeamCanPickup();

#endif

private:

	CNetworkVar( bool,			m_bDisabled );	// Enabled/Disabled?
	CNetworkVar( ETFFlagType,	m_nGameType );	// Type of game this flag will be used for.
	CNetworkVar( int, m_nFlagStatus );

	CNetworkVar( float, m_flResetDelay );

protected:
	CNetworkVar( float, m_flResetTime );		// Time until the flag is placed back at spawn.
private:
	CNetworkVar( float, m_flNeutralTime );	// Time until the flag becomes neutral (used for the invade gametype)
	CNetworkHandle( CBaseEntity, m_hPrevOwner );


	CNetworkVar( int, m_nUseTrailEffect );
	CNetworkString( m_szHudIcon, MAX_PATH );
	CNetworkString( m_szPaperEffect, MAX_PATH );
	CNetworkVar( bool, m_bVisibleWhenDisabled );
	CNetworkVar( bool, m_bGlowEnabled );
	CNetworkVar( int, m_nNeutralType );
	CNetworkVar( int, m_nScoringType );

	CNetworkVar( bool, m_bLocked );
	CNetworkVar( float, m_flUnlockTime );
	CNetworkVar( float, m_flUnlockDelay );

	CNetworkVar( float, m_flPickupTime );
	CNetworkVar( bool, m_bTeamRedCanPickup );
	CNetworkVar( bool, m_bTeamBlueCanPickup );
	CNetworkVar( bool, m_bTeamGreenCanPickup );
	CNetworkVar( bool, m_bTeamYellowCanPickup );
	CNetworkVar( bool, m_bTeamDisableAfterCapture );

protected:
	int				m_iOriginalTeam;
private:
	float			m_flOwnerPickupTime;

	EHANDLE		m_hReturnIcon;

	// Parenting
	EHANDLE		m_pOriginalParent;
	unsigned char	m_iOriginalParentAttachment;

	Vector		m_vParentOffset;
	QAngle		m_qParentOffset;

#ifdef GAME_DLL
	DECLARE_DATADESC();

protected:
	Vector			m_vecResetPos;		// The position the flag should respawn (reset) at.
	QAngle			m_vecResetAng;		// The angle the flag should respawn (reset) at.
private:

	COutputEvent	m_outputOnReturn;	// Fired when the flag is returned via timer.
	COutputEvent	m_outputOnPickUp;	// Fired when the flag is picked up.
	COutputEvent	m_outputOnPickUpTeam1;	
	COutputEvent	m_outputOnPickUpTeam2;	
	COutputEvent	m_outputOnPickUpTeam3;	
	COutputEvent	m_outputOnPickUpTeam4;	
	COutputEvent	m_outputOnDrop;		// Fired when the flag is dropped.
	COutputEvent	m_outputOnCapture;	// Fired when the flag is captured.
	COutputEvent	m_outputOnCapTeam1;	
	COutputEvent	m_outputOnCapTeam2;
	COutputEvent	m_outputOnCapTeam3;	
	COutputEvent	m_outputOnCapTeam4;	
	COutputEvent	m_outputOnTouchSameTeam;

protected:
	bool			m_bAllowOwnerPickup;
private:
	bool			m_bCaptured;
	string_t		m_szModel;
	string_t		m_szTrailEffect;
	CHandle<CSpriteTrail>	m_hGlowTrail;

	CUtlVector<CHandle<CTFBot>> m_BotFollowers;

	int				m_iLimitToClass;

	bool			m_bExplodeOnReturn;
	string_t		m_szExpolosionParticle;
	string_t		m_szExplosionSound;
	int				m_iExplosionRadius;
	int				m_iExplosionDamage;

#else
	IMaterial	*m_pReturnProgressMaterial_Empty;		// For labels above players' heads.
	IMaterial	*m_pReturnProgressMaterial_Full;		

	int			m_iOldTeam;
	int			m_nOldFlagStatus;
	bool		m_bWasGlowEnabled;

	CNewParticleEffect	*m_pPaperTrailEffect;
	CGlowObject	*m_pGlowEffect;
#endif
};

#endif // ENTITY_CAPTURE_FLAG_H