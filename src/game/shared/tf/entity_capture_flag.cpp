//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF Flag.
//
//=============================================================================//
#include "cbase.h"
#include "entity_capture_flag.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"

#ifdef CLIENT_DLL
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui/IScheme.h>
#include "hudelement.h"
#include "iclientmode.h"
#include "hud_numericdisplay.h"
#include "tf_imagepanel.h"
#include "c_tf_player.h"
#include "c_tf_team.h"
#include "tf_hud_objectivestatus.h"
#include "view.h"
#include "glow_outline_effect.h"
#include "functionproxy.h"

ConVar cl_flag_return_size( "cl_flag_return_size", "20", FCVAR_CHEAT );

#else
#include "tf_player.h"
#include "tf_team.h"
#include "tf_objective_resource.h"
#include "tf_gamestats.h"
#include "func_respawnroom.h"
#include "datacache/imdlcache.h"
#include "func_respawnflag.h"
#include "func_flagdetectionzone.h"
#include "tf_control_point_master.h"
#include "tf_bot.h"
#include "tf_announcer.h"
#include "tf_fx.h"

#define DEFAULT_EXPLOSION_PARTICLE "ExplosionCore_MidAir"
#define DEFAULT_EXPLOSION_SOUND "Weapon_Grenade_Mirv.MainExplode"
#define DEFAULT_EXPLOSION_DMG 80
#define DEFAULT_EXPLOSION_RADIUS 250

extern ConVar tf_flag_caps_per_round;

ConVar cl_flag_return_height( "cl_flag_return_height", "82", FCVAR_CHEAT );
ConVar tf2c_ctf_touch_return( "tf2c_ctf_touch_return", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "If enabled, flags instantly return to their base when touched by a teammate." );
ConVar tf2c_ctf_reset_time_decay( "tf2c_ctf_reset_time_decay", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Carried flags remember their reset time (instead of resetting to full)" );
ConVar tf2c_ctf_4team_no_repeats( "tf2c_ctf_4team_no_repeats", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "You can capture each enemy team's flag only once. (Neutral flags unaffected.)" );

extern CUtlVector<CHandle<CTeamControlPointMaster>> g_hControlPointMasters;
#endif

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Used to make flag glow with color of its carrier.
//-----------------------------------------------------------------------------
class CProxyFlagGlow : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetVecValue( 1, 1, 1 );
			return;
		}

		Vector vecColor( 1, 1, 1 );

		C_TFPlayer *pPlayer = ToTFPlayer( pEntity->GetMoveParent() );

		if ( pPlayer )
		{
			switch ( pPlayer->GetTeamNumber() )
			{
			case TF_TEAM_RED:
				vecColor = Vector( 1.7f, 0.5f, 0.5f );
				break;
			case TF_TEAM_BLUE:
				vecColor = Vector( 0.7f, 0.7f, 1.9f );
				break;
			case TF_TEAM_GREEN:
				vecColor = Vector( 0.2f, 1.5f, 0.2f );
				break;
			case TF_TEAM_YELLOW:
				vecColor = Vector( 1.6f, 1.4f, 0.1f );
				break;
			}
		}

		m_pResult->SetVecValue( vecColor.Base(), 3 );
	}
};

EXPOSE_INTERFACE( CProxyFlagGlow, CResultProxy, "FlagGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif

//=============================================================================
//
// CTF Flag tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlag, DT_CaptureFlag )

BEGIN_NETWORK_TABLE( CCaptureFlag, DT_CaptureFlag )

#ifdef GAME_DLL
	SendPropBool( SENDINFO( m_bDisabled ) ),
	SendPropInt( SENDINFO( m_nGameType ), 5, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nFlagStatus ), 3, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flResetTime ) ),
	SendPropTime( SENDINFO( m_flNeutralTime ) ),
	SendPropTime( SENDINFO( m_flResetDelay ) ),
	SendPropEHandle( SENDINFO( m_hPrevOwner ) ),
	SendPropInt( SENDINFO( m_nUseTrailEffect ), 2, SPROP_UNSIGNED ),
	SendPropString( SENDINFO( m_szHudIcon ) ),
	SendPropString( SENDINFO( m_szPaperEffect ) ),
	SendPropBool( SENDINFO( m_bVisibleWhenDisabled ) ),
	SendPropInt( SENDINFO( m_nNeutralType ), 2, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nScoringType ), 1, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bGlowEnabled ) ),
	SendPropBool( SENDINFO( m_bLocked ) ),
	SendPropTime( SENDINFO( m_flUnlockTime ) ),
	SendPropTime( SENDINFO( m_flUnlockDelay ) ),
	SendPropTime( SENDINFO( m_flPickupTime ) ),
	SendPropBool( SENDINFO( m_bTeamRedCanPickup ) ),
	SendPropBool( SENDINFO( m_bTeamBlueCanPickup ) ),
	SendPropBool( SENDINFO( m_bTeamGreenCanPickup ) ),
	SendPropBool( SENDINFO( m_bTeamYellowCanPickup ) ),
	SendPropBool( SENDINFO( m_bTeamDisableAfterCapture ) ),
#else
	RecvPropInt( RECVINFO( m_bDisabled ) ),
	RecvPropInt( RECVINFO( m_nGameType ) ),
	RecvPropInt( RECVINFO( m_nFlagStatus ) ),
	RecvPropTime( RECVINFO( m_flResetTime ) ),
	RecvPropTime( RECVINFO( m_flNeutralTime ) ),
	RecvPropTime( RECVINFO( m_flResetDelay ) ),
	RecvPropEHandle( RECVINFO( m_hPrevOwner ) ),
	RecvPropInt( RECVINFO( m_nUseTrailEffect ) ),
	RecvPropString( RECVINFO( m_szHudIcon ) ),
	RecvPropString( RECVINFO( m_szPaperEffect ) ),
	RecvPropBool( RECVINFO( m_bVisibleWhenDisabled ) ),
	RecvPropInt( RECVINFO( m_nNeutralType ) ),
	RecvPropInt( RECVINFO( m_nScoringType ) ),
	RecvPropBool( RECVINFO( m_bGlowEnabled ) ),
	RecvPropBool( RECVINFO( m_bLocked ) ),
	RecvPropTime( RECVINFO( m_flUnlockTime ) ),
	RecvPropTime( RECVINFO( m_flUnlockDelay ) ),
	RecvPropTime( RECVINFO( m_flPickupTime ) ),
	RecvPropBool( RECVINFO( m_bTeamRedCanPickup ) ),
	RecvPropBool( RECVINFO( m_bTeamBlueCanPickup ) ),
	RecvPropBool( RECVINFO( m_bTeamGreenCanPickup ) ),
	RecvPropBool( RECVINFO( m_bTeamYellowCanPickup ) ),
	RecvPropBool( RECVINFO( m_bTeamDisableAfterCapture ) ),
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CCaptureFlag )

	// Keyfields.
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_KEYFIELD( m_flResetDelay, FIELD_FLOAT, "ReturnTime" ),
	DEFINE_KEYFIELD( m_nGameType, FIELD_INTEGER, "GameType" ),
	DEFINE_KEYFIELD( m_szModel, FIELD_STRING, "flag_model" ),
	DEFINE_KEYFIELD( m_szTrailEffect, FIELD_STRING, "flag_trail" ),
	DEFINE_KEYFIELD( m_nUseTrailEffect, FIELD_INTEGER, "trail_effect" ),
	DEFINE_KEYFIELD( m_bVisibleWhenDisabled, FIELD_BOOLEAN, "VisibleWhenDisabled" ),
	DEFINE_KEYFIELD( m_nNeutralType, FIELD_INTEGER, "NeutralType" ),
	DEFINE_KEYFIELD( m_nScoringType, FIELD_INTEGER, "ScoringType" ),
	DEFINE_KEYFIELD( m_bLocked, FIELD_BOOLEAN, "StartLocked" ),
	DEFINE_KEYFIELD( m_iLimitToClass, FIELD_INTEGER, "LimitToClass" ),
	DEFINE_KEYFIELD( m_bExplodeOnReturn, FIELD_BOOLEAN, "ExplodeOnReturn" ),
	DEFINE_KEYFIELD( m_szExpolosionParticle, FIELD_STRING, "ExplosionParticle" ),
	DEFINE_KEYFIELD( m_szExplosionSound, FIELD_STRING, "ExplosionSound" ),
	DEFINE_KEYFIELD( m_iExplosionDamage, FIELD_INTEGER, "ExplosionDamage" ),
	DEFINE_KEYFIELD( m_iExplosionRadius, FIELD_INTEGER, "ExplosionRadius" ),

	// Inputs.
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RoundActivate", InputRoundActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceDrop", InputForceDrop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceReset", InputForceReset ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceResetSilent", InputForceResetSilent ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceResetAndDisableSilent", InputForceResetAndDisableSilent ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetReturnTime", InputSetReturnTime ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "ShowTimer", InputShowTimer ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "ForceGlowDisabled", InputForceGlowDisabled ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetLocked", InputSetLocked ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetUnlockTime", InputSetUnlockTime ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTeamCanPickup", InputSetTeamCanPickup ),

	// Outputs.
	DEFINE_OUTPUT( m_outputOnReturn, "OnReturn" ),
	DEFINE_OUTPUT( m_outputOnPickUp, "OnPickUp" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam1, "OnPickupTeam1" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam2, "OnPickupTeam2" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam3, "OnPickupTeam3" ),
	DEFINE_OUTPUT( m_outputOnPickUpTeam4, "OnPickupTeam4" ),
	DEFINE_OUTPUT( m_outputOnDrop, "OnDrop" ),
	DEFINE_OUTPUT( m_outputOnCapture, "OnCapture" ),
	DEFINE_OUTPUT( m_outputOnCapTeam1, "OnCapTeam1" ),
	DEFINE_OUTPUT( m_outputOnCapTeam2, "OnCapTeam2" ),
	DEFINE_OUTPUT( m_outputOnCapTeam3, "OnCapTeam3" ),
	DEFINE_OUTPUT( m_outputOnCapTeam4, "OnCapTeam4" ),
	DEFINE_OUTPUT( m_outputOnTouchSameTeam, "OnTouchSameTeam" ),

