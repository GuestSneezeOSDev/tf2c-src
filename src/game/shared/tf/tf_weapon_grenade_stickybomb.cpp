//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Pipebomb Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weapon_grenade_stickybomb.h"
#include "tf_weapon_pipebomblauncher.h"
#include "tf_gamerules.h"

#ifdef GAME_DLL
#include "te_effect_dispatch.h"
#include "props.h"
#include "tf_player.h"
#include "tf_team.h"
#include "tf_obj.h"
#include "entity_healthkit.h"
#include "achievements_tf.h"
#include "tf_gamestats.h"
#else
#include "c_tf_player.h"
#include "glow_outline_effect.h"
#include "functionproxy.h"
#endif

#define TF_WEAPON_STICKYBOMB_MODEL		"models/weapons/w_models/w_stickybomb.mdl"
#define TF_WEAPON_PROXYBOMB_MODEL		"models/weapons/w_models/w_minebomb.mdl"
#define TF_WEAPON_PROXYBOMB_ARMTIME 0.8f
#define TF_WEAPON_PROXYBOMB_DELAYTIME 0.5f
#define TF_WEAPON_PROXYBOMB_DELAYTIME_ENEMY 0.4f

#define TF_WEAPON_PIPEBOMB_ARMTIME 0.8f

#define TF_WEAPON_BOMB_FULL_ARMTIME 5.0f

// ConVar tf_grenadelauncher_livetime( "tf_grenadelauncher_livetime", "0.8", FCVAR_CHEAT | FCVAR_REPLICATED );

ConVar tf_grenade_force_sleeptime( "tf_grenade_force_sleeptime", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED );	// How long after being shot will we re-stick to the world.

extern ConVar tf_grenade_forcefrom_blast;
extern ConVar tf_pipebomb_force_to_move;

ConVar tf2c_sticky_rampup_mindmg( "tf2c_sticky_rampup_mindmg", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Initial stickybomb damage factor" );
ConVar tf2c_sticky_rampup_time( "tf2c_sticky_rampup_time", "2.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Seconds after arm time to ramp up to full damage" );
ConVar tf2c_sticky_touch_fix( "tf2c_sticky_touch_fix", "1", FCVAR_REPLICATED, "0 = off, 1 = stickybombs attach to static func_brush and func_breakable, 2 = also attach to static func_door" );
ConVar tf2c_sticky_rampup_minradius( "tf2c_sticky_rampup_minradius", "0.85", FCVAR_CHEAT | FCVAR_REPLICATED, "Initial stickybomb radius factor" );

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Used to make stickybomb glow with color of its owner.
//-----------------------------------------------------------------------------
class CProxyStickyGlow : public CResultProxy
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

EXPOSE_INTERFACE( CProxyStickyGlow, CResultProxy, "StickyGlowColor" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeStickybombProjectile, DT_TFProjecile_Stickybomb )

BEGIN_NETWORK_TABLE( CTFGrenadeStickybombProjectile, DT_TFProjecile_Stickybomb )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flCreationTime ) ),
	RecvPropTime( RECVINFO( m_flTouchedTime ) ),
	RecvPropBool( RECVINFO( m_bProxyMine ) ),
#else
	SendPropTime( SENDINFO( m_flCreationTime ) ),
	SendPropTime( SENDINFO( m_flTouchedTime ) ),
	SendPropBool( SENDINFO( m_bProxyMine ) ),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_projectile_pipe_remote, CTFGrenadeStickybombProjectile );
PRECACHE_REGISTER( tf_projectile_pipe_remote );

CTFGrenadeStickybombProjectile::CTFGrenadeStickybombProjectile()
{
#ifdef GAME_DLL
	m_bTouched = false;
#else
	m_bPulsed = false;
	m_bPulsed2 = false;
	m_pGlowEffect = NULL;
#endif
}

CTFGrenadeStickybombProjectile::~CTFGrenadeStickybombProjectile()
{
#ifdef CLIENT_DLL
	ParticleProp()->StopEmission();

	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
#endif
}


