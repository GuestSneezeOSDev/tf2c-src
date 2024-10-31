//====== Copyright  1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "c_tf_player.h"
#include "c_user_message_register.h"
#include "view.h"
#include "iclientvehicle.h"
#include "ivieweffects.h"
#include "input.h"
#include "IEffects.h"
#include "fx.h"
#include "c_basetempentity.h"
#include "hud_macros.h"
#include "engine/ivdebugoverlay.h"
#include "smoke_fog_overlay.h"
#include "playerandobjectenumerator.h"
#include "bone_setup.h"
#include "in_buttons.h"
#include "r_efx.h"
#include "dlight.h"
#include "shake.h"
#include "cl_animevent.h"
#include "tf_weaponbase.h"
#include "c_tf_playerresource.h"
#include "toolframework/itoolframework.h"
#include "tier1/KeyValues.h"
#include "tier0/vprof.h"
#include "prediction.h"
#include "effect_dispatch_data.h"
#include "c_te_effect_dispatch.h"
#include "tf_fx_muzzleflash.h"
#include "tf_gamerules.h"
#include "view_scene.h"
#include "c_baseobject.h"
#include "toolframework_client.h"
#include "soundenvelope.h"
#include "voice_status.h"
#include "clienteffectprecachesystem.h"
#include "functionproxy.h"
#include "toolframework_client.h"
#include "choreoevent.h"
#include "vguicenterprint.h"
#include "eventlist.h"
#include "tf_hud_statpanel.h"
#include "input.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_weapon_umbrella.h"
#include "in_main.h"
#include "c_team.h"
#include "collisionutils.h"
// for spy material proxy
#include "proxyentity.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "baseanimatedtextureproxy.h"
#include "animation.h"
#include "choreoscene.h"
#include "c_tf_team.h"
#include "tf_viewmodel.h"
#include "c_tf_objective_resource.h"
#include "tf_inventory.h"
#include "tf_wearable.h"
#include "tf_playermodelpanel.h"
#include "cam_thirdperson.h"
#include "tf_hud_chat.h"
#include "iclientmode.h"
#include "steam/steam_api.h"
#include "achievementmgr.h"
#include "tf_hud_itemeffectmeter.h"
#include "tf_playermodelpanel.h"

#include "tf_weapon_beacon.h"	// !!! foxysen beacon
#include "tf_weapon_riot.h"

#ifdef TF2C_BETA
#include "tf_hud_patienthealthbar.h"
#endif

#include "c_tf_projectile_arrow.h"
#include  "cdll_client_int.h"

#include "fmtstr.h"
#include "debugoverlay_shared.h"

#if defined( CTFPlayer )
#undef CTFPlayer
#endif

#include "materialsystem/imesh.h"		//for materials->FindMaterial
#include "iviewrender.h"				//for view->

#include "tf_randomizer_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar tf_playergib_forceup( "tf_playersgib_forceup", "1.0", FCVAR_CHEAT, "Upward added velocity for gibs." );
ConVar tf_playergib_force( "tf_playersgib_force", "500.0", FCVAR_CHEAT, "Gibs force." );
ConVar tf_playergib_maxspeed( "tf_playergib_maxspeed", "400", FCVAR_CHEAT, "Max gib speed." );