END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( item_teamflag, CCaptureFlag );

//=============================================================================
//
// CTF Flag functions.
//

CCaptureFlag::CCaptureFlag()
{
#ifdef CLIENT_DLL
	m_pPaperTrailEffect = NULL;
	m_pGlowEffect = NULL;
	m_bWasGlowEnabled = false;

	ListenForGameEvent( "localplayer_changeteam" );
#else
	m_hReturnIcon = NULL;
	m_hGlowTrail = NULL;
	m_flResetDelay = TF_CTF_RESET_TIME;
	m_bGlowEnabled = true;
	m_nNeutralType = TF_FLAGNEUTRAL_DEFAULT;
	m_nScoringType = TF_FLAGSCORING_SCORE;

	m_szModel = MAKE_STRING( "models/flag/briefcase.mdl" );
	m_szTrailEffect = MAKE_STRING( "flagtrail" );
	V_strncpy( m_szHudIcon.GetForModify(), "../hud/objectives_flagpanel_carried", MAX_PATH );
	V_strncpy( m_szPaperEffect.GetForModify(), "player_intel_papertrail", MAX_PATH );

	m_iLimitToClass = TF_CLASS_UNDEFINED;

	m_bExplodeOnReturn = false;
	m_szExpolosionParticle = MAKE_STRING( DEFAULT_EXPLOSION_PARTICLE );
	m_szExplosionSound = MAKE_STRING( DEFAULT_EXPLOSION_SOUND );
	m_iExplosionRadius = DEFAULT_EXPLOSION_RADIUS;
	m_iExplosionDamage = DEFAULT_EXPLOSION_DMG;
#endif	

	UseClientSideAnimation();
	m_nUseTrailEffect = TF_FLAGEFFECTS_ALL;
	m_flUnlockTime = -1.0f;

	m_flPickupTime = -1.0f;

	m_bTeamRedCanPickup = true;
	m_bTeamBlueCanPickup = true;
	m_bTeamGreenCanPickup = true;
	m_bTeamYellowCanPickup = true;
	m_bTeamDisableAfterCapture = false;
}

CCaptureFlag::~CCaptureFlag()
{
#ifdef CLIENT_DLL
	delete m_pGlowEffect;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Precache the model and sounds.
//-----------------------------------------------------------------------------
void CCaptureFlag::FireGameEvent( IGameEvent *event )
{
#ifdef CLIENT_DLL
	if ( V_strcmp( event->GetName(), "localplayer_changeteam" ) == 0 )
	{
		UpdateGlowEffect();
		UpdateFlagVisibility();
	}
#endif
}

#ifdef GAME_DLL

bool CCaptureFlag::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "flag_icon" ) )
	{
		V_strncpy( m_szHudIcon.GetForModify(), szValue, MAX_PATH );
		return true;
	}
	if ( FStrEq( szKeyName, "flag_paper" ) )
	{
		V_strncpy( m_szPaperEffect.GetForModify(), szValue, MAX_PATH );
		return true;
	}
	if ( !V_strncmp( szKeyName, "team_cantpickup_", 16 ) )
	{
		int iTeam = atoi( szKeyName + 16 );
		Assert( iTeam >= 0 && iTeam < TF_TEAM_COUNT );

		SetTeamCanPickup( iTeam, ( atoi( szValue ) == 0 ) );
		return true;
	}
	if ( FStrEq( szKeyName, "team_disable_after_capture" ) )
	{
		m_bTeamDisableAfterCapture = ( atoi( szValue ) != 0 );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//-----------------------------------------------------------------------------
// Purpose: Precache the model and sounds.
//-----------------------------------------------------------------------------
void CCaptureFlag::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( STRING( m_szModel ) );

	// Team colored trail
	for ( int i = FIRST_GAME_TEAM; i < TF_TEAM_COUNT; i++ )
	{
		char szModel[MAX_PATH];
		GetTrailEffect( i, szModel, MAX_PATH );
		PrecacheModel( szModel );
	}

	PrecacheParticleSystem( m_szPaperEffect );

	PrecacheScriptSound( TF_CTF_FLAGSPAWN );
	PrecacheScriptSound( TF_AD_CAPTURED_SOUND );
	PrecacheScriptSound( TF_SD_FLAGSPAWN );

	PrecacheMaterial( "vgui/flagtime_full" );
	PrecacheMaterial( "vgui/flagtime_empty" );

	PrecacheParticleSystem( STRING ( m_szExpolosionParticle ) );
	PrecacheScriptSound( STRING ( m_szExplosionSound ) );
}