void CTFGrenadeStickybombProjectile::UpdateOnRemove( void )
{
	// Tell our launcher that we were removed.
	CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher *>( m_hLauncher.Get() );
	if ( pLauncher )
	{
		pLauncher->DeathNotice( this );
	}

	BaseClass::UpdateOnRemove();
}

#ifdef CLIENT_DLL
//=============================================================================
//
// TF Stickybomb Projectile functions (Client specific).
//


void CTFGrenadeStickybombProjectile::CreateTrails( void )
{
	const char *pszEffect = GetProjectileParticleName( "stickybombtrail_%s", m_hLauncher );
	ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN_FOLLOW );

	if ( m_bCritical )
	{
		const char *pszEffectName = GetProjectileParticleName( "critical_grenade_%s", m_hLauncher, m_bCritical );
		ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );
	}
}


void CTFGrenadeStickybombProjectile::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher *>( m_hLauncher.Get() );
		if ( pLauncher )
		{
			pLauncher->AddPipeBomb( this );
		}

		CreateTrails();

		if ( GetOwnerEntity() && C_BasePlayer::GetLocalPlayer() == GetOwnerEntity() )
		{
			int iIsProxyBomb = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( pLauncher, iIsProxyBomb, mod_sticky_is_proxy );
			if ( iIsProxyBomb )
			{
				UpdateGlowEffect();
			}
		}
	}

#if 0
	if ( m_iOldTeamNum && m_iOldTeamNum != m_iTeamNum )
	{
		ParticleProp()->StopEmission();
		CreateTrails();
	}
#endif
}


void CTFGrenadeStickybombProjectile::UpdateGlowEffect( void )
{
	if ( !m_pGlowEffect )
	{
		m_pGlowEffect = new CGlowObject( this, vec3_origin, 1.0f, true, true );
	}

	Vector vecColor;
	TFGameRules()->GetTeamGlowColor( GetTeamNumber(), vecColor.x, vecColor.y, vecColor.z );
	m_pGlowEffect->SetColor( vecColor );
}