ConVar cl_autorezoom( "cl_autorezoom", "1", FCVAR_USERINFO | FCVAR_ARCHIVE, "When set to 1, sniper rifle will re-zoom after firing a zoomed shot." );
ConVar cl_autoreload( "cl_autoreload", "1",  FCVAR_USERINFO | FCVAR_ARCHIVE, "When set to 1, clip-using weapons will automatically be reloaded whenever they're not being fired." );
ConVar tf_remember_activeweapon( "tf_remember_activeweapon", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Setting this to 1 will make the active weapon persist between lives." );
ConVar tf_remember_lastswitched( "tf_remember_lastswitched", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Setting this to 1 will make the 'last weapon' persist between lives." );

ConVar tf2c_muzzleflash( "tf2c_muzzleflash", "1", FCVAR_ARCHIVE, "Enable muzzleflash" );
ConVar tf2c_model_muzzleflash( "tf2c_model_muzzleflash", "0", FCVAR_ARCHIVE, "Use the tf2 beta model based muzzleflash" );
ConVar tf2c_cig_light( "tf2c_cig_light", "1", FCVAR_ARCHIVE, "Enable dynamic light for Spy's cigarette." );

ConVar tf2c_dev_mark( "tf2c_dev_mark", "1", FCVAR_ARCHIVE | FCVAR_USERINFO );
ConVar tf2c_proximity_voice( "tf2c_proximity_voice", "0", FCVAR_ARCHIVE | FCVAR_USERINFO );
ConVar tf2c_zoom_hold_sniper( "tf2c_zoom_hold_sniper", "0", FCVAR_ARCHIVE | FCVAR_USERINFO );

ConVar tf2c_debug_player_brightness("tf2c_debug_player_brightness", "0", FCVAR_CHEAT, "Visualize players' light level.");

// Disabled (Enabled) for now.
ConVar tf2c_legacy_invulnerability_material( "tf2c_legacy_invulnerability_material", "1", FCVAR_HIDDEN | FCVAR_ARCHIVE );

ConVar cl_fp_ragdoll( "cl_fp_ragdoll", "0", FCVAR_ARCHIVE, "See ragdoll deaths in first person" );

ConVar tf2c_fixedspread_preference("tf2c_fixedspread_preference", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_NOT_CONNECTED, "Sets non-random spread distrubtion on weapons that fire multiple pellets per shot for player when server's tf_use_fixed_weaponspreads is set to 2.");

ConVar tf2c_centerfire_preference("tf2c_centerfire_preference", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_NOT_CONNECTED, "Sets projectiles' spawn position Y coordinate to 0 for player");

ConVar tf2c_avoid_becoming_vip("tf2c_avoid_becoming_vip", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "If set, player won't be picked at random as a VIP unless no other candidate is left. Can still be picked by a vote.");

ConVar tf2c_pincushion_limit( "tf2c_pincushion_limit", "10", FCVAR_ARCHIVE, "Max amount of arrows that are allowed to stick in one player at a time." );

ConVar tf2c_spywalk_inverted( "tf2c_spywalk_inverted", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Sets Spywalk as default behaviour. Hold +SPEED for normal movement" );

ConVar tf2c_show_nemesis_relationships( "tf2c_show_nemesis_relationships", "1", FCVAR_ARCHIVE, "Show Nemesis Relationships client-side." );

ConVar tf2c_boioing("tf2c_boioing", "1", FCVAR_ARCHIVE, "Enable boioing and stomp client-side commands for ragdolls");

extern ConVar tf2c_spywalk;
extern ConVar tf2c_taunting_detonate_stickies;

//int C_TFPlayer::ms_nPlayerPatternCounter = CTeamPatternObject::CB_TEAM_NONE;

//-----------------------------------------------------------------------------
// Purpose: Refreshes the colourblindness team pattern effects
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateTeamPatternEffect(void)
{
	DestroyTeamPatternEffect();

	// Create
	int n_playerTeam;
	int n_teamColour;

	// Enemy spies should be drawn as their disguise team.
	if ((m_bDisguised || m_Shared.IsDisguised()) && GetLocalPlayerTeam() != GetTeamNumber())
	{
		n_playerTeam = m_Shared.GetDisguiseTeam();
#ifdef DEBUG_COLOURBLIND
		DevMsg("Enemy spy %s is on team %s but is disguised as team %s. Desired disguise team is %s.\n",
			GetPlayerName(),
			TeamIndexToString(GetTeamNumber()),
			TeamIndexToString(m_Shared.GetDisguiseTeam()),
			TeamIndexToString(m_Shared.GetDesiredDisguiseTeam()));
#endif
	}
	// Other classes and allied spies should be drawn as their real team.
	else
	{
		n_playerTeam = GetTeamNumber();
#ifdef DEBUG_COLOURBLIND
		DevMsg("Player %s is not a spy or is a spy on the local player's team.\n", GetPlayerName());
#endif
	}
	
	switch (n_playerTeam)
	{
	case TF_TEAM_RED:
		n_teamColour = CTeamPatternObject::CB_TEAM_RED;
		break;
	case TF_TEAM_BLUE:
		n_teamColour = CTeamPatternObject::CB_TEAM_BLU;
		break;
	case TF_TEAM_YELLOW:
		n_teamColour = CTeamPatternObject::CB_TEAM_YLW;
		break;
	case TF_TEAM_GREEN:
		n_teamColour = CTeamPatternObject::CB_TEAM_GRN;
		break;
	case TF_TEAM_GLOBAL:
		n_teamColour = CTeamPatternObject::CB_TEAM_GLB;
	default:
		n_teamColour = CTeamPatternObject::CB_TEAM_NONE;
		break;
	}
	
	m_pTeamPatternEffect = new CTeamPatternObject(this, n_teamColour);

	if (n_teamColour != CTeamPatternObject::CB_TEAM_NONE)
	{
		m_bColorBlindInitialised = true;
	}
#ifdef DEBUG_COLOURBLIND
	else
	{
		DevWarning("Team for player %s was default! Refreshing next update...\n");
	}
#endif

#ifdef DEBUG_COLOURBLIND
	DevMsg("Updated team pattern for %s. \n", GetPlayerName());
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Safely destroys the player's colourblind pattern effect object
//-----------------------------------------------------------------------------
void C_TFPlayer::DestroyTeamPatternEffect(void)
{
	// Destroy
	if (m_pTeamPatternEffect)
	{
		delete m_pTeamPatternEffect;
		m_pTeamPatternEffect = NULL;
	}
}

#define BDAY_HAT_MODEL		"models/effects/bday_hat.mdl"
#define SANTA_HAT_MODEL		"models/effects/santa_hat.mdl"
#define ENGINEER_HELM_MODEL "models/player/gibs/engineergib007.mdl"
#define SOLDIER_HELM_MODEL  "models/player/gibs/soldiergib008.mdl"

IMaterial	*g_pHeadLabelMaterial[4] = { NULL, NULL, NULL, NULL }; 
void	SetupHeadLabelMaterials( void );

const char *pszHeadLabelNames[] =
{
	"effects/speech_voice_red",
	"effects/speech_voice_blue",
	"effects/speech_voice_green",
	"effects/speech_voice_yellow"
};

#define TF_PLAYER_HEAD_LABEL_RED 0
#define TF_PLAYER_HEAD_LABEL_BLUE 1
#define TF_PLAYER_HEAD_LABEL_GREEN 2
#define TF_PLAYER_HEAD_LABEL_YELLOW 3

const char *pszInvulnerableMaterialNames[] =
{
	"models/effects/invulnfx_red",
	"models/effects/invulnfx_blue",
	"models/effects/invulnfx_green",
#ifdef TF2C_BETA
	"models/effects/invulnfx_yellow",
	"models/effects/invulnfx_global"
#else
	"models/effects/invulnfx_yellow"
#endif
};

CLIENTEFFECT_REGISTER_BEGIN( PrecacheInvuln )
	CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_red.vmt" )
	CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_blue.vmt" )
	CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_green.vmt" )
	CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_yellow.vmt" )
#ifdef TF2C_BETA
	// used on tf_playermodelpanel only for now
	CLIENTEFFECT_MATERIAL( "models/effects/invulnfx_global.vmt" )
#endif
CLIENTEFFECT_REGISTER_END()

CLIENTEFFECT_REGISTER_BEGIN( PrecacheGlobalDisguise )
	CLIENTEFFECT_MATERIAL( "dev/global_disguise_gray.vmt" )
CLIENTEFFECT_REGISTER_END()

extern CBaseEntity *BreakModelCreateSingle( CBaseEntity *pOwner, breakmodel_t *pModel, const Vector &position, 
										   const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, int nSkin, const breakablepropparams_t &params );

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //
class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		VPROF( "C_TEPlayerAnimEvent::PostDataUpdate" );

		// Create the effect.
		if ( m_iPlayerIndex == 0 )
			return;

		EHANDLE hPlayer = cl_entitylist->GetNetworkableHandle( m_iPlayerIndex );
		if ( !hPlayer )
			return;

		C_TFPlayer *pPlayer = dynamic_cast< C_TFPlayer* >( hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
		}	
	}

public:
	CNetworkVar( int, m_iPlayerIndex );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

//-----------------------------------------------------------------------------
// Data tables and prediction tables.
//-----------------------------------------------------------------------------
BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropInt( RECVINFO( m_iPlayerIndex ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()


//=============================================================================
//
// Ragdoll
//
// ----------------------------------------------------------------------------- //
// Client ragdoll entity.
// ----------------------------------------------------------------------------- //
ConVar cl_ragdoll_physics_enable( "cl_ragdoll_physics_enable", "1", 0, "Enable/disable ragdoll physics." );
ConVar cl_ragdoll_fade_time( "cl_ragdoll_fade_time", "15", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_forcefade( "cl_ragdoll_forcefade", "0", FCVAR_CLIENTDLL );
ConVar cl_ragdoll_pronecheck_distance( "cl_ragdoll_pronecheck_distance", "64", FCVAR_GAMEDLL );

ConVar tf_always_deathanim( "tf_always_deathanim", "0", FCVAR_CLIENTDLL, "Force death anims to always play." );

ConVar tf2c_burning_deathanim("tf2c_burning_deathanim", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Enable burning death animations");

class C_TFRagdoll : public C_BaseFlex
{
	DECLARE_CLASS( C_TFRagdoll, C_BaseFlex );
	DECLARE_CLIENTCLASS();
	
public:
	C_TFRagdoll();
	~C_TFRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	IRagdoll *GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );

	void ClientThink( void );
	void StartFadeOut( float fDelay );
	void EndFadeOut();

	C_TFPlayer *GetOwningPlayer( void ) 	
	{
		return ToTFPlayer( GetOwnerEntity() );
	}

	bool IsRagdollVisible();
	float GetBurnStartTime() { return m_flBurnEffectStartTime; }

	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );
	virtual float FrameAdvance( float flInterval = 0.0f );

	virtual	void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed );

	void DismemberHead( void );

	float GetInvisibilityLevel( void )
	{
		if ( m_flUncloakCompleteTime == 0.0f )
			return 0.0f;

		return RemapValClamped( m_flUncloakCompleteTime - gpGlobals->curtime, 0.0f, 2.0f, 0.0f, 1.0f );
	}

	bool IsPlayingDeathAnim( void ) { return m_bPlayingDeathAnim; }

private:
	C_TFRagdoll( const C_TFRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity );

	void CreateTFRagdoll( void );
	void CreateTFGibs( void );

	virtual void		UpdateTeamPatternEffect(void);
	virtual void		DestroyTeamPatternEffect(void);
	bool				m_bColorBlindInitialised;
	CTeamPatternObject	*m_pTeamPatternEffect;
	CTeamPatternObject	*GetTeamPatternObject(void){ return m_pTeamPatternEffect; }

private:
	Vector  m_vecRagdollVelocity;
	Vector  m_vecRagdollOrigin;
	float	m_fDeathTime;
	bool	m_bFadingOut;
	int		m_iRagdollFlags;

	float	m_flInvisibilityLevel;
	float	m_flUncloakCompleteTime;
	float	m_flTimeToDissolve;
	ETFDmgCustom m_iDamageCustom;
	int m_bitsDamageType;
	int		m_iTeam;
	int		m_iClass;
	float	m_flBurnEffectStartTime;	// Start time of burning, or 0 if not burning.

	bool	m_bPlayingDeathAnim;
	bool	m_bFinishedDeathAnim;

	bool	m_bDismemberHead;

	bool	m_bBoioingEnabled;
	int		m_bBoioingCount;
	Vector	m_vecBoioingVelocity;
	float	m_flBoioingIntervalTime;
};

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_TFRagdoll, DT_TFRagdoll, CTFRagdoll )
	RecvPropVector( RECVINFO( m_vecRagdollOrigin ) ),
	RecvPropEHandle( RECVINFO( m_hOwnerEntity ) ),
	RecvPropVector( RECVINFO( m_vecForce ) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) ),
	RecvPropInt( RECVINFO( m_nForceBone ) ),
	RecvPropInt( RECVINFO( m_iRagdollFlags ) ),
	RecvPropFloat( RECVINFO( m_flInvisibilityLevel ) ),
	RecvPropInt( RECVINFO( m_iDamageCustom ) ),
	RecvPropInt( RECVINFO(m_bitsDamageType) ),
	RecvPropInt( RECVINFO( m_iTeam ) ),
	RecvPropInt( RECVINFO( m_iClass ) )
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TFRagdoll::C_TFRagdoll()
{
	m_fDeathTime = -1;
	m_bFadingOut = false;
	m_iRagdollFlags = 0;

	m_flInvisibilityLevel = 0.0f;
	m_flUncloakCompleteTime = 0.0f;
	m_flTimeToDissolve = 0.3f;
	m_iDamageCustom = TF_DMG_CUSTOM_NONE;
	m_bitsDamageType = DMG_GENERIC;
	m_flBurnEffectStartTime = 0.0f;

	m_iTeam = -1;
	m_iClass = -1;
	m_nForceBone = -1;

	m_bPlayingDeathAnim = false;
	m_bFinishedDeathAnim = false;
	m_bColorBlindInitialised = false;

	m_bDismemberHead = false;

	m_bBoioingEnabled = tf2c_boioing.GetBool();
	m_bBoioingCount = 0;
	m_vecBoioingVelocity = vec3_origin;
	m_flBoioingIntervalTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
C_TFRagdoll::~C_TFRagdoll()
{
	PhysCleanupFrictionSounds( this );

	DestroyTeamPatternEffect();
}

//-----------------------------------------------------------------------------
// Purpose: Refreshes the corpse's colourblindness team pattern effects
//-----------------------------------------------------------------------------
void C_TFRagdoll::UpdateTeamPatternEffect(void)
{
	DestroyTeamPatternEffect();

	if (!ShouldDraw())
		return;

	// Create
	int n_playerTeam = 0;
	if (GetOwningPlayer())
		n_playerTeam = GetOwningPlayer()->GetTeamNumber();
	int n_teamColour;
	switch (n_playerTeam)
	{
	case TF_TEAM_RED:
		n_teamColour = CTeamPatternObject::CB_TEAM_RED;
		break;
	case TF_TEAM_BLUE:
		n_teamColour = CTeamPatternObject::CB_TEAM_BLU;
		break;
	case TF_TEAM_YELLOW:
		n_teamColour = CTeamPatternObject::CB_TEAM_YLW;
		break;
	case TF_TEAM_GREEN:
		n_teamColour = CTeamPatternObject::CB_TEAM_GRN;
		break;
	default:
		n_teamColour = CTeamPatternObject::CB_TEAM_NONE;
		break;
	}

	m_pTeamPatternEffect = new CTeamPatternObject(this, n_teamColour);

	m_bColorBlindInitialised = true;
}

//-----------------------------------------------------------------------------
// Purpose: Safely destroys the corpse's colourblind pattern effect object
//-----------------------------------------------------------------------------
void C_TFRagdoll::DestroyTeamPatternEffect(void)
{
	// Destroy
	if (m_pTeamPatternEffect)
	{
		delete m_pTeamPatternEffect;
		m_pTeamPatternEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSourceEntity - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;
	
	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();
    	
	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		const char *pszName = pDestEntry->watcher->GetDebugName();
		for (int j = 0; j < pSrc->m_Entries.Count(); j++)
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(), pszName ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup vertex weights for drawing
//-----------------------------------------------------------------------------
void C_TFRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	// While we're dying, we want to mimic the facial animation of the player. Once they respawn, we just stay as we are.
	bool bAlive;
	C_TFPlayer *pPlayer = GetOwningPlayer();
	if ( pPlayer )
	{
		if ( !pPlayer->IsDormant() )
		{
			bAlive = pPlayer->IsAlive();
		}
		else
		{
			bAlive = g_TF_PR && g_TF_PR->IsAlive( pPlayer->entindex() );
		}
	}
	else
	{
		bAlive = true;
	}

	if ( !bAlive )
	{
		pPlayer->SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
	else
	{
		BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTrace - 
//			iDamageType - 
//			*pCustomImpactName - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	VPROF( "C_TFRagdoll::ImpactTrace" );
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( !pPhysicsObject )
		return;

	Vector vecDir;
	VectorSubtract( pTrace->endpos, pTrace->startpos, vecDir );

	if ( iDamageType == DMG_BLAST )
	{
		// Adjust the impact strength and apply the force at the center of mass.
		vecDir *= 4000;
		pPhysicsObject->ApplyForceCenter( vecDir );
	}
	else
	{
		// Find the apporx. impact point.
		Vector vecHitPos;
		VectorMA( pTrace->startpos, pTrace->fraction, vecDir, vecHitPos );
		VectorNormalize( vecDir );

		// Adjust the impact strength and apply the force at the impact point.
		vecDir *= 4000;
		pPhysicsObject->ApplyForceOffset( vecDir, vecHitPos );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}

// ---------------------------------------------------------------------------- -
// Purpose: 
// Input  : flInterval - 
// Output : float
//-----------------------------------------------------------------------------
float C_TFRagdoll::FrameAdvance( float flInterval )
{
	float flRet = BaseClass::FrameAdvance( flInterval );

	// Turn into a ragdoll once animation is over.
	if ( !m_bFinishedDeathAnim && m_bPlayingDeathAnim && IsSequenceFinished() )
	{
		m_bFinishedDeathAnim = true;

		if ( cl_ragdoll_physics_enable.GetBool() )
		{
			// Make us a ragdoll.
			m_nRenderFX = kRenderFxRagdoll;

			matrix3x4_t boneDelta0[MAXSTUDIOBONES];
			matrix3x4_t boneDelta1[MAXSTUDIOBONES];
			matrix3x4_t currentBones[MAXSTUDIOBONES];
			const float boneDt = 0.1f;

			GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt, false );
			SetAbsVelocity( vec3_origin );
		}
		else
		{
			EndFadeOut();
		}
	}

	return flRet;
}

//-----------------------------------------------------------------------------
// Purpose: Scale the bones that need to be scaled for gore
//-----------------------------------------------------------------------------
void C_TFRagdoll::BuildTransformations( CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( pStudioHdr, pos, q, cameraTransform, boneMask, boneComputed );

	if( m_bDismemberHead )
	{
		int iBone = -1;

		const char* boneNames[] = { "bip_head", "prp_helmet", "prp_hat", "prp_cig" };

		for ( int i = 0; i < 4; i++ )
		{
			iBone = LookupBone( boneNames[i] );
			if ( iBone != -1 )
				MatrixScaleBy( 0.001f, GetBoneForWrite( iBone ) );
		}
	}
}

void C_TFRagdoll::DismemberHead()
{
	m_bDismemberHead = true;

	int m_HeadBodygroup = FindBodygroupByName("head");

	if (m_HeadBodygroup >= 0)
		SetBodygroup(m_HeadBodygroup, 2);


	//this causes the particle to rotate depending on the class. This is fine for the particle we are using but is good to keep in mind
	int iAttach = LookupBone("bip_neck");

	if (iAttach != -1)
	{
		ParticleProp()->Create("blood_decap", PATTACH_BONE_FOLLOW, "bip_neck");
		//ParticleProp()->Create("env_sawblood_mist", PATTACH_BONE_FOLLOW, "bip_neck");

		EmitSound("TFPlayer.Decapitated");
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::CreateTFRagdoll( void )
{
	// Get the player.
	C_TFPlayer *pPlayer = GetOwningPlayer();

	TFPlayerClassData_t *pData = GetPlayerClassData( m_iClass );
	if ( pData )
	{
		int nModelIndex = modelinfo->GetModelIndex( pData->GetModelName() );
		if ( pPlayer && pPlayer->GetPlayerClass()->HasCustomModel() )
		{
			nModelIndex = modelinfo->GetModelIndex( pPlayer->GetPlayerClass()->GetModelName() );
		}
		
		SetModelIndex( nModelIndex );
		SetModelScale( pPlayer ? pPlayer->GetModelScale() : 1.0f );
		m_nSkin = m_iTeam < TF_TEAM_GREEN ? m_iTeam - 2 : m_iTeam;
	}

#ifdef _DEBUG
	DevMsg( 2, "CreateTFRagdoll %d %d\n", gpGlobals->framecount, pPlayer ? pPlayer->entindex() : 0 );
#endif
	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// Move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.		
		if ( !pPlayer->IsLocalPlayer() && pPlayer->IsInterpolationEnabled() )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin.
			SetAbsOrigin( pPlayer->GetRenderOrigin() );
			SetAbsAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( m_vecRagdollVelocity );

			// Hack! Find a neutral standing pose or use the idle.
			int iSeq = LookupSequence( "RagdollSpawn" );
			if ( iSeq == -1 )
			{
				Assert( false );
				iSeq = 0;
			}
			SetSequence( iSeq );
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}

		pPlayer->CreateBoneAttachmentsFromWearables( this );
		pPlayer->MoveBoneAttachments( this );
	}
	else
	{
		// Overwrite network origin so later interpolation will use this position.
		SetNetworkOrigin( m_vecRagdollOrigin );
		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );
	}

	// See if we should play a custom death animation.
	if ( pPlayer && !pPlayer->IsDormant() &&
		( m_iRagdollFlags & TF_RAGDOLL_ONGROUND ) &&
		( tf_always_deathanim.GetBool() || !RandomInt( 0, 3 ) ) )
	{
		int iSeq = pPlayer->m_Shared.GetSequenceForDeath( this, m_iDamageCustom );

		// disable burning death animation cvar
		if( !tf2c_burning_deathanim.GetBool() && m_iDamageCustom == TF_DMG_CUSTOM_BURNING )
			iSeq = -1;

		if ( iSeq != -1 )
		{
			// Doing this here since the server doesn't send the value over.
			ForceClientSideAnimationOn();

			// Slam velocity when doing death animation.
			pPlayer->MoveToLastReceivedPosition( true );
			SetNetworkOrigin( pPlayer->GetRenderOrigin() );
			SetNetworkAngles( pPlayer->GetRenderAngles() );
			SetAbsVelocity( vec3_origin );
			m_vecForce = vec3_origin;

			SetSequence( iSeq );
			ResetSequenceInfo();
			m_bPlayingDeathAnim = true;
			
			CreateShadow();

			if (!(m_bitsDamageType & DMG_CRITICAL) && m_iDamageCustom == TF_DMG_CUSTOM_BURNING)
			{
				EmitSound(GetPlayerClassData(m_iClass)->m_szCritDeathSound);
			}
		}
	}
	else if (!(m_bitsDamageType & DMG_CRITICAL) && m_iDamageCustom == TF_DMG_CUSTOM_BURNING)
	{
		EmitSound(GetPlayerClassData(m_iClass)->m_szDeathSound);
	}

	// Turn it into a ragdoll.
	if ( !m_bPlayingDeathAnim )
	{
		if ( cl_ragdoll_physics_enable.GetBool() )
		{
			// Make us a ragdoll.
			m_nRenderFX = kRenderFxRagdoll;

			matrix3x4_t boneDelta0[MAXSTUDIOBONES];
			matrix3x4_t boneDelta1[MAXSTUDIOBONES];
			matrix3x4_t currentBones[MAXSTUDIOBONES];
			const float boneDt = 0.05f;

			// We have to make sure that we're initting this client ragdoll off of the same model.
			// GetRagdollInitBoneArrays uses the *player* Hdr, which may be a different model than
			// the ragdoll Hdr, if we try to create a ragdoll in the same frame that the player
			// changes their player model.
			CStudioHdr *pRagdollHdr = GetModelPtr();
			CStudioHdr *pPlayerHdr = pPlayer ? pPlayer->GetModelPtr() : NULL;

			bool bChangedModel = false;
			if ( pRagdollHdr && pPlayerHdr )
			{
				bChangedModel = pRagdollHdr->GetVirtualModel() != pPlayerHdr->GetVirtualModel();

				Assert( !bChangedModel && "C_TFRagdoll::CreateTFRagdoll: Trying to create ragdoll with a different model than the player it's based on" );
			}

			if ( pPlayer && !pPlayer->IsDormant() && !bChangedModel )
			{
				pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			}
			else
			{
				GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
			}

			InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
		}
		else
		{
			// Remove it immediately.
			EndFadeOut();
		}
	}

	if ( m_iRagdollFlags & TF_RAGDOLL_BURNING )
	{
		m_flBurnEffectStartTime = gpGlobals->curtime;
		ParticleProp()->Create( "burningplayer_corpse", PATTACH_ABSORIGIN_FOLLOW );
	}

	if ( m_iRagdollFlags & TF_RAGDOLL_CHARGED )
	{
		const char *pszEffect = ConstructTeamParticle( "electrocuted_%s", m_iTeam );
		ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );
		EmitSound( "TFPlayer.MedicChargedDeath" );
	}

	if ( ( m_iRagdollFlags & TF_RAGDOLL_ASH ) && !( m_iRagdollFlags & TF_RAGDOLL_GIB ) )
	{
		ParticleProp()->Create( "drg_fiery_death", PATTACH_ABSORIGIN_FOLLOW );
		m_flTimeToDissolve = 0.5f;
	}

	if ( m_flInvisibilityLevel != 0.0f )
	{
		m_flUncloakCompleteTime = gpGlobals->curtime + 2.0f * m_flInvisibilityLevel;
	}

	if ( pPlayer )
	{
		bool bDropPartyHat = false, bDropHelmet = false;
		if ( TFGameRules() && ( TFGameRules()->IsBirthday() || TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) ) )
		{
			bDropPartyHat = true;
		}

		if ( m_iDamageCustom == TF_DMG_CUSTOM_HEADSHOT && m_iClass == TF_CLASS_SOLDIER )
		{
			if ( RandomInt( 1, 6 ) == 1 )
			{
				bDropHelmet = true;
			}
		}

		if( m_iDamageCustom == TF_DMG_CUSTOM_DECAPITATION )
		{
			DismemberHead();
			Vector vecUpward( 0, 0, 1 );
			pPlayer->CreatePlayerGibs( m_vecRagdollOrigin, vecUpward, vecUpward.Length(), m_iRagdollFlags & TF_RAGDOLL_BURNING, 1 );
		}

		if ( bDropPartyHat || bDropHelmet )
		{
			AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
			Vector vecBreakVelocity = m_vecForce + m_vecRagdollVelocity;
			vecBreakVelocity.z += tf_playergib_forceup.GetFloat();
			VectorNormalize( vecBreakVelocity );
			vecBreakVelocity *= tf_playergib_force.GetFloat();

			// Cap the impulse.
			float flSpeed = vecBreakVelocity.Length();
			if ( flSpeed > tf_playergib_maxspeed.GetFloat() )
			{
				VectorScale( vecBreakVelocity, tf_playergib_maxspeed.GetFloat() / flSpeed, vecBreakVelocity );
			}

			breakablepropparams_t breakParams( m_vecRagdollOrigin, GetRenderAngles(), vecBreakVelocity, angularImpulse );
			breakParams.impactEnergyScale = 1.0f;

			// Birthday mode.
			if ( bDropPartyHat )
			{
				pPlayer->DropPartyHat( breakParams, m_vecRagdollVelocity );
			}

			if ( bDropHelmet )
			{
				pPlayer->DropHelmet( breakParams, m_vecRagdollVelocity );

				SetBodygroup( 2, 1 );
				EmitSound( "TFPlayer.HeadshotHelmet" );
			}
		}

		switch (m_iDamageCustom)
		{
			case TF_DMG_CUSTOM_SUICIDE_BOIOING:
				m_vecBoioingVelocity = Vector(0, 0, 2000);
				break;
			case TF_DMG_CUSTOM_SUICIDE_STOMP:
				m_vecBoioingVelocity = Vector(0, 0, -2000);
				break;
		}
	}

	// Fade out the ragdoll in a while.
	StartFadeOut( cl_ragdoll_fade_time.GetFloat() );
	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_TFRagdoll::CreateTFGibs( void )
{
	SetAbsOrigin( m_vecRagdollOrigin );

	C_TFPlayer *pPlayer = GetOwningPlayer();
	if ( pPlayer && !pPlayer->m_hFirstGib )
	{
		Vector vecVelocity = m_vecForce + m_vecRagdollVelocity;
		VectorNormalize( vecVelocity );
		pPlayer->CreatePlayerGibs( m_vecRagdollOrigin, vecVelocity, m_vecForce.Length(), m_iRagdollFlags & TF_RAGDOLL_BURNING );
	}

	if ( m_iRagdollFlags & TF_RAGDOLL_CHARGED )
	{
		DispatchParticleEffect( ConstructTeamParticle( "electrocuted_gibbed_%s", m_iTeam ), m_vecRagdollOrigin, vec3_angle );
		EmitSound( "TFPlayer.MedicChargedDeath" );
	}

	if ( pPlayer && TFGameRules() && TFGameRules()->IsBirthday() )
	{
		DispatchParticleEffect( "bday_confetti", pPlayer->GetAbsOrigin() + Vector( 0, 0, 32 ), vec3_angle );

		C_BaseEntity::EmitSound( "Game.HappyBirthday" );
	}

	EndFadeOut();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
//-----------------------------------------------------------------------------
void C_TFRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		bool bCreateRagdoll = true;

		C_TFPlayer *pPlayer = GetOwningPlayer();
		if ( pPlayer )
		{
			// If we're getting the initial update for this player (e.g., after resetting entities after
			// lots of packet loss, then don't create gibs, ragdolls if the player and it's gib/ragdoll
			// both show up on same frame.
			if ( abs( pPlayer->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}
		else if ( C_BasePlayer::GetLocalPlayer() )
		{
			// Ditto for recreation of the local player.
			if ( abs( C_BasePlayer::GetLocalPlayer()->GetCreationTick() - gpGlobals->tickcount ) < TIME_TO_TICKS( 1.0f ) )
			{
				bCreateRagdoll = false;
			}
		}

		if ( bCreateRagdoll )
		{
			if ( m_iRagdollFlags & TF_RAGDOLL_GIB )
			{
				CreateTFGibs();
			}
			else
			{
				CreateTFRagdoll();
			}
		}

		m_bColorBlindInitialised = false;
	}
	else  if ( !cl_ragdoll_physics_enable.GetBool() )
	{
		// Don't let it set us back to a ragdoll with data from the server.
		m_nRenderFX = kRenderFxNone;
	}

	if (!m_bColorBlindInitialised)
	{
		UpdateTeamPatternEffect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll *C_TFRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFRagdoll::IsRagdollVisible()
{
	Vector vMins = Vector( -1, -1, -1 );	//WorldAlignMins();
	Vector vMaxs = Vector( 1, 1, 1 );		//WorldAlignMaxs();
	Vector origin = GetAbsOrigin();
	if ( !engine->IsBoxInViewCluster( vMins + origin, vMaxs + origin ) || engine->CullBox( vMins + origin, vMaxs + origin ) )
		return false;

	return true;
}


void C_TFRagdoll::ClientThink(void)
{
	SetNextClientThink(CLIENT_THINK_ALWAYS);

	// Extinguish if we're burnin' up.
	if ((m_iRagdollFlags & TF_RAGDOLL_BURNING) && (UTIL_PointContents(GetAbsOrigin()) & MASK_WATER))
	{
		m_iRagdollFlags &= ~TF_RAGDOLL_BURNING;
		m_flBurnEffectStartTime = 0.0f;

		ParticleProp()->StopParticlesNamed("burningplayer_corpse");
	}

	if (!(m_iRagdollFlags & TF_RAGDOLL_GIB))
	{
		if (m_iRagdollFlags & TF_RAGDOLL_ASH)
		{
			m_flTimeToDissolve -= gpGlobals->frametime;
			if (m_flTimeToDissolve <= 0.0f)
			{
				// Hide everything.
				AddEffects(EF_NODRAW);
				ParticleProp()->StopParticlesNamed("drg_fiery_death", true, true);

				// Hide all attached entities.
				for (C_BaseEntity* pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity))
				{
					if (pEntity->GetFollowedEntity() == this)
					{
						pEntity->AddEffects(EF_NODRAW);
					}
				}

				return;
			}
		}
	}

	if (m_bFadingOut)
	{
		int iAlpha = Max(GetRenderColor().a - (int)(600.0f * gpGlobals->frametime), 0);
		SetRenderMode(kRenderTransAlpha);
		SetRenderColorA(iAlpha);

		// Remove clientside ragdoll.
		if (iAlpha == 0)
		{
			EndFadeOut();
		}

		return;
	}

	// horrible. absolutely horrible. very very vile. never let me write code ever again
	// i have been banging my head against the wall for days on end going absolutely Insane i have missed several classes across several days all due to my own stubborness and i can not figure out any fucking way to make it work with the values the server sends
	// if i want to do that i'll have to wait for the client to interpolate a couple server frames to get enemy player velocities but thats still ping dependant and you can not trust interp history to make it reliable
	// ragdolls owned by the local player use generated fake interp history and that works perfectly fine for the local player but if you do that for anyone else it shits the bed for reasons I could not explain even if i knew
	// so either i MANUALLY edit the interpolated bone matrices that dictate the ragdoll's velocity and direction OR i do this monstrosity. and no! SetVelocityInstantaneous does not work as it should! fuck all!
	// anyway. this client think function runs on CLIENT_THINK_ALWAYS so its framerate dependent. lord GabeN had the BRILLIANT idea to NOT implement Think Contexts for clients. you can only have ONE think function per entity!!!!!
	// whatever. ill just use a variable with curtime to control the interval. i guess boioing is fixed. - azzy

	if (m_bBoioingEnabled)
	{
		if (m_flBoioingIntervalTime < gpGlobals->curtime)
		{
			if (m_vecBoioingVelocity != vec3_origin)
			{
				// i want this all to occur within 0.1 seconds
				if (m_bBoioingCount <= 5)
				{
					// this value with this amount of iterations makes the ragdoll of some of the classes do a bunch of flips which is funny
					// heavy and pyro's one is really stiff for some reason.. heavy in particular doesnt seem to go very high, while scout and spy slam the skybox. oh well. at least it preserves momentum
					IPhysicsObject* pPhysicsObject = VPhysicsGetObject();
					if ( pPhysicsObject )
					{
						pPhysicsObject->AddVelocity(&m_vecBoioingVelocity, NULL);
					}
					m_bBoioingCount++;
				}

				m_flBoioingIntervalTime = gpGlobals->curtime + 0.02f; // anything less than this makes it very unreliable for some reason.....
			}
		}
	}

	// If the player is looking at us, delay the fade.
	if ( IsRagdollVisible() )
	{
		if ( cl_ragdoll_forcefade.GetBool() )
		{
			m_bFadingOut = true;
			float flDelay = cl_ragdoll_fade_time.GetFloat() * 0.33f;
			m_fDeathTime = gpGlobals->curtime + flDelay;

			// If we were just fully healed, remove all decals.
			RemoveAllDecals();
		}

		StartFadeOut( cl_ragdoll_fade_time.GetFloat() * 0.33f );
		return;
	}

	if ( m_fDeathTime > gpGlobals->curtime )
		return;

	// Remove clientside ragdoll.
	EndFadeOut();
}


void C_TFRagdoll::StartFadeOut( float fDelay )
{
	if ( !cl_ragdoll_forcefade.GetBool() )
	{
		m_fDeathTime = gpGlobals->curtime + fDelay;
	}

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}


void C_TFRagdoll::EndFadeOut()
{
	// HACK!!!
	ParticleProp()->OwnerSetDormantTo( true );
	ParticleProp()->StopParticlesInvolving( this );

	SetNextClientThink( CLIENT_THINK_NEVER );
	ClearRagdoll();
	SetRenderMode( kRenderNone );
	DestroyBoneAttachments();
	UpdateVisibility();
	DestroyTeamPatternEffect();
}


//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CSpyInvisProxy : public IMaterialProxy
{
public:
						CSpyInvisProxy( void );
	virtual				~CSpyInvisProxy( void );
	virtual void		Release( void ) { delete this; }
	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void		OnBind( void *pC_BaseEntity );
	virtual IMaterial *	GetMaterial();

	void				HandleCloakTinting( C_BasePlayer *pPlayer );
	void				DoCloakLevel();

private:
	IMaterialVar		*m_pPercentInvisible;
	IMaterialVar		*m_pCloakColorTint;

};


CSpyInvisProxy::CSpyInvisProxy( void )
{
	m_pPercentInvisible = NULL;
	m_pCloakColorTint = NULL;
}


CSpyInvisProxy::~CSpyInvisProxy( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CSpyInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var.
	bool bInvis;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bInvis );

	bool bTint;
	m_pCloakColorTint = pMaterial->FindVar( "$cloakColorTint", &bTint );

	return bInvis && bTint;
}

ConVar tf_teammate_max_invis( "tf_teammate_max_invis", "0.95", FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CSpyInvisProxy::OnBind( void *pC_BaseEntity )
{
	if ( !m_pPercentInvisible || !m_pCloakColorTint )
		return;

	IClientRenderable *pRend = (IClientRenderable *)pC_BaseEntity;
	C_BaseEntity *pEntity = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
	if ( !pEntity )
	{
		// Have the 3D Player Model also reflect the player's cloak level.
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

		CTFPlayerModelPanel *pPlayerHUD = static_cast<CTFPlayerModelPanel *>( pRend );
		if ( pPlayer && ( pPlayerHUD && pPlayerHUD->ShouldDisplayPlayerEffect( kCloak ) ) )
		{
			m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );

			HandleCloakTinting( pPlayer );
		}
		else
		{
			m_pPercentInvisible->SetFloatValue( 0.0f );
		}

		return;
	}

	// Could be a wearable attached to the player.
	C_TFPlayer *pPlayer = ToTFPlayer( pEntity );
	if ( !pPlayer )
	{
		pPlayer = ToTFPlayer( pEntity->GetMoveParent() );
	}

	if ( pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
	}
	else
	{
		C_TFRagdoll *pRagdoll = dynamic_cast<C_TFRagdoll *>( pEntity );
		if ( pRagdoll )
		{
			pPlayer = pRagdoll->GetOwningPlayer();
			m_pPercentInvisible->SetFloatValue( pRagdoll->GetInvisibilityLevel() );
		}
	}

	if ( !pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	HandleCloakTinting( pPlayer );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CSpyInvisProxy::HandleCloakTinting( C_BasePlayer *pPlayer )
{
	Vector vecColor;

	switch ( pPlayer->GetTeamNumber() )
	{
		default:
		case TF_TEAM_RED:
			vecColor.Init( 1.0f, 0.5f, 0.4f );
			break;
		case TF_TEAM_BLUE:
			vecColor.Init( 0.4f, 0.5f, 1.0f );
			break;
		case TF_TEAM_GREEN:
			vecColor.Init( 0.4f, 1.0f, 0.5f );
			break;
		case TF_TEAM_YELLOW:
			vecColor.Init( 1.0f, 0.8f, 0.5f );
			break;
	}

	m_pCloakColorTint->SetVecValue( vecColor.Base(), 3 );
}


IMaterial *CSpyInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}
EXPOSE_INTERFACE( CSpyInvisProxy, IMaterialProxy, "spy_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for invulnerability material
//			Returns 1 if the player is invulnerable, and 0 if the player is losing / doesn't have invuln.
//-----------------------------------------------------------------------------
class CProxyInvulnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_TFPlayer *pPlayer = NULL;

		IClientRenderable *pRend = (IClientRenderable *)pC_BaseEntity;
		C_BaseEntity *pEntity = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
		if ( !pEntity )
		{
			// Have the 3D Player Model also reflect the player's crit glows.
			pPlayer = C_TFPlayer::GetLocalTFPlayer();
			CTFPlayerModelPanel *pPlayerHUD = static_cast<CTFPlayerModelPanel *>( pRend );
			if ( !pPlayer && !( pPlayerHUD && pPlayerHUD->ShouldDisplayPlayerEffect( kInvuln ) ) )
			{
				m_pResult->SetFloatValue( 1.0f );
				return;
			}
		}
		else
		{
			pPlayer = ToTFPlayer( pEntity );
		}

		if ( !pPlayer )
		{
			IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pEntity );
			if ( pOwnerInterface )
			{
				pPlayer = ToTFPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}

		if ( pPlayer )
		{
			if ( pPlayer->m_Shared.IsInvulnerable() && !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_WEARINGOFF )  )
			{
				m_pResult->SetFloatValue( 1.0f );
			}
			else
			{
				m_pResult->SetFloatValue( 0.0f );
			}
		}
		else
		{
			m_pResult->SetFloatValue( 1.0f );
		}

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};
EXPOSE_INTERFACE( CProxyInvulnLevel, IMaterialProxy, "InvulnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for burning material on player models.
// Returns 0.0->1.0 for level of burn to show on player skin.
//-----------------------------------------------------------------------------
class CProxyBurnLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( 0.0f );
			return;
		}

		// Default to zero.
		float flBurnStartTime = 0.0f;
			
		C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer *>( pEntity );
		if ( pPlayer )		
		{
			// Is the player burning?
			if (  pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
			{
				flBurnStartTime = pPlayer->m_flBurnEffectStartTime;
			}
		}
		else
		{
			// Is the ragdoll burning?
			C_TFRagdoll *pRagDoll = dynamic_cast<C_TFRagdoll *>( pEntity );
			if ( pRagDoll )
			{
				flBurnStartTime = pRagDoll->GetBurnStartTime();
			}
		}

		// If player/ragdoll is burning, set the burn level on the skin.
		float flResult;
		if ( flBurnStartTime > 0.0f )
		{
			float flBurnPeakTime = flBurnStartTime + 0.3f;
			float flTempResult;
			if ( gpGlobals->curtime < flBurnPeakTime )
			{
				// fFade in from 0->1 in 0.3 seconds.
				flTempResult = RemapValClamped( gpGlobals->curtime, flBurnStartTime, flBurnPeakTime, 0.0f, 1.0f );
			}
			else
			{
				// Fade out from 1->0 in the remaining time until flame extinguished.
				flTempResult = RemapValClamped( gpGlobals->curtime, pPlayer ? Max( flBurnPeakTime, pPlayer->m_flBurnRenewTime ) : flBurnPeakTime,
					pPlayer ? pPlayer->m_Shared.GetFlameRemoveTime() : flBurnStartTime + 10.0f, 1.0f, 0.0f );
			}	

			// We have to do some more calc here instead of in materialvars.
			flResult = 1.0f - abs( flTempResult - 1.0f );
		}
		else
		{
			flResult = 0.0f;
		}

		m_pResult->SetFloatValue( flResult );

		if ( ToolsEnabled() )
		{
			ToolFramework_RecordMaterialParams( GetMaterial() );
		}
	}
};
EXPOSE_INTERFACE( CProxyBurnLevel, IMaterialProxy, "BurnLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for jarate
//			Returns the RGB value for the appropriate tint condition.
//-----------------------------------------------------------------------------
class CProxyUrineLevel : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetVecValue( 1.0f, 1.0f, 1.0f );
			return;
		}

		C_TFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( !pPlayer )
		{
			IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pEntity );
			if ( pOwnerInterface )
			{
				pPlayer = ToTFPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}

		Vector vecColor( 1.0f, 1.0f, 1.0f );

		if ( pPlayer )
		{
			if ( pPlayer->m_Shared.InCond( TF_COND_URINE ) )
			{
				int iTeam = pPlayer->GetTeamNumber();
				if ( pPlayer->IsDisguisedEnemy() )
				{
					iTeam = pPlayer->m_Shared.GetDisguiseTeam();
				}

				switch ( iTeam )
				{
					case TF_TEAM_RED:
						vecColor.Init( 7.0f, 5.0f, 1.0f );
						break;
					case TF_TEAM_BLUE:
						vecColor.Init( 9.0f, 6.0f, 2.0f );
						break;
					case TF_TEAM_GREEN:
						vecColor.Init( 5.0f, 7.0f, 1.0f );
						break;
					case TF_TEAM_YELLOW:
						vecColor.Init( 9.0f, 6.0f, 1.0f );
						break;
				}
			}
		}

		m_pResult->SetVecValue( vecColor.Base(), 3 );
	}
};
EXPOSE_INTERFACE( CProxyUrineLevel, IMaterialProxy, "YellowLevel" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for the weapon glow color when critted
//-----------------------------------------------------------------------------
class CProxyModelGlowColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		bool bForceDisguise = false;
		C_TFPlayer *pPlayer = NULL;

		IClientRenderable *pRend = (IClientRenderable *)pC_BaseEntity;
		C_BaseEntity *pEntity = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
		if ( !pEntity )
		{
			// Have the 3D Player Model also reflect the player's crit glows.
			pPlayer = C_TFPlayer::GetLocalTFPlayer();
			CTFPlayerModelPanel *pPlayerHUD = static_cast<CTFPlayerModelPanel *>( pRend );
			if ( pPlayerHUD )
			{
				bForceDisguise = true;

				if ( !pPlayerHUD->ShouldDisplayPlayerEffect( kCrit ) || !pPlayer )
				{
					m_pResult->SetVecValue( 1.0f, 1.0f, 1.0f );
					return;
				}
			}
		}
		else
		{
			// Once everything is hit by the Uber texture, it becomes unidentifiable, let's make it once again.
			IModelGlowController *pGlowController = dynamic_cast<IModelGlowController *>( pEntity );
			if ( pGlowController && !pGlowController->ShouldGlow() )
			{
				m_pResult->SetVecValue( 1.0f, 1.0f, 1.0f );
				return;
			}

			pPlayer = ToTFPlayer( pEntity );
		}

		Vector vecColor( 1.0f, 1.0f, 1.0f );

		if ( !pPlayer )
		{
			IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pEntity );
			if ( pOwnerInterface )
			{
				pPlayer = ToTFPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}

		/*
			Live TF2 crit glow colors:
			RED Crit: 94 8 5
			BLU Crit: 6 21 80
			RED Mini-Crit: 237 140 55
			BLU Mini-Crit: 28 168 112
			Hype Mode: 50 2 50

			TF2C crit glow colors:
			GRN Crit: 1 28 9
			YLW Crit: 28 28 9
			GRN Mini-Crit: 58 180 25
			YLW Mini-Crit: 125 125 25
		*/

		if ( pPlayer )
		{
			CTFWeaponBase* pWpn = pPlayer->GetActiveTFWeapon();	// !!! foxysen beacon
			CTFBeacon* pWpnBeacon = nullptr;
			if (pWpn && pWpn->GetWeaponID() == TF_WEAPON_BEACON)
			{
				pWpnBeacon = static_cast<CTFBeacon*>(pWpn);
			}

			int iTeamNumber = pPlayer->m_Shared.IsDisguised() && (bForceDisguise || pPlayer->GetTeamNumber() != GetLocalPlayerTeam()) ? pPlayer->m_Shared.GetDisguiseTeam() : pPlayer->GetTeamNumber();
			if ((pPlayer->m_Shared.IsCritBoosted() && !pPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_HIDDEN)) || 
				(pPlayer->m_Shared.GetStoredCrits()) || (pPlayer->GetActiveTFWeapon() && pPlayer->GetActiveTFWeapon()->IsFiringRapidFireStoredCrit()) ||
				(pWpnBeacon && pWpnBeacon->IsStoredBeaconCritFilled()))
			{
				switch ( iTeamNumber )
				{
					case TF_TEAM_RED:
						vecColor.Init( 94.0f, 8.0f, 5.0f );
						break;
					case TF_TEAM_BLUE:
						vecColor.Init( 6.0f, 21.0f, 80.0f );
						break;
					case TF_TEAM_GREEN:
						vecColor.Init( 1.0f, 28.0f, 9.0f );
						break;
					case TF_TEAM_YELLOW:
						vecColor.Init( 28.0f, 28.0f, 9.0f );
						break;
				}
			}
			else if ( pPlayer->m_Shared.InCond( TF_COND_DAMAGE_BOOST )
				|| (pPlayer->GetActiveTFWeapon() && pPlayer->GetActiveTFWeapon()->IsWeaponDamageBoosted()))
			{
				switch ( iTeamNumber )
				{
					case TF_TEAM_RED:
						vecColor.Init( 237.0f, 140.0f, 55.0f );
						break;
					case TF_TEAM_BLUE:
						vecColor.Init( 28.0f, 168.0f, 112.0f );
						break;
					case TF_TEAM_GREEN:
						vecColor.Init( 58.0f, 180.0f, 25.0f );
						break;
					case TF_TEAM_YELLOW:
						vecColor.Init( 125.0f, 125.0f, 25.0f );
						break;
				}
			}
		}

		m_pResult->SetVecValue( vecColor.Base(), 3 );
	}
};
EXPOSE_INTERFACE( CProxyModelGlowColor, IMaterialProxy, "ModelGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for the weapon glow color when critted
//-----------------------------------------------------------------------------
class CProxyModelGlowColorBeaconWearable : public CResultProxy
{
public:
	void OnBind(void* pC_BaseEntity)
	{
		Assert(m_pResult);

		bool bForceDisguise = false;
		C_TFPlayer* pPlayer = NULL;

		IClientRenderable* pRend = (IClientRenderable*)pC_BaseEntity;
		C_BaseEntity* pEntity = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
		if (!pEntity)
		{
			// Have the 3D Player Model also reflect the player's crit glows.
			pPlayer = C_TFPlayer::GetLocalTFPlayer();
			CTFPlayerModelPanel* pPlayerHUD = static_cast<CTFPlayerModelPanel*>(pRend);
			if (pPlayerHUD)
			{
				bForceDisguise = true;

				if (!pPlayerHUD->ShouldDisplayPlayerEffect(kCrit) || !pPlayer)
				{
					m_pResult->SetVecValue(1.0f, 1.0f, 1.0f);
					return;
				}
			}
		}
		else
		{
			pPlayer = ToTFPlayer(pEntity);
		}

		Vector vecColor(1.0f, 1.0f, 1.0f);

		if (!pPlayer)
		{
			IHasOwner* pOwnerInterface = dynamic_cast<IHasOwner*>(pEntity);
			if (pOwnerInterface)
			{
				pPlayer = ToTFPlayer(pOwnerInterface->GetOwnerViaInterface());
			}
		}

		/*
			Live TF2 crit glow colors:
			RED Crit: 94 8 5
			BLU Crit: 6 21 80
			RED Mini-Crit: 237 140 55
			BLU Mini-Crit: 28 168 112
			Hype Mode: 50 2 50

			TF2C crit glow colors:
			GRN Crit: 1 28 9
			YLW Crit: 28 28 9
			GRN Mini-Crit: 58 180 25
			YLW Mini-Crit: 125 125 25
		*/

		if (pPlayer)
		{
			CTFWeaponBase* pWpn = pPlayer->Weapon_OwnsThisID(TF_WEAPON_BEACON);	// !!! foxysen beacon
			CTFBeacon* pWpnBeacon = nullptr;
			if (pWpn && pWpn->GetWeaponID() == TF_WEAPON_BEACON)
			{
				pWpnBeacon = static_cast<CTFBeacon*>(pWpn);
			}

			int iTeamNumber = pPlayer->m_Shared.IsDisguised() && (bForceDisguise || pPlayer->GetTeamNumber() != GetLocalPlayerTeam()) ? pPlayer->m_Shared.GetDisguiseTeam() : pPlayer->GetTeamNumber();
			if (pWpnBeacon && pWpnBeacon->IsStoredBeaconCritFilled())
			{
				switch (iTeamNumber)
				{
				case TF_TEAM_RED:
					vecColor.Init(94.0f, 8.0f, 5.0f);
					break;
				case TF_TEAM_BLUE:
					vecColor.Init(6.0f, 21.0f, 80.0f);
					break;
				case TF_TEAM_GREEN:
					vecColor.Init(1.0f, 28.0f, 9.0f);
					break;
				case TF_TEAM_YELLOW:
					vecColor.Init(28.0f, 28.0f, 9.0f);
					break;
				}
			}
		}

		m_pResult->SetVecValue(vecColor.Base(), 3);
	}
};
EXPOSE_INTERFACE(CProxyModelGlowColorBeaconWearable, IMaterialProxy, "ModelGlowColorBeaconWearable" IMATERIAL_PROXY_INTERFACE_VERSION);

//-----------------------------------------------------------------------------
// Purpose: Used for the weapon glow color when critted
//-----------------------------------------------------------------------------
class CProxyModelGlowColorChekhovsWeapon : public CResultProxy
{
public:
	void OnBind(void* pC_BaseEntity)
	{
		Assert(m_pResult);

		bool bForceDisguise = false;
		C_TFPlayer* pPlayer = NULL;

		IClientRenderable* pRend = (IClientRenderable*)pC_BaseEntity;
		C_BaseEntity* pEntity = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
		if (!pEntity)
		{
			// Have the 3D Player Model also reflect the player's crit glows.
			pPlayer = C_TFPlayer::GetLocalTFPlayer();
			CTFPlayerModelPanel* pPlayerHUD = static_cast<CTFPlayerModelPanel*>(pRend);
			if (pPlayerHUD)
			{
				bForceDisguise = true;

				if (!pPlayerHUD->ShouldDisplayPlayerEffect(kCrit) || !pPlayer)
				{
					m_pResult->SetVecValue(1.0f, 1.0f, 1.0f);
					return;
				}
			}
		}
		else
		{
			// Once everything is hit by the Uber texture, it becomes unidentifiable, let's make it once again.
			IModelGlowController* pGlowController = dynamic_cast<IModelGlowController*>(pEntity);
			if (pGlowController && !pGlowController->ShouldGlow())
			{
				m_pResult->SetVecValue(1.0f, 1.0f, 1.0f);
				return;
			}

			pPlayer = ToTFPlayer(pEntity);
		}

		Vector vecColor(1.0f, 1.0f, 1.0f);

		if (!pPlayer)
		{
			IHasOwner* pOwnerInterface = dynamic_cast<IHasOwner*>(pEntity);
			if (pOwnerInterface)
			{
				pPlayer = ToTFPlayer(pOwnerInterface->GetOwnerViaInterface());
			}
		}

		/*
			Live TF2 crit glow colors:
			RED Crit: 94 8 5
			BLU Crit: 6 21 80
			RED Mini-Crit: 237 140 55
			BLU Mini-Crit: 28 168 112
			Hype Mode: 50 2 50

			TF2C crit glow colors:
			GRN Crit: 1 28 9
			YLW Crit: 28 28 9
			GRN Mini-Crit: 58 180 25
			YLW Mini-Crit: 125 125 25
		*/

		if (pPlayer)
		{
			int iTeamNumber = pPlayer->m_Shared.IsDisguised() && (bForceDisguise || pPlayer->GetTeamNumber() != GetLocalPlayerTeam()) ? pPlayer->m_Shared.GetDisguiseTeam() : pPlayer->GetTeamNumber();
			if ((pPlayer->m_Shared.IsCritBoosted() && !pPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_HIDDEN)) ||
				(pPlayer->m_Shared.GetStoredCrits() == pPlayer->m_Shared.GetStoredCritsCapacity()) || (pPlayer->GetActiveTFWeapon() && pPlayer->GetActiveTFWeapon()->IsFiringRapidFireStoredCrit()))
			{
				switch (iTeamNumber)
				{
				case TF_TEAM_RED:
					vecColor.Init(94.0f, 8.0f, 5.0f);
					break;
				case TF_TEAM_BLUE:
					vecColor.Init(6.0f, 21.0f, 80.0f);
					break;
				case TF_TEAM_GREEN:
					vecColor.Init(1.0f, 28.0f, 9.0f);
					break;
				case TF_TEAM_YELLOW:
					vecColor.Init(28.0f, 28.0f, 9.0f);
					break;
				}
			}
			else if (pPlayer->m_Shared.InCond(TF_COND_DAMAGE_BOOST)
				|| (pPlayer->GetActiveTFWeapon() && pPlayer->GetActiveTFWeapon()->IsWeaponDamageBoosted()))
			{
				switch (iTeamNumber)
				{
				case TF_TEAM_RED:
					vecColor.Init(237.0f, 140.0f, 55.0f);
					break;
				case TF_TEAM_BLUE:
					vecColor.Init(28.0f, 168.0f, 112.0f);
					break;
				case TF_TEAM_GREEN:
					vecColor.Init(58.0f, 180.0f, 25.0f);
					break;
				case TF_TEAM_YELLOW:
					vecColor.Init(125.0f, 125.0f, 25.0f);
					break;
				}
			}

			m_pResult->SetVecValue(vecColor.Base(), 3);
		}
	}
};
EXPOSE_INTERFACE(CProxyModelGlowColorChekhovsWeapon, IMaterialProxy, "ModelGlowColorChekhovsWeapon" IMATERIAL_PROXY_INTERFACE_VERSION);