void CCaptureFlag::Spawn( void )
{
	if ( m_szModel == NULL_STRING )
	{
		Warning( "item_teamflag at %.0f %.0f %0.f missing modelname\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
		UTIL_Remove( this );
		return;
	}

	// Precache the model and sounds.  Set the flag model.
	Precache();

	SetRenderMode( kRenderTransColor );
	//AddEffects( EF_BONEMERGE_FASTCULL );

	// Set the flag solid and the size for touching.
	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	SetMoveType( MOVETYPE_NONE );
	m_takedamage = DAMAGE_NO;
	
	SetModel( STRING( m_szModel ) );

	// Base class spawn.
	BaseClass::Spawn();

	// Set collision bounds so that they're independent from the model.
	SetCollisionBounds( Vector( -19.5f, -22.5f, -6.5f ), Vector( 19.5f, 22.5f, 6.5f ) );

	// Bloat the box for player pickup
	CollisionProp()->UseTriggerBounds( true, 24 );

	// Save the starting position, so we can reset the flag later if need be.
	m_vecResetPos = GetAbsOrigin();
	m_vecResetAng = GetAbsAngles();

	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	m_bCaptured = false;

	if ( GetParent() )
	{
		m_pOriginalParent = GetParent();
		m_iOriginalParentAttachment = GetParentAttachment();

		m_vParentOffset = GetParent()->GetAbsOrigin() - GetAbsOrigin();
		m_qParentOffset = GetParent()->GetAbsAngles() - GetAbsAngles();
	}

	/*
	for ( int i = FIRST_GAME_TEAM; i < TF_TEAM_COUNT; i++ )
	{
		SetTeamCanPickup( i, true );
	}
	*/

	SetDisabled( m_bDisabled );
}


void CCaptureFlag::Activate( void )
{
	BaseClass::Activate();

	m_iOriginalTeam = GetTeamNumber();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the flag position state.
//-----------------------------------------------------------------------------
void CCaptureFlag::Reset( void )
{
	// Set the flag position.
	if ( m_pOriginalParent )
	{
		SetParent( m_pOriginalParent, m_iOriginalParentAttachment );

		SetAbsOrigin( m_pOriginalParent->GetAbsOrigin() - m_vParentOffset );
		SetAbsAngles( m_pOriginalParent->GetAbsAngles() - m_qParentOffset );
	}
	else
	{
		SetParent( NULL );

		SetAbsOrigin( m_vecResetPos );
		SetAbsAngles( m_vecResetAng );
	}

	// No longer dropped, if it was.
	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	m_bAllowOwnerPickup = true;
	m_hPrevOwner = NULL;

	if ( m_nGameType == TF_FLAGTYPE_INVADE || m_nGameType == TF_FLAGTYPE_RESOURCE_CONTROL )
	{
		ChangeTeam( m_iOriginalTeam );
	}

	SetMoveType( MOVETYPE_NONE );
}


void CCaptureFlag::ResetMessage( void )
{
	switch ( m_nGameType )
	{
	case TF_FLAGTYPE_CTF:
	case TF_FLAGTYPE_RETRIEVE:
	{
		// Tell the owning team that their intelligence has returned to their base!
		CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
		g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_CTF_ENEMYRETURNED );

		TFGameRules()->SendHudNotification( filterFriendly, HUD_NOTIFY_YOUR_FLAG_RETURNED, GetTeamNumber() );

		for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			if ( iTeam != GetTeamNumber() )
			{
				// Tell the enemy teams that the owning team's intelligence has returned to their base!
				CTeamRecipientFilter filterEnemy( iTeam, true );
				g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_CTF_TEAMRETURNED );

				if ( GetTeamNumber() <= TEAM_SPECTATOR )
				{
					TFGameRules()->SendHudNotification( filterEnemy, HUD_NOTIFY_NEUTRAL_FLAG_RETURNED, GetTeamNumber() );
				}
				else
				{
					TFGameRules()->SendHudNotification( filterEnemy, HUD_NOTIFY_ENEMY_FLAG_RETURNED, GetTeamNumber() );
				}
			}
		}

		// Returned sound
		EmitSound( TF_CTF_FLAGSPAWN );
		break;
	}
	case TF_FLAGTYPE_ATTACK_DEFEND:
	{
		// Tell the owning team that their intelligence has returned to their base!
		CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
		g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_AD_TEAMRETURNED );
		TFGameRules()->SendHudNotification( filterFriendly, "#TF_AD_FlagReturned", "ico_notify_flag_home", GetTeamNumber() );

		for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			if ( iTeam != GetTeamNumber() )
			{
				// Tell the enemy teams that the owning team's intelligence has returned to their base!
				CTeamRecipientFilter filterEnemy( iTeam, true );
				g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_AD_ENEMYRETURNED );
				TFGameRules()->SendHudNotification( filterEnemy, "#TF_AD_FlagReturned", "ico_notify_flag_home", iTeam );
			}
		}
		break;
	}
	case TF_FLAGTYPE_INVADE:
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			// The flag has returned!
			CTeamRecipientFilter filter( iTeam, true );
			g_TFAnnouncer.Speak( filter, TF_ANNOUNCER_INVADE_RETURNED );
			TFGameRules()->SendHudNotification( filter, "#TF_Invade_FlagReturned", "ico_notify_flag_home", iTeam );
		}
		break;
	}
	case TF_FLAGTYPE_RESOURCE_CONTROL:
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			// The australium is ready for pickup, GO!
			CTeamRecipientFilter filter( iTeam, true );
			g_TFAnnouncer.Speak( filter, TF_ANNOUNCER_SD_RETURNED );
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
		if ( event )
		{
			event->SetInt( "eventtype", TF_FLAGEVENT_RETURN );
			event->SetInt( "priority", 8 );
			event->SetInt( "team", GetTeamNumber() );

			gameeventmanager->FireEvent( event );
		}

		EmitSound( TF_SD_FLAGSPAWN );
		break;
	}
	case TF_FLAGTYPE_VIP:
	{
		// Tell the defenders that the intelligence has retured to the enemy base.
		CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
		g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_VIP_RETURNED );
		TFGameRules()->SendHudNotification( filterFriendly, "#TF_VIP_TeamDeviceReturned", "ico_notify_flag_home", GetTeamNumber() );

		for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			if ( iTeam != GetTeamNumber() )
			{
				// Tell the attackers that the intelligence has retured to their base.
				CTeamRecipientFilter filterEnemy( iTeam, true );
				g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_VIP_RETURNED );
				TFGameRules()->SendHudNotification( filterEnemy, "#TF_VIP_EnemyDeviceReturned", "ico_notify_flag_home", iTeam );
			}
		}
		break;
	}
	}

	// Output.
	m_outputOnReturn.FireOutput( this, this );

	UpdateReturnIcon();
}


void CCaptureFlag::ResetExplode( void )
{
	if( !m_bExplodeOnReturn )
		return;

	// Return explosion particle.
	CPVSFilter filter( GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0.0, STRING( m_szExpolosionParticle ), WorldSpaceCenter(), GetAbsAngles() );

	// Return explosion sound.
	EmitSound( STRING ( m_szExplosionSound ) );

	// Return explosion damage.
	CTakeDamageInfo info_modified( this, GetPrevOwner(), m_iExplosionDamage, DMG_BLAST | DMG_HALF_FALLOFF );
	RadiusDamage( info_modified, WorldSpaceCenter(), m_iExplosionRadius, CLASS_NONE, this );
}

//-----------------------------------------------------------------------------
// Purpose: Centralize gamemode-specific "can that team touch this flag" logic.
//-----------------------------------------------------------------------------
bool CCaptureFlag::CanTouchThisFlagType( const CBaseEntity *pOther )
{
	switch ( m_nGameType )
	{
	case TF_FLAGTYPE_CTF:
	case TF_FLAGTYPE_RETRIEVE:
		// Cannot touch flags owned by my own team
		return ( GetTeamNumber() != pOther->GetTeamNumber() );

	case TF_FLAGTYPE_VIP:
	case TF_FLAGTYPE_ATTACK_DEFEND:
	case TF_FLAGTYPE_TERRITORY_CONTROL:
		// Can only touch flags owned by my own team
		return ( GetTeamNumber() == pOther->GetTeamNumber() );

	case TF_FLAGTYPE_INVADE:
	case TF_FLAGTYPE_RESOURCE_CONTROL:
		// Neutral flags can be touched by anyone
		if ( GetTeamNumber() == TEAM_UNASSIGNED )
			return true;
		// Otherwise: Can only touch flags owned by my own team
		return ( GetTeamNumber() == pOther->GetTeamNumber() );

	default:
		// Any other flag type: sure, go ahead, touch all you want!
		return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Touch rules for teams on custom maps (especially 4Team).
//-----------------------------------------------------------------------------
bool CCaptureFlag::TeamCanTouchThisFlag( const CTFPlayer* pPlayer )
{
	switch ( pPlayer->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		return m_bTeamRedCanPickup;

	case TF_TEAM_BLUE:
		return m_bTeamBlueCanPickup;

	case TF_TEAM_GREEN:
		return m_bTeamGreenCanPickup;

	case TF_TEAM_YELLOW:
		return m_bTeamYellowCanPickup;

	default:
		return false;
	}
}

bool UTIL_ItemCanBeTouchedByPlayer( CBaseEntity *pItem, CBasePlayer *pPlayer );


void CCaptureFlag::FlagTouch( CBaseEntity *pOther )
{
	// Is the flag enabled?
	if ( IsDisabled() || IsStolen() )
		return;

	if ( m_bLocked )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	// Can only be touched by a live player.
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer || !pPlayer->IsAlive() )
		return;

	if ( m_iLimitToClass != TF_CLASS_UNDEFINED && pPlayer->GetPlayerClass()->GetClassIndex() != m_iLimitToClass )
		return;

	// Don't let the person who threw this flag pick it up until it hits the ground.
	// This way we can throw the flag to people, but not touch it as soon as we throw it ourselves
	if ( m_hPrevOwner.Get() && m_hPrevOwner.Get() == pOther && !m_bAllowOwnerPickup )
		return;

	// Prevent players from picking up flags through walls.
	if ( !UTIL_ItemCanBeTouchedByPlayer( this, pPlayer ) )
		return;

	if ( pOther->GetTeamNumber() == GetTeamNumber() )
	{
		m_outputOnTouchSameTeam.FireOutput( pOther, this );

		if ( tf2c_ctf_touch_return.GetBool() && m_nGameType == TF_FLAGTYPE_CTF && IsDropped() )
		{
			// Reset flag when touched by a teammate.
			Reset();
			ResetMessage();
			
			IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
			if ( event )
			{
				event->SetInt( "player", pPlayer->entindex() );
				event->SetInt( "eventtype", TF_FLAGEVENT_RETURN );
				event->SetInt( "team", GetTeamNumber() );
				event->SetInt( "priority", 8 );
				gameeventmanager->FireEvent( event );
			}

			CTF_GameStats.Event_PlayerDefendedPoint( pPlayer );

			return;
		}
	}

	// Can that team touch this flag?
	if ( !CanTouchThisFlagType( pOther ) )
		return;

	// Can that team touch this flag?
	if ( !TeamCanTouchThisFlag( pPlayer ) )
	{
		CSingleUserRecipientFilter filter( pPlayer );
		TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_FLAG_CANNOT_CAPTURE, GetTeamNumber() );
		return;
	}

	// Is the touching player about to teleport?
	if ( pPlayer->m_Shared.InCond( TF_COND_SELECTED_TO_TELEPORT ) )
		return;

	// Don't let invulnerable players pick up flags.
	if ( pPlayer->m_Shared.IsInvulnerable() )
		return;

	// Don't let stealthed spies pickup the flag.
 	if ( pPlayer->m_Shared.IsStealthed() || pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) || pPlayer->m_Shared.GetPercentInvisible() > 0.25f )
 		return;

	// Do not allow the player to pick up multiple flags.
	if ( pPlayer->HasTheFlag() )
		return;

	//if ( PointInRespawnRoom( pPlayer, pPlayer->WorldSpaceCenter() ) )
	//	return;

	// Find out whether we're in a respawn room or not
	CBaseEntity *pEntity = NULL;
	while ( ( pEntity = gEntList.FindEntityByClassname( pEntity, "func_respawnroom" ) ) != NULL )
	{
		CFuncRespawnRoom *pRespawnRoom = (CFuncRespawnRoom *)pEntity;

		// Are we within this respawn room?
		if ( pRespawnRoom && pRespawnRoom->GetActive() )
		{
			if ( pRespawnRoom->PointIsWithin( pPlayer->WorldSpaceCenter() ) || pRespawnRoom->IsTouching( pPlayer ) )
			{
				if ( !pRespawnRoom->GetAllowFlag() )
				{
					return;
				}
				break;
			}
		}
	}

	// Pick up the flag.
	PickUp( pPlayer, true );
}