void CTFGrenadeStickybombProjectile::Simulate( void )
{
	BaseClass::Simulate();

	if ( !m_bPulsed )
	{
		if ( ( gpGlobals->curtime - m_flCreationTime ) >= TF_WEAPON_PIPEBOMB_ARMTIME )
		{
			const char *pszEffectName = ConstructTeamParticle( "stickybomb_pulse_%s", GetTeamNumber() );
			ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW );

			m_bPulsed = true;
		}
	}

	if (!m_bPulsed2)
	{
		if ((gpGlobals->curtime - m_flCreationTime) >= TF_WEAPON_BOMB_FULL_ARMTIME)
		{
			const char* pszEffectName = ConstructTeamParticle("stickybomb_pulse_%s", GetTeamNumber());
			ParticleProp()->Create(pszEffectName, PATTACH_ABSORIGIN_FOLLOW);

			m_bPulsed2 = true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw if we haven't yet gone past our original spawn point
// Input  : flags - 
//-----------------------------------------------------------------------------
int CTFGrenadeStickybombProjectile::DrawModel( int flags )
{
	if ( gpGlobals->curtime < ( m_flCreationTime + 0.1 ) )
		return 0;

	return BaseClass::DrawModel( flags );
}
#else
BEGIN_DATADESC( CTFGrenadeStickybombProjectile )
END_DATADESC()


CTFGrenadeStickybombProjectile *CTFGrenadeStickybombProjectile::Create( const Vector &position, const QAngle &angles,
	const Vector &velocity, const AngularImpulse &angVelocity,
	CBaseEntity *pOwner, CBaseEntity *pWeapon )
{
	return static_cast<CTFGrenadeStickybombProjectile *>( CTFBaseGrenade::Create( "tf_projectile_pipe_remote",
		position, angles, velocity, angVelocity, pOwner, pWeapon ) );
}


void CTFGrenadeStickybombProjectile::Spawn( void )
{
	// Set this to max, so effectively they do not self-implode.
	SetModel( TF_WEAPON_STICKYBOMB_MODEL );
	SetDetonateTimerLength( FLT_MAX );
	SetTouch( NULL );

	BaseClass::Spawn();

	m_flCreationTime = gpGlobals->curtime;
	m_flTouchedTime = FLT_MAX;
	m_flMinSleepTime = 0.0f;

	// Double bbox for bullet hit detection
	UTIL_SetSize( this, Vector( -4.0f, -4.0f, -4.0f ), Vector( 4.0f, 4.0f, 4.0f ) );

	AddSolidFlags( FSOLID_TRIGGER );
}


void CTFGrenadeStickybombProjectile::Precache( void )
{
	int iModel = PrecacheModel( TF_WEAPON_STICKYBOMB_MODEL );
	PrecacheGibsForModel( iModel );
	iModel = PrecacheModel( TF_WEAPON_PROXYBOMB_MODEL );
	PrecacheGibsForModel( iModel );
	PrecacheTeamParticles( "stickybombtrail_%s" );
	PrecacheTeamParticles( "stickybomb_pulse_%s" );
	PrecacheTeamParticles( "proxymine_pulse_%s" );

	PrecacheScriptSound( "Weapon_ProxyBomb.LeadIn" );
	PrecacheScriptSound( "Weapon_ProxyBomb.Timer" );

	BaseClass::Precache();
}


void CTFGrenadeStickybombProjectile::BlipSound( void )
{
	int iNoSound = 0;
	CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher *>( m_hLauncher.Get() );
	if ( pLauncher )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, iNoSound, mod_proxymine_no_sound );
	}
	if ( iNoSound != 0 )
		return;

	if ( m_bProxyMine && GetDetonateTime() == FLT_MAX && gpGlobals->curtime - m_flTouchedTime >= TF_WEAPON_PROXYBOMB_ARMTIME )
	{
		if ( gpGlobals->curtime >= m_flNextBlipTime )
		{
			// Filter sound parameters by team. Enemies need to hear, teammates not as much.
			EmitSound_t sound;
			sound.m_pSoundName = "Weapon_ProxyBomb.Timer";
			sound.m_nFlags = SND_CHANGE_PITCH;

			for ( int iTeam = TEAM_UNASSIGNED; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				// Pitchshift to communicate mine's team to enemies
				// 94, 98, 102, 106
				sound.m_nPitch = 86 + GetTeamNumber() * 4;

				if ( iTeam == GetTeamNumber() )
				{
					sound.m_nFlags |= SND_CHANGE_VOL;
					sound.m_flVolume = 0.6f;
				}
				else
				{
					sound.m_flVolume = VOL_NORM;
				}

				CTeamRecipientFilter teamFilter( iTeam, true );
				EmitSound( teamFilter, entindex(), sound );
			}

			const char *pszEffect = ConstructTeamParticle( "proxymine_pulse_%s", GetTeamNumber() );
			DispatchParticleEffect( pszEffect, PATTACH_ABSORIGIN_FOLLOW, this );

			m_flNextBlipTime = gpGlobals->curtime + 2.0f;
		}
	}
}