//-----------------------------------------------------------------------------
// Purpose: Used for paintable items in live TF2.
//-----------------------------------------------------------------------------
class CProxyItemTintColor : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		m_pResult->SetVecValue( 0.0f, 0.0f, 0.0f );
	}
};
EXPOSE_INTERFACE( CProxyItemTintColor, IMaterialProxy, "ItemTintColor" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Using for sparkle effect on Community items in live TF2.
//-----------------------------------------------------------------------------
class CProxyCommunityWeapon : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
	}
};
EXPOSE_INTERFACE( CProxyCommunityWeapon, IMaterialProxy, "CommunityWeapon" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Used for killstreak sheens in live TF2.
//-----------------------------------------------------------------------------
class CProxyAnimatedWeaponSheen : public CBaseAnimatedTextureProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
	}

	float GetAnimationStartTime( void* pBaseEntity )
	{
		return 0.0f;
	}
};
EXPOSE_INTERFACE( CProxyAnimatedWeaponSheen, IMaterialProxy, "AnimatedWeaponSheen" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Universal proxy from Live TF2 used for spy invisiblity material
// It's purpose is to replace weapon_invis, vm_invis and spy_invis.
//-----------------------------------------------------------------------------
class CInvisProxy : public IMaterialProxy
{
public:
	CInvisProxy( void );
	virtual				~CInvisProxy( void );
	virtual void		Release( void ) { delete this; }
	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void		OnBind( void *pC_BaseEntity );
	virtual IMaterial	*GetMaterial();

	virtual void		HandleSpyInvis( C_TFPlayer *pPlayer );
	virtual void		HandleVMInvis( C_BaseEntity *pVM );
	virtual void		HandleWeaponInvis( C_BaseEntity *pC_BaseEntity );

private:
	IMaterialVar		*m_pPercentInvisible;
	IMaterialVar		*m_pCloakColorTint;
};


CInvisProxy::CInvisProxy( void )
{
	m_pPercentInvisible = NULL;
	m_pCloakColorTint = NULL;
}


CInvisProxy::~CInvisProxy( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var.
	bool bInvis;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bInvis );

	bool bTint;
	m_pCloakColorTint = pMaterial->FindVar( "$cloakColorTint", &bTint );

	// If we have $cloakColorTint, it's spy_invis.
	if ( bTint )
		return bInvis;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::OnBind( void *pC_BaseEntity )
{
	IClientRenderable *pRend = (IClientRenderable *)pC_BaseEntity;
	C_BaseEntity *pEnt = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
	if ( !pEnt )
	{
		if ( m_pPercentInvisible )
		{
			// Do it here, too.
			C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
			CTFPlayerModelPanel *pPlayerHUD = static_cast<CTFPlayerModelPanel *>( pRend );
			if ( pPlayer && ( pPlayerHUD && pPlayerHUD->ShouldDisplayPlayerEffect( kCloak ) ) )
			{
				m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
			}
			else
			{
				m_pPercentInvisible->SetFloatValue( 0.0f );
			}
		}
		return;
	}

	C_TFPlayer *pPlayer = ToTFPlayer( pEnt );
	if ( pPlayer )
	{
		HandleSpyInvis( pPlayer );
		return;
	}

	C_BaseAnimating *pAnim = pEnt->GetBaseAnimating();
	if ( !pAnim )
		return;

	if ( pAnim->IsViewModel() )
	{
		HandleVMInvis( pAnim );
		return;
	}

	HandleWeaponInvis( pAnim );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandleSpyInvis( C_TFPlayer *pPlayer )
{
	if ( !m_pPercentInvisible || !m_pCloakColorTint )
		return;

	m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );

	Vector vecColor;

	switch ( pPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			vecColor.Init( 1.0f, 0.5f, 0.4f );
			break;

		case TF_TEAM_BLUE:
			vecColor.Init( 0.4f, 0.5f, 1.0f );
			break;

		case TF_TEAM_GREEN:
			vecColor.Init( 0.4f, 1.0f, 0.5f );
			break;

		case TF_TEAM_YELLOW:
			vecColor.Init( 1.0f, 0.8f, 0.5f );
			break;

		default:
			vecColor.Init( 1.0f, 0.5f, 0.4f );
			break;
	}

	m_pCloakColorTint->SetVecValue( vecColor.Base(), 3 );
}

extern ConVar tf_vm_min_invis;
extern ConVar tf_vm_max_invis;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandleVMInvis( C_BaseEntity *pVM )
{
	if ( !m_pPercentInvisible )
		return;

	IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pVM );
	if ( !pOwnerInterface )
		return;

	C_TFPlayer *pPlayer = ToTFPlayer( pOwnerInterface->GetOwnerViaInterface() );
	if ( !pPlayer )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	// Remap from 0.22 to 0.5,
	// but drop to 0.0 if we're not invis at all.
	float flPercentInvisible = pPlayer->GetPercentInvisible();
	m_pPercentInvisible->SetFloatValue( pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ? 0.3f : flPercentInvisible < 0.01f ? 0.0f : RemapVal( flPercentInvisible, 0.0f, 1.0f, tf_vm_min_invis.GetFloat(), tf_vm_max_invis.GetFloat() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CInvisProxy::HandleWeaponInvis( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible )
		return;

	C_BaseEntity *pMoveParent = pEnt->GetMoveParent();
	if ( !pMoveParent || !pMoveParent->IsPlayer() )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	C_TFPlayer *pPlayer = ToTFPlayer( pMoveParent );
	Assert( pPlayer );

	m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
}


IMaterial *CInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}
EXPOSE_INTERFACE( CInvisProxy, IMaterialProxy, "invis" IMATERIAL_PROXY_INTERFACE_VERSION );


class CBuildingInvisProxy : public CEntityMaterialProxy
{
public:
	CBuildingInvisProxy();

	virtual bool		Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void		OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial	*GetMaterial();

private:
	IMaterialVar		*m_pPercentInvisible;
};


CBuildingInvisProxy::CBuildingInvisProxy()
{
	m_pPercentInvisible = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input  : *pMaterial - 
//-----------------------------------------------------------------------------
bool CBuildingInvisProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var.
	bool bInvis;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bInvis );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :
//-----------------------------------------------------------------------------
void CBuildingInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if ( !m_pPercentInvisible )
		return;

	m_pPercentInvisible->SetFloatValue( 0.0f );
}


IMaterial *CBuildingInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}
EXPOSE_INTERFACE( CBuildingInvisProxy, IMaterialProxy, "building_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Flash shield if player just took damage.
//-----------------------------------------------------------------------------
class CProxyResistShield : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

#if 1
		m_pResult->SetFloatValue( 1.0f );
#else
		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( 1.0f );
			return;
		}

		C_TFPlayer *pPlayer = ToTFPlayer( pEntity->GetOwnerEntity() );

		if ( false )
		{
			float flValue = RemapValClamped( gpGlobals->curtime - pPlayer->m_flLastDamageTime,
				0.0f, 0.4f, 7.0f, -4.0f );

			m_pResult->SetFloatValue( flValue );
		}
		else
		{
			m_pResult->SetFloatValue( 1.0f );
		}
#endif
	}
};
EXPOSE_INTERFACE( CProxyResistShield, IMaterialProxy, "ShieldFalloff" IMATERIAL_PROXY_INTERFACE_VERSION );

//-----------------------------------------------------------------------------
// Purpose: Item meter charge level (Used by the Taser).
//-----------------------------------------------------------------------------
class CProxyElectricCharge : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );

		C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
		if ( !pEntity )
		{
			m_pResult->SetFloatValue( 1.0f );
			return;
		}

		C_TFPlayer *pPlayer = ToTFPlayer( pEntity );
		if ( !pPlayer )
		{
			IHasOwner *pOwnerInterface = dynamic_cast<IHasOwner *>( pEntity );
			if ( pOwnerInterface )
			{
				pPlayer = ToTFPlayer( pOwnerInterface->GetOwnerViaInterface() );
			}
		}

		if ( pPlayer )
		{
			if ( pPlayer->IsPlayerUnderwater() )
			{
				m_pResult->SetFloatValue( 0.0f );
				return;
			}

			C_TFWeaponBase *pActiveWeapon = pPlayer->GetActiveTFWeapon();
			if ( pActiveWeapon && pActiveWeapon->GetProgress() < 1.0f )
			{
				m_pResult->SetFloatValue( RemapValClamped( pActiveWeapon->GetProgress(), 0.0f, 1.0f, 0.0f, 0.25f ) );
			}
			else
			{
				m_pResult->SetFloatValue( 1.0f );
			}
		}
		else
		{
			m_pResult->SetFloatValue( 1.0f );
		}
	}
};
EXPOSE_INTERFACE( CProxyElectricCharge, IMaterialProxy, "ElectricCharge" IMATERIAL_PROXY_INTERFACE_VERSION );

// Commented out this section, it's not ready nor complete nor really useful yet.
//-----------------------------------------------------------------------------
// Purpose: Tries to get the owning player from a proxy.
//-----------------------------------------------------------------------------
/*C_TFPlayer *GetOwnerFromProxyEntity( void *pEntity )
{
	IClientRenderable *pRend = (IClientRenderable *) pEntity;
	CBaseEntity *pBaseEntity = pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;

	if ( pBaseEntity )
	{
		// Get the entity's owner and see if it's a C_TFPlayer.
		CBaseEntity *pOwner = pBaseEntity->GetOwnerEntity();
		if ( pOwner )
		{
			return ToTFPlayer( pOwner );
		}
	}

	return NULL;
}

// Used for overriding entire material ie Uber
struct TextureVarSetter
{
	TextureVarSetter( IMaterialVar *pDestVar, ITexture *pDefaultMaterial ) 
	: m_pDestVar( pDestVar )
	, m_pTexture( pDefaultMaterial )
	{ 
		Assert( pDestVar && pDefaultTexture ); 
	}

	void SetMaterial( ITexture *pTexture ) { m_pTexture = pTexture; }

	~TextureVarSetter()
	{
		m_pDestVar->SetTextureValue( m_pTexture );
	}

	IMaterialVar *m_pDestVar;
	ITexture *m_pTexture;
};

//-----------------------------------------------------------------------------
// Purpose: Makes the weapon appear all bright when Invulnerable
// for materials using this proxy.
//-----------------------------------------------------------------------------
class CProxyUber : public IMaterialProxy
{
public:
	CProxyUber( void )
	: m_pMaterial( NULL )
	, m_pBaseTextureVar( NULL )
	, m_pBaseTextureOrig( NULL )
	{
	}

	~CProxyUber()
	{
		SafeRelease( &m_pBaseTextureOrig );
	}

	inline bool TestAndSetBaseTexture()
	{
		if ( m_pBaseTextureOrig )
			return true;

		Assert( m_pBaseTextureVar != NULL );

		// Check if the material is actually loaded. 
		if ( !m_pBaseTextureVar->IsTexture() )
			return false;

		ITexture* baseTexture = m_pBaseTextureVar->GetTextureValue();
		Assert( baseTexture != NULL );
		SafeAssign( &m_pBaseTextureOrig, baseTexture );
		return true;
	}

	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
	{
		// We don't support DX8
		ConVarRef mat_dxlevel( "mat_dxlevel" );
		if ( mat_dxlevel.GetInt() < 90 )
			return false;

		Assert( pMaterial );
		m_pMaterial = pMaterial;

		bool bFound = false;
		m_pBaseTextureVar = m_pMaterial->FindVar( "$basetexture", &bFound );
		if ( !bFound ) 
			return false;

		// We don't actually care if the code succeeds here.
		TestAndSetBaseTexture();
	
		return true;
	}

	virtual void OnBind( void *pC_BaseEntity )
	{
		// If the base texture isn't ready yet, we can't really do anything.
		if ( !TestAndSetBaseTexture() )
			return;

		Assert( m_pBaseTextureVar );

		// This will set the texture when it goes out of scope.
		TextureVarSetter setter( m_pBaseTextureVar, m_pBaseTextureOrig );
	}

	virtual void Release() { delete this; }
	virtual IMaterial *	GetMaterial() { return m_pMaterial; }

private:
	IMaterial			*m_pMaterial;

	IMaterialVar		*m_pBaseTextureVar;
	ITexture			*m_pBaseTextureOrig;
};
EXPOSE_INTERFACE( CProxyUber, IMaterialProxy, "Uber" IMATERIAL_PROXY_INTERFACE_VERSION );*/

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Player's object UtlVector to entindexes
//-----------------------------------------------------------------------------
void RecvProxy_PlayerObjectList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_TFPlayer *pPlayer = (C_TFPlayer *)pStruct;
	CBaseHandle *pHandle = (CBaseHandle *)( &( pPlayer->m_aObjects[pData->m_iElement] ) ); 
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );
}


void RecvProxyArrayLength_PlayerObjects( void *pStruct, int objectID, int currentArrayLength )
{
	C_TFPlayer *pPlayer = (C_TFPlayer *)pStruct;
	if ( pPlayer->m_aObjects.Count() != currentArrayLength )
	{
		pPlayer->m_aObjects.SetSize( currentArrayLength );
	}

	pPlayer->ForceUpdateObjectHudState();
}

// Specific to the local player.
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFLocalPlayerExclusive )
	RecvPropVectorXY( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_vecNetworkOrigin[2], m_vecOrigin[2] ) ),
	RecvPropArray2( 
		RecvProxyArrayLength_PlayerObjects,
		RecvPropInt( "player_object_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_PlayerObjectList ), 
		MAX_OBJECTS_PER_PLAYER, 
		0, 
		"player_object_array"
	),

	RecvPropQAngles( RECVINFO( m_angEyeAngles ) ),

	RecvPropEHandle( RECVINFO( m_hOffHandWeapon ) ),
	RecvPropInt( RECVINFO( m_nForceTauntCam ) ),
	RecvPropBool( RECVINFO( m_bArenaSpectator ) ),
#ifdef ITEM_TAUNTING
	RecvPropBool( RECVINFO( m_bAllowMoveDuringTaunt ) ),
	RecvPropBool( RECVINFO( m_bTauntForceForward ) ),
	RecvPropFloat( RECVINFO( m_flTauntSpeed ) ),
	RecvPropFloat( RECVINFO( m_flTauntTurnSpeed ) ),
#endif

	RecvPropFloat( RECVINFO( m_flRespawnTimeOverride ) ),
END_RECV_TABLE()

// All players except the local player.
BEGIN_RECV_TABLE_NOBASE( C_TFPlayer, DT_TFNonLocalPlayerExclusive )
	RecvPropVectorXY( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_vecNetworkOrigin[2], m_vecOrigin[2] ) ),

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

	RecvPropInt( RECVINFO( m_nActiveWpnClip ) ),
	RecvPropInt( RECVINFO( m_nActiveWpnAmmo ) ),

	RecvPropBool( RECVINFO( m_bTyping ) ),
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_TFPlayer, DT_TFPlayer, CTFPlayer )
	RecvPropBool( RECVINFO( m_bSaveMeParity ) ),
	RecvPropBool( RECVINFO( m_bIsABot ) ),

	// This will create a race condition with the local player, but the data will be the same so.....
	RecvPropInt( RECVINFO( m_nWaterLevel ) ),

	RecvPropEHandle( RECVINFO( m_hItem ) ),

	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),

	RecvPropDataTable( RECVINFO_DT( m_PlayerClass ), 0, &REFERENCE_RECV_TABLE( DT_TFPlayerClassShared ) ),
	RecvPropDataTable( RECVINFO_DT( m_Shared ), 0, &REFERENCE_RECV_TABLE( DT_TFPlayerShared ) ),
	RecvPropDataTable( RECVINFO_DT( m_AttributeManager ), 0, &REFERENCE_RECV_TABLE( DT_AttributeContainerPlayer ) ),

	RecvPropDataTable( "tflocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_TFLocalPlayerExclusive ) ),
	RecvPropDataTable( "tfnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_TFNonLocalPlayerExclusive ) ),

	RecvPropInt( RECVINFO( m_iSpawnCounter ) ),
	RecvPropTime( RECVINFO( m_flLastDamageTime ) ),
	RecvPropBool( RECVINFO( m_bAutoReload ) ),
	RecvPropBool( RECVINFO( m_bFlipViewModel ) ),
	RecvPropFloat( RECVINFO( m_flViewModelFOV ) ),
	RecvPropVector( RECVINFO( m_vecViewModelOffset ) ),
	RecvPropBool( RECVINFO( m_bMinimizedViewModels ) ),
	RecvPropBool( RECVINFO( m_bInvisibleArms ) ),
	RecvPropBool( RECVINFO( m_bFixedSpreadPreference ) ),
	RecvPropBool( RECVINFO( m_bSpywalkInverted ) ),
	RecvPropBool( RECVINFO( m_bCenterFirePreference ) ),
END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_TFPlayer )
	DEFINE_PRED_TYPEDESCRIPTION( m_Shared, CTFPlayerShared ),
	DEFINE_PRED_FIELD( m_nSkin, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nBody, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_nMuzzleFlashParity, FIELD_CHARACTER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE  ),
	DEFINE_PRED_FIELD( m_hOffHandWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_angEyeAngles, FIELD_VECTOR, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flWaterEntryTime, FIELD_FLOAT, 0 ),
END_PREDICTION_DATA()

// ------------------------------------------------------------------------------------------ //
// C_TFPlayer implementation.
// ------------------------------------------------------------------------------------------ //
C_TFPlayer::C_TFPlayer() : 
	m_iv_angEyeAngles( "C_TFPlayer::m_iv_angEyeAngles" ),
	m_mapOverheadEffects( DefLessFunc( const char * ) )
{
	m_pAttributes = this;

	m_PlayerAnimState = CreateTFPlayerAnimState( this );
	m_Shared.Init( this );

	m_iIDEntIndex = 0;

	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_pTeleporterEffect = NULL;
	m_flBurnEffectStartTime = 0;
	m_flBurnRenewTime = 0;
	m_pSaveMeEffect = NULL;
	m_pTypingEffect = NULL;
	m_pVoiceEffect = NULL;
	m_pOverhealEffect = NULL;
	m_pTauntSound = NULL;
	
	m_aGibs.Purge();

	m_pCigaretteSmoke = NULL;
	m_pCigaretteLight = NULL;

	m_hRagdoll.Set( NULL );

	m_iPreviousMetal = 0;
	m_bIsDisplayingNemesisIcon = false;

	m_bWasTaunting = false;
	m_flTauntOffTime = 0.0f;
	m_angTauntPredViewAngles.Init();
	m_angTauntEngViewAngles.Init();

	m_flWaterEntryTime = 0;
	m_nOldWaterLevel = WL_NotInWater;
	m_bWaterExitEffectActive = false;

	m_iOldObserverMode = OBS_MODE_NONE;

	m_bUpdateObjectHudState = false;

	m_bUpdateAttachedModels = false;

	m_bTyping = false;

	m_pBlastJumpLoop = NULL;
	m_flBlastJumpLaunchTime = 0.0f;

	m_flChangeClassTime = 0.0f;

	m_pJumppadJumpEffect = nullptr;
	m_pSpeedEffect = nullptr;

	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "rocket_jump" );
	ListenForGameEvent( "rocket_jump_landed" );
	ListenForGameEvent( "sticky_jump" );
	ListenForGameEvent( "sticky_jump_landed" );

	m_bColorBlindInitialised = false;
	PrecacheMaterial("effects/colourblind/cba_redteam");
	PrecacheMaterial("effects/colourblind/cba_greenteam");
	PrecacheMaterial("effects/colourblind/cba_blueteam");
	PrecacheMaterial("effects/colourblind/cba_yellowteam");
	PrecacheMaterial("dev/glow_cba");

	// Load phonemes for MP3s.
	engine->AddPhonemeFile( "scripts/game_sounds_vo_phonemes.txt" );
	engine->AddPhonemeFile( nullptr ); // Optimization by Valve; nullptr tells the engine that we have loaded all phomeme files we want.
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
C_TFPlayer::~C_TFPlayer()
{
	if ( m_bIsDisplayingNemesisIcon )
	{
		ShowNemesisIcon( false );
	}

	m_PlayerAnimState->Release();

	if ( m_pBlastJumpLoop )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		controller.SoundDestroy( m_pBlastJumpLoop );
		m_pBlastJumpLoop = NULL;
	}

	// Free the team pattern effect object
	DestroyTeamPatternEffect();

	if ( IsLocalPlayer() )
	{
		g_ItemEffectMeterManager.ClearExistingMeters();
	}
}