void CCaptureFlag::PickUp( CTFPlayer *pPlayer, bool bInvisible )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( !m_bAllowOwnerPickup && ( m_hPrevOwner.Get() && m_hPrevOwner.Get() == pPlayer ) )
		return;

	// Check whether we have a weapon that's prohibiting us from picking the flag up
	if ( !pPlayer->IsAllowedToPickUpFlag() )
		return;

	// Call into the base class pickup.
	BaseClass::PickUp( pPlayer, false );

	// Store time we left the base
	if ( GetFlagStatus() == TF_FLAGINFO_NONE )
	{
		m_flPickupTime = gpGlobals->curtime;
	}

	pPlayer->TeamFortress_SetSpeed();

	// Update the parent to set the correct place on the model to attach the flag.
	int iAttachment = pPlayer->LookupAttachment( "flag" );
	if ( iAttachment != -1 )
	{
		SetParent( pPlayer, iAttachment );
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
	}

	// Remove the player's disguse if they're a Spy.
	/*if ( pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY )
	{
		if ( pPlayer->m_Shared.IsDisguised() ||
			pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			pPlayer->m_Shared.RemoveDisguise();
		}
	}*/

	// Remove the touch function.
	SetTouch( NULL );

	m_hPrevOwner = pPlayer;
	m_bAllowOwnerPickup = true;

	switch ( m_nGameType )
	{
		case TF_FLAGTYPE_CTF:
		case TF_FLAGTYPE_RETRIEVE:
		{
			// Tell the owning team that their flag has been taken!
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_CTF_ENEMYSTOLEN );

			TFGameRules()->SendHudNotification( filterFriendly, HUD_NOTIFY_YOUR_FLAG_TAKEN, pPlayer->GetTeamNumber() );

			// Tell the thieving team that they've taken someone else's flag!
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_CTF_TEAMSTOLEN );

			// Except the guy who just picked it up.
			filterEnemy.RemoveRecipient( pPlayer );
			if ( GetTeamNumber() <= TEAM_SPECTATOR )
			{
				TFGameRules()->SendHudNotification( filterEnemy, HUD_NOTIFY_NEUTRAL_FLAG_TAKEN, GetTeamNumber() );
			}
			else
			{
				TFGameRules()->SendHudNotification( filterEnemy, HUD_NOTIFY_ENEMY_FLAG_TAKEN, GetTeamNumber() );
			}
			break;
		}
		case TF_FLAGTYPE_ATTACK_DEFEND:
		{
			// Tell the owning team that their flag has been taken!
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_AD_ENEMYSTOLEN );

			// Tell the thieving team that they've taken someone else's flag!
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_AD_TEAMSTOLEN );

			TFGameRules()->SendHudNotification( filterEnemy, "#TF_AD_TakeFlagToPoint", "ico_notify_flag_moving", pPlayer->GetTeamNumber() );
			break;
		}
		case TF_FLAGTYPE_INVADE:
		{
			// Handle messages to the screen.
			CSingleUserRecipientFilter playerFilter( pPlayer );
			TFGameRules()->SendHudNotification( playerFilter, "#TF_Invade_PlayerPickup", "ico_notify_flag_moving", pPlayer->GetTeamNumber() );

			// Tell the owning team that their flag has been taken!
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_INVADE_ENEMYSTOLEN );
			TFGameRules()->SendHudNotification( filterFriendly, "#TF_Invade_OtherTeamPickup", "ico_notify_flag_moving", GetTeamNumber() );

			// Tell the thieving team that they've taken someone else's flag!
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_INVADE_TEAMSTOLEN );

			// Except the guy who just picked it up.
			filterEnemy.RemoveRecipient( pPlayer );
			TFGameRules()->SendHudNotification( filterEnemy, "#TF_Invade_PlayerTeamPickup", "ico_notify_flag_moving", pPlayer->GetTeamNumber() );

			// Set the flag's team to match the player's team.
			ChangeTeam( pPlayer->GetTeamNumber() );
			break;
		}
		case TF_FLAGTYPE_RESOURCE_CONTROL:
		{
			// Tell the owning team that their flag has been taken!
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_SD_ENEMYSTOLEN );

			// Tell the thieving team that they've taken someone else's flag!
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_SD_TEAMSTOLEN );

			// Set the flag's team to match the player's team.
			ChangeTeam( pPlayer->GetTeamNumber() );
			break;
		}
		case TF_FLAGTYPE_VIP:
		{
			// Tell the owning team that their flag has been taken!
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_VIP_ENEMYSTOLEN );

			// Tell the thieving team that they've taken someone else's flag!
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_VIP_TEAMSTOLEN );

			TFGameRules()->SendHudNotification( filterEnemy, "#TF_VIP_TakeDeviceToPoint", "ico_notify_flag_moving", pPlayer->GetTeamNumber() );
			break;
		}
	}

	SetFlagStatus( TF_FLAGINFO_STOLEN );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TF_FLAGEVENT_PICKUP );
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "priority", 8 );
		gameeventmanager->FireEvent( event );
	}

	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGPICKUP );

	// Output.
	m_outputOnPickUp.FireOutput( pPlayer, this );

	switch ( pPlayer->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		m_outputOnPickUpTeam1.FireOutput( pPlayer, this );
		break;

	case TF_TEAM_BLUE:
		m_outputOnPickUpTeam2.FireOutput( pPlayer, this );
		break;

	case TF_TEAM_GREEN:
		m_outputOnPickUpTeam3.FireOutput( pPlayer, this );
		break;

	case TF_TEAM_YELLOW:
		m_outputOnPickUpTeam4.FireOutput( pPlayer, this );
		break;
	}

	UpdateReturnIcon();

	HandleFlagPickedUpInDetectionZone( pPlayer );
}