void CTFGrenadeStickybombProjectile::DetonateThink( void )
{
	BaseClass::DetonateThink();

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	// Detach if the entity we touched suddenly starts moving.
	// This handles e.g. func_doors that are static for
	// most of the round, but open or close at a specific time.
	// Also detach if it disappears (e.g. breakables).
	if ( m_bTouched )
	{
		if ( !GetGroundEntity() || ( GetGroundEntity() && GetGroundEntity()->IsMoving() ) )
		{
			if ( pPhysicsObject )
			{
				pPhysicsObject->EnableMotion( true );
				pPhysicsObject->Wake();

				Vector vecForce;
				vecForce = RandomVector( -500, 500 );
				pPhysicsObject->ApplyForceCenter( vecForce );
			}

			m_flMinSleepTime = gpGlobals->curtime + tf_grenade_force_sleeptime.GetFloat();
			m_bTouched = false;
		}
	}

	// Proximity mine radius check.
	if ( m_bProxyMine && GetDetonateTime() == FLT_MAX && ( gpGlobals->curtime - m_flTouchedTime ) >= TF_WEAPON_PROXYBOMB_ARMTIME )
	{
		CBaseEntity *pTarget = FindProxyTarget();
		if ( pTarget )
		{
			// Play one sound to everyone but the owner.
			CPASFilter filter( GetAbsOrigin() );

			// Always play sound to owner
			CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
			if ( pOwner )
			{
				Vector covector = (GetAbsOrigin() - pOwner->EyePosition());
				float flMaxDist = covector.Length();
				Vector ownerEarPos = pOwner->EyePosition() + covector.Normalized() * min( 128.0f, flMaxDist );
				CSingleUserRecipientFilter singleFilter( pOwner );
				EmitSound( singleFilter, -1, "Weapon_ProxyBomb.LeadIn", &ownerEarPos );
			}
			
			// Play to everyone else
			filter.RemoveRecipient( pOwner );
			EmitSound( filter, entindex(), "Weapon_ProxyBomb.LeadIn" );

			float flDelayTime = TF_WEAPON_PROXYBOMB_DELAYTIME;
			// Detonate faster for enemies
			if ( pTarget != pOwner )
			{
				flDelayTime = TF_WEAPON_PROXYBOMB_DELAYTIME_ENEMY;
			}

			CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher *>( m_hLauncher.Get() );
			if ( pLauncher )
			{
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, flDelayTime, mult_proxymine_detonate_time );
			}
			SetDetonateTimerLength( flDelayTime );

			m_pProxyTarget = pTarget;
		}
	}
}