C_TFPlayer* C_TFPlayer::GetLocalTFPlayer()
{
	return ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
}


const QAngle &C_TFPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
		return vec3_angle;
	
	return m_PlayerAnimState->GetRenderAngles();
}


void C_TFPlayer::UpdateOnRemove( void )
{
	// Stop the taunt.
	if ( m_bWasTaunting )
	{
		TurnOffTauntCam();
	}

	// HACK!!! ChrisG needs to fix this in the particle system.
	ParticleProp()->OwnerSetDormantTo( true );
	ParticleProp()->StopParticlesInvolving( this );

	// Remove all conditions, this should also kill all effects and looping sounds.
	m_Shared.RemoveAllCond();

	if ( IsLocalPlayer() )
	{
		CTFStatPanel *pStatPanel = GetStatPanel();
		pStatPanel->OnLocalPlayerRemove( this );
	}

#ifdef ITEM_TAUNTING
	StopTauntSoundLoop();
#endif

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: returns max health for this player
//-----------------------------------------------------------------------------
int C_TFPlayer::GetMaxHealth( void ) const
{	
	return g_TF_PR ? g_TF_PR->GetMaxHealth( entindex() ) : 1;
}

//-----------------------------------------------------------------------------
// Purpose: Deal with recording
//-----------------------------------------------------------------------------
void C_TFPlayer::GetToolRecordingState( KeyValues *msg )
{
#ifndef _XBOX
	BaseClass::GetToolRecordingState( msg );
	BaseEntityRecordingState_t *pBaseEntityState = ( BaseEntityRecordingState_t * )msg->GetPtr( "baseentity" );

	bool bDormant = IsDormant();
	bool bDead = !IsAlive();
	bool bSpectator = ( GetTeamNumber() == TEAM_SPECTATOR );
	bool bNoRender = ( GetRenderMode() == kRenderNone );
	bool bDeathCam = ( GetObserverMode() == OBS_MODE_DEATHCAM );
	bool bNoDraw = IsEffectActive( EF_NODRAW );

	bool bVisible = 
		!bDormant && 
		!bDead && 
		!bSpectator &&
		!bNoRender &&
		!bDeathCam &&
		!bNoDraw;

	bool changed = m_bToolRecordingVisibility != bVisible;
	// Remember state
	m_bToolRecordingVisibility = bVisible;

	pBaseEntityState->m_bVisible = bVisible;
	if ( changed && !bVisible )
	{
		// If the entity becomes invisible this frame, we still want to record a final animation sample so that we have data to interpolate
		//  toward just before the logs return "false" for visiblity.  Otherwise the animation will freeze on the last frame while the model
		//  is still able to render for just a bit.
		pBaseEntityState->m_bRecordFinalVisibleSample = true;
	}
#endif
}


void C_TFPlayer::UpdateClientSideAnimation()
{
	// Update the animation data.
	m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}


void C_TFPlayer::SetDormant( bool bDormant )
{
	if ( !IsDormant() && bDormant )
	{
		if ( m_bIsDisplayingNemesisIcon )
		{
			ShowNemesisIcon( false );
		}

		UpdatedMarkedForDeathEffect( true );

		// Kill the cig light.
		if ( m_pCigaretteLight && m_pCigaretteLight->key == entindex() )
		{
			m_pCigaretteLight->die = gpGlobals->curtime;
		}

		m_pCigaretteLight = NULL;
	}

	if ( IsDormant() && !bDormant )
	{
		// Update client-side models attached to us.
		m_bUpdatePartyHat = true;
		m_bUpdateAttachedModels = true;
	}

	m_Shared.UpdateLoopingSounds( bDormant );

	// Deliberately skip base player.
	C_BaseEntity::SetDormant( bDormant );

	// Kill speech bubbles.
	UpdateSpeechBubbles();
}


void C_TFPlayer::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldHealth = m_iHealth;
	m_iOldPlayerClass = m_PlayerClass.GetClassIndex();
	m_iOldSpawnCounter = m_iSpawnCounter;
	m_bOldSaveMeParity = m_bSaveMeParity;
	m_nOldWaterLevel = GetWaterLevel();

	m_iOldTeam = GetTeamNumber();
	m_bDisguised = m_Shared.IsDisguised();

	// Just to clear some things up here
	// We use GetDisguiseTeam here instead of GetTrueDisguiseTeam
	// The reason for that is that this variable is used exclusively for party hat skins
	// Which SHOULD update when the local player changes team as well
	// So making it follow global disguises gives us that effect
	// If we ever do need the true disguise team here
	// Simply create a new variable to save it to - Kay
	m_iOldDisguiseTeam = m_Shared.GetDisguiseTeam();
	m_iOldDisguiseClass = m_Shared.GetDisguiseClass();
	m_hOldActiveWeapon.Set( GetActiveTFWeapon() );

	C_BaseEntity *pObserverTarget = GetObserverTarget();
	m_hOldObserverTarget = pObserverTarget;
	m_iOldObserverMode = GetObserverMode();
	m_iOldObserverTeam = pObserverTarget ? pObserverTarget->GetTeamNumber() : TEAM_UNASSIGNED;

	m_Shared.OnPreDataChanged();

	m_bOldCustomModelVisible = m_PlayerClass.CustomModelIsVisibleToSelf();

	m_iOldStoredCrits = m_Shared.GetStoredCrits();
}


void C_TFPlayer::OnDataChanged( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );

	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		InitInvulnerableMaterial();
#ifdef TF2C_BETA
		InitInvulnerableMaterialPlayerModelPanel();
#endif
		m_bColorBlindInitialised = false;
	}
	else
	{
		if (m_iOldTeam != GetTeamNumber() || m_iOldDisguiseTeam != m_Shared.GetDisguiseTeam())
		{
			m_bUpdatePartyHat = true;

			InitInvulnerableMaterial();
#ifdef TF2C_BETA
			InitInvulnerableMaterialPlayerModelPanel();
#endif
			m_bColorBlindInitialised = false;
#ifdef DEBUG_COLOURBLIND
			if (m_iOldTeam != GetTeamNumber())
			{
				DevMsg("Team was changed; updating CBA for %s\n", GetPlayerName());
			}
			if (m_iOldDisguiseTeam != m_Shared.GetDisguiseTeam())
			{
				DevMsg("Disguise was changed; updating CBA for %s\n", GetPlayerName());
			}
#endif
		}

		UpdateWearables();
	}

	if ( GetActiveTFWeapon() != m_hOldActiveWeapon.Get() )
	{
		m_Shared.UpdateCritBoostEffect();

		if ( !GetPredictable() )
		{
			m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );
		}
	}

	if (m_Shared.GetStoredCrits() != m_iOldStoredCrits)
	{
		m_Shared.UpdateCritBoostEffect();
	}

	// Check for full health and remove decals.
	if ( ( m_iHealth > m_iOldHealth && m_iHealth >= GetMaxHealth() ) || m_Shared.IsInvulnerable() )
	{
		// If we were just fully healed, remove all decals.
		RemoveAllDecals();
	}

	// Detect class changes.
	if ( m_iOldPlayerClass != m_PlayerClass.GetClassIndex() )
	{
		OnPlayerClassChange();

		m_bColorBlindInitialised = false;
	}

	if ( m_iOldSpawnCounter != m_iSpawnCounter )
	{
		ClientPlayerRespawn();

		m_bUpdatePartyHat = true;
	}

	if ( m_bSaveMeParity != m_bOldSaveMeParity )
	{
		// Player has triggered a save me command
		CreateSaveMeEffect( false );
	}

	UpdateSpeechBubbles();

	// See if we should show or hide nemesis icon for this player
	if ( m_bIsDisplayingNemesisIcon != ShouldShowNemesisIcon() )
	{
		ShowNemesisIcon( ShouldShowNemesisIcon() );
	}

	m_Shared.OnDataChanged();

	UpdateClientSideGlow();

	if (!m_bColorBlindInitialised)
	{
		UpdateTeamPatternEffect();
	}

	if ( m_iOldHealth != m_iHealth && ( HasTheFlag() || IsVIP() ) && GetGlowObject() )
	{
		// Update the glow color on flag carrier according to their health.
		Vector vecColor;
		GetGlowEffectColor( &vecColor.x, &vecColor.y, &vecColor.z );

		GetGlowObject()->SetColor( vecColor );
	}

	if ( IsLocalPlayer() )
	{
		if ( updateType == DATA_UPDATE_CREATED )
		{
			SetupHeadLabelMaterials();
			GetClientVoiceMgr()->SetHeadLabelOffset( 50 );
		}

		if ( m_iOldTeam != GetTeamNumber() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeteam" );
			if ( event )
			{
				gameeventmanager->FireEventClientSide( event );
			}
		}

		if ( !IsPlayerClass( m_iOldPlayerClass, true ) )
		{
			m_flChangeClassTime = gpGlobals->curtime;

			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeclass" );
			if ( event )
			{
				event->SetInt( "updateType", updateType );
				gameeventmanager->FireEventClientSide( event );
			}
		}


		if ( m_iOldPlayerClass == TF_CLASS_SPY &&
		   ( m_bDisguised != m_Shared.IsDisguised() || m_iOldDisguiseClass != m_Shared.GetDisguiseClass() ) )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changedisguise" );
			if ( event )
			{
				event->SetBool( "disguised", m_Shared.IsDisguised() );
				gameeventmanager->FireEventClientSide( event );
			}

			m_bColorBlindInitialised = false;
			m_bDisguised = true;
		}

		// If our metal amount changed, send a game event.
		int iCurrentMetal = GetAmmoCount( TF_AMMO_METAL );	
		if ( iCurrentMetal != m_iPreviousMetal )
		{
			// MSG
			IGameEvent *event = gameeventmanager->CreateEvent( "player_account_changed" );
			if ( event )
			{
				event->SetInt( "old_account", m_iPreviousMetal );
				event->SetInt( "new_account", iCurrentMetal );
				gameeventmanager->FireEventClientSide( event );
			}

			m_iPreviousMetal = iCurrentMetal;
		}

		C_BaseEntity *pObserverTarget = GetObserverTarget();
		if ( pObserverTarget != m_hOldObserverTarget.Get() )
		{
			// Update effects on players when chaging targets.
			C_TFPlayer *pOldPlayer = ToTFPlayer( m_hOldObserverTarget.Get() );
			if ( pOldPlayer )
			{
				pOldPlayer->ThirdPersonSwitch( false );
			}

			C_TFPlayer *pNewPlayer = ToTFPlayer( pObserverTarget );
			if ( pNewPlayer )
			{
				pNewPlayer->ThirdPersonSwitch( GetObserverMode() == OBS_MODE_IN_EYE );
			}

			// HACK: This is only really here to keep the overlays updated to an observer target's team.
			if ( pObserverTarget && pObserverTarget->GetTeamNumber() != m_iOldObserverTeam )
			{
				IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_changeteam" );
				if ( event )
				{
					gameeventmanager->FireEventClientSide( event );
				}
			}
		}
		else if ( GetObserverMode() != m_iOldObserverMode &&
			( GetObserverMode() == OBS_MODE_IN_EYE || m_iOldObserverMode == OBS_MODE_IN_EYE ) )
		{
			// Update effects on the spectated player when switching between first and third person view.
			C_TFPlayer *pPlayer = ToTFPlayer( GetObserverTarget() );
			if ( pPlayer )
			{
				pPlayer->ThirdPersonSwitch( GetObserverMode() != OBS_MODE_IN_EYE );
			}
		}

		if ( m_bOldCustomModelVisible != m_PlayerClass.CustomModelIsVisibleToSelf() )
		{
			UpdateVisibility();
		}
	}

	// Some time in this network transmit we changed the size of the object array,
	// recalculate the whole thing and update the HUD.
	if ( m_bUpdateObjectHudState )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "building_info_changed" );
		if ( event )
		{
			event->SetInt( "building_type", -1 );
			event->SetInt( "object_mode", OBJECT_MODE_NONE );
			gameeventmanager->FireEventClientSide( event );
		}
	
		m_bUpdateObjectHudState = false;
	}
}


void C_TFPlayer::InitInvulnerableMaterial( void )
{
	// If this player is disguised and on the other team, use disguise team.
	int iTeam = IsDisguisedEnemy() ? m_Shared.GetDisguiseTeam() : GetTeamNumber();
	const char *pszMaterial = pszInvulnerableMaterialNames[GetTeamSkin( iTeam )];
	if ( pszMaterial )
	{
		m_InvulnerableMaterial.Init( pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
	else
	{
		m_InvulnerableMaterial.Shutdown();
	}
}
#ifdef TF2C_BETA
void C_TFPlayer::InitInvulnerableMaterialPlayerModelPanel(void)
{
	// same for InitInvulnerableMaterial but also counts global team, only for playermodelpanel
	int iTeam = m_Shared.IsDisguised() ? m_Shared.GetTrueDisguiseTeam() : GetTeamNumber();
	const char* pszMaterial = pszInvulnerableMaterialNames[GetTeamSkin(iTeam)];
	if (pszMaterial)
	{
		m_InvulnerableMaterialPlayerModelPanel.Init(pszMaterial, TEXTURE_GROUP_CLIENT_EFFECTS);
	}
	else
	{
		m_InvulnerableMaterialPlayerModelPanel.Shutdown();
	}
}
#endif
void C_TFPlayer::UpdateRecentlyTeleportedEffect( void )
{
	if ( m_pTeleporterEffect )
	{
		ParticleProp()->StopEmission( m_pTeleporterEffect );
		m_pTeleporterEffect = NULL;
	}

	if ( m_Shared.ShouldShowRecentlyTeleported() )
	{
		m_pTeleporterEffect = ParticleProp()->Create( ConstructTeamParticle( "player_recent_teleport_%s", IsDisguisedEnemy() ? m_Shared.GetDisguiseTeam() : GetTeamNumber() ), PATTACH_ABSORIGIN_FOLLOW );
	}
}


void C_TFPlayer::UpdateJumppadTrailEffect(void)
{
	if (m_pJumppadJumpEffect)
	{
		ParticleProp()->StopEmission(m_pJumppadJumpEffect);
		m_pJumppadJumpEffect = nullptr;
	}

	if (m_Shared.InCond(TF_COND_LAUNCHED) && !m_pJumppadJumpEffect && !m_Shared.IsStealthed())
	{
		const char* pszSparklesEffect = ConstructTeamParticle( "jumppad_trail_%s", IsDisguisedEnemy() ? m_Shared.GetDisguiseTeam() : GetTeamNumber() );
		// Longer trail prototype, unpolished
		//const char *pszSparklesEffect = ConstructTeamParticle("jumppad_trail_%s_long", IsDisguisedEnemy() ? m_Shared.GetDisguiseTeam() : GetTeamNumber());
		m_pJumppadJumpEffect = ParticleProp()->Create(pszSparklesEffect, PATTACH_ABSORIGIN);
	}
}

void C_TFPlayer::UpdateSpeedBoostTrailEffect(void)
{
	if ( m_pSpeedEffect )
	{
		ParticleProp()->StopEmission( m_pSpeedEffect );
		m_pSpeedEffect = nullptr;
	}

	C_TFPlayer* pTFCondProvider = ToTFPlayer(m_Shared.GetConditionProvider(TF_COND_CIV_SPEEDBUFF));
	if (!m_pSpeedEffect && m_Shared.InCond(TF_COND_CIV_SPEEDBUFF) && 
		!(IsEnemyPlayer() && (m_Shared.IsStealthed() || // don't apply effect on enemy spies if (they are stealthed) or if (they are disguised AND cond provider is enemy)
			(m_Shared.IsDisguised() && pTFCondProvider && pTFCondProvider->IsEnemyPlayer()) )))
	{
		const char* pszEffectName = "speed_boost_trail";
		m_pSpeedEffect = ParticleProp()->Create(pszEffectName, PATTACH_ABSORIGIN_FOLLOW, 0);
	}
}

void C_TFPlayer::UpdatedMarkedForDeathEffect( bool bForceStop )
{
	// Dont show the particle over the local player's head.  They have the icon that shows
	// up over their health in the HUD which serves this purpose.
	if ( IsLocalPlayer() )
		return;

	// Force stop.
	bool bShouldShow;
	if ( bForceStop || m_Shared.IsStealthed() || m_Shared.InCond( TF_COND_DISGUISED ) )
	{
		bShouldShow = false;
	}
	else
	{
		bShouldShow = m_Shared.InCond( TF_COND_MARKEDFORDEATH ) || m_Shared.InCond( TF_COND_MARKEDFORDEATH_SILENT )
			|| m_Shared.InCond(TF_COND_SUPERMARKEDFORDEATH) || m_Shared.InCond(TF_COND_SUPERMARKEDFORDEATH_SILENT);
	}

	if ( !bShouldShow )
	{
		// Stop and then go.
		RemoveOverheadEffect( "mark_for_death", true );
	}
	else
	{
		AddOverheadEffect( "mark_for_death" );
	}
}


void C_TFPlayer::OnPlayerClassChange( void )
{
	// Init the anim movement vars.
	m_PlayerAnimState->SetRunSpeed( GetPlayerClass()->GetMaxSpeed() );
	m_PlayerAnimState->SetWalkSpeed( GetPlayerClass()->GetMaxSpeed() * 0.5f );

	// Execute the class cfg.
	if ( IsLocalPlayer() )
	{
		engine->ExecuteClientCmd( VarArgs( "exec %s.cfg\n", GetPlayerClass()->GetName() ) );
	}

	if ( IsLocalPlayer() )
	{
		g_ItemEffectMeterManager.SetPlayer( this );
	}

	// Destroy nemesis icon since attachments may have changed.
	ShowNemesisIcon( false );
}


void C_TFPlayer::InitPhonemeMappings()
{
	CStudioHdr *pStudio = GetModelPtr();
	if ( pStudio )
	{
		char szBasename[MAX_PATH];
		Q_StripExtension( pStudio->pszName(), szBasename, sizeof( szBasename ) );
		char szExpressionName[MAX_PATH];
		V_sprintf_safe( szExpressionName, "%s/phonemes/phonemes", szBasename );
		if ( FindSceneFile( szExpressionName ) )
		{
			SetupMappings( szExpressionName );	
		}
		else
		{
			BaseClass::InitPhonemeMappings();
		}
	}
}


void C_TFPlayer::ResetFlexWeights( CStudioHdr *pStudioHdr )
{
	if ( !pStudioHdr || pStudioHdr->numflexdesc() == 0 )
		return;

	// Reset the flex weights to their starting position.
	LocalFlexController_t iController;
	for ( iController = LocalFlexController_t(0); iController < pStudioHdr->numflexcontrollers(); ++iController )
	{
		SetFlexWeight( iController, 0.0f );
	}

	// Reset the prediction interpolation values.
	m_iv_flexWeight.Reset();
}


CStudioHdr *C_TFPlayer::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	// Initialize the gibs.
	InitPlayerGibs();

	InitializePoseParams();

	// Init flexes, cancel any scenes we're playing
	ClearSceneEvents( NULL, false );

	// Reset the flex weights.
	ResetFlexWeights( hdr );

	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
	{
		m_PlayerAnimState->OnNewModel();
	}

	if ( hdr )
	{
		InitPhonemeMappings();
	}

	m_bUpdatePartyHat = true;

	return hdr;
}


void C_TFPlayer::UpdateWearables( void )
{
	BaseClass::UpdateWearables();

	// Update our weapon visibility as we do.
	C_TFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( pWeapon )
	{
		pWeapon->UpdateVisibility();
		pWeapon->CreateShadow();
	}

	pWeapon = m_Shared.GetDisguiseWeapon();
	if ( pWeapon )
	{
		pWeapon->UpdateVisibility();
		pWeapon->CreateShadow();
	}
}

//-----------------------------------------------------------------------------
// Purpose: GUITAR RIFFFFF
//-----------------------------------------------------------------------------
void C_TFPlayer::OnAchievementAchieved( int iAchievement )
{
	bool bOverriddenSound = false;
	CBaseAchievement *pAchievement = engine->GetAchievementMgr()->GetAchievementByID( iAchievement );

	if ( pAchievement )
	{
		switch ( pAchievement->GetType() )
		{
			case ACHIEVEMENT_TYPE_TRIVIAL:
				EmitSound( "Achievement.Earned_Trivial" );
				bOverriddenSound = true;
				break;
			case ACHIEVEMENT_TYPE_MINOR:
				EmitSound( "Achievement.Earned_Minor" );
				bOverriddenSound = true;
				break;
			case ACHIEVEMENT_TYPE_MAJOR:
				EmitSound( "Achievement.Earned_Major" );
				bOverriddenSound = true;
				break;
			case ACHIEVEMENT_TYPE_PLAYER:
				EmitSound( "Achievement.Earned_Player" );
				bOverriddenSound = true;
				break;
			case ACHIEVEMENT_TYPE_ROUND:
				EmitSound( "Achievement.Earned_Round" );
				bOverriddenSound = true;
				break;
			case ACHIEVEMENT_TYPE_MISTAKE:
				EmitSound( "Achievement.Earned_Mistake" );
				bOverriddenSound = true;
				break;
		}
	}

	if ( !bOverriddenSound )
	{
		EmitSound( "Achievement.Earned" );
	}
}


void C_TFPlayer::UpdatePartyHat( void )
{
	if ( TFGameRules() && ( TFGameRules()->IsBirthday() || TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) ) && IsAlive() && !IsPlayerClass( TF_CLASS_UNDEFINED, true ) )
	{
		if ( m_hPartyHat )
		{
			m_hPartyHat->Release();
		}

		if ( IsLocalPlayer() && !::input->CAM_IsThirdPerson() )
			return;

		// C_PlayerAttachedModel::Create can return NULL!
		const char* strModel = TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) ? SANTA_HAT_MODEL : BDAY_HAT_MODEL;
		m_hPartyHat = C_PlayerAttachedModel::Create( strModel, this, LookupAttachment( "partyhat" ), vec3_origin, PAM_PERMANENT, 0 );
		if ( m_hPartyHat )
		{
			int iVisibleTeam = GetTeamNumber();
			if ( IsDisguisedEnemy() )
			{
				iVisibleTeam = m_Shared.GetDisguiseTeam();
			}

			m_hPartyHat->m_nSkin = iVisibleTeam - 2;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: For checking how Spy's cloak and disguise effects should apply.
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsEnemyPlayer( void )
{
	// Spectators are nobody's enemy.
	int iLocalTeam = GetLocalPlayerTeam();
	if ( iLocalTeam < FIRST_GAME_TEAM )
		return false;

	// Players from other teams are enemies.
	return ( GetTeamNumber() != iLocalTeam );
}


bool C_TFPlayer::IsDisguisedEnemy( bool bTeammateDisguise /*= false*/ )
{
	return ( m_Shared.IsDisguised() &&
		IsEnemyPlayer() &&
		( !bTeammateDisguise || m_Shared.GetDisguiseTeam() == GetLocalPlayerTeam() ) );
}

//-----------------------------------------------------------------------------
// Purpose: Displays a nemesis icon on this player to the local player
//-----------------------------------------------------------------------------
void C_TFPlayer::ShowNemesisIcon( bool bShow )
{
	const char *pszEffect = ConstructTeamParticle( "particle_nemesis_%s", GetTeamNumber() );
	if ( bShow )
	{
		AddOverheadEffect( pszEffect );
	}
	else
	{
		// stop effects for both team colors (to make sure we remove effects in event of team change)
		RemoveOverheadEffect( pszEffect, true );
	}

	m_bIsDisplayingNemesisIcon = bShow;
}

extern ConVar tf_tauntcam_dist;


void C_TFPlayer::TurnOnTauntCam( void )
{
	if ( !IsLocalPlayer() )
		return;

	// Already in third person?
	if ( InThirdPersonShoulder() )
		return;

	m_flTauntOffTime = 0.0f;

	// If we're already in taunt cam just reset the distance.
	if ( m_bWasTaunting )
	{
		g_ThirdPersonManager.SetDesiredCameraOffset( Vector( tf_tauntcam_dist.GetFloat(), 0.0f, 0.0f ) );
		return;
	}

	m_bWasTaunting = true;

	::input->CAM_ToThirdPerson();
	ThirdPersonSwitch( true );
}


void C_TFPlayer::TurnOffTauntCam( void )
{
	if ( !IsLocalPlayer() )
		return;

	m_bWasTaunting = false;
	m_flTauntOffTime = 0.0f;

	::input->CAM_ToFirstPerson();

	// Force the feet to line up with the view direction post taunt.
	m_PlayerAnimState->m_bForceAimYaw = true;
}

ConVar tf_taunt_first_person( "tf_taunt_first_person", "0", FCVAR_NONE, "1 = taunts remain first-person" );


void C_TFPlayer::HandleTaunting( void )
{
	// Clear the taunt slot.
	if ( !tf_taunt_first_person.GetBool() && 
		( !m_bWasTaunting || m_flTauntOffTime != 0.0f ) && (
		m_Shared.InCond( TF_COND_TAUNTING ) ||
		m_Shared.IsLoser() ||
		m_Shared.IsControlStunned() ||
		m_nForceTauntCam || 
		m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) ||
		m_Shared.InCond( TF_COND_HALLOWEEN_GIANT ) ||
		m_Shared.InCond( TF_COND_HALLOWEEN_TINY ) ||
		m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) ) )
	{
		// Handle the camera for the local player.
		TurnOnTauntCam();
	}

	if ( m_bWasTaunting && m_flTauntOffTime == 0.0f && (
		!m_Shared.InCond( TF_COND_TAUNTING ) &&
		!m_Shared.IsLoser() &&
		!m_Shared.IsControlStunned() &&
		!m_nForceTauntCam &&
		!m_Shared.InCond( TF_COND_PHASE ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_BOMB_HEAD ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_GIANT ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_TINY ) &&
		!m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) ) )
	{
		m_flTauntOffTime = gpGlobals->curtime;

		// Clear the vcd slot.
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
	}

	TauntCamInterpolation();
}