void CCaptureFlag::Capture( CTFPlayer *pPlayer, int nCapturePoint )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	switch ( m_nGameType )
	{
	case TF_FLAGTYPE_CTF:
	{
		bool bNotify = true;

		// don't play any sounds if this is going to win the round for one of the teams (victory sound will be played instead)
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			int nCaps = TFTeamMgr()->GetFlagCaptures( pPlayer->GetTeamNumber() );

			if ( ( nCaps >= 0 ) && ( tf_flag_caps_per_round.GetInt() - nCaps <= 1 ) )
			{
				// this cap is going to win, so don't play a sound
				bNotify = false;
			}
		}

		if ( bNotify )
		{
			// Tell our owning team that it has been captured...
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_CTF_ENEMYCAPTURED );

			TFGameRules()->SendHudNotification( filterFriendly, HUD_NOTIFY_YOUR_FLAG_CAPTURED, pPlayer->GetTeamNumber() );
			
			// Tell the capturing team that they've got it!
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_CTF_TEAMCAPTURED );

			if ( GetTeamNumber() <= TEAM_SPECTATOR )
			{
				TFGameRules()->SendHudNotification( filterEnemy, HUD_NOTIFY_NEUTRAL_FLAG_CAPTURED, GetTeamNumber() );
			}
			else
			{
				TFGameRules()->SendHudNotification( filterEnemy, HUD_NOTIFY_ENEMY_FLAG_CAPTURED, GetTeamNumber() );
			}
		}

		// Give temp crit boost to capper's team.
		if ( TFGameRules() )
		{
			TFGameRules()->HandleCTFCaptureBonus( pPlayer->GetTeamNumber() );
		}

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );

		// Reward the team
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
		else
		{
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_CTF_CAPTURED_TEAM_FRAGS );
		}

		break;
	}
	case TF_FLAGTYPE_ATTACK_DEFEND:
	{
#if 0
		if ( g_hControlPointMasters.Count() && g_hControlPointMasters[0] )
		{
			CTeamControlPoint *pPoint = g_hControlPointMasters[0]->GetControlPoint( nCapturePoint );

			if ( pPoint )
			{
				variant_t sVariant;
				sVariant.SetInt( pPlayer->GetTeamNumber() );
				pPoint->AcceptInput( "SetOwner", this, pPlayer, sVariant, 0 );
			}
		}
#endif

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );
		break;
	}
	case TF_FLAGTYPE_INVADE:
	{
		bool bNotify = true;

		// don't play any sounds if this is going to win the round for one of the teams (victory sound will be played instead)
		if ( m_nScoringType == TF_FLAGSCORING_CAPS && tf_flag_caps_per_round.GetInt() > 0 )
		{
			int nCaps = TFTeamMgr()->GetFlagCaptures( pPlayer->GetTeamNumber() );

			if ( ( nCaps >= 0 ) && ( tf_flag_caps_per_round.GetInt() - nCaps <= 1 ) )
			{
				// this cap is going to win, so don't play a sound
				bNotify = false;
			}
		}

		if ( bNotify )
		{
			CSingleUserRecipientFilter playerFilter( pPlayer );
			TFGameRules()->SendHudNotification( playerFilter, "#TF_Invade_PlayerCapture", "ico_notify_flag_home", pPlayer->GetTeamNumber() );

			// Tell our owning team that it has been captured...
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_INVADE_ENEMYCAPTURED );
			TFGameRules()->SendHudNotification( filterFriendly, "#TF_Invade_OtherTeamCapture", "ico_notify_flag_home", GetTeamNumber() );

			// Tell the capturing team that they've got it!
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_INVADE_TEAMCAPTURED );

			filterEnemy.RemoveRecipient( pPlayer );
			TFGameRules()->SendHudNotification( filterEnemy, "#TF_Invade_PlayerTeamCapture", "ico_notify_flag_home", pPlayer->GetTeamNumber() );
		}

		// Give temp crit boost to capper's team.
		TFGameRules()->HandleCTFCaptureBonus( pPlayer->GetTeamNumber() );

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );

		// Reward the team
		if ( m_nScoringType == TF_FLAGSCORING_CAPS && tf_flag_caps_per_round.GetInt() > 0 )
		{
			TFTeamMgr()->IncrementFlagCaptures( pPlayer->GetTeamNumber() );
		}
		else
		{
			TFTeamMgr()->AddTeamScore( pPlayer->GetTeamNumber(), TF_CTF_CAPTURED_TEAM_FRAGS );
		}

		break;
	}
	case TF_FLAGTYPE_RESOURCE_CONTROL:
	{
		for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
		{
			if ( iTeam != pPlayer->GetTeamNumber() )
			{
				// The enemy has captured the australium!
				CTeamRecipientFilter filter( iTeam, true );
				g_TFAnnouncer.Speak( filter, TF_ANNOUNCER_SD_ENEMYCAPTURED );
			}
			else
			{
				// We've captured the australium!
				CTeamRecipientFilter filter( iTeam, true );
				g_TFAnnouncer.Speak( filter, TF_ANNOUNCER_SD_TEAMCAPTURED );
			}
		}

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );
		break;
	}
	case TF_FLAGTYPE_RETRIEVE:
	{
		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );
		break;
	}
	case TF_FLAGTYPE_VIP:
	{
		// Tell the owning team that VIP has captured the intelligence.
		CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
		g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_VIP_ENEMYCAPTURED );

		// Tell the VIP's team that they captured the intelligence.
		CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
		g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_VIP_TEAMCAPTURED );

		// Reward the player
		CTF_GameStats.Event_PlayerCapturedPoint( pPlayer );
		break;
	}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "teamplay_flag_event" );
	if ( event )
	{
		event->SetInt( "player", pPlayer->entindex() );
		event->SetInt( "eventtype", TF_FLAGEVENT_CAPTURE );
		event->SetInt( "team", GetTeamNumber() );
		event->SetInt( "priority", 9 );
		gameeventmanager->FireEvent( event );
	}

	SetFlagStatus( TF_FLAGINFO_NONE );
	ResetFlagReturnTime();
	ResetFlagNeutralTime();

	HandleFlagCapturedInDetectionZone( pPlayer );
	HandleFlagDroppedInDetectionZone( pPlayer );

	// Reset the flag.
	BaseClass::Drop( pPlayer, true );

	Reset();

	pPlayer->TeamFortress_SetSpeed();
	pPlayer->SpeakConceptIfAllowed( MP_CONCEPT_FLAGCAPTURED );

	// Output.
	m_outputOnCapture.FireOutput( pPlayer, this );

	switch ( pPlayer->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		m_outputOnCapTeam1.FireOutput( pPlayer, this );
		if ( ( TFGameRules()->IsFourTeamGame() && tf2c_ctf_4team_no_repeats.GetBool() && GetTeamNumber() > TEAM_SPECTATOR )
			|| m_bTeamDisableAfterCapture )
			SetTeamCanPickup( TF_TEAM_RED, false );
		break;

	case TF_TEAM_BLUE:
		m_outputOnCapTeam2.FireOutput( pPlayer, this );
		if ( ( TFGameRules()->IsFourTeamGame() && tf2c_ctf_4team_no_repeats.GetBool() && GetTeamNumber() > TEAM_SPECTATOR )
			|| m_bTeamDisableAfterCapture )
			SetTeamCanPickup( TF_TEAM_BLUE, false );
		break;

	case TF_TEAM_GREEN:
		m_outputOnCapTeam3.FireOutput( pPlayer, this );
		if ( ( TFGameRules()->IsFourTeamGame() && tf2c_ctf_4team_no_repeats.GetBool() && GetTeamNumber() > TEAM_SPECTATOR )
			|| m_bTeamDisableAfterCapture )
			SetTeamCanPickup( TF_TEAM_GREEN, false );
		break;

	case TF_TEAM_YELLOW:
		m_outputOnCapTeam4.FireOutput( pPlayer, this );
		if ( ( TFGameRules()->IsFourTeamGame() && tf2c_ctf_4team_no_repeats.GetBool() && GetTeamNumber() > TEAM_SPECTATOR )
			|| m_bTeamDisableAfterCapture )
			SetTeamCanPickup( TF_TEAM_YELLOW, false );
		break;
	}

	m_bCaptured = true;
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( TFGameRules()->InStalemate() )
	{
		// whoever capped the flag is the winner, give them enough caps to win
		CTFTeam *pTeam = pPlayer->GetTFTeam();
		if ( !pTeam )
			return;

		// if we still need more caps to trigger a win, give them to us
		if ( pTeam->GetFlagCaptures() < tf_flag_caps_per_round.GetInt() )
		{
			pTeam->SetFlagCaptures( tf_flag_caps_per_round.GetInt() );
		}
	}

	ManageSpriteTrail();
}