//-----------------------------------------------------------------------------
// Look for a target
//-----------------------------------------------------------------------------
CBaseEntity* CTFGrenadeStickybombProjectile::FindProxyTarget()
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( !pOwner )
		return NULL;

	if ( pOwner && pOwner->m_Shared.IsLoser() && !pOwner->m_Shared.IsLoserStateStunned() )
		return NULL;

	float flRadius2 = Square( GetDamageRadius() * 0.8f );

	CTFPipebombLauncher *pLauncher = dynamic_cast<CTFPipebombLauncher *>( m_hLauncher.Get() );
	if ( pLauncher )
	{
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, flRadius2, mult_proxymine_radius );
	}

	Vector vecSegment;
	Vector vecTargetPoint;
	CBaseEntity *pTarget = NULL;
	float flDist2 = 0;

	// Check owner first
	// Crouching doesn't activate mines
	int iProxyIgnoreCrouchingOwner = 0;
	if ( pLauncher )
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, iProxyIgnoreCrouchingOwner, mod_proxymine_crouchwalk_ignore_owner );

	bool bIsCrouchwalking = ( pOwner->GetFlags() & FL_DUCKING ) && ( pOwner->GetFlags() & FL_ONGROUND );

	if ( !iProxyIgnoreCrouchingOwner || ( iProxyIgnoreCrouchingOwner && !bIsCrouchwalking ) )
	{
		vecTargetPoint = pOwner->GetAbsOrigin();
		vecTargetPoint += pOwner->GetViewOffset();
		VectorSubtract( vecTargetPoint, GetAbsOrigin(), vecSegment );
		flDist2 = vecSegment.LengthSqr();
		if ( flDist2 <= flRadius2 )
		{
			trace_t	tr;
			UTIL_TraceLine( GetAbsOrigin(), vecTargetPoint, MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr );
			if ( tr.fraction >= 1.0f )
			{
				pTarget = pOwner;
				return pTarget;
			}
		}
	}

	int i, c;

	int iProxyIgnoreCrouching = 0;
	if ( pLauncher )
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, iProxyIgnoreCrouching, mod_proxymine_crouchwalk_ignore );

	// Loop through enemy players.
	ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			return true;

		for ( i = 0, c = pTeam->GetNumPlayers(); i < c; ++i )
		{
			CTFPlayer *pPlayer = pTeam->GetTFPlayer( i );
			if ( !pPlayer )
				continue;
			
			if ( !pPlayer->IsAlive() )
				continue;

			if ( pPlayer->GetFlags() & FL_NOTARGET )
				continue;

			if ( pPlayer->m_Shared.IsStealthed() )
				continue;

			if ( pPlayer->m_Shared.IsDisguised() &&
				pPlayer->m_Shared.DisguiseFoolsTeam( pOwner->GetTeamNumber() ) )
				continue;

			// Crouching doesn't activate mines
			if ( iProxyIgnoreCrouching &&
				pPlayer->GetFlags() & FL_DUCKING &&
				pPlayer->GetFlags() & FL_ONGROUND )
				continue;

			vecTargetPoint = pPlayer->GetAbsOrigin();
			vecTargetPoint += pPlayer->GetViewOffset();
			VectorSubtract( vecTargetPoint, GetAbsOrigin(), vecSegment );
			flDist2 = vecSegment.LengthSqr();
			if ( flDist2 <= flRadius2 )
			{
				trace_t	tr;
				UTIL_TraceLine( GetAbsOrigin(), vecTargetPoint, MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr );
				if ( tr.fraction >= 1.0f )
				{
					pTarget = pPlayer;
					return true;
				}
			}
		}

		return true;
	} );

	// Loop through enemy objects.
	ForEachEnemyTFTeam( GetTeamNumber(), [&]( int iTeam )
	{
		CTFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			return true;

		for ( i = 0, c = pTeam->GetNumObjects(); i < c; ++i )
		{
			CBaseObject *pObject = pTeam->GetObject( i );
			if ( !pObject )
				continue;

			if ( pObject->IsPlacing() )
				continue;

			// Ignore sappers & payload carts.
			if ( pObject->MustBeBuiltOnAttachmentPoint() )
				continue;

			if ( pObject->GetObjectFlags() & OF_IS_CART_OBJECT )
				continue;

			vecTargetPoint = pObject->GetAbsOrigin();
			vecTargetPoint += pObject->GetViewOffset();
			VectorSubtract( vecTargetPoint, GetAbsOrigin(), vecSegment );
			flDist2 = vecSegment.LengthSqr();
			if ( flDist2 <= flRadius2 )
			{
				trace_t	tr;
				UTIL_TraceLine( GetAbsOrigin(), vecTargetPoint, MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr );
				if ( tr.fraction >= 1.0f )
				{
					pTarget = pObject;
					return true;
				}
			}
		}

		return true;
	} );

	// That's right, you can bait it with Sandvich now
	for ( auto pHealthKit : CHealthKit::AutoList() )
	{
		CTFPlayer *pOwner = ToTFPlayer( pHealthKit->GetOwnerEntity() );
		if ( pOwner )
		{
			if ( !pOwner->IsEnemy( this ) )
			{
				continue;
			}

			vecTargetPoint = pHealthKit->GetAbsOrigin();
			vecTargetPoint += pHealthKit->GetViewOffset();
			VectorSubtract( vecTargetPoint, GetAbsOrigin(), vecSegment );
			flDist2 = vecSegment.LengthSqr();
			if ( flDist2 <= flRadius2 )
			{
				trace_t	tr;
				UTIL_TraceLine( GetAbsOrigin(), vecTargetPoint, MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr );
				if ( tr.fraction >= 1.0f )
				{
					pTarget = pHealthKit;
					break;
				}
			}
		}
	}

	// Found target, actually return outside lambdas.
	if ( pTarget )
		return pTarget;

	return nullptr;
}