//---------------------------------------------------------------------------- -
// Purpose:
//-----------------------------------------------------------------------------
void C_TFPlayer::TauntCamInterpolation( void )
{
	if ( m_flTauntOffTime != 0.0f )
	{
		// Pull the camera back in over the course of half a second.
		float flDist = RemapValClamped( gpGlobals->curtime - m_flTauntOffTime, 0.0f, 0.5f, tf_tauntcam_dist.GetFloat(), 0.0f );
		if ( flDist == 0.0f || !m_bWasTaunting || !IsAlive() )
		{
			// Snap the camera back into first person.
			TurnOffTauntCam();
		}
		else
		{
			g_ThirdPersonManager.SetDesiredCameraOffset( Vector( flDist, 0.0f, 0.0f ) );
		}
	}
}

#ifdef ITEM_TAUNTING

void C_TFPlayer::PlayTauntSoundLoop( const char *pszSound )
{
	if ( !m_pTauntSound )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		CLocalPlayerFilter filter;
		m_pTauntSound = controller.SoundCreate( filter, entindex(), pszSound );
		controller.Play( m_pTauntSound, 1.0f, 100.0f );
	}
}


void C_TFPlayer::StopTauntSoundLoop( void )
{
	if ( m_pTauntSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pTauntSound );
		m_pTauntSound = NULL;
	}
}
#endif

extern ConVar tf2c_thirdperson;


bool C_TFPlayer::InThirdPersonShoulder( void )
{
	if ( !TFGameRules()->AllowThirdPersonCamera() )
		return false;

	if ( IsObserver() )
		return false;

	if ( m_Shared.InCond( TF_COND_ZOOMED ) )
		return false;

	if ( InTauntCam() )
		return false;

	return tf2c_thirdperson.GetBool();
}


void C_TFPlayer::ThirdPersonSwitch( bool bThirdPerson )
{
	BaseClass::ThirdPersonSwitch( bThirdPerson );

	// Update the party hat so the player can see his own.
	UpdatePartyHat();

	// Update any effects affected by camera mode.
	m_Shared.UpdateCritBoostEffect();

	// Notify the weapon so it can update effects, etc.
	C_TFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
	if ( pActiveWeapon )
	{
		pActiveWeapon->ThirdPersonSwitch( bThirdPerson );
	}
}


bool C_TFPlayer::CanLightCigarette( void )
{
	// Start smoke if we're not invisible or disguised.
	if ( IsPlayerClass( TF_CLASS_SPY, true ) && IsAlive() &&				// Only on Spy's model.
		!IsDisguisedEnemy() &&												// Disguise doesn't show for teammates.
		GetPercentInvisible() <= 0 &&										// Don't start if invis.
		!InFirstPersonView() && 											// Don't show in first person view.
		!m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) )				// Don't show if we're a dispenser.
		return true;

	return false;
}


void C_TFPlayer::PreThink( void )
{
	BaseClass::PreThink();

	m_Shared.ConditionThink();
	m_Shared.InvisibilityThink();
}


void C_TFPlayer::PostThink( void )
{
	BaseClass::PostThink();

	m_angEyeAngles = EyeAngles();

	// The "Spy-Walk"
	if ( tf2c_spywalk.GetBool() )
	{
		if ( !IsSpywalkInverted() )
		{
			if ( m_afButtonPressed & IN_SPEED || m_afButtonReleased & IN_SPEED )
			{
				TeamFortress_SetSpeed();
			}
		}
		else
		{
			if ( !(m_afButtonPressed & IN_SPEED) || !(m_afButtonReleased & IN_SPEED) )
			{
				TeamFortress_SetSpeed();
			}
		}
	}
}


void C_TFPlayer::PhysicsSimulate( void )
{
#if !defined( NO_ENTITY_PREDICTION )
	VPROF( "C_TFPlayer::PhysicsSimulate" );
	// If we've got a moveparent, we must simulate that first.
	CBaseEntity *pMoveParent = GetMoveParent();
	if ( pMoveParent )
	{
		pMoveParent->PhysicsSimulate();
	}

	// Make sure not to simulate this guy twice per frame
	if ( m_nSimulationTick == gpGlobals->tickcount )
		return;

	m_nSimulationTick = gpGlobals->tickcount;

	if ( !IsLocalPlayer() )
		return;

	C_CommandContext *ctx = GetCommandContext();
	Assert( ctx );
	Assert( ctx->needsprocessing );
	if ( !ctx->needsprocessing )
		return;

	ctx->needsprocessing = false;
	CUserCmd *ucmd = &ctx->cmd;

	// Zero out roll on view angles, it should always be zero under normal conditions and hacking it messes up movement (speedhacks).
	ucmd->viewangles[ROLL] = 0.0f;

	// Handle FL_FROZEN.
	if ( GetFlags() & FL_FROZEN )
	{
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
		ucmd->upmove = 0;
		ucmd->buttons = 0;
		ucmd->impulse = 0;
		//VectorCopy ( pl.v_angle, ucmd->viewangles );
	}
	else if ( m_Shared.IsMovementLocked() )
	{
		// Don't allow player to perform any actions while taunting or stunned.
		// Not preventing movement since some taunts have special movement which needs to be handled in CTFGameMovement.
		// This is duplicated on server side in CTFPlayer::RunPlayerCommand.
		int nRemoveButtons = ucmd->buttons;

		// Allow IN_ATTACK2 if player owns sticky launcher. This is done so they can detonate stickies while taunting.
		if ( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) && tf2c_taunting_detonate_stickies.GetBool() )
		{
			nRemoveButtons &= ~IN_ATTACK2;
		}

		ucmd->buttons &= ~nRemoveButtons;
		ucmd->weaponselect = 0;
		ucmd->weaponsubtype = 0;

		// Don't allow the player to turn around.
		VectorCopy( EyeAngles(), ucmd->viewangles );
	}

	// Run the next command.
	prediction->RunCommand( this, ucmd, MoveHelper() );
#endif
}


void C_TFPlayer::ClientThink()
{
	// Pass on through to the base class.
	BaseClass::ClientThink();

	UpdateIDTarget();

	UpdateLookAt();

	// If we stopped taunting but the animation is still active then kill it.
	if ( !m_Shared.InCond( TF_COND_TAUNTING ) && m_PlayerAnimState->IsGestureSlotActive( GESTURE_SLOT_VCD ) )
	{
		m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
	}

	// Clear our healer, it'll be reset by the medigun client think if we're being healed
	m_hHealer = NULL;

	// Find other healers. Implemented to catch Heal Launcher (Nurnberg Nader). May need more filtering
	if ( m_Shared.InCond( TF_COND_HEALTH_BUFF ) )
	{
		C_TFPlayer* pTFCondProvider = ToTFPlayer( m_Shared.GetConditionProvider( TF_COND_HEALTH_BUFF ) );
		if ( pTFCondProvider )
		{
			// We don't need to see the names of Dispensers since we're next to them anyway
			// and can see the builder name.
			// We don't need to see our own name.
			if ( !pTFCondProvider->IsBaseObject() && pTFCondProvider != this )
			{
				m_hHealer = pTFCondProvider;
			}
		}
	}

	if ( CanLightCigarette() )
	{
		if ( !m_pCigaretteSmoke )
		{
			m_pCigaretteSmoke = ParticleProp()->Create( "cig_smoke", PATTACH_POINT_FOLLOW, "cig_smoke" );
		}
	}
	else	// stop the smoke otherwise if its active
	{
		if ( m_pCigaretteSmoke )
		{
			ParticleProp()->StopEmission( m_pCigaretteSmoke );
			m_pCigaretteSmoke = NULL;
		}
	}

	if ( m_bWaterExitEffectActive && !IsAlive() )
	{
		ParticleProp()->StopParticlesNamed( "water_playeremerge", false );
		m_bWaterExitEffectActive = false;
	}

	if ( m_bUpdatePartyHat )
	{
		UpdatePartyHat();
		m_bUpdatePartyHat = false;
	}

	if ( m_bUpdateAttachedModels )
	{
		UpdateSpyMask();
		m_bUpdateAttachedModels = false;
	}

	if ( m_pBlastJumpLoop )
	{
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		if ( !IsAlive() )
		{
			controller.SoundDestroy( m_pBlastJumpLoop );
			m_pBlastJumpLoop = NULL;
		}
		else
		{
			float flTimeAloft = gpGlobals->curtime - m_flBlastJumpLaunchTime;
			controller.SoundChangePitch( m_pBlastJumpLoop, RemapValClamped( flTimeAloft, 0.1f, 3.f, 200.f, 100.f ), 0.1f );
			controller.SoundChangeVolume( m_pBlastJumpLoop, RemapValClamped( flTimeAloft, 0.1f, 2.f, 0.25f, 0.95f ), 0.1f );
		}
	}

	UpdatedMarkedForDeathEffect();

	if ( m_pSaveMeEffect )
	{
		// Kill the effect if either
		// a) the player is dead
		// b) the enemy disguised spy is now invisible
		if ( !IsAlive() || ( IsDisguisedEnemy() && GetPercentInvisible() > 0 ) )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pSaveMeEffect );
			m_pSaveMeEffect = NULL;
		}
	}

	if( IsAlive() && GetActiveTFWeapon() && GetActiveTFWeapon()->IsWeapon( TF_WEAPON_RIOT_SHIELD ) )
		static_cast<CTFRiot *>(GetActiveTFWeapon())->DrawShieldWireframe( m_nActiveWpnAmmo );

	if ( IsLocalPlayer() )
	{
		g_ItemEffectMeterManager.Update( this );
	}
}


void C_TFPlayer::UpdateLookAt( void )
{
	bool bFoundViewTarget = false;

	Vector vForward;
	AngleVectors( GetLocalAngles(), &vForward );

	Vector vMyOrigin =  GetAbsOrigin();

	Vector vecLookAtTarget = vec3_origin;

	for ( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
		if ( !pEnt || !pEnt->IsPlayer() )
			continue;

		if ( !pEnt->IsAlive() )
			continue;

		if ( pEnt == this )
			continue;

		Vector vDir = pEnt->GetAbsOrigin() - vMyOrigin;
		if ( vDir.Length() > 300 ) 
			continue;

		VectorNormalize( vDir );
		if ( DotProduct( vForward, vDir ) < 0.0f )
			continue;

		vecLookAtTarget = pEnt->EyePosition();
		bFoundViewTarget = true;
		break;
	}

	if ( !bFoundViewTarget )
	{
		// No target, look forward.
		vecLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	// Orient eyes.
	m_viewtarget = vecLookAtTarget;

	// Blinking.
	if ( m_blinkTimer.IsElapsed() )
	{
		m_blinktoggle = !m_blinktoggle;
		m_blinkTimer.Start( RandomFloat( 1.5f, 4.0f ) );
	}

	/*
	// Figure out where we want to look in world space.
	QAngle desiredAngles;
	Vector to = vecLookAtTarget - EyePosition();
	VectorAngles( to, desiredAngles );

	// Figure out where our body is facing in world space.
	QAngle bodyAngles( 0, 0, 0 );
	bodyAngles[YAW] = GetLocalAngles()[YAW];

	float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	m_flLastBodyYaw = bodyAngles[YAW];

	// Set the head's yaw.
	float desired = AngleNormalize( desiredAngles[YAW] - bodyAngles[YAW] );
	desired = clamp( -desired, m_headYawMin, m_headYawMax );
	m_flCurrentHeadYaw = ApproachAngle( desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime );

	// Counterrotate the head from the body rotation so it doesn't rotate past its target.
	m_flCurrentHeadYaw = AngleNormalize( m_flCurrentHeadYaw - flBodyYawDiff );

	SetPoseParameter( m_headYawPoseParam, m_flCurrentHeadYaw );

	// Set the head's yaw.
	desired = AngleNormalize( desiredAngles[PITCH] );
	desired = clamp( desired, m_headPitchMin, m_headPitchMax );

	m_flCurrentHeadPitch = ApproachAngle( -desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime );
	m_flCurrentHeadPitch = AngleNormalize( m_flCurrentHeadPitch );
	SetPoseParameter( m_headPitchPoseParam, m_flCurrentHeadPitch );
	*/
}


//-----------------------------------------------------------------------------
// Purpose: Try to steer away from any players and objects we might interpenetrate
//-----------------------------------------------------------------------------
#define TF_AVOID_MAX_RADIUS_SQR		5184.0f			// Based on player extents and max buildable extents.
#define TF_OO_AVOID_MAX_RADIUS_SQR	0.00019f

ConVar tf_max_separation_force ( "tf_max_separation_force", "256", FCVAR_DEVELOPMENTONLY );

extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

void C_TFPlayer::AvoidPlayers( CUserCmd *pCmd )
{
	// Turn off the avoid player code.
	if ( !tf_avoidteammates.GetBool() || !tf_avoidteammates_pushaway.GetBool() )
		return;

	// Don't test if the player doesn't exist or is dead.
	if ( !IsAlive() )
		return;

	C_Team *pTeam = (C_Team *)GetTeam();
	if ( !pTeam )
		return;

	// Up vector.
	static Vector vecUp( 0.0f, 0.0f, 1.0f );

	Vector vecTFPlayerCenter = GetAbsOrigin();
	Vector vecTFPlayerMin = GetPlayerMins();
	Vector vecTFPlayerMax = GetPlayerMaxs();
	float flZHeight = vecTFPlayerMax.z - vecTFPlayerMin.z;
	vecTFPlayerCenter.z += 0.5f * flZHeight;
	VectorAdd( vecTFPlayerMin, vecTFPlayerCenter, vecTFPlayerMin );
	VectorAdd( vecTFPlayerMax, vecTFPlayerCenter, vecTFPlayerMax );

	// Find an intersecting player or object.
	C_TFPlayer *pIntersectPlayer = NULL;
	CBaseObject *pIntersectObject = NULL;
	float flAvoidRadius = 0.0f;

	// Bots let human players through teleporters if they stand on them.
	// So don't get pushed away by bots if we're standing on a tele.
	bool bStandingOnTele = false;

	if ( !IsPlayerClass( TF_CLASS_SCOUT, true ) && GetGroundEntity() && GetGroundEntity()->IsBaseObject() )
	{
		C_BaseObject *pObject = static_cast<C_BaseObject *>( GetGroundEntity() );
		bStandingOnTele = ( pObject->GetType() == OBJ_TELEPORTER && pObject->GetObjectMode() == TELEPORTER_TYPE_ENTRANCE );
	}

	int i;
	Vector vecAvoidCenter, vecAvoidMin, vecAvoidMax;
	for ( i = 0; i < pTeam->GetNumPlayers(); ++i )
	{
		C_TFPlayer *pAvoidPlayer = static_cast< C_TFPlayer * >( pTeam->GetPlayer( i ) );
		if ( pAvoidPlayer == NULL )
			continue;

		// Is the avoid player me?
		if ( pAvoidPlayer == this )
			continue;

		// Check to see if the avoid player is dormant.
		if ( pAvoidPlayer->IsDormant() )
			continue;

		// Is the avoid player solid?
		if ( pAvoidPlayer->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
			continue;

		if ( bStandingOnTele && pAvoidPlayer->IsNextBot() )
			continue;

		Vector t1, t2;

		vecAvoidCenter = pAvoidPlayer->GetAbsOrigin();
		vecAvoidMin = pAvoidPlayer->GetPlayerMins();
		vecAvoidMax = pAvoidPlayer->GetPlayerMaxs();
		flZHeight = vecAvoidMax.z - vecAvoidMin.z;
		vecAvoidCenter.z += 0.5f * flZHeight;
		VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
		VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

		if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
		{
			// Need to avoid this player.
			if ( !pIntersectPlayer )
			{
				pIntersectPlayer = pAvoidPlayer;
				break;
			}
		}
	}

	// We didn't find a player - look for objects to avoid.
	if ( !pIntersectPlayer )
	{
		C_TFTeam *pTeam = (C_TFTeam *)GetTeam();
		for ( int iObject = 0; iObject < pTeam->GetNumObjects(); ++iObject )
		{
			CBaseObject *pAvoidObject = pTeam->GetObject( iObject );
			if ( !pAvoidObject )
				continue;

			// Check to see if the object is dormant.
			if ( pAvoidObject->IsDormant() )
				continue;

			// Is the object solid.
			if ( pAvoidObject->IsSolidFlagSet( FSOLID_NOT_SOLID ) )
				continue;

			// If we shouldn't avoid it, see if we intersect it.
			if ( pAvoidObject->ShouldPlayersAvoid() )
			{
				vecAvoidCenter = pAvoidObject->WorldSpaceCenter();
				vecAvoidMin = pAvoidObject->WorldAlignMins();
				vecAvoidMax = pAvoidObject->WorldAlignMaxs();
				VectorAdd( vecAvoidMin, vecAvoidCenter, vecAvoidMin );
				VectorAdd( vecAvoidMax, vecAvoidCenter, vecAvoidMax );

				if ( IsBoxIntersectingBox( vecTFPlayerMin, vecTFPlayerMax, vecAvoidMin, vecAvoidMax ) )
				{
					// Need to avoid this object.
					pIntersectObject = pAvoidObject;
					break;
				}
			}
		}
	}

	// Anything to avoid?
	if ( !pIntersectPlayer && !pIntersectObject )
		return;

	// Calculate the push strength and direction.
	Vector vecDelta;

	// Avoid a player - they have precedence.
	if ( pIntersectPlayer )
	{
		VectorSubtract( pIntersectPlayer->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectPlayer->WorldAlignMaxs() - pIntersectPlayer->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}
	// Avoid a object.
	else
	{
		VectorSubtract( pIntersectObject->WorldSpaceCenter(), vecTFPlayerCenter, vecDelta );

		Vector vRad = pIntersectObject->WorldAlignMaxs() - pIntersectObject->WorldAlignMins();
		vRad.z = 0;

		flAvoidRadius = vRad.Length();
	}

	float flPushStrength = RemapValClamped( vecDelta.Length(), flAvoidRadius, 0, 0, tf_max_separation_force.GetInt() ); //flPushScale;

	//Msg( "PushScale = %f\n", flPushStrength );

	// Check to see if we have enough push strength to make a difference.
	if ( flPushStrength < 0.01f )
		return;

	Vector vecPush;
	if ( GetAbsVelocity().Length2DSqr() > 0.1f )
	{
		Vector vecVelocity = GetAbsVelocity();
		vecVelocity.z = 0.0f;
		CrossProduct( vecUp, vecVelocity, vecPush );
		VectorNormalize( vecPush );
	}
	else
	{
		// We are not moving, but we're still intersecting.
		QAngle angView = pCmd->viewangles;
		angView.x = 0.0f;
		AngleVectors( angView, NULL, &vecPush, NULL );
	}

	// Move away from the other player/object.
	Vector vecSeparationVelocity;
	if ( vecDelta.Dot( vecPush ) < 0 )
	{
		vecSeparationVelocity = vecPush * flPushStrength;
	}
	else
	{
		vecSeparationVelocity = vecPush * -flPushStrength;
	}

	// Don't allow the max push speed to be greater than the max player speed.
	float flCropFraction = 1.33333333f;
	float flMaxPlayerSpeed = MaxSpeed();
	if ( ( GetFlags() & FL_DUCKING ) && GetGroundEntity() )
	{	
		flMaxPlayerSpeed *= flCropFraction;
	}	

	if ( vecSeparationVelocity.LengthSqr() > ( flMaxPlayerSpeed * flMaxPlayerSpeed ) )
	{
		vecSeparationVelocity.NormalizeInPlace();
		VectorScale( vecSeparationVelocity, flMaxPlayerSpeed, vecSeparationVelocity );
	}

	QAngle vAngles = pCmd->viewangles;
	vAngles.x = 0;
	Vector currentdir;
	Vector rightdir;

	AngleVectors( vAngles, &currentdir, &rightdir, NULL );

	Vector vDirection = vecSeparationVelocity;

	VectorNormalize( vDirection );

	float fwd = currentdir.Dot( vDirection );
	float rt = rightdir.Dot( vDirection );

	float forward = fwd * flPushStrength;
	float side = rt * flPushStrength;

	//Msg( "fwd: %f - rt: %f - forward: %f - side: %f\n", fwd, rt, forward, side );

	pCmd->forwardmove	+= forward;
	pCmd->sidemove		+= side;

	// Clamp the move to within legal limits, preserving direction. This is a little
	// complicated because we have different limits for forward, back, and side

	//Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

	float flForwardScale = 1.0f;
	if ( pCmd->forwardmove > fabs( cl_forwardspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_forwardspeed.GetFloat() ) / pCmd->forwardmove;
	}
	else if ( pCmd->forwardmove < -fabs( cl_backspeed.GetFloat() ) )
	{
		flForwardScale = fabs( cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove );
	}

	float flSideScale = 1.0f;
	if ( fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat() ) )
	{
		flSideScale = fabs( cl_sidespeed.GetFloat() ) / fabs( pCmd->sidemove );
	}

	float flScale = Min( flForwardScale, flSideScale );
	pCmd->forwardmove *= flScale;
	pCmd->sidemove *= flScale;

	//Msg( "Pforwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInputSampleTime - 
//			*pCmd - 
//-----------------------------------------------------------------------------
bool C_TFPlayer::CreateMove( float flInputSampleTime, CUserCmd *pCmd )
{	
	// HACK: We're using an unused bit in buttons var to set the typing status based on whether player's chat panel is open.
	if ( GetTFChatHud() && GetTFChatHud()->GetMessageMode() != MM_NONE )
	{
		pCmd->buttons |= IN_TYPING;
	}

	if ( InThirdPersonShoulder() )
	{
		pCmd->buttons |= IN_THIRDPERSON;
	}

	BaseClass::CreateMove( flInputSampleTime, pCmd );

	AvoidPlayers( pCmd );

#if 0
	// If player is taunting and in first person lock the camera angles.
	if ( m_Shared.IsMovementLocked() && InFirstPersonView() )
	{
		pCmd->viewangles = EyeAngles();
	}
#endif

	return true;
}


void C_TFPlayer::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() && !prediction->IsFirstTimePredicted() )
		return;

	MDLCACHE_CRITICAL_SECTION();
	m_PlayerAnimState->DoAnimationEvent( event, nData );
}


Vector C_TFPlayer::GetObserverCamOrigin( void )
{
	if ( !IsAlive() )
	{
		if ( m_hFirstGib )
		{
			IPhysicsObject *pPhysicsObject = m_hFirstGib->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				Vector vecMassCenter = pPhysicsObject->GetMassCenterLocalSpace();
				Vector vecWorld;
				m_hFirstGib->CollisionProp()->CollisionToWorldSpace( vecMassCenter, &vecWorld );
				return vecWorld;
			}
			return m_hFirstGib->GetRenderOrigin();
		}

		IRagdoll *pRagdoll = GetRepresentativeRagdoll();
		if ( pRagdoll )
			return pRagdoll->GetRagdollOrigin();
	}

	return BaseClass::GetObserverCamOrigin();	
}