//-----------------------------------------------------------------------------
// Purpose: A player drops the flag.
//-----------------------------------------------------------------------------
void CCaptureFlag::Drop( CTFPlayer *pPlayer, bool bVisible, bool bThrown /*= false*/, bool bMessage /*= true*/ )
{
	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	// Call into the base class drop.
	BaseClass::Drop( pPlayer, bVisible );

	pPlayer->TeamFortress_SetSpeed();

	if ( bThrown )
	{
		m_bAllowOwnerPickup = false;
		m_flOwnerPickupTime = gpGlobals->curtime + 3.0f;
	}

	Vector vecStart = pPlayer->EyePosition();
	Vector vecEnd = vecStart;
	vecEnd.z -= 8000.0f;
	trace_t trace;
	UTIL_TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trace );
	SetAbsOrigin( trace.endpos );

	// HACK: Parent the flag if it's dropped on a train.
	// Fixes autstralium getting stuck in mid-air if dropped on the elevator in sd_doomsday.
	if ( trace.m_pEnt && trace.m_pEnt->GetMoveType() == MOVETYPE_PUSH )
	{
		SetParent( trace.m_pEnt );
	}

	switch ( m_nGameType )
	{
	case TF_FLAGTYPE_CTF:
	case TF_FLAGTYPE_RETRIEVE:
	{
		if ( bMessage )
		{
			// Tell the owning team that their intelligence has been dropped.
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_CTF_ENEMYDROPPED );

			TFGameRules()->SendHudNotification( filterFriendly, HUD_NOTIFY_YOUR_FLAG_DROPPED, pPlayer->GetTeamNumber() );

			// Tell the enemy team that the intelligence has been dropped.
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_CTF_TEAMDROPPED );

			if ( GetTeamNumber() <= TEAM_SPECTATOR )
			{
				TFGameRules()->SendHudNotification( filterEnemy, HUD_NOTIFY_NEUTRAL_FLAG_DROPPED, GetTeamNumber() );
			}
			else
			{
				TFGameRules()->SendHudNotification( filterEnemy, HUD_NOTIFY_ENEMY_FLAG_DROPPED, GetTeamNumber() );
			}
		}

		SetFlagReturnIn( tf2c_ctf_reset_time_decay.GetBool() ? max( m_flResetDelay - ( gpGlobals->curtime - m_flPickupTime ), TF2C_CTF_MINIMUM_RESET_TIME ) : m_flResetDelay );
		break;
	}
	case TF_FLAGTYPE_ATTACK_DEFEND:
	{
		if ( bMessage )
		{
			// Tell the owning team that their intelligence has been dropped.
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_AD_ENEMYDROPPED );

			// Tell the enemy team that the intelligence has been dropped.
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_AD_TEAMDROPPED );
		}

		SetFlagReturnIn( tf2c_ctf_reset_time_decay.GetBool() ? max( m_flResetDelay - ( gpGlobals->curtime - m_flPickupTime ), TF2C_CTF_MINIMUM_RESET_TIME ) : m_flResetDelay );
		break;
	}
	case TF_FLAGTYPE_INVADE:
	{
		if ( bMessage )
		{
			// Handle messages to the screen.
			CSingleUserRecipientFilter playerFilter( pPlayer );
			TFGameRules()->SendHudNotification( playerFilter, "#TF_Invade_PlayerFlagDrop", "ico_notify_flag_dropped", pPlayer->GetTeamNumber() );

			// Tell the owning team that their intelligence has been dropped.
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_INVADE_ENEMYDROPPED );
			TFGameRules()->SendHudNotification( filterFriendly, "#TF_Invade_FlagDrop", "ico_notify_flag_dropped", GetTeamNumber() );

			// Tell the enemy team that the intelligence has been dropped.
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_INVADE_TEAMDROPPED );

			filterEnemy.RemoveRecipient( pPlayer );
			TFGameRules()->SendHudNotification( filterEnemy, "#TF_Invade_FlagDrop", "ico_notify_flag_dropped", pPlayer->GetTeamNumber() );
		}

		SetFlagReturnIn( tf2c_ctf_reset_time_decay.GetBool() ? max( m_flResetDelay - ( gpGlobals->curtime - m_flPickupTime ), TF2C_CTF_MINIMUM_RESET_TIME ) : m_flResetDelay );

		switch ( m_nNeutralType )
		{
		case TF_FLAGNEUTRAL_DEFAULT:
			SetFlagNeutralIn( 30.0f );
			break;
		case TF_FLAGNEUTRAL_HALFRETURN:
			SetFlagNeutralIn( m_flResetDelay * 0.5f );
			break;
		case TF_FLAGNEUTRAL_NEVER:
		default:
			break;
		}

		break;
	}
	case TF_FLAGTYPE_RESOURCE_CONTROL:
	{
		if ( bMessage )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				// The australium has been dropped!
				if ( iTeam != pPlayer->GetTeamNumber() )
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TF_ANNOUNCER_SD_ENEMYDROPPED );
				}
				else
				{
					CTeamRecipientFilter filter( iTeam, true );
					g_TFAnnouncer.Speak( filter, TF_ANNOUNCER_SD_TEAMDROPPED );
				}
			}
		}

		SetFlagReturnIn( tf2c_ctf_reset_time_decay.GetBool() ? max( m_flResetDelay - ( gpGlobals->curtime - m_flPickupTime ), TF2C_CTF_MINIMUM_RESET_TIME ) : m_flResetDelay );
		break;
	}
	case TF_FLAGTYPE_VIP:
	{
		if ( bMessage )
		{
			// Tell the defending team that the intelligence has been dropped.
			CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_VIP_ENEMYDROPPED );

			// Tell the attacking team that the intelligence has been dropped.
			CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
			g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_VIP_TEAMDROPPED );
		}

		SetFlagReturnIn( tf2c_ctf_reset_time_decay.GetBool() ? max( m_flResetDelay - ( gpGlobals->curtime - m_flPickupTime ), TF2C_CTF_MINIMUM_RESET_TIME ) : m_flResetDelay );
		break;
	}
	}

	// Reset the flag's angles.
	SetAbsAngles( m_vecResetAng );

	// Reset the touch function.
	SetTouch( &CCaptureFlag::FlagTouch );

	SetFlagStatus( TF_FLAGINFO_DROPPED );

	// Output.
	m_outputOnDrop.FireOutput( pPlayer, this );

	UpdateReturnIcon();

	HandleFlagDroppedInDetectionZone( pPlayer );

	if ( PointInRespawnFlagZone( GetAbsOrigin() ) )
	{
		Reset();
		ResetMessage();
	}
}


void CCaptureFlag::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;

	if ( bDisabled )
	{
		SetTouch( NULL );
		SetThink( NULL );
	}
	else
	{
		SetTouch( &CCaptureFlag::FlagTouch );
		SetThink( &CCaptureFlag::Think );
		SetNextThink( gpGlobals->curtime );
	}
}


void CCaptureFlag::Think( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	StudioFrameAdvance();
	DispatchAnimEvents( this );

	// Is the flag enabled?
	if ( IsDisabled() )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( m_bCaptured )
	{
		m_bCaptured = false;
		SetTouch( &CCaptureFlag::FlagTouch );
	}

	ManageSpriteTrail();

	if ( !m_bAllowOwnerPickup )
	{
		if ( m_flOwnerPickupTime && gpGlobals->curtime > m_flOwnerPickupTime )
		{
			m_bAllowOwnerPickup = true;
		}
	}

	if ( IsDropped() )
	{
		ThinkDropped();
	}
	else if ( m_bLocked )
	{
		if ( m_flUnlockTime > 0.0f && gpGlobals->curtime > m_flUnlockTime )
		{
			SetLocked( false );
			UpdateReturnIcon();
		}
	}
	else if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
	{
		ResetFlagReturnTime();
		UpdateReturnIcon();
	}
}

void CCaptureFlag::ThinkDropped( void )
{
	if ( m_nGameType == TF_FLAGTYPE_INVADE )
	{
		if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
		{
			ResetExplode();
			Reset();
			ResetMessage();
		}
		else if ( m_flNeutralTime && gpGlobals->curtime > m_flNeutralTime )
		{
			for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				CTeamRecipientFilter filter( iTeam, true );
				TFGameRules()->SendHudNotification( filter, "#TF_Invade_FlagNeutral", "ico_notify_flag_dropped", iTeam );
			}

			// reset the team to the original team setting (when it spawned)
			ChangeTeam( m_iOriginalTeam );

			ResetFlagNeutralTime();
		}
	}
	else
	{
		if ( m_flResetTime && gpGlobals->curtime > m_flResetTime )
		{
			ResetExplode();
			Reset();
			ResetMessage();
		}
	}
}

void CCaptureFlag::UpdateReturnIcon( void )
{
	if ( m_flResetTime > 0.0f || m_flUnlockTime > 0.0f )
	{
		if ( !m_hReturnIcon.Get() )
		{
			m_hReturnIcon = CBaseEntity::Create( "item_teamflag_return_icon", GetAbsOrigin() + Vector( 0, 0, cl_flag_return_height.GetFloat() ), vec3_angle, this );

			if ( m_hReturnIcon )
			{
				m_hReturnIcon->SetParent( this );
			}
		}
	}
	else
	{
		if ( m_hReturnIcon.Get() )
		{
			UTIL_Remove( m_hReturnIcon );
			m_hReturnIcon = NULL;
		}
	}
}


void CCaptureFlag::GetTrailEffect( int iTeamNum, char *pszBuf, int iBufSize )
{
	V_snprintf( pszBuf, iBufSize, "effects/%s_%s.vmt", STRING( m_szTrailEffect ), g_aTeamNamesShort[iTeamNum] );
}

//-----------------------------------------------------------------------------
// Purpose: Handles the team colored trail sprites
//-----------------------------------------------------------------------------
void CCaptureFlag::ManageSpriteTrail( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPrevOwner() );

	if ( IsStolen() &&
		( m_nUseTrailEffect == TF_FLAGEFFECTS_ALL || m_nUseTrailEffect == TF_FLAGEFFECTS_COLORTRAIL_ONLY ) &&
		pPlayer && pPlayer->GetAbsVelocity().Length() >= pPlayer->MaxSpeed() * 0.5f )
	{
		if ( !m_hGlowTrail )
		{
			char szEffect[128];
			GetTrailEffect( pPlayer->GetTeamNumber(), szEffect, sizeof( szEffect ) );

			m_hGlowTrail = CSpriteTrail::SpriteTrailCreate( szEffect, GetLocalOrigin(), false );
			m_hGlowTrail->FollowEntity( this );
			m_hGlowTrail->SetTransparency( kRenderTransAlpha, -1, -1, -1, 96, 0 );
			m_hGlowTrail->SetBrightness( 96 );
			m_hGlowTrail->SetStartWidth( 32.0f );
			m_hGlowTrail->SetTextureResolution( 0.01f );
			m_hGlowTrail->SetLifeTime( 0.7f );
			m_hGlowTrail->SetTransmit( false );
		}
	}
	else
	{
		if ( m_hGlowTrail )
		{
			m_hGlowTrail->Remove();
			m_hGlowTrail = NULL;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the flag status
//-----------------------------------------------------------------------------
void CCaptureFlag::SetFlagStatus( int iStatus )
{
	MDLCACHE_CRITICAL_SECTION();

	m_nFlagStatus = iStatus;

	switch ( m_nFlagStatus )
	{
	case TF_FLAGINFO_NONE:
	case TF_FLAGINFO_DROPPED:
		ResetSequence( LookupSequence( "spin" ) );	// set spin animation if it's not being held
		break;
	case TF_FLAGINFO_STOLEN:
		ResetSequence( LookupSequence( "idle" ) );	// set idle animation if it is being held
		break;
	default:
		AssertOnce( false );	// invalid stats
		break;
	}
}


void CCaptureFlag::InputEnable( inputdata_t &inputdata )
{
	SetDisabled( false );
}


void CCaptureFlag::InputDisable( inputdata_t &inputdata )
{
	SetDisabled( true );
}


void CCaptureFlag::InputRoundActivate( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->HasItem() && ( pPlayer->GetItem() == this ) )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
}


void CCaptureFlag::InputForceDrop( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->GetTheFlag() == this )
	{
		pPlayer->DropFlag();
	}
}