int	CTFGrenadeStickybombProjectile::GetDamageType( void ) const
{
	int iDmgType = BaseClass::GetDamageType();

	// Do distance based damage falloff for just the first few seconds of our life.
	if ( ( gpGlobals->curtime - m_flCreationTime ) < TF_WEAPON_BOMB_FULL_ARMTIME)
	{
		iDmgType |= DMG_USEDISTANCEMOD;
	}

	return iDmgType;
}


float CTFGrenadeStickybombProjectile::GetDamage( void )
{
	m_flDamage = BaseClass::GetDamage();

	// Do time based damage rampup for just the first few seconds of our life.
	// Time spent in the chamber, i.e. time spent charging, counts towards this
	if ( gpGlobals->curtime - m_flCreationTime >= TF_WEAPON_PIPEBOMB_ARMTIME )
	{
		m_flDamage *= RemapValClamped( ( gpGlobals->curtime - m_flChargeBeginTime ),
			TF_WEAPON_PIPEBOMB_ARMTIME,
			TF_WEAPON_PIPEBOMB_ARMTIME + tf2c_sticky_rampup_time.GetFloat(),
			tf2c_sticky_rampup_mindmg.GetFloat(),
			1.0f );
	}
	else
	{
		m_flDamage *= tf2c_sticky_rampup_mindmg.GetFloat();
	}

	return m_flDamage;
}


float CTFGrenadeStickybombProjectile::GetDamageRadius( void )
{
	// Do time based radius rampup for just the first few seconds of our life.
	// Time spent in the chamber, i.e. time spent charging, does NOT count towards this
	// because we're mimicking live TF2

	float flDamageRadius = BaseClass::GetDamageRadius();

	if ( m_bTouched == false )
	{
		flDamageRadius *= RemapValClamped( ( gpGlobals->curtime - m_flCreationTime ),
			TF_WEAPON_PIPEBOMB_ARMTIME,
			TF_WEAPON_PIPEBOMB_ARMTIME + tf2c_sticky_rampup_time.GetFloat(),
			tf2c_sticky_rampup_minradius.GetFloat(),
			1.0f );
	}

	return flDamageRadius;
}


int CTFGrenadeStickybombProjectile::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}


int CTFGrenadeStickybombProjectile::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	// Always transmit to the player who fired it.
	if ( GetOwnerEntity() && pInfo->m_pClientEnt == GetOwnerEntity()->edict() )
		return FL_EDICT_ALWAYS;

	return BaseClass::ShouldTransmit( pInfo );
}


void CTFGrenadeStickybombProjectile::Detonate()
{
	// If we're detonating stickies then we're currently inside prediction
	// so we gotta make sure all effects show up.
	CDisablePredictionFiltering disabler;

	if ( ShouldNotDetonate() )
	{
		RemoveGrenade();
		return;
	}

	if ( m_bFizzle )
	{
		CEffectData data;
		data.m_vOrigin = GetAbsOrigin();
		data.m_vAngles = GetAbsAngles();
		data.m_nMaterial = GetModelIndex();
		
		DispatchEffect( "BreakModel", data );

		RemoveGrenade();
		return;
	}

	BaseClass::Detonate();
}


void CTFGrenadeStickybombProjectile::Fizzle( void )
{
	m_bFizzle = true;
}