//-----------------------------------------------------------------------------
// Purpose: Consider the viewer and other factors when determining resulting
// invisibility
//-----------------------------------------------------------------------------
float C_TFPlayer::GetEffectiveInvisibilityLevel( void )
{
	float flPercentInvisible = GetPercentInvisible();

	// If this is a teammate of the local player or viewer is observer,
	// dont go above a certain max invis.
	if ( !IsEnemyPlayer() )
	{
		float flMax = tf_teammate_max_invis.GetFloat();
		if ( flPercentInvisible > flMax )
		{
			flPercentInvisible = flMax;
		}
	}
	else
	{
		// If this player just killed me, show them slightly
		// less than full invis in the deathcam and freezecam.
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			int iObserverMode = pLocalPlayer->GetObserverMode();
			if ( ( iObserverMode == OBS_MODE_FREEZECAM || iObserverMode == OBS_MODE_DEATHCAM ) && 
				pLocalPlayer->GetObserverTarget() == this )
			{
				float flMax = tf_teammate_max_invis.GetFloat();
				if ( flPercentInvisible > flMax )
				{
					flPercentInvisible = flMax;
				}
			}
		}
	}

	return flPercentInvisible;
}


int C_TFPlayer::DrawModel( int flags )
{
	// If we're a dead player with a fresh ragdoll, don't draw
	if ( m_nRenderFX == kRenderFxRagdoll )
		return 0;

	// Don't draw the model at all if we're fully invisible
	if ( GetEffectiveInvisibilityLevel() >= 1.0f )
	{
		if ( m_hPartyHat && g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 90 && !m_hPartyHat->IsEffectActive( EF_NODRAW ) )
		{
			m_hPartyHat->SetEffects( EF_NODRAW );
		}
		return 0;
	}
	else
	{
		if ( m_hPartyHat && g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 90 && m_hPartyHat->IsEffectActive( EF_NODRAW ) )
		{
			m_hPartyHat->RemoveEffects( EF_NODRAW );
		}
	}

	m_Shared.RecalculatePlayerBodygroups();

	bool bRet = BaseClass::DrawModel( flags );

	m_Shared.RestorePlayerBodygroups();

	if (tf2c_debug_player_brightness.GetBool())
	{
		Vector worldLight = engine->GetLightForPoint(WorldSpaceCenter(), true);
		Vector    tint;
		float    luminosity;
		if (worldLight == vec3_origin)
		{
			tint = vec3_origin;
			luminosity = 0.0f;
		}
		else
		{
			UTIL_GetNormalizedColorTintAndLuminosity(worldLight, &tint, &luminosity);
		}

		luminosity *= 255.0;

		CFmtStr str("luminosity %.1f", luminosity);
		NDebugOverlay::EntityTextAtPosition(WorldSpaceCenter(), 0, str.Access(), 0, 255, 255, 255, 255);
		NDebugOverlay::Line(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, luminosity*0.25), 255, 255, 255, true, 0);
	}

	return bRet;
}


bool C_TFPlayer::OnInternalDrawModel( ClientModelRenderInfo_t *pInfo )
{
	// Consistent lighting origin for all players.
	pInfo->pLightingOrigin = &WorldSpaceCenter();

	return BaseClass::OnInternalDrawModel( pInfo );
}


void C_TFPlayer::ProcessMuzzleFlashEvent()
{
	CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	bool bInToolRecordingMode = ToolsEnabled() && clienttools->IsInRecordingMode();

	// Don't show the world muzzle flash to the local player.
	if ( pLocalPlayer == this && !bInToolRecordingMode )
		return;

	if ( pLocalPlayer && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
	{
		// Also, don't show when observing other players in first person mode.
		if ( pLocalPlayer->GetObserverTarget() == this )
			return;
	}

	C_TFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
	if ( !pActiveWeapon )
		return;

	pActiveWeapon->ProcessMuzzleFlashEvent();
}


int C_TFPlayer::GetIDTarget() const
{
	return m_iIDEntIndex;
}


void C_TFPlayer::SetForcedIDTarget( int iTarget )
{
	m_iForcedIDTarget = iTarget;
}

//-----------------------------------------------------------------------------
// Purpose: Update this client's targetid entity
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateIDTarget()
{
	if ( !IsLocalPlayer() )
		return;

	// Don't show IDs if mp_fadetoblack is on.
	if ( GetTeamNumber() > TEAM_SPECTATOR && mp_fadetoblack.GetBool() && !IsAlive() )
	{
		m_iIDEntIndex = 0;
		return;
	}

	if ( m_iForcedIDTarget )
	{
		m_iIDEntIndex = m_iForcedIDTarget;
		return;
	}

	// If we're spectating someone, then ID them.
	/*int iObserverMode = GetObserverMode();
	if ( iObserverMode == OBS_MODE_DEATHCAM || 
		iObserverMode == OBS_MODE_IN_EYE || 
		iObserverMode == OBS_MODE_CHASE )
	{
		C_BaseEntity *pObserverTarget = GetObserverTarget();
		if ( pObserverTarget && pObserverTarget != this )
		{
			m_iIDEntIndex = pObserverTarget->entindex();
			return;
		}
	}*/

	// Clear old target and find a new one.
	m_iIDEntIndex = 0;

	trace_t tr;
	Vector vecOrigin, vecForward, vecStart, vecEnd;
	vecOrigin = MainViewOrigin();
	vecForward = MainViewForward();

	VectorMA( vecOrigin, MAX_TRACE_LENGTH, vecForward, vecEnd );
	VectorMA( vecOrigin, 10, vecForward, vecStart );

	// If we're in observer mode, ignore our observer target. Otherwise, ignore ourselves.
	int iObserverMode = GetObserverMode();
	bool bObserver = ( iObserverMode == OBS_MODE_DEATHCAM || iObserverMode == OBS_MODE_IN_EYE || iObserverMode == OBS_MODE_CHASE );
	if ( bObserver )
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, GetObserverTarget(), COLLISION_GROUP_NONE, &tr );
	}
	else
	{
		UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	}

	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		C_BaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity && pEntity != this && ( !bObserver || pEntity != GetObserverTarget() ) )
		{
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}


void C_TFPlayer::GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes, bool &bShowingAmmo )
{
	sDataString[0] = '\0';
	bShowingAmmo = false;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;
	
	bool bDisguisedEnemy = IsDisguisedEnemy();

	if ( m_Shared.IsDisguised() && ( !IsEnemyPlayer() || m_Shared.GetDisguiseClass() == TF_CLASS_SPY ) )
	{
		// Use team-specific strings for Four-Team Mode.
		const char *szDisguiseTeam;
		if ( TFGameRules()->IsFourTeamGame() )
		{
			switch ( m_Shared.GetTrueDisguiseTeam() )
			{
				case TF_TEAM_RED:
					szDisguiseTeam = "#TF_RedTeam_Name";
					break;
				case TF_TEAM_BLUE:
					szDisguiseTeam = "#TF_BlueTeam_Name";
					break;
				case TF_TEAM_GREEN:
					szDisguiseTeam = "#TF_GreenTeam_Name";
					break;
				case TF_TEAM_YELLOW:
					szDisguiseTeam = "#TF_YellowTeam_Name";
					break;
				default:
					szDisguiseTeam = "#TF_enemy";
					break;
			}
		}
		else
		{
			szDisguiseTeam = "#TF_enemy";
		}

		// The target is a disguised friendly spy or enemy spy disguised as a spy.
		// Add the disguise team & class to the target ID element.
		bool bDisguisedAsEnemy = m_Shared.GetTrueDisguiseTeam() == TF_TEAM_GLOBAL || (m_Shared.GetDisguiseTeam() != pLocalPlayer->GetTeamNumber());
		const wchar_t *wszAlignment = g_pVGuiLocalize->Find( bDisguisedAsEnemy || bDisguisedEnemy ? szDisguiseTeam : "#TF_friendly" );

		int classindex = bDisguisedEnemy ? m_Shared.GetMaskClass() : m_Shared.GetDisguiseClass();
		const wchar_t *wszClassName = g_pVGuiLocalize->Find( g_aPlayerClassNames[classindex] );

		// Build a string with disguise information.
		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_friendlyspy_disguise" ),
			2, wszAlignment, wszClassName );
	}
	else if ( IsPlayerClass( TF_CLASS_MEDIC ) || ( bDisguisedEnemy && m_Shared.GetDisguiseClass() == TF_CLASS_MEDIC ) )
	{
		wchar_t wszChargeLevel[10];
		V_swprintf_safe( wszChargeLevel, L"%.0f", MedicGetChargeLevel() * 100.0f );

		ITFHealingWeapon *pHealingWep = GetMedigun();
		if ( pHealingWep ) {
			C_TFWeaponBase *pWeapon = GetMedigun()->GetWeapon();

			if ( pWeapon ) {
				// If they're using non-stock Medigun, show its name.
				CEconItemDefinition *pItemDef = pWeapon ? pWeapon->GetItem()->GetStaticData() : NULL;
				if ( pItemDef && !pItemDef->baseitem )
				{
					g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_mediccharge_wpn" ), 2,
						wszChargeLevel, pItemDef->GenerateLocalizedFullItemName() );
				}
				else
				{
					g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_mediccharge" ), 1,
						wszChargeLevel );
				}
			}
		}
	}
	else if ( pLocalPlayer->MedicGetHealTarget() == this )
	{
		// If this is a disguised enemy spy then pretend they have full ammo for their disguise weapon.
		int iClip = -1, iAmmo = -1;

		if ( bDisguisedEnemy )
		{
			iClip = m_Shared.GetDisguiseAmmoClip();
			iAmmo = m_Shared.GetDisguiseAmmoCount();
		}
		else
		{
			iClip = m_nActiveWpnClip;
			iAmmo = m_nActiveWpnAmmo;
		}

		if ( iAmmo != -1 )
		{
			wchar_t wszAmmo[32];
			if ( iClip != WEAPON_NOCLIP )
			{
				V_swprintf_safe( wszAmmo, L"%2d / %d", iClip, iAmmo );
			}
			else
			{
				V_swprintf_safe( wszAmmo, L"%d", iAmmo );
			}

			g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_ammo" ), 1, wszAmmo );
			bShowingAmmo = true;
		}
	}
	else if ( IsPlayerClass( TF_CLASS_CIVILIAN ) || ( bDisguisedEnemy && m_Shared.GetDisguiseClass() == TF_CLASS_CIVILIAN ) )
	{
		wchar_t wszChargeLevel[10];
		if ( IsPlayerClass( TF_CLASS_SPY, true ) && m_Shared.IsDisguised() )
		{
			// For now, say that our boost is always ready.
			V_swprintf_safe( wszChargeLevel, L"%.0f", 100.0f );
		}
		else
		{
			C_TFUmbrella *pUmbrella = assert_cast<C_TFUmbrella *>( Weapon_OwnsThisID( TF_WEAPON_UMBRELLA ) );
			V_swprintf_safe( wszChargeLevel, L"%.0f", ( pUmbrella ? pUmbrella->GetProgress() : 0.0f ) * 100.0f );
		}

		g_pVGuiLocalize->ConstructString( sDataString, iMaxLenInBytes, g_pVGuiLocalize->Find( "#TF_playerid_civiliancharge" ), 1,
			wszChargeLevel );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Display appropriate hints for the target we're looking at.
//-----------------------------------------------------------------------------
void C_TFPlayer::DisplaysHintsForTarget( C_BaseEntity *pTarget )
{
	// If the entity provides hints, ask them if they have one for this player.
	ITargetIDProvidesHint *pHintInterface = dynamic_cast<ITargetIDProvidesHint *>( pTarget );
	if ( pHintInterface )
	{
		pHintInterface->DisplayHintTo( this );
	}
}


int C_TFPlayer::GetRenderTeamNumber( void )
{
	return m_nSkin;
}

static Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
static Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );


void C_TFPlayer::CalcDeathCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	CBaseEntity	*pKiller = GetObserverTarget();

	// Swing to face our killer within half the death anim time
	float interpolation = ( gpGlobals->curtime - m_flDeathTime ) / ( TF_DEATH_ANIMATION_TIME * 0.5f );
	interpolation = clamp( interpolation, 0.0f, 1.0f );
	interpolation = SimpleSpline( interpolation );

	m_flObserverChaseDistance += gpGlobals->frametime * 48.0f;
	m_flObserverChaseDistance = clamp( m_flObserverChaseDistance, CHASE_CAM_DISTANCE_MIN, CHASE_CAM_DISTANCE_MAX );

	QAngle aForward = eyeAngles = EyeAngles();
	Vector origin = EyePosition();

	C_TFRagdoll *pRagdollEnt = static_cast<C_TFRagdoll *>( m_hRagdoll.Get() );
	if ( pRagdollEnt )
	{
		if ( pRagdollEnt->IsPlayingDeathAnim() )
		{
			// Bring the camera up if playing death animation.
			origin.z += VEC_DEAD_VIEWHEIGHT.z * 4.0f;
		}
		else
		{
			IRagdoll *pRagdoll = pRagdollEnt->GetIRagdoll();
			if ( pRagdoll )
			{
				origin = pRagdoll->GetRagdollOrigin();
				origin.z += VEC_DEAD_VIEWHEIGHT.z; // Look over ragdoll, not through.

				if ( cl_fp_ragdoll.GetBool() )
				{
					// Set origin and lerp angles to eyes attachment.
					QAngle aEyes;
					pRagdollEnt->GetAttachment( pRagdollEnt->LookupAttachment( "eyes" ), eyeOrigin, aEyes );
					InterpolateAngles( aForward, aEyes, eyeAngles, interpolation );

					// DM: Don't use first person view when we are very close to something.
					Vector vForward;
					AngleVectors( eyeAngles, &vForward );
					trace_t tr;
					UTIL_TraceLine( eyeOrigin, eyeOrigin + ( vForward * 128 ), MASK_ALL, pRagdollEnt, COLLISION_GROUP_NONE, &tr );
					if ( tr.fraction >= 1.0f || tr.endpos.DistTo( eyeOrigin ) > 24 )
						return;
				}
			}
		}
	}

	if ( pKiller && pKiller != this )
	{
		Vector vKiller = pKiller->EyePosition() - origin;
		QAngle aKiller; VectorAngles( vKiller, aKiller );
		InterpolateAngles( aForward, aKiller, eyeAngles, interpolation );
	}

	Vector vForward; AngleVectors( eyeAngles, &vForward );
	VectorNormalize( vForward );
	VectorMA( origin, -m_flObserverChaseDistance, vForward, eyeOrigin );

	trace_t trace; // Clip against world.
	C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK: Don't recompute positions while doing the trace!
	UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID, this, COLLISION_GROUP_NONE, &trace );
	C_BaseEntity::PopEnableAbsRecomputations();

	if ( trace.fraction < 1.0f )
	{
		eyeOrigin = trace.endpos;
		m_flObserverChaseDistance = VectorLength( origin - eyeOrigin );
	}

	fov = GetFOV();
}

//-----------------------------------------------------------------------------
// Purpose: Do nothing multiplayer_animstate takes care of animation.
// Input  : playerAnim - 
//-----------------------------------------------------------------------------
void C_TFPlayer::SetAnimation( PLAYER_ANIM playerAnim )
{
}


float C_TFPlayer::GetMinFOV() const
{
	// Min FOV for Sniper Rifle.
	return 20;
}


const QAngle &C_TFPlayer::EyeAngles()
{
	// We cannot use the local camera angles when taunting since player cannot turn.
	if ( IsLocalPlayer() && !m_Shared.IsMovementLocked() )
		return BaseClass::EyeAngles();
	
	return m_angEyeAngles;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bCopyEntity - 
// Output : C_BaseAnimating *
//-----------------------------------------------------------------------------
C_BaseAnimating *C_TFPlayer::BecomeRagdollOnClient()
{
	// Let the C_TFRagdoll take care of this.
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
// Output : IRagdoll*
//-----------------------------------------------------------------------------
IRagdoll *C_TFPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_TFRagdoll *pRagdoll = static_cast<C_TFRagdoll *>( m_hRagdoll.Get() );
		if ( !pRagdoll )
			return NULL;

		return pRagdoll->GetIRagdoll();
	}
	
	return NULL;
}


void C_TFPlayer::InitPlayerGibs( void )
{
	// Clear out the gib list and create a new one.
	m_aGibs.Purge();
	BuildGibList( m_aGibs, GetPlayerClass()->HasCustomModel() ? modelinfo->GetModelIndex( GetPlayerClass()->GetModelName() ) : GetModelIndex(), 1.0f, COLLISION_GROUP_NONE );

	if ( TFGameRules() && TFGameRules()->IsBirthday() )
	{
		for ( int i = 0; i < m_aGibs.Count(); i++ )
		{
			if ( RandomFloat( 0.0f, 1.0f ) < 0.75f )
			{
				V_strcpy_safe( m_aGibs[i].modelName, g_pszBDayGibs[RandomInt( 0, ARRAYSIZE( g_pszBDayGibs ) - 1 )] );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecOrigin - 
//			&vecVelocity - 
//			&vecImpactVelocity - 
//-----------------------------------------------------------------------------
void C_TFPlayer::CreatePlayerGibs( const Vector &vecOrigin, const Vector &vecVelocity, float flImpactScale, bool bBurning, int iMaxGibs )
{
	// Make sure we have Gibs to create.
	if ( m_aGibs.Count() == 0 )
		return;

	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0f );

	Vector vecBreakVelocity = vecVelocity;
	vecBreakVelocity.z += tf_playergib_forceup.GetFloat();
	VectorNormalize( vecBreakVelocity );
	vecBreakVelocity *= tf_playergib_force.GetFloat();

	// Cap the impulse.
	float flSpeed = vecBreakVelocity.Length();
	if ( flSpeed > tf_playergib_maxspeed.GetFloat() )
	{
		VectorScale( vecBreakVelocity, tf_playergib_maxspeed.GetFloat() / flSpeed, vecBreakVelocity );
	}

	breakablepropparams_t breakParams( vecOrigin, GetRenderAngles(), vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;

	// Break up the player.
	m_hSpawnedGibs.Purge();
	m_hFirstGib = CreateGibsFromList( m_aGibs, GetPlayerClass()->HasCustomModel() ? modelinfo->GetModelIndex( GetPlayerClass()->GetModelName() ) : GetModelIndex(), NULL, breakParams, this, iMaxGibs, false, true, &m_hSpawnedGibs, bBurning );

	if ( g_TF_PR )
	{
		// Gib skin numbers don't match player skin numbers so we gotta fix it up here.
		for ( int i = 0; i < m_hSpawnedGibs.Count(); i++ )
		{
			C_BaseAnimating *pGib = static_cast<C_BaseAnimating *>( m_hSpawnedGibs[i].Get() );
			pGib->m_nSkin = GetTeamSkin( g_TF_PR->GetTeam( entindex() ) );
		}
	}

	DropPartyHat( breakParams, vecBreakVelocity );
}


void C_TFPlayer::DropHelmet( breakablepropparams_t &breakParams, Vector &vecBreakVelocity )
{
	breakmodel_t breakModel;
	V_strcpy_safe( breakModel.modelName, SOLDIER_HELM_MODEL );
	breakModel.health = 1;
	breakModel.fadeTime = RandomFloat( 5.0f, 10.0f );
	breakModel.fadeMinDist = 0.0f;
	breakModel.fadeMaxDist = 0.0f;
	breakModel.burstScale = breakParams.defBurstScale;
	breakModel.collisionGroup = COLLISION_GROUP_DEBRIS;
	breakModel.isRagdoll = false;
	breakModel.isMotionDisabled = false;
	breakModel.placementName[0] = 0;
	breakModel.placementIsBone = false;
	BreakModelCreateSingle( this, &breakModel, GetAbsOrigin(), GetAbsAngles(), vecBreakVelocity, breakParams.angularVelocity, GetTeamSkin( GetTeamNumber() ), breakParams );
}


void C_TFPlayer::DropPartyHat( breakablepropparams_t &breakParams, Vector &vecBreakVelocity )
{
	if ( m_hPartyHat )
	{
		breakmodel_t breakModel;
		const char* strModel = TFGameRules()->IsHolidayActive(TF_HOLIDAY_WINTER) ? SANTA_HAT_MODEL : BDAY_HAT_MODEL;
		V_strcpy_safe( breakModel.modelName, strModel );
		breakModel.health = 1;
		breakModel.fadeTime = RandomFloat( 5.0f, 10.0f );
		breakModel.fadeMinDist = 0.0f;
		breakModel.fadeMaxDist = 0.0f;
		breakModel.burstScale = breakParams.defBurstScale;
		breakModel.collisionGroup = COLLISION_GROUP_DEBRIS;
		breakModel.isRagdoll = false;
		breakModel.isMotionDisabled = false;
		breakModel.placementName[0] = 0;
		breakModel.placementIsBone = false;
		breakModel.offset = GetAbsOrigin() - m_hPartyHat->GetAbsOrigin();
		BreakModelCreateSingle( this, &breakModel, m_hPartyHat->GetAbsOrigin(), m_hPartyHat->GetAbsAngles(), vecBreakVelocity, breakParams.angularVelocity, m_hPartyHat->m_nSkin, breakParams );

		m_hPartyHat->Release();
	}
}


void C_TFPlayer::CreateBoneAttachmentsFromWearables( C_TFRagdoll *pRagdoll )
{
	// Calculate our bodygroups for the ragdoll.
	m_Shared.RecalculatePlayerBodygroups( true );

	pRagdoll->m_nBody = m_nBody;

	m_Shared.RestorePlayerBodygroups();

	// Create a fake wearable on ragdoll for every wearable carried by this player.
	C_TFWearable *pWearable;
	C_BaseAnimating *pWearableCopy;
	for ( int i = 0, c = GetNumWearables(); i < c; i++ )
	{
		pWearable = dynamic_cast<C_TFWearable *>( GetWearable( i ) );
		if ( !pWearable || pWearable->IsDisguiseWearable() || pWearable->GetWeaponAssociatedWith())
			continue;

		pWearableCopy = new C_BaseAnimating();
		if ( !pWearableCopy->InitializeAsClientEntity( modelinfo->GetModelName( pWearable->GetModel() ), RENDER_GROUP_OPAQUE_ENTITY ) )
		{
			delete pWearableCopy;
			continue;
		}

		pWearableCopy->m_nSkin = pRagdoll->m_nSkin > 1 ? pRagdoll->m_nSkin - 2 : pRagdoll->m_nSkin;
		pWearableCopy->AttachEntityToBone( pRagdoll );
	}

	// Create a fake wearable on ragdoll for every extra wearable carried by the player
	for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot < TF_LOADOUT_SLOT_COUNT; iSlot++)
	{
		CTFWeaponBase* pWeapon = GetTFWeaponBySlot(iSlot);
		if (!pWeapon)
			continue;
		CEconItemView* pItem = pWeapon->GetItem();
		if (!(pItem && pItem->GetExtraWearableModel() && !pItem->GetExtraWearableModelVisibilityRules()))
			continue;

		pWearableCopy = new C_BaseAnimating();
		if (!pWearableCopy->InitializeAsClientEntity(pItem->GetExtraWearableModel(), RENDER_GROUP_OPAQUE_ENTITY))
		{
			delete pWearableCopy;
			continue;
		}

		pWearableCopy->m_nSkin = pRagdoll->m_nSkin > 1 ? pRagdoll->m_nSkin - 2 : pRagdoll->m_nSkin;
		pWearableCopy->AttachEntityToBone(pRagdoll);
	}
}

//-----------------------------------------------------------------------------
// Purpose: How many buildables does this player own
//-----------------------------------------------------------------------------
int	C_TFPlayer::GetObjectCount( void )
{
	return m_aObjects.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
C_BaseObject *C_TFPlayer::GetObject( int index )
{
	return m_aObjects[index].Get();
}

//-----------------------------------------------------------------------------
// Purpose: Get a specific buildable that this player owns
//-----------------------------------------------------------------------------
C_BaseObject *C_TFPlayer::GetObjectOfType( int iObjectType, int iObjectMode /*= 0*/ )
{
	for ( int i = 0, c = m_aObjects.Count(); i < c; i++ )
	{
		C_BaseObject *pObject = m_aObjects[i].Get();
		if ( !pObject )
			continue;

		if ( pObject->IsDormant() || pObject->IsMarkedForDeletion() )
			continue;

		if ( pObject->GetType() != iObjectType )
			continue;

		if ( pObject->GetObjectMode() != iObjectMode )
			continue;

		return pObject;
	}
	
	return NULL;
}


float C_TFPlayer::GetPercentInvisible( void )
{
	return m_Shared.GetPercentInvisible();
}


int C_TFPlayer::GetSkin()
{
	C_TFPlayer *pLocalPlayer = GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return 0;

	int nSkin;
	int iVisibleTeam = GetTeamNumber();

	// if this player is disguised and on the other team, use disguise team
	if ( IsDisguisedEnemy() )
	{
		iVisibleTeam = m_Shared.GetDisguiseTeam();
	}

	switch ( iVisibleTeam )
	{
		case TF_TEAM_RED:
			nSkin = 0;
			break;

		case TF_TEAM_BLUE:
			nSkin = 1;
			break;

		case TF_TEAM_GREEN:
			nSkin = 4;
			break;

		case TF_TEAM_YELLOW:
			nSkin = 5;
			break;

		default:
			nSkin = 0;
			break;
	}

	// 3 and 4 are invulnerable
	if ( m_Shared.IsInvulnerable() &&
		( !m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) || gpGlobals->curtime - m_flLastDamageTime < 2.0f ) )
	{
		nSkin += 2;
	}

	return nSkin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iClass - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::IsPlayerClass( int iClass, bool bIgnoreRandomizer /*= false*/ )
{
	if ( !bIgnoreRandomizer && GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS ) )
		return true;

	return m_PlayerClass.IsClass( iClass );
}

//-----------------------------------------------------------------------------
// Purpose: Don't take damage decals while stealthed
//-----------------------------------------------------------------------------
void C_TFPlayer::AddDecal( const Vector& rayStart, const Vector& rayEnd,
							const Vector& decalCenter, int hitbox, int decalIndex, bool doTrace, trace_t& tr, int maxLODToDecal )
{
	if ( m_Shared.IsStealthed() )
		return;

	if ( m_Shared.IsDisguised() )
		return;

	if ( m_Shared.IsInvulnerable() )
	{ 
		Vector vecDir = rayEnd - rayStart;
		VectorNormalize(vecDir);
		g_pEffects->Ricochet( rayEnd - (vecDir * 8), -vecDir );
		return;
	}

	// Don't decal from inside the player.
	if ( tr.startsolid )
		return;

	BaseClass::AddDecal( rayStart, rayEnd, decalCenter, hitbox, decalIndex, doTrace, tr, maxLODToDecal );
}

//-----------------------------------------------------------------------------
// Called every time the player respawns
//-----------------------------------------------------------------------------
void C_TFPlayer::ClientPlayerRespawn( void )
{
	if ( IsLocalPlayer() )
	{
		// DoD called these, not sure why.
		//MoveToLastReceivedPosition( true );
		//ResetLatched();

		// Reset the camera.
		m_bWasTaunting = false;
		HandleTaunting();

		ResetToneMapping( 1.0f );

		// Release the duck toggle key.
		KeyUp( &in_ducktoggle, NULL );

		LoadInventory();
	}
	else
	{
		SetViewOffset( GetClassEyeHeight() );
	}

	ParticleProp()->StopEmissionAndDestroyImmediately();
	m_mapOverheadEffects.RemoveAll();
	m_bIsDisplayingNemesisIcon = false;

	m_nBody = 0;
	m_Shared.m_nDisguiseBody = 0;

	UpdateVisibility();
	DestroyBoneAttachments();

	m_hFirstGib = NULL;
	m_hSpawnedGibs.Purge();
}


bool C_TFPlayer::ShouldDraw()
{
	if ( IsLocalPlayer() && ( m_PlayerClass.HasCustomModel() && !m_PlayerClass.CustomModelIsVisibleToSelf() ) )
		return false;

	return BaseClass::ShouldDraw();
}


void C_TFPlayer::CreateSaveMeEffect( bool bAuto )
{
	// Don't create them for the local player.
	if ( !ShouldDrawThisPlayer() )
		return;

	// Only show the bubble to teammates and players who are on our disguise team.
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( GetTeamNumber() != pLocalPlayer->GetTeamNumber() && !IsDisguisedEnemy( true ) )
		return;

	if ( m_pSaveMeEffect )
	{
		ParticleProp()->StopEmission( m_pSaveMeEffect );
		m_pSaveMeEffect = NULL;
	}

	if ( bAuto )
	{
		m_pSaveMeEffect = ParticleProp()->Create( "speech_mediccall_auto", PATTACH_POINT_FOLLOW, "head" );
		EmitSound( "Medic.AutoCallerAnnounce" );
	}
	else
	{
		m_pSaveMeEffect = ParticleProp()->Create( "speech_mediccall", PATTACH_POINT_FOLLOW, "head" );
		if ( m_pSaveMeEffect )
		{
			// Set "redness" of the bubble based on player's health.
			float flHealthRatio = clamp( (float)GetHealth() / (float)GetMaxHealth(), 0.0f, 1.0f );
			m_pSaveMeEffect->SetControlPoint( 1, Vector( flHealthRatio ) );
		}
	}

	// If the local player is a Medic, add this player to our list of medic callers.
	if ( pLocalPlayer && pLocalPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && pLocalPlayer->IsAlive() )
	{
		Vector vecPos;
		if ( GetAttachmentLocal( LookupAttachment( "head" ), vecPos ) )
		{
			vecPos += Vector( 0, 0, 18 ); // Particle effect is 18 units above the attachment
			CTFMedicCallerPanel::AddMedicCaller( this, 5.0, vecPos, bAuto );
		}
	}
}

#ifdef TF2C_BETA
//-----------------------------------------------------------------------------
// Purpose: Creates an overhead healthcross/bar allowing a Medic to see a
// player's health without targeting them.
//-----------------------------------------------------------------------------
void C_TFPlayer::CreateOverheadHealthbar()
{
	// Don't create them for the local player.
	if ( !ShouldDrawThisPlayer() || C_TFPlayer::GetLocalTFPlayer() == this )
		return;

	// Only show the bubble to teammates and players who are on our disguise team.
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( GetTeamNumber() != pLocalPlayer->GetTeamNumber() && !IsDisguisedEnemy( true ) )
		return;

	// If the local player is a Medic, add this player to our list of medic callers.
	if ( pLocalPlayer && pLocalPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && pLocalPlayer->IsAlive() )
	{
		Vector vecPos;
		if ( GetAttachmentLocal( LookupAttachment( "head" ), vecPos ) )
		{
			//vecPos += Vector( 0, 0, 18 ); // Particle effect is 18 units above the attachment
			return CTFPatientHealthbarPanel::AddPatientHealthbar( this, vecPos );
		}
	}
}
#endif


bool C_TFPlayer::IsOverridingViewmodel( void )
{
	bool bRet = BaseClass::IsOverridingViewmodel();
	if ( !tf2c_legacy_invulnerability_material.GetBool() )
		return bRet;

	C_TFPlayer *pPlayer = GetObservedPlayer( true );
	if ( pPlayer->m_Shared.IsInvulnerable() && !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) )
		return true;

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: Draw my viewmodel in some special way
//-----------------------------------------------------------------------------
int	C_TFPlayer::DrawOverriddenViewmodel( C_BaseViewModel *pViewmodel, int flags )
{
	int iRet = 0;

	C_TFPlayer *pPlayer = GetObservedPlayer( true );
	if ( pPlayer->m_Shared.IsInvulnerable() && !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) )
	{
		// Force the invulnerable material
		modelrender->ForcedMaterialOverride( *pPlayer->GetInvulnMaterialRef() );

		iRet = pViewmodel->DrawOverriddenViewmodel( flags );

		modelrender->ForcedMaterialOverride( NULL );
	}

	return iRet;
}


void C_TFPlayer::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed );

	m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );
	BuildFirstPersonMeathookTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed, "bip_head" );
}