void CCaptureFlag::InputForceReset( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->GetTheFlag() == this )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
	ResetMessage();
}


void CCaptureFlag::InputForceResetSilent( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->GetTheFlag() == this )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
}


void CCaptureFlag::InputForceResetAndDisableSilent( inputdata_t &inputdata )
{
	CTFPlayer *pPlayer = ToTFPlayer( m_hPrevOwner.Get() );

	// If the player has a capture flag, drop it.
	if ( pPlayer && pPlayer->GetTheFlag() == this )
	{
		Drop( pPlayer, true, false, false );
	}

	Reset();
	SetDisabled( true );
}


void CCaptureFlag::InputSetReturnTime( inputdata_t &inputdata )
{
	float flTime = (float)inputdata.value.Int();
	m_flResetDelay = flTime;
	// Only show and reset the return timer with the updated return time when the flag is currently dropped.
	if ( IsDropped() )
	{
		SetFlagReturnIn( flTime );
		UpdateReturnIcon();
	}
}


void CCaptureFlag::InputShowTimer( inputdata_t &inputdata )
{
	// Show return icon with the specified time.
	float flTime = (float)inputdata.value.Int();
	SetFlagReturnIn( flTime );
	m_flResetDelay = flTime;
	UpdateReturnIcon();
}


void CCaptureFlag::InputForceGlowDisabled( inputdata_t &inputdata )
{
	m_bGlowEnabled = !inputdata.value.Bool();
}


void CCaptureFlag::InputSetLocked( inputdata_t &inputdata )
{
	SetLocked( inputdata.value.Bool() );
}


void CCaptureFlag::InputSetUnlockTime( inputdata_t &inputdata )
{
	int nTime = inputdata.value.Int();

	if ( nTime <= 0 )
	{
		SetLocked( false );
		UpdateReturnIcon();
		return;
	}

	m_flUnlockTime = gpGlobals->curtime + (float)nTime;
	m_flUnlockDelay = (float)nTime;
	UpdateReturnIcon();
}


void CCaptureFlag::SetLocked( bool bLocked )
{
	m_bLocked = bLocked;

	if ( !m_bLocked )
	{
		m_flUnlockTime = -1.0f;
	}
}

void CCaptureFlag::SetTeamCanPickup( int iTeamNum, bool bCanPickup )
{
	switch ( iTeamNum )
	{
	case TF_TEAM_RED:
		m_bTeamRedCanPickup = bCanPickup;
		return;

	case TF_TEAM_BLUE:
		m_bTeamBlueCanPickup = bCanPickup;
		return;

	case TF_TEAM_GREEN:
		m_bTeamGreenCanPickup = bCanPickup;
		return;

	case TF_TEAM_YELLOW:
		m_bTeamYellowCanPickup = bCanPickup;
		return;
	}
}

void CCaptureFlag::InputSetTeamCanPickup( inputdata_t& inputdata )
{
	// Get the interaction name & target
	char parseString[255];
	Q_strncpy( parseString, inputdata.value.String(), sizeof( parseString ) );

	char* pszParam = strtok( parseString, " " );
	if ( pszParam && pszParam[0] )
	{
		int iTeam = atoi( pszParam );
		pszParam = strtok( NULL, " " );

		if ( pszParam && pszParam[0] )
		{
			bool bCanPickup = ( atoi( pszParam ) != 0 );

			SetTeamCanPickup( iTeam, bCanPickup );
			return;
		}
	}

	Warning( "%s(%s) received SetTeamCanPickup input with invalid format. Format should be: <team number> <can cap (0/1)>.\n", GetClassname(), GetDebugName() );
}


void CCaptureFlag::AddFollower( CTFBot *pBot )
{
	if ( !m_BotFollowers.HasElement( pBot ) )
	{
		m_BotFollowers.AddToTail( pBot );

		// TODO: bot tag stuff
	}
}


void CCaptureFlag::RemoveFollower( CTFBot *pBot )
{
	int it = m_BotFollowers.Find( pBot );
	if ( it >= 0 )
	{
		m_BotFollowers.FastRemove( it );

		// TODO: bot tag stuff
	}
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CCaptureFlag::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

#else


void CCaptureFlag::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_nOldFlagStatus = m_nFlagStatus;
	m_iOldTeam = GetTeamNumber();
	m_bWasGlowEnabled = m_bGlowEnabled;
}


void CCaptureFlag::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		UpdateGlowEffect();
	}
	else
	{
		if ( m_nFlagStatus != m_nOldFlagStatus || GetTeamNumber() != m_iOldTeam || m_bGlowEnabled != m_bWasGlowEnabled )
		{
			UpdateGlowEffect();
		}
	}

	UpdateFlagVisibility();
}


int CCaptureFlag::GetSkin( void )
{
	switch ( GetTeamNumber() )
	{
	case TF_TEAM_RED:
		return IsStolen() ? 3 : 0;
		break;
	case TF_TEAM_BLUE:
		return IsStolen() ? 4 : 1;
		break;
	case TF_TEAM_GREEN:
		return IsStolen() ? 8 : 6;
		break;
	case TF_TEAM_YELLOW:
		return IsStolen() ? 9 : 7;
		break;
	default:
		return IsStolen() ? 5 : 2;
		break;
	}
}


void CCaptureFlag::Simulate( void )
{
	BaseClass::Simulate();
	ManageTrailEffects();
}


void CCaptureFlag::ManageTrailEffects( void )
{
	if ( m_nFlagStatus == TF_FLAGINFO_STOLEN &&
		( m_nUseTrailEffect == TF_FLAGEFFECTS_ALL || m_nUseTrailEffect == TF_FLAGEFFECTS_PAPERTRAIL_ONLY ) )
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPrevOwner() );

		if ( pPlayer )
		{
			Vector vecVelocity; pPlayer->EstimateAbsVelocity( vecVelocity );
			if ( vecVelocity.Length() >= pPlayer->MaxSpeed() * 0.5f )
			{
				if ( m_pPaperTrailEffect == NULL )
				{
					m_pPaperTrailEffect = ParticleProp()->Create( m_szPaperEffect, PATTACH_ABSORIGIN_FOLLOW );
				}
			}
			else
			{
				if ( m_pPaperTrailEffect )
				{
					ParticleProp()->StopEmission( m_pPaperTrailEffect );
					m_pPaperTrailEffect = NULL;
				}
			}
		}

	}
	else
	{
		if ( m_pPaperTrailEffect )
		{
			ParticleProp()->StopEmission( m_pPaperTrailEffect );
			m_pPaperTrailEffect = NULL;
		}
	}
}

bool CCaptureFlag::GetMyTeamCanPickup()
{
	CTFPlayer* pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( pPlayer )
	{
		switch ( pPlayer->GetTeamNumber() )
		{
		case TF_TEAM_RED:
			return m_bTeamRedCanPickup.Get();

		case TF_TEAM_BLUE:
			return m_bTeamBlueCanPickup.Get();

		case TF_TEAM_GREEN:
			return m_bTeamGreenCanPickup.Get();

		case TF_TEAM_YELLOW:
			return m_bTeamYellowCanPickup.Get();
		}
	}

	return true;
}

void CCaptureFlag::UpdateGlowEffect( void )
{
	C_TFPlayer *pCarrier = ToTFPlayer( m_hPrevOwner.Get() );

	// If the flag is stolen only show the glow to the teammates of the carrier.
	if ( !m_bGlowEnabled
		|| ( IsStolen() && pCarrier && pCarrier->IsEnemyPlayer() )
		|| !GetMyTeamCanPickup() )
	{
		if ( m_pGlowEffect )
		{
			delete m_pGlowEffect;
			m_pGlowEffect = NULL;
		}
	}
	else
	{
		if ( !m_pGlowEffect )
		{
			m_pGlowEffect = new CGlowObject( this, vec3_origin, 1.0f, true, true );
		}

		Vector vecColor;
		TFGameRules()->GetTeamGlowColor( GetTeamNumber(), vecColor.x, vecColor.y, vecColor.z );
		m_pGlowEffect->SetColor( vecColor );
	}
}