void CTFGrenadeStickybombProjectile::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	BaseClass::VPhysicsCollision( index, pEvent );

	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if ( !pHitEntity )
		return;

	if ( PropDynamic_CollidesWithGrenades( pHitEntity ) )
		return;

	// Handle hitting skybox (disappear).
	surfacedata_t *pprops = physprops->GetSurfaceData( pEvent->surfaceProps[otherIndex] );
	if ( pprops->game.material == 'X' )
	{
		// uncomment to destroy grenade upon hitting sky brush
		//SetThink( &CTFGrenadePipebombProjectile::SUB_Remove );
		//SetNextThink( gpGlobals->curtime );
		return;
	}

	// Adding these because maps can use them as essentially static geometry, so we'll bounce off if they move.
	// Beats having to manually check for the model name of sawblades...
	bool bIsDynamicProp = ( dynamic_cast<CDynamicProp *>( pHitEntity ) != NULL );
	bool bIsFuncBrush = false;
	bool bIsBreakable = false;
	bool bIsDoor = false;

	if ( tf2c_sticky_touch_fix.GetInt() > 0 )
	{
		bIsFuncBrush = ( dynamic_cast<CFuncBrush *>( pHitEntity ) != NULL );
		bIsBreakable = ( FClassnameIs( pHitEntity, "func_breakable" ) );
		if ( tf2c_sticky_touch_fix.GetInt() > 1 )
		{
			bIsDoor = ( dynamic_cast<CBaseDoor *>( pHitEntity ) != NULL );
		}
	}

	// func_rotating doesn't let us access its speed and doesn't give angular velocity for children so just bounce off...
	CBaseEntity* pParent = pHitEntity->GetRootMoveParent();
	if ( pParent && FClassnameIs( pParent, "func_rotating" ) )
		return;

	bool bDontStick = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), bDontStick, stickies_dont_stick);

	// Sitckybombs stick to the world when they touch it.
	if ( !bDontStick && !pHitEntity->IsMoving() && ( pHitEntity->IsWorld() || bIsDynamicProp || bIsFuncBrush || bIsBreakable || bIsDoor ) && gpGlobals->curtime > m_flMinSleepTime )
	{
		m_bTouched = true;
		m_flTouchedTime = gpGlobals->curtime;

		IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->EnableMotion( false );
		}

		SetGroundEntity( pHitEntity );

		// Save impact data for explosions.
		m_bUseImpactNormal = true;
		pEvent->pInternalData->GetSurfaceNormal( m_vecImpactNormal );
		m_vecImpactNormal.Negate();

		// Proxy mines align to hit surfaces.
		if ( m_bProxyMine )
		{
			QAngle vAngles;
			VectorAngles( m_vecImpactNormal, vAngles );
			vAngles.x += 90.0f; // Compensate for mesh angle.

			Vector vecPos;
			pEvent->pInternalData->GetContactPoint( vecPos );

			// Fix cases of bad placement angle on edges or corners.
			trace_t	tr;
			UTIL_TraceLine( GetAbsOrigin(), vecPos, MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr );
			
			if ( tr.fraction < 1.0f && pHitEntity->IsWorld() )
			{
				// If we are inside an object, we also point in the opposite direction for some reason.
				// Do this easy fix instead of doing vector math with behind the scenes vphysics.
				vAngles.x += 180.0f;
				vecPos = vecPos + ( m_vecImpactNormal * -2 ); // Pull out in opposite direction.
			}
			else
			{
				vecPos = vecPos + ( m_vecImpactNormal * 2 ); // Pull out as normal.
			}

			if ( pPhysicsObject )
			{
				pPhysicsObject->SetPosition( vecPos, vAngles, true );
			}

			// Generous bbox for bullet hit detection (collision mesh does precise check)
			UTIL_SetSize( this, Vector( -8.0f, -8.0f, -8.0f ), Vector( 8.0f, 8.0f, 8.0f ) );
		}
	}
}