void C_TFPlayer::SetHealer( C_TFPlayer *pHealer, float flChargeLevel )
{
	// We may be getting healed by multiple healers. Show the healer
	// who's got the highest charge level.
	if ( m_hHealer && m_flHealerChargeLevel > flChargeLevel )
		return;

	m_hHealer = pHealer;
	m_flHealerChargeLevel = flChargeLevel;
}


void C_TFPlayer::InitializePoseParams( void )
{
	/*m_headYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );*/

	CStudioHdr *hdr = GetModelPtr();
	Assert( hdr );
	if ( !hdr )
		return;

	for ( int i = 0; i < hdr->GetNumPoseParameters(); i++ )
	{
		SetPoseParameter( hdr, i, 0.0 );
	}
}


Vector C_TFPlayer::GetChaseCamViewOffset( CBaseEntity *target )
{
	if ( target->IsBaseObject() )
		return Vector( 0, 0, 64 );

	return BaseClass::GetChaseCamViewOffset( target );
}

//-----------------------------------------------------------------------------
// Purpose: Called from PostDataUpdate to update the model index
//-----------------------------------------------------------------------------
void C_TFPlayer::ValidateModelIndex( void )
{
	if ( m_Shared.InCond( TF_COND_DISGUISED_AS_DISPENSER ) && IsEnemyPlayer() && GetGroundEntity() && IsDucked() )
	{
		m_nModelIndex = modelinfo->GetModelIndex( "models/buildables/dispenser_light.mdl" );

		if ( GetLocalPlayer() != this )
		{
			SetAbsAngles( vec3_angle );
		}
	}
	else if ( IsDisguisedEnemy() )
	{
		m_nModelIndex = modelinfo->GetModelIndex( GetPlayerClassData( m_Shared.GetDisguiseClass() )->GetModelName() );
	}
	else
	{
		C_TFPlayerClass *pClass = GetPlayerClass();
		if ( pClass )
		{
			m_nModelIndex = modelinfo->GetModelIndex( pClass->GetModelName() );
		}
	}

	BaseClass::ValidateModelIndex();
}


void C_TFPlayer::UpdateVisibility( void )
{
	BaseClass::UpdateVisibility();

	// Update visibility on everything attached to us.
	UpdateWearables();

	for ( C_BaseEntity *pChild = FirstMoveChild(); pChild; pChild = pChild->NextMovePeer() )
	{
		pChild->UpdateVisibility();
		pChild->CreateShadow();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Simulate the player for this frame
//-----------------------------------------------------------------------------
void C_TFPlayer::Simulate( void )
{
	// Frame updates
	if ( IsLocalPlayer() )
	{
		// Update the flashlight
		Flashlight();
	}

	// Update step sound for first person players.
	if ( !GetPredictable() && !ShouldDrawThisPlayer() )
	{
		Vector vel;
		EstimateAbsVelocity( vel );
		UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
	}

	if ( tf2c_cig_light.GetBool() && CanLightCigarette() )
	{
		if ( !m_pCigaretteLight || m_pCigaretteLight->key != entindex() )
		{
			m_pCigaretteLight = effects->CL_AllocElight( entindex() );
			m_pCigaretteLight->color.r = 255;
			m_pCigaretteLight->color.g = 100;
			m_pCigaretteLight->color.b = 10;
			m_pCigaretteLight->die = FLT_MAX;
			m_pCigaretteLight->radius = 16.0f;
			m_pCigaretteLight->style = 6;
		}

		GetAttachment( "cig_smoke", m_pCigaretteLight->origin );
	}
	else
	{
		if ( m_pCigaretteLight && m_pCigaretteLight->key == entindex() )
		{
			m_pCigaretteLight->die = gpGlobals->curtime;
		}

		m_pCigaretteLight = NULL;
	}

	// TF doesn't do step sounds based on velocity, instead using anim events
	// So we deliberately skip over the base player simulate, which calls them.
	BaseClass::BaseClass::Simulate();
}

//unsigned lodinv(void* nada)
//{
//	for (int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; iClass++)
//	{
//		for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot < TF_LOADOUT_SLOT_COUNT; iSlot++)
//		{
//			ThreadSleep(10);
//			ThreadJoin(ThreadGetCurrentHandle());
//			engine->ExecuteClientCmd(VarArgs("setitempreset %d %d %d;", iClass, iSlot, GetTFInventory()->GetItemPreset(iClass, (ETFLoadoutSlot)iSlot)));
//		}
//	}
//	return 0;
//}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::LoadInventory( void )
{
	// NOTE: If and after a certain number of classes are added,
	// this function will start causing issues and a new method will
	// need to be written up, or else clients will be forcefully disconnected.

	// void* nada = nullptr;
	// ThreadHandle_t h = CreateSimpleThread(lodinv, nada, 1024);
	// ThreadDetach(h);
	// This should be prolly threaded so we don't SLAM the server with ~200 stringcmds when connecting to the server, lol
	// Also, ideally, you'd run the current selected class FIRST, on the main thread, with no ThreadSleeps
	// I leave this all as an exercise to the reader
	// -sappho

#if 0
	// dummied kv version, which is known not to work on Dedicated Linux server, possibly
	KeyValues* pKV = new KeyValues("LoadInventory");
	if (!GetTFInventory()->GetAllItemPresets(pKV))
	{
		pKV->deleteThis();
		return;
	}

	// callee frees pKV for us
	engine->ServerCmdKeyValues(pKV);
#endif
	C_TFPlayerClass* pClass = nullptr;
	pClass = GetPlayerClass();

	// this shouldn't happen
	if (!pClass)
	{
		Assert(pClass);
		return;
	}

	int classidx = pClass->GetClassIndex();
	// They haven't selected a class yet (0), or their class is BUNK;
	if (classidx < TF_CLASS_SCOUT || classidx > TF_CLASS_CIVILIAN)
	{
		for (int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; iClass++)
		{
			for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot < TF_LOADOUT_SLOT_COUNT; iSlot++)
			{
				engine->ExecuteClientCmd(VarArgs("setitempreset %d %d %d;", iClass, iSlot, GetTFInventory()->GetItemPreset(iClass, (ETFLoadoutSlot)iSlot)));
			}
		}
		return;
	}
	// They have selected a class, don't run useless setitempreset cmds
	else
	{
		for (int iSlot = TF_LOADOUT_SLOT_PRIMARY; iSlot < TF_LOADOUT_SLOT_COUNT; iSlot++)
		{
			engine->ExecuteClientCmd(VarArgs("setitempreset %d %d %d;", classidx, iSlot, GetTFInventory()->GetItemPreset(classidx, (ETFLoadoutSlot)iSlot)));
		}
		return;
	}

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFPlayer::EditInventory( ETFLoadoutSlot iSlot, int iWeapon )
{
	GetTFInventory()->SetItemPreset( GetPlayerClass()->GetClassIndex(), iSlot, iWeapon );
}


void C_TFPlayer::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
	switch ( event )
	{
		case TF_AE_FOOTSTEP:
		{
			// Force a footstep sound.
			// Set this to -1 to indicate we're about to make a forced footstep sound.
			m_flStepSoundTime = -1.0f;
			Vector vel;
			EstimateAbsVelocity( vel );
			UpdateStepSound( GetGroundSurface(), GetAbsOrigin(), vel );
			break;
		}
		case AE_WPN_HIDE:
		{
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( pActiveWeapon )
			{
				pActiveWeapon->SetWeaponVisible( false );

				m_Shared.UpdateCritBoostEffect();
			}
			break;
		}
		case AE_WPN_UNHIDE:
		{
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( pActiveWeapon )
			{
				pActiveWeapon->SetWeaponVisible( true );

				m_Shared.UpdateCritBoostEffect();
			}
			break;
		}
		case AE_WPN_PLAYWPNSOUND:
		{
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( pActiveWeapon )
			{
				int iSnd = GetWeaponSoundFromString( options );
				if ( iSnd != -1 )
				{
					pActiveWeapon->WeaponSound( (WeaponSound_t)iSnd );
				}
			}
			break;
		}
		case TF_AE_CIGARETTE_THROW:
		{
			CEffectData data;
			int iAttach = LookupAttachment( options );
			GetAttachment( iAttach, data.m_vOrigin, data.m_vAngles );

			data.m_vAngles = GetRenderAngles();

			data.m_hEntity = ClientEntityList().EntIndexToHandle( entindex() );
			DispatchEffect( "TF_ThrowCigarette", data );
			break;
		}
		case AE_CL_BODYGROUP_SET_VALUE_CMODEL_WPN:
		{
			// Pass it through to weapon.
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( pActiveWeapon )
			{
				pActiveWeapon->GetAppropriateWorldOrViewModel()->FireEvent( origin, angles, AE_CL_BODYGROUP_SET_VALUE, options );
			}
			break;
		}
		case TF_AE_WPN_EJECTBRASS:
		{
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( !pActiveWeapon || pActiveWeapon->UsingViewModel() )
				return;

			pActiveWeapon->EjectBrass();
			break;
		}
		case AE_CL_MAG_EJECT:
		{
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( !pActiveWeapon || pActiveWeapon->UsingViewModel() )
				return;

			// Check if we're actually in the middle of reload anim and it wasn't just interrupted by firing.
			C_AnimationLayer *pAnimLayer = m_PlayerAnimState->GetGestureSlotLayer( GESTURE_SLOT_ATTACK_AND_RELOAD );	
			if ( pAnimLayer->m_flCycle == 1.0f )
				return;

			// If they have auto-reload, only do this with an empty clip otherwise it looks nasty.
			if ( m_bAutoReload && pActiveWeapon->Clip1() != 0 )
				return;

			Vector vecForce;
			UTIL_StringToVector( vecForce.Base(), options );

			pActiveWeapon->EjectMagazine( vecForce );
			break;
		}
		case AE_CL_MAG_EJECT_UNHIDE:
		{
			CTFWeaponBase *pActiveWeapon = GetActiveTFWeapon();
			if ( !pActiveWeapon || pActiveWeapon->UsingViewModel() )
				return;

			pActiveWeapon->UnhideMagazine();
			break;
		}
		default:
			BaseClass::FireEvent( origin, angles, event, options );
			break;
	}
}

// Shadows

ConVar cl_blobbyshadows( "cl_blobbyshadows", "0", FCVAR_CLIENTDLL );
extern ConVar tf2c_disable_player_shadows;


ShadowType_t C_TFPlayer::ShadowCastType( void ) 
{
	if ( tf2c_disable_player_shadows.GetBool() )
		return SHADOWS_NONE;

	// Removed the GetPercentInvisible - should be taken care off in BindProxy now.
	if ( !IsVisible() /*|| GetPercentInvisible() > 0.0f*/ )
		return SHADOWS_NONE;

	if ( IsEffectActive( EF_NODRAW | EF_NOSHADOW ) )
		return SHADOWS_NONE;

	// If in ragdoll mode.
	if ( m_nRenderFX == kRenderFxRagdoll )
		return SHADOWS_NONE;

	// If we're first person spectating this player.
	if ( !ShouldDrawThisPlayer() )
		return SHADOWS_NONE;

	if ( cl_blobbyshadows.GetBool() )
		return SHADOWS_SIMPLE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}


void C_TFPlayer::GetShadowRenderBounds( Vector &mins, Vector &maxs, ShadowType_t shadowType )
{
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Don't let the render bounds change when we're using blobby shadows, or else the shadow
		// will pop and stretch.
		mins = CollisionProp()->OBBMins();
		maxs = CollisionProp()->OBBMaxs();
	}
	else
	{
		GetRenderBounds( mins, maxs );

		// We do this because the normal bbox calculations don't take pose params into account, and 
		// the rotation of the guy's upper torso can place his gun a ways out of his bbox, and 
		// the shadow will get cut off as he rotates.
		//
		// Thus, we give it some padding here.
		const Vector vecBloat( 36, 36, 0 );
		mins -= vecBloat;
		maxs += vecBloat;
	}
}


void C_TFPlayer::GetRenderBounds( Vector& theMins, Vector& theMaxs )
{
	// TODO POSTSHIP - this hack/fix goes hand-in-hand with a fix in CalcSequenceBoundingBoxes in utils/studiomdl/simplify.cpp.
	// When we enable the fix in CalcSequenceBoundingBoxes, we can get rid of this.
	//
	// What we're doing right here is making sure it only uses the bbox for our lower-body sequences since,
	// with the current animations and the bug in CalcSequenceBoundingBoxes, are WAY bigger than they need to be.
	C_BaseAnimating::GetRenderBounds( theMins, theMaxs );
}


bool C_TFPlayer::GetShadowCastDirection( Vector *pDirection, ShadowType_t shadowType ) const
{ 
	if ( shadowType == SHADOWS_SIMPLE )
	{
		// Blobby shadows should sit directly underneath us.
		pDirection->Init( 0, 0, -1 );
		return true;
	}
	
	return BaseClass::GetShadowCastDirection( pDirection, shadowType );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether we should show the nemesis icon for this player
//-----------------------------------------------------------------------------
bool C_TFPlayer::ShouldShowNemesisIcon()
{
	if ( !tf2c_show_nemesis_relationships.GetBool() )
		return false;

	if ( m_PlayerClass.HasCustomModel() )
		return false;

	// We should show the nemesis effect on this player if he is the nemesis of the local player,
	// and is not dead, cloaked or disguised.
	if ( g_TF_PR && g_TF_PR->IsPlayerDominating( entindex() ) )
	{
		if ( IsAlive() && !m_Shared.IsStealthed() && !m_Shared.IsDisguised() )
			return true;
	}

	return false;
}


bool C_TFPlayer::IsWeaponLowered( void )
{
	CTFWeaponBase *pWeapon = GetActiveTFWeapon();
	if ( !pWeapon )
		return false;

	// Lower losing team's weapons in bonus round.
	CTFGameRules *pGameRules = TFGameRules();
	if ( pGameRules->State_Get() == GR_STATE_TEAM_WIN && pGameRules->GetWinningTeam() != GetTeamNumber() )
		return true;

	// Hide all view models after the game is over.
	if ( pGameRules->State_Get() == GR_STATE_GAME_OVER )
		return true;

	return false;
}


bool C_TFPlayer::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	switch ( event->GetType() )
	{
		case CChoreoEvent::SEQUENCE:
		case CChoreoEvent::GESTURE:
			return StartGestureSceneEvent( info, scene, event, actor, pTarget );
		default:
			return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
	}
}


bool C_TFPlayer::StartGestureSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	// Get the (gesture) sequence.
	info->m_nSequence = LookupSequence( event->GetParameters() );
	if ( info->m_nSequence < 0 )
		return false;

	// Fix up for looping taunts.
	float flCycle = 0.0f;
	bool bLooping = ( ( GetSequenceFlags( GetModelPtr(), info->m_nSequence ) & STUDIO_LOOPING ) != 0 );
	if ( !bLooping )
	{
		flCycle = clamp( ( scene->GetTime() - event->GetStartTime() ) / SequenceDuration( info->m_nSequence ), 0.0f, 1.0f );
	}

	// Player the (gesture) sequence.
	m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_VCD );
	m_PlayerAnimState->AddVCDSequenceToGestureSlot( GESTURE_SLOT_VCD, info->m_nSequence, flCycle );
	return true;
}


int C_TFPlayer::GetNumActivePipebombs( void )
{
	if ( IsPlayerClass( TF_CLASS_DEMOMAN ) )
	{
		CTFPipebombLauncher *pWeapon = dynamic_cast<CTFPipebombLauncher *>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );
		if ( pWeapon )
		{
			return pWeapon->GetPipeBombCount();
		}
	}

	return 0;
}


bool C_TFPlayer::IsAllowedToSwitchWeapons( void )
{
	if ( IsWeaponLowered() )
		return false;

	return BaseClass::IsAllowedToSwitchWeapons();
}


IMaterial *C_TFPlayer::GetHeadLabelMaterial( void )
{
	if ( !g_pHeadLabelMaterial[0] )
		SetupHeadLabelMaterials();

	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_RED];
			break;

		case TF_TEAM_BLUE:
			return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_BLUE];
			break;

		case TF_TEAM_GREEN:
			return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_GREEN];
			break;

		case TF_TEAM_YELLOW:
			return g_pHeadLabelMaterial[TF_PLAYER_HEAD_LABEL_YELLOW];
			break;

	}

	return BaseClass::GetHeadLabelMaterial();
}


void SetupHeadLabelMaterials( void )
{
	for ( int i = 0; i < ( TF_TEAM_COUNT - 2 ); i++ )
	{
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->DecrementReferenceCount();
			g_pHeadLabelMaterial[i] = NULL;
		}

		g_pHeadLabelMaterial[i] = materials->FindMaterial( pszHeadLabelNames[i], TEXTURE_GROUP_VGUI );
		if ( g_pHeadLabelMaterial[i] )
		{
			g_pHeadLabelMaterial[i]->IncrementReferenceCount();
		}
	}
}


void C_TFPlayer::ComputeFxBlend( void )
{
	BaseClass::ComputeFxBlend();

	float flInvisible = GetPercentInvisible();
	if ( flInvisible != 0.0f )
	{
		// Inform our shadow about this.
		ClientShadowHandle_t hShadow = GetShadowHandle();
		if ( hShadow != CLIENTSHADOW_INVALID_HANDLE )
		{
			g_pClientShadowMgr->SetFalloffBias( hShadow, flInvisible * 255 );
		}
	}
}


void C_TFPlayer::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	HandleTaunting();

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}


void C_TFPlayer::ForceUpdateObjectHudState( void )
{
	m_bUpdateObjectHudState = true;
}