float CCaptureFlag::GetReturnProgress()
{
	if ( m_bLocked && m_flUnlockTime != 0.0f )
	{
		return RemapValClamped(
			m_flUnlockTime - gpGlobals->curtime,
			0.0f, m_flUnlockDelay,
			1.0f, 0.0f );
	}

	// In Invade the flag becomes neutral halfway through reset time.
	if ( m_nGameType == TF_FLAGTYPE_INVADE )
	{
		float flEventTime = ( m_flNeutralTime.Get() != 0.0f ) ? m_flNeutralTime.Get() : m_flResetTime.Get();
		float flTotalTime;

		switch ( m_nNeutralType )
		{
		case TF_FLAGNEUTRAL_DEFAULT:
			flTotalTime = ( m_flNeutralTime.Get() != 0.0f ) ? 30.0f : m_flResetDelay.Get() - 30.0f;
			break;
		case TF_FLAGNEUTRAL_HALFRETURN:
			flTotalTime = m_flResetDelay * 0.5f;
			break;
		case TF_FLAGNEUTRAL_NEVER:
		default:
			flTotalTime = m_flResetDelay;
			break;
		}

		return RemapValClamped(
			flEventTime - gpGlobals->curtime,
			0.0f, flTotalTime,
			1.0f, 0.0f );
	}

	return RemapValClamped(
		m_flResetTime - gpGlobals->curtime,
		0.0f, m_flResetDelay,
		1.0f, 0.0f );
}

void CCaptureFlag::UpdateFlagVisibility( void )
{
	if ( m_bDisabled )
	{
		// Make it either transparent or invisible.
		if ( m_bVisibleWhenDisabled )
		{
			SetRenderColorA( 180 );
		}
		else if ( !IsEffectActive( EF_NODRAW ) )
		{
			AddEffects( EF_NODRAW );
		}
	}
	else
	{
		if ( IsEffectActive( EF_NODRAW ) )
		{
			RemoveEffects( EF_NODRAW );
		}

		// Show it as transparent when locked.
		if ( m_bLocked )
		{
			SetRenderColorA( 180 );
		}
		else
		{
			// Also show as transparent when team is not allowed to pick up (again).
			if ( !GetMyTeamCanPickup() )
			{
				SetRenderColorA( 180 );
			}
			else
			{
				SetRenderColorA( 255 );
			}
		}
	}
}


void CCaptureFlag::GetHudIcon( int iTeamNum, char *pszBuf, int iBufSize )
{
	V_snprintf( pszBuf, iBufSize, "%s_%s", m_szHudIcon.Get(), GetTeamSuffix( iTeamNum, g_aTeamLowerNames, "neutral" ) );
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( CaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )
BEGIN_NETWORK_TABLE( CCaptureFlagReturnIcon, DT_CaptureFlagReturnIcon )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( item_teamflag_return_icon, CCaptureFlagReturnIcon );

CCaptureFlagReturnIcon::CCaptureFlagReturnIcon()
{
#ifdef CLIENT_DLL
	m_pReturnProgressMaterial_Empty = NULL;
	m_pReturnProgressMaterial_Full = NULL;
#endif
}

#ifdef GAME_DLL

void CCaptureFlagReturnIcon::Spawn( void )
{
	BaseClass::Spawn();

	UTIL_SetSize( this, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );

	CollisionProp()->SetCollisionBounds( Vector( -50, -50, -50 ), Vector( 50, 50, 50 ) );
}

int CCaptureFlagReturnIcon::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_PVSCHECK );
}
#endif

#ifdef CLIENT_DLL

// This defines the properties of the 8 circle segments
// in the circular progress bar.
progress_segment_t Segments[8] =
{
	{ 0.125, 0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ 0.25, 1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ 0.375, 1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ 0.50, 1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ 0.625, 0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ 0.75, 0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ 0.875, 0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ 1.0, 0.0, 0.0, 0.5, 0.0, 1, 0 },
};


RenderGroup_t CCaptureFlagReturnIcon::GetRenderGroup( void )
{
	return RENDER_GROUP_TRANSLUCENT_ENTITY;
}

void CCaptureFlagReturnIcon::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	theMins.Init( -20, -20, -20 );
	theMaxs.Init( 20, 20, 20 );
}


int CCaptureFlagReturnIcon::DrawModel( int flags )
{
	int nRetVal = BaseClass::DrawModel( flags );

	DrawReturnProgressBar();

	return nRetVal;
}

//-----------------------------------------------------------------------------
// Purpose: Draw progress bar above the flag indicating when it will return
//-----------------------------------------------------------------------------
void CCaptureFlagReturnIcon::DrawReturnProgressBar( void )
{
	CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *> ( GetOwnerEntity() );

	if ( !pFlag )
		return;

	// Don't draw if this flag is not going to reset
	if ( pFlag->GetResetDelay() <= 0 )
		return;

	if ( !TFGameRules()->FlagsMayBeCapped() )
		return;

	if ( !m_pReturnProgressMaterial_Full )
	{
		m_pReturnProgressMaterial_Full = materials->FindMaterial( "vgui/flagtime_full", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Empty )
	{
		m_pReturnProgressMaterial_Empty = materials->FindMaterial( "vgui/flagtime_empty", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Full || !m_pReturnProgressMaterial_Empty )
	{
		return;
	}

	CMatRenderContextPtr pRenderContext( materials );

	Vector vOrigin = GetAbsOrigin();
	QAngle vAngle = vec3_angle;

	// Align it towards the viewer
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();
	if ( fabs( vRight.z ) > 0.95 )	// don't draw it edge-on
		return;

	vRight.z = 0;
	VectorNormalize( vRight );

	float flSize = cl_flag_return_size.GetFloat();

	unsigned char ubColor[4];
	ubColor[3] = 255;

	switch ( pFlag->GetTeamNumber() )
	{
	case TF_TEAM_RED:
		ubColor[0] = 232;
		ubColor[1] = 28;
		ubColor[2] = 28;
		break;
	case TF_TEAM_BLUE:
		ubColor[0] = 28;
		ubColor[1] = 28;
		ubColor[2] = 232;
		break;
	case TF_TEAM_GREEN:
		ubColor[0] = 20;
		ubColor[1] = 200;
		ubColor[2] = 20;
		break;
	case TF_TEAM_YELLOW:
		ubColor[0] = 200;
		ubColor[1] = 200;
		ubColor[2] = 20;
		break;
	default:
		ubColor[0] = 200;
		ubColor[1] = 200;
		ubColor[2] = 200;
		break;
	}

	// First we draw a quad of a complete icon, background
	CMeshBuilder meshBuilder;

	pRenderContext->Bind( m_pReturnProgressMaterial_Empty );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0, 0, 0 );
	meshBuilder.Position3fv( ( vOrigin + ( vRight * -flSize ) + ( vUp * flSize ) ).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0, 1, 0 );
	meshBuilder.Position3fv( ( vOrigin + ( vRight * flSize ) + ( vUp * flSize ) ).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0, 1, 1 );
	meshBuilder.Position3fv( ( vOrigin + ( vRight * flSize ) + ( vUp * -flSize ) ).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0, 0, 1 );
	meshBuilder.Position3fv( ( vOrigin + ( vRight * -flSize ) + ( vUp * -flSize ) ).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();

	float flProgress = pFlag->GetReturnProgress();

	pRenderContext->Bind( m_pReturnProgressMaterial_Full );
	pMesh = pRenderContext->GetDynamicMesh();

	vRight *= flSize * 2;
	vUp *= flSize * -2;

	// Next we're drawing the circular progress bar, in 8 segments
	// For each segment, we calculate the vertex position that will draw
	// the slice.
	int i;
	for ( i = 0; i < 8; i++ )
	{
		if ( flProgress < Segments[i].maxProgress )
		{
			CMeshBuilder meshBuilder_Full;

			meshBuilder_Full.Begin( pMesh, MATERIAL_TRIANGLES, 3 );

			// vert 0 is ( 0.5, 0.5 )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, 0.5, 0.5 );
			meshBuilder_Full.Position3fv( vOrigin.Base() );
			meshBuilder_Full.AdvanceVertex();

			// Internal progress is the progress through this particular slice
			float internalProgress = RemapVal( flProgress, Segments[i].maxProgress - 0.125, Segments[i].maxProgress, 0.0, 1.0 );
			internalProgress = clamp( internalProgress, 0.0, 1.0 );

			// Calculate the x,y of the moving vertex based on internal progress
			float swipe_x = Segments[i].vert2x - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_x;
			float swipe_y = Segments[i].vert2y - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_y;

			// vert 1 is calculated from progress
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, swipe_x, swipe_y );
			meshBuilder_Full.Position3fv( ( vOrigin + ( vRight * ( swipe_x - 0.5 ) ) + ( vUp *( swipe_y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, Segments[i].vert2x, Segments[i].vert2y );
			meshBuilder_Full.Position3fv( ( vOrigin + ( vRight * ( Segments[i].vert2x - 0.5 ) ) + ( vUp *( Segments[i].vert2y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			meshBuilder_Full.End();

			pMesh->Draw();
		}
	}
}

#endif