int CTFGrenadeStickybombProjectile::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( !m_takedamage )
		return 0;

	CBaseEntity *pAttacker = info.GetAttacker();
	Assert( pAttacker );
	if ( !pAttacker )
		return 0;

	if ( m_bTouched && IsEnemy( info.GetAttacker() ) )
	{
		// Sticky bombs get destroyed by bullets and melee, not pushed.
		if ( info.GetDamageType() & ( DMG_BULLET | DMG_CLUB | DMG_SLASH ) )
		{
			CTFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
			if (pTFAttacker)
			{
				if (m_bCritical)
				{
					CTF_GameStats.Event_PlayerBlockedDamage(pTFAttacker, GetDamage()); // only 1/3 but then it's crit so *3 too
					CTF_GameStats.Event_PlayerAwardBonusPoints(pTFAttacker, this, 1);
				}
				else
				{
					CTF_GameStats.Event_PlayerBlockedDamage(pTFAttacker, GetDamage() / 3.0f); // Deliver only 1/3 because it's potential protection with low chance
				}
				// TF2C_ACHIEVEMENT_STICKY_MINES_REMOVAL
				if (pTFAttacker->IsPlayerClass(TF_CLASS_HEAVYWEAPONS))
				{
					//pTFAttacker->AwardAchievement(TF2C_ACHIEVEMENT_STICKY_MINES_REMOVAL);
				}
			}

			// Proxy mines explode when shot?
			int iExplodeWhenShot = 0;
			CTFPipebombLauncher* pLauncher = dynamic_cast<CTFPipebombLauncher*>( m_hLauncher.Get() );
			if ( pLauncher )
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pLauncher, iExplodeWhenShot, mod_sticky_explode_when_shot );
			
			if ( !iExplodeWhenShot )
			{
				Fizzle();

				if ( pTFAttacker )
				{
					CPASFilter filter( GetAbsOrigin() );
					EmitSound( filter, 0, "Plastic_Box.Break" );

					EmitSound_t params;
					params.m_pSoundName = "Plastic_Box.Break";
					filter.RemoveRecipient( pTFAttacker );
					EmitSound( filter, 0, params );

					CSingleUserRecipientFilter attackerFilter( pTFAttacker );
					EmitSound( attackerFilter, pTFAttacker->entindex(), params );
				}
			}
			Detonate();
			return 1;
		}

		if ( info.GetDamageType() & DMG_BLAST )
		{
			// If the force is sufficient, detach & move the sticky.
			Vector vecForce = info.GetDamageForce() * tf_grenade_forcefrom_blast.GetFloat();
			float flMultPushToStickies = 1.0f;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(info.GetWeapon(), flMultPushToStickies, mult_dmg_push_force_to_stickies);
			vecForce *= flMultPushToStickies;
			if (vecForce.IsLengthGreaterThan(tf_pipebomb_force_to_move.GetFloat()))
			{
				if ( VPhysicsGetObject() )
				{
					VPhysicsGetObject()->EnableMotion( true );
				}

				CTakeDamageInfo newInfo = info;
				newInfo.SetDamageForce( vecForce );

				VPhysicsTakeDamage( newInfo );

				// The bomb will re-stick to the ground after this time expires.
				m_flMinSleepTime = gpGlobals->curtime + tf_grenade_force_sleeptime.GetFloat();
				// Un-setting m_bTouched here caused sliding bombs (especially mines) to never get it set back to True,
				// keeping them invincible on the ground despite being stationary
				// m_bTouched = false;

				// It has moved the data is no longer valid.
				m_bUseImpactNormal = false;
				m_vecImpactNormal.Init();

				return 1;
			}
		}
	}

	return 0;
}


void CTFGrenadeStickybombProjectile::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	BaseClass::Deflected( pDeflectedBy, vecDir );

	// Force this momentarily so repeated airblasts work more consistently.
	m_bTouched = true;

	// This is kind of lame.
	Vector vecPushSrc = pDeflectedBy->WorldSpaceCenter();
	Vector vecPushDir = GetAbsOrigin() - vecPushSrc;

	CTakeDamageInfo info( pDeflectedBy, pDeflectedBy, 100, DMG_BLAST );
	CalculateExplosiveDamageForce( &info, vecPushDir, vecPushSrc );
	TakeDamage( info );
}


int CTFGrenadeStickybombProjectile::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		char tempstr[512];

		V_sprintf_safe( tempstr, "Touched: %s", m_bTouched ? "True" : "False");
		EntityText( text_offset, tempstr, 0 );
		text_offset++;
	}

	return text_offset;
}
#endif