C_TFTeam *C_TFPlayer::GetTFTeam( void ) const
{
	// Grumble: server-side CBaseEntity::GetTeam is const,
	// but client-side C_BaseEntity::GetTeam is non-const,
	// for absolutely no good reason whatsoever.
	return assert_cast<C_TFTeam *>( const_cast<C_TFPlayer *>( this )->GetTeam() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether the weapon passed in would occupy a slot already occupied by the carrier
// Input  : *pWeapon - weapon to test for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFPlayer::Weapon_SlotOccupied( CBaseCombatWeapon *pWeapon )
{
	if ( !pWeapon )
		return false;

	//Check to see if there's a resident weapon already in this slot
	if ( !Weapon_GetSlot( pWeapon->GetSlot() ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the weapon (if any) in the requested slot
// Input  : slot - which slot to poll
//-----------------------------------------------------------------------------
CTFWeaponBase *C_TFPlayer::Weapon_GetSlot( int slot ) const
{
	int	targetSlot = slot;

	// Check for that slot being occupied already
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTFWeaponBase *pWeapon = GetTFWeapon( i );
		if ( pWeapon )
		{
			// If the slots match, it's already occupied
			if ( pWeapon->GetSlot() == targetSlot )
				return pWeapon;
		}
	}

	return NULL;
}

void C_TFPlayer::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( !V_strcmp( pszEventName, "localplayer_changeteam" ) )
	{
		if ( !IsLocalPlayer() )
		{
			UpdateSpyStateChange();
		}
	}
	else if ( !V_strcmp( pszEventName, "rocket_jump" ) || !V_strcmp( pszEventName, "sticky_jump" ) )
	{
		// Play a special sound when blast jumping with weapons that don't hurt the player
		bool bWhistle = event->GetBool( "playsound" );
		if ( bWhistle && GetUserID() == event->GetInt( "userid" ) )
		{
			if ( !m_pBlastJumpLoop )
			{
				CBroadcastRecipientFilter filter;
				CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
				m_pBlastJumpLoop = controller.SoundCreate( filter, entindex(), "BlastJump.Whistle" );
				controller.Play( m_pBlastJumpLoop, 0.25, 200 );
				m_flBlastJumpLaunchTime = gpGlobals->curtime;
			}
		}
	}
	else if ( !V_strcmp( pszEventName, "rocket_jump_landed" ) || !V_strcmp( pszEventName, "sticky_jump_landed" ) )
	{
		if ( m_pBlastJumpLoop )
		{
			if ( GetUserID() == event->GetInt( "userid" ) )
			{
				CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
				controller.SoundDestroy( m_pBlastJumpLoop );
				m_pBlastJumpLoop = NULL;
			}
		}
	}
	else
	{
		BaseClass::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update any effects affected by cloak and disguise.
//-----------------------------------------------------------------------------
void C_TFPlayer::UpdateSpyStateChange( void )
{
	if ( m_Shared.InCond( TF_COND_BURNING ) )
	{
		m_Shared.OnAddBurning();
	}

	m_Shared.UpdateCritBoostEffect();
	UpdateOverhealEffect();
	UpdateRecentlyTeleportedEffect();
	UpdateJumppadTrailEffect();
	UpdateSpeedBoostTrailEffect();
	UpdateOverheadEffects();
	UpdateSpyMask();
	InitInvulnerableMaterial();
#ifdef TF2C_BETA
	InitInvulnerableMaterialPlayerModelPanel();
#endif
}


void C_TFPlayer::UpdateSpyMask( void )
{
	C_TFSpyMask *pMask = m_hSpyMask.Get();
	if ( m_Shared.IsDisguised() )
	{
		// Create mask if we don't already have one.
		if ( !pMask )
		{
			pMask = C_TFSpyMask::Create( this );		
			if ( !pMask )
				return;

			m_hSpyMask = pMask;
		}

		pMask->UpdateVisibility();
	}
	else if ( pMask )
	{
		pMask->Release();
		m_hSpyMask = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check if the local player should see a Spy with his disguised bodygroups.
//-----------------------------------------------------------------------------
int C_TFPlayer::GetBody( void )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( GetTeamNumber() != pLocalPlayer->GetTeamNumber() && IsDisguisedEnemy( true ) )
	{
		// This enemy is disguised, show the disguise body.
		return m_Shared.GetDisguiseBody();
	}
	
	return BaseClass::GetBody();
}


bool C_TFPlayer::CanShowSpeechBubbles( void )
{
	if ( IsDormant() )
		return false;

	if ( !GameRules()->ShouldDrawHeadLabels() )
		return false;

	extern ConVar *sv_alltalk;

	if ( !sv_alltalk )
		sv_alltalk = cvar->FindVar( "sv_alltalk" );

	if ( !sv_alltalk || !sv_alltalk->GetBool() )
	{
		int iLocalTeam = GetLocalPlayerTeam();
		if ( iLocalTeam != GetTeamNumber() && iLocalTeam != TEAM_SPECTATOR )
			return false;
	}

	if ( !IsAlive() )
		return false;

	if ( m_Shared.IsStealthed() && IsEnemyPlayer() )
		return false;

	return true;
}


void C_TFPlayer::UpdateSpeechBubbles( void )
{
	// Don't show the bubble for local player since they don't need it.
	if ( IsLocalPlayer() )
		return;

	if ( m_bTyping && CanShowSpeechBubbles() )
	{
		if ( !m_pTypingEffect )
		{
			m_pTypingEffect = ParticleProp()->Create( "speech_typing", PATTACH_POINT_FOLLOW, "head" );
		}
	}
	else
	{
		if ( m_pTypingEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pTypingEffect );
			m_pTypingEffect = NULL;
		}
	}

	if ( GetClientVoiceMgr()->IsPlayerSpeaking( entindex() ) && CanShowSpeechBubbles() )
	{
		if ( !m_pVoiceEffect )
		{
			m_pVoiceEffect = ParticleProp()->Create( "speech_voice", PATTACH_POINT_FOLLOW, "head" );
		}
	}
	else
	{
		if ( m_pVoiceEffect )
		{
			ParticleProp()->StopEmissionAndDestroyImmediately( m_pVoiceEffect );
			m_pVoiceEffect = NULL;
		}
	}
}


void C_TFPlayer::UpdateOverhealEffect( void )
{
	bool bShouldShow;
	if ( !m_Shared.InCond( TF_COND_HEALTH_OVERHEALED ) )
	{
		bShouldShow = false;
	}
	else if ( m_Shared.IsStealthed() )
	{
		// Don't give away cloaked and disguised spies.
		bShouldShow = false;
	}
	else
	{
		bShouldShow = true;
	}

	if ( m_pOverhealEffect )
	{
		ParticleProp()->StopEmission( m_pOverhealEffect );
		m_pOverhealEffect = NULL;
	}

	if ( bShouldShow )
	{
		m_pOverhealEffect = ParticleProp()->Create( ConstructTeamParticle( "overhealedplayer_%s_pluses", IsDisguisedEnemy() ? m_Shared.GetDisguiseTeam() : GetTeamNumber() ), PATTACH_ABSORIGIN_FOLLOW );
	}
}


void C_TFPlayer::UpdateClientSideGlow( void )
{
	bool bShouldGlow = false;
	if ( !IsLocalPlayer() ) // Never show glow on local player.
	{
		if (IsEnemyPlayer()) // if they are enemies
		{
			if (m_Shared.InCond(TF_COND_MARKED_OUTLINE)) // outline
			{
				bShouldGlow = true;
			}
			else // or spy radar
			{
				auto pTFLocalPlayer = GetLocalTFPlayer();
				if (pTFLocalPlayer && 
					!(m_Shared.IsStealthed() || m_Shared.DisguiseFoolsTeam(pTFLocalPlayer->GetTeamNumber())))
				{
					float flRadarRange = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(pTFLocalPlayer->GetActiveTFWeapon(), flRadarRange, sapper_is_radar);
					if (flRadarRange > 0.0f)
					{
						float flRadius2 = Square(flRadarRange); 
						float flDist2 = 0.0f;
						Vector vecSegment;
						Vector vecTargetPoint;

						vecTargetPoint = pTFLocalPlayer->GetAbsOrigin();
						vecTargetPoint += pTFLocalPlayer->GetViewOffset();
						VectorSubtract(vecTargetPoint, GetAbsOrigin(), vecSegment);
						flDist2 = vecSegment.LengthSqr();
						if (flDist2 <= flRadius2)
						{
							bShouldGlow = true;
						}
					}
				}
			}
		}
		else // if they are teammates
		{
			bShouldGlow = (HasTheFlag() || IsVIP());
		}
	}

	if ( bShouldGlow != IsClientSideGlowEnabled() )
	{
		SetClientSideGlowEnabled( bShouldGlow );
	}
}


void C_TFPlayer::GetGlowEffectColor( float *r, float *g, float *b )
{
	// Set the glow color on flag carrier according to their health.
	if ( ( HasTheFlag() || IsVIP() ) && !IsEnemyPlayer() )
	{
		*r = RemapValClamped( GetHealth(), GetMaxHealth() * 0.5f, GetMaxHealth(), 0.75f, 0.33f );
		*g = RemapValClamped( GetHealth(), 0.0f, GetMaxHealth() * 0.5f, 0.23f, 0.75f );
		*b = 0.23f;
	}
	else
	{
		TFGameRules()->GetTeamGlowColor( GetTeamNumber(), *r, *g, *b );
	}
}


bool C_TFPlayer::AddOverheadEffect( const char *pszEffectName )
{
	int index_ = m_mapOverheadEffects.Find( pszEffectName );

	// particle is added already
	if ( index_ != m_mapOverheadEffects.InvalidIndex() )
		return false;

	CNewParticleEffect *pEffect = ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW, 0, GetOverheadEffectPosition() );
	if ( pEffect )
	{
		if ( m_mapOverheadEffects.Count() == 0 )
		{
			m_flOverheadEffectStartTime = gpGlobals->curtime;
		}

		m_mapOverheadEffects.Insert( pszEffectName, pEffect );

		return true;
	}

	return false;
}



void C_TFPlayer::RemoveOverheadEffect( const char *pszEffectName, bool bRemoveInstantly )
{
	int index_ = m_mapOverheadEffects.Find( pszEffectName );

	// particle is added already
	if ( index_ != m_mapOverheadEffects.InvalidIndex() )
	{
		if ( bRemoveInstantly )
			ParticleProp()->StopEmissionAndDestroyImmediately( m_mapOverheadEffects[index_] );
		ParticleProp()->StopParticlesNamed( pszEffectName, bRemoveInstantly );
		m_mapOverheadEffects.RemoveAt( index_ );
	}
}


void C_TFPlayer::UpdateOverheadEffects()
{
	if ( IsLocalPlayer() )
		return;

	const int nOverheadEffectCount = m_mapOverheadEffects.Count();
	if ( nOverheadEffectCount == 0 )
		return;

	Vector vecOverheadEffectPosition = GetOverheadEffectPosition();
	C_TFPlayer *pLocalPlayer = GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	Vector vecHeadToHead = EyePosition() - pLocalPlayer->EyePosition();

	const float flEffectGap = 24.f;
	Vector vecRightOffset = CrossProduct( vecHeadToHead, Vector( 0, 0, 1 ) ).Normalized();
	float flFirstEffectOffset = -flEffectGap * 0.5f * ( nOverheadEffectCount - 1 );
	int iValidParticleIndex = 0;
	FOR_EACH_MAP_FAST( m_mapOverheadEffects, i )
	{
		HPARTICLEFFECT hEffect = m_mapOverheadEffects[i];
		if ( hEffect )
		{
			float flCurrentOffset = flFirstEffectOffset + flEffectGap * iValidParticleIndex;
			Vector vecOffset = vecOverheadEffectPosition + flCurrentOffset * vecRightOffset;
			ParticleProp()->AddControlPoint( hEffect, 0, this, PATTACH_ABSORIGIN_FOLLOW, 0, vecOffset );
			iValidParticleIndex++;
		}
	}
}


Vector C_TFPlayer::GetOverheadEffectPosition()
{
	return GetClassEyeHeight() + Vector( 0, 0, 20 );
}


bool C_TFPlayer::AudioStateIsUnderwater( Vector vecMainViewOrigin )
{
	if ( m_Shared.InCond( TF_COND_TRANQUILIZED ) )
		return true;

	return BaseClass::AudioStateIsUnderwater( vecMainViewOrigin );
}


float C_TFPlayer::GetDesaturationAmount( void )
{
	if ( m_Shared.InCond( TF_COND_TRANQUILIZED ) )
	{
		float flTimeLeft = m_Shared.GetConditionDuration( TF_COND_TRANQUILIZED );
		if ( flTimeLeft == PERMANENT_CONDITION )
			return 1.0f;

		return RemapValClamped( flTimeLeft, 1.0f, 0.0f, 1.0f, 0.0f );
	}

	return 0.0f;
}


void C_TFPlayer::CollectVisibleSteamUsers( CUtlVector<CSteamID> &userList )
{
	CSteamID steamID;

	// Add our spectator target.
	C_TFPlayer *pTarget = NULL;

	if ( IsObserver() && GetObserverMode() != OBS_MODE_ROAMING )
	{
		pTarget = ToTFPlayer( GetObserverTarget() );
		if ( pTarget && pTarget->GetSteamID( &steamID ) )
		{
			userList.AddToTail( steamID );
		}
	}

	// Add everyone we can see.
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		C_TFPlayer *pOther = ToTFPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pOther ||
			pOther == this ||
			pOther->IsDormant() ||
			!pOther->IsAlive() ||
			pOther == pTarget ||
			pOther->m_Shared.IsStealthed() ||
			!pOther->GetSteamID( &steamID ) )
			continue;

		// Check that they are on the screen.
		int x, y;
		if ( GetVectorInScreenSpace( pOther->EyePosition(), x, y ) == false )
			continue;

		// Make sure there are no obstructions.
		trace_t tr;
		UTIL_TraceLine( MainViewOrigin(), pOther->EyePosition(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0f )
		{
			userList.AddToTail( steamID );
		}
	}
}


const Vector& C_TFPlayer::GetRenderOrigin( void )
{
	if ( GetPlayerClass()->HasCustomModel() )
	{
		m_vecCustomModelOrigin = BaseClass::GetRenderOrigin() + GetPlayerClass()->GetCustomModelOffset();
		return m_vecCustomModelOrigin;
	}

	return BaseClass::GetRenderOrigin();
}

void C_TFPlayer::AddArrow( C_TFProjectile_Arrow *pArrow )
{
	m_aArrows.AddToTail( pArrow );

	while( !m_aArrows[0] && m_aArrows.Count() )
		m_aArrows.Remove(0);

	while( m_aArrows.Count() > tf2c_pincushion_limit.GetInt() )
	{
		m_aArrows[0]->Release();
		m_aArrows.Remove(0);
	}
}

void C_TFPlayer::RemoveArrow( C_TFProjectile_Arrow *pArrow )
{
	m_aArrows.FindAndRemove( pArrow );
}

//-----------------------------------------------------------------------------
// Purpose: Crash
//-----------------------------------------------------------------------------
static void cc_tf_crashclient()
{
	C_TFPlayer *pPlayer = NULL;
	pPlayer->ComputeFxBlend();
}
static ConCommand tf_crashclient( "tf_crashclient", cc_tf_crashclient, "Crashes this client for testing.", FCVAR_CHEAT );

#include "c_obj_sentrygun.h"


static void cc_tf_debugsentrydmg()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	pPlayer->UpdateIDTarget();
	int iTarget = pPlayer->GetIDTarget();
	if ( iTarget > 0 )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( iTarget );
		C_ObjectSentrygun *pSentry = dynamic_cast<C_ObjectSentrygun *>( pEnt );
		if ( pSentry )
		{
			pSentry->DebugDamageParticles();
		}
	}
}
static ConCommand tf_debugsentrydamage( "tf_debugsentrydamage", cc_tf_debugsentrydmg, "", FCVAR_CHEAT );


vgui::IImage *GetDefaultAvatarImage( int iPlayerIndex )
{
	if ( g_PR && g_PR->IsConnected( iPlayerIndex ) && g_PR->GetTeam( iPlayerIndex ) >= FIRST_GAME_TEAM )
	{
		const char *pszImage = VarArgs( "../vgui/avatar_default_%s", g_aTeamLowerNames[g_PR->GetTeam( iPlayerIndex )] );
		return vgui::scheme()->GetImage( pszImage, true );
	}

	return NULL;
}


/*vgui::IImage *GetMedalImage( int iPlayerIndex )
{
	if ( g_PR && g_PR->IsConnected( iPlayerIndex ) && g_PR->GetTeam( iPlayerIndex ) >= FIRST_GAME_TEAM )
	{
		const char *pszMedalType;
		switch ( g_TF_PR->GetPlayerRank( iPlayerIndex ) )
		{
			case TF_RANK_DEVELOPER:
				pszMedalType = "dev";
				break;
			case TF_RANK_CONTRIBUTOR:
				pszMedalType = "contributor";
				break;
			case TF_RANK_PLAYTESTER:
				pszMedalType = "tester";
				break;
			default:
				return NULL;
		}

		const char *pszImage = VarArgs( "medal_%s_%s", pszMedalType, g_aTeamLowerNames[g_PR->GetTeam( iPlayerIndex )] );
		return vgui::scheme()->GetImage( pszImage, true );
	}

	return NULL;
}*/


C_TFPlayer *GetLocalObservedPlayer( bool bFirstPerson )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return NULL;

	return pLocalPlayer->GetObservedPlayer( bFirstPerson );
}

ConVar tf2c_viewbob( "tf2c_viewbob", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Enables HL1-like camera motions such as head bobbing, sideways roll and idle camera sway." );

//-----------------------------------------------------------------------------
// Purpose: HL1's view bob, roll and idle effects.
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
	BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );

	if ( tf2c_viewbob.GetBool() )
	{
		Vector Velocity;
		EstimateAbsVelocity( Velocity );

		if ( Velocity.Length() == 0 && !m_Shared.InCond( TF_COND_AIMING ) )
		{
			IdleScale += gpGlobals->frametime * 0.05;
			if ( IdleScale > 1.0 )
				IdleScale = 1.0;
		}
		else
		{
			IdleScale -= gpGlobals->frametime;
			if ( IdleScale < 0.0 )
				IdleScale = 0.0;
		}

		CalcViewBob( eyeOrigin );
		CalcViewIdle( eyeAngles );
	}
}

ConVar tf2c_viewbob_rollspeed( "tf2c_viewbob_rollspeed", "300.0", FCVAR_USERINFO | FCVAR_ARCHIVE, "At which move speed to reach max rollangle" );
ConVar tf2c_viewbob_rollangle( "tf2c_viewbob_rollangle", "0.65", FCVAR_USERINFO | FCVAR_ARCHIVE, "1/4 of desired max angle" );

//-----------------------------------------------------------------------------
// Purpose: HL1 view roll camera effect
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcViewRoll( QAngle& eyeAngles )
{
	if ( tf2c_viewbob.GetBool() )
	{
		if ( GetMoveType() == MOVETYPE_NOCLIP )
			return;

		float Side = CalcRoll( GetAbsAngles(), GetAbsVelocity(), tf2c_viewbob_rollangle.GetFloat(), tf2c_viewbob_rollspeed.GetFloat() ) * 4.0;
		eyeAngles[ROLL] += Side;

		if ( GetHealth() <= 0 )
		{
			eyeAngles[ROLL] = 80;
			return;
		}
	}
}

ConVar tf2c_viewbob_bobcycle( "tf2c_viewbob_bobcycle", "0.8", FCVAR_USERINFO | FCVAR_ARCHIVE, "Seconds to complete one view bobbing cycle" );
ConVar tf2c_viewbob_bob( "tf2c_viewbob_bob", "0.01", FCVAR_USERINFO | FCVAR_ARCHIVE );
ConVar tf2c_viewbob_bobup( "tf2c_viewbob_bobup", "0.5", FCVAR_USERINFO | FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Purpose: HL1 view bob camera effect
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcViewBob( Vector& eyeOrigin )
{
	float Cycle;
	Vector Velocity;

	if ( GetGroundEntity() == nullptr || gpGlobals->curtime == BobLastTime || m_Shared.InCond( TF_COND_AIMING ) || tf2c_viewbob_bobcycle.GetFloat() <= 0.0f )
	{
		eyeOrigin.z += ViewBob;
		return;
	}

	BobLastTime = gpGlobals->curtime;
	BobTime += gpGlobals->frametime;

	Cycle = BobTime - (int)( BobTime / tf2c_viewbob_bobcycle.GetFloat() ) * tf2c_viewbob_bobcycle.GetFloat();
	Cycle /= tf2c_viewbob_bobcycle.GetFloat();

	if ( Cycle < tf2c_viewbob_bobup.GetFloat() )
		Cycle = M_PI * Cycle / tf2c_viewbob_bobup.GetFloat();
	else
		Cycle = M_PI + M_PI * ( Cycle - tf2c_viewbob_bobup.GetFloat() ) / ( 1.0 - tf2c_viewbob_bobup.GetFloat() );

	EstimateAbsVelocity( Velocity );
	Velocity.z = 0;

	ViewBob = sqrt( Velocity.x * Velocity.x + Velocity.y * Velocity.y ) * tf2c_viewbob_bob.GetFloat();
	ViewBob = ViewBob * 0.3 + ViewBob * 0.7 * sin( Cycle );
	ViewBob = min( ViewBob, 4 );
	ViewBob = max( ViewBob, -7 );

	eyeOrigin.z += ViewBob;
}

ConVar tf2c_viewbob_iyaw_cycle( "tf2c_viewbob_iyaw_cycle", "2.0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Seconds to complete one idle cam yaw cycle" );
ConVar tf2c_viewbob_iroll_cycle( "tf2c_viewbob_iroll_cycle", "0.5", FCVAR_USERINFO | FCVAR_ARCHIVE, "Seconds to complete one idle cam roll cycle" );
ConVar tf2c_viewbob_ipitch_cycle( "tf2c_viewbob_ipitch_cycle", "1.0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Seconds to complete one idle cam pitch cycle" );
ConVar tf2c_viewbob_iyaw_level( "tf2c_viewbob_iyaw_level", "0.3", FCVAR_USERINFO | FCVAR_ARCHIVE, "Idle cam yaw (look left/right) strength" );
ConVar tf2c_viewbob_iroll_level( "tf2c_viewbob_iroll_level", "0.1", FCVAR_USERINFO | FCVAR_ARCHIVE, "Idle cam roll (rotate view left/right) strength" );
ConVar tf2c_viewbob_ipitch_level( "tf2c_viewbob_ipitch_level", "0.3", FCVAR_USERINFO | FCVAR_ARCHIVE, "Idle cam pitch (look up/down) strength" );

//-----------------------------------------------------------------------------
// Purpose: HL1 idle camera effect
//-----------------------------------------------------------------------------
void C_TFPlayer::CalcViewIdle( QAngle& eyeAngles )
{
	eyeAngles[ROLL] += IdleScale * sin( gpGlobals->curtime * tf2c_viewbob_iroll_cycle.GetFloat() ) * tf2c_viewbob_iroll_level.GetFloat();
	eyeAngles[PITCH] += IdleScale * sin( gpGlobals->curtime * tf2c_viewbob_ipitch_cycle.GetFloat() ) * tf2c_viewbob_ipitch_level.GetFloat();
	eyeAngles[YAW] += IdleScale * sin( gpGlobals->curtime * tf2c_viewbob_iyaw_cycle.GetFloat() ) * tf2c_viewbob_iyaw_level.GetFloat();
}


CON_COMMAND( tf2c_spec_playerinfo, "Shows some information about the spectated player." )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || !pLocalPlayer->IsObserver() )
		return;

	C_TFPlayer *pPlayer = ToTFPlayer( pLocalPlayer->GetObserverTarget() );
	if ( !pPlayer )
		return;

	Msg( "Name: %s\nFOV: %d\nViewmodel FOV: %g\nViewmodel offset: %g, %g, %g\n",
		pPlayer->GetPlayerName(),
		pPlayer->GetDefaultFOV(),
		pPlayer->GetViewModelFOV(),
		pPlayer->GetViewModelOffset().x, pPlayer->GetViewModelOffset().y, pPlayer->GetViewModelOffset().z );
}

void C_TFPlayer::DrawBBoxVisualizations(void)
{
	// Call base to draw any boxes that all entities have
	C_BaseEntity::DrawBBoxVisualizations();

	// Now draw the projectile headshot level, if applicable.
	if (m_fBBoxVisFlags & VISUALIZE_HEADSHOT_LEVEL)
	{
		float fHeadshotLevel = CTFPlayerShared::HeadshotFractionForClass( GetPlayerClass()->GetClassIndex() );
		
		Vector vecCollisionMins, vecCollisionMaxs;
		CollisionProp()->WorldSpaceSurroundingBounds(&vecCollisionMins, &vecCollisionMaxs);

		float flHeight = abs(vecCollisionMins.z - vecCollisionMaxs.z);
		float flBoxBottom = vecCollisionMins.z;
		float flMinZForHead = flBoxBottom + ((1.0f - clamp(fHeadshotLevel, 0.0f, 1.0f)) * flHeight);

		Vector vecBoxMins = vecCollisionMins - Vector(0.1, 0.1, 0.1);
		Vector vecBoxMaxs = vecCollisionMaxs + Vector(0.1, 0.1, 0.1);
		vecBoxMins.z = flMinZForHead;

		debugoverlay->AddBoxOverlay(vec3_origin, vecBoxMins,
			vecBoxMaxs, vec3_angle, 255, 255, 255, 0, 0.01);
	}

	if (m_fBBoxVisFlags & VISUALIZE_NEW_PROJECTILE_COLLIDERS)
	{
		//debugoverlay->Add
		//NDebugOverlay::Circle(vec3_origin, 8, 0, 255, 0, false, TF_SHIELD_DEBUG_TIME);
		Vector vecCollisionMins, vecCollisionMaxs;
		CollisionProp()->WorldSpaceSurroundingBounds(&vecCollisionMins, &vecCollisionMaxs);
		float flHeight = abs(vecCollisionMins.z - vecCollisionMaxs.z);

		const int nCircles = 16;
		const float flHeightOverCircles = flHeight / (float)nCircles;
		Vector iter = Vector(0, 0, flHeightOverCircles);
		
		QAngle playerRot = GetAbsAngles();
		Vector playerUp, playerRight, playerForward;
		AngleVectors(playerRot, &playerForward, &playerRight, &playerUp);
		QAngle circleRot;
		VectorAngles(playerUp, playerForward, circleRot);
		const float radius = m_Shared.ProjetileHurtCylinderForClass(m_Shared.ProjetileHurtCylinderForClass(GetPlayerClass()->GetClassIndex()));
		for (float i = 0.0f; i < nCircles; i++) {
			NDebugOverlay::Circle(CollisionProp()->WorldSpaceCenter() - Vector(0, 0, flHeight/2.0f) + (iter*i), circleRot, radius, 255, 255, 255, 0, true, 0.01);
		}
		//NDebugOverlay::
	}
}


void C_TFPlayer::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	BaseClass::NotifyShouldTransmit( state );

	if ( state == SHOULDTRANSMIT_START )
	{
		// Ensure we are visible
		UpdateVisibility();
	}
	else if ( state == SHOULDTRANSMIT_END )
	{
		// Nothing
	}
}



bool C_TFPlayer::ShouldAnnounceAchievement(void)
{
	if (C_BasePlayer::ShouldAnnounceAchievement())
	{
		if (IsEnemyPlayer() && (m_Shared.IsStealthed() || m_Shared.IsDisguised()))
		{
			return false;
		}
		return true;
	}
	return false;
}