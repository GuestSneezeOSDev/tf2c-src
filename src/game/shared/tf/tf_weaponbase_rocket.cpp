//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_rocket.h"
#include "tf_gamerules.h"

// Server specific.
#ifdef GAME_DLL
#include "soundent.h"
#include "te_effect_dispatch.h"
#include "tf_fx.h"
#include "tf_obj.h"
#include "tf_gamestats.h"
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#endif

#define RPG_WHISTLE			"Weapon_RPG.Whistle"
#define RPG_WHISTLE_CRIT	"Weapon_RPG.WhistleCrit"

//=============================================================================
//
// TF Base Rocket tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFBaseRocket, DT_TFBaseRocket )

BEGIN_NETWORK_TABLE( CTFBaseRocket, DT_TFBaseRocket )
	// Client specific.
#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO( m_vecInitialVelocity ) ),

	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),

	RecvPropInt( RECVINFO( m_iDeflected ) ),
	RecvPropEHandle( RECVINFO( m_hLauncher ) ),

	RecvPropVector( RECVINFO( m_vecVelocity ), 0, RecvProxy_LocalVelocity ),

	RecvPropBool( RECVINFO( m_bCritical ) ),
	RecvPropInt( RECVINFO( m_iType ) ),

	// Server specific.
#else
	SendPropVector( SENDINFO( m_vecInitialVelocity ), 12 /*nbits*/, 0 /*flags*/, -3500.0f /*low value*/, 3500.0f /*high value*/ ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	SendPropVector( SENDINFO( m_vecOrigin ), -1, SPROP_COORD_MP_INTEGRAL | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropQAngles( SENDINFO( m_angRotation ), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),

	SendPropInt( SENDINFO( m_iDeflected ), 4, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO( m_hLauncher ) ),

	SendPropVector( SENDINFO( m_vecVelocity ), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),

	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropInt( SENDINFO( m_iType ), Q_log2( TF_NUM_PROJECTILES ) + 1, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFBaseRocket )
	DEFINE_ENTITYFUNC( RocketTouch ),
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_THINKFUNC( DetonateThink ),
END_DATADESC()
#endif

#ifdef GAME_DLL
ConVar tf_rocket_show_radius( "tf_rocket_show_radius", "0", FCVAR_CHEAT, "Render rocket radius." );
ConVar tf2c_homing_rockets( "tf2c_homing_rockets", "0", FCVAR_CHEAT, "What is \"Rocket + x = Death\"?" );
ConVar tf2c_homing_deflected_rockets( "tf2c_homing_deflected_rockets", "0", FCVAR_CHEAT, "Homing Crit Rockets 2: Back with Vengeance" );
ConVar tf2c_bouncing_rockets( "tf2c_bouncing_rockets", "0", FCVAR_CHEAT, "ROCKET STORM!" );
extern ConVar tf2c_experimental_projectilehitdet;
#endif

//=============================================================================
//
// Shared (client/server) functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFBaseRocket::CTFBaseRocket()
{
	m_vecInitialVelocity.Init();
	m_iDeflected = 0;
	m_hLauncher = NULL;

#ifdef CLIENT_DLL
	// Client specific.
	m_flSpawnTime = 0.0f;
	m_bWhistling = false;
#else
	// Server specific.
	m_flDamage = 0.0f;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFBaseRocket::~CTFBaseRocket()
{
}


void CTFBaseRocket::Precache( void )
{
	PrecacheScriptSound( RPG_WHISTLE );
	PrecacheScriptSound( RPG_WHISTLE_CRIT );

	BaseClass::Precache();
}


void CTFBaseRocket::StopLoopingSounds( void )
{
	StopSound( RPG_WHISTLE );
	StopSound( RPG_WHISTLE_CRIT );

	BaseClass::StopLoopingSounds();
}


void CTFBaseRocket::Spawn( void )
{
	// Precache.
	Precache();

	BaseClass::Spawn();

#ifdef CLIENT_DLL
	// Client specific.
	m_flSpawnTime = gpGlobals->curtime;
#else
	// Server specific.
	//Derived classes must have set model.
	Assert( GetModel() );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	AddEffects( EF_NOSHADOW );

	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS_NOTSOLID );

	UTIL_SetSize( this, -Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	SetGravity( 0.0f );

	// Setup the touch and think functions.
	SetTouch( &CTFBaseRocket::RocketTouch );
	SetThink( &CTFBaseRocket::FlyThink );
	SetNextThink( gpGlobals->curtime );

	float flDetonateTime = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flDetonateTime, rocket_lifetime );
	if ( flDetonateTime )
	{
		RegisterThinkContext( "Detonate" );
		SetContextThink( &CTFBaseRocket::DetonateThink, gpGlobals->curtime + flDetonateTime, "Detonate" );
	}
#endif
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL


void CTFBaseRocket::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_hOldOwner = GetOwnerEntity();
}


void CTFBaseRocket::PostDataUpdate( DataUpdateType_t type )
{
	// Pass through to the base class.
	BaseClass::PostDataUpdate( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		// Now stick our initial velocity and angles into the interpolation history.
		CInterpolatedVar<Vector> &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();

		CInterpolatedVar<QAngle> &rotInterpolator = GetRotationInterpolator();
		rotInterpolator.ClearHistory();

		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

		// Add a sample 1 second back.
		Vector vCurOrigin = GetLocalOrigin() - m_vecInitialVelocity;
		interpolator.AddToHead( flChangeTime - 1.0f, &vCurOrigin, false );

		QAngle vCurAngles = GetLocalAngles();
		rotInterpolator.AddToHead( flChangeTime - 1.0f, &vCurAngles, false );

		// Add the current sample.
		vCurOrigin = GetLocalOrigin();
		interpolator.AddToHead( flChangeTime, &vCurOrigin, false );

		rotInterpolator.AddToHead( flChangeTime - 1.0, &vCurAngles, false );
	}
}



int CTFBaseRocket::DrawModel( int flags )
{
	// During the first 0.2 seconds of our life, don't draw ourselves.
	if ( gpGlobals->curtime - m_flSpawnTime < 0.2f )
		return 0;

	return BaseClass::DrawModel( flags );
}

void CTFBaseRocket::Simulate( void )
{
	// Make sure the rocket is facing movement direction.
	if ( GetMoveType() == MOVETYPE_FLYGRAVITY )
	{
		QAngle angForward;
		VectorAngles( GetAbsVelocity(), angForward );
		SetAbsAngles( angForward );

		if ( m_hLauncher && !m_bWhistling )
		{
			bool bWhistle = false;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( m_hLauncher, bWhistle, rocket_whistle );
			if( bWhistle )
			{
				float flGravity = 0.0f;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flGravity, mod_rocket_gravity );
				if ( flGravity )
				{
					if ( GetAbsVelocity().z < -100.0 )
					{
						EmitSound( m_bCritical ? RPG_WHISTLE_CRIT : RPG_WHISTLE );

						m_bWhistling = true;
					}
				}
				else
				{
					m_bWhistling = true;
				}
			}
		}
	}

	BaseClass::Simulate();
}

//=============================================================================
//
// Server specific functions.
//
#else

CTFBaseRocket *CTFBaseRocket::Create( CBaseEntity *pWeapon, const char *pszClassname, const Vector &vecOrigin,
	const QAngle &vecAngles, CBaseEntity *pOwner /*= NULL*/, ProjectileType_t iType /*= TF_PROJECTILE_NONE*/ )
{
	CTFBaseRocket *pRocket = static_cast<CTFBaseRocket*>( CBaseEntity::CreateNoSpawn( pszClassname, vecOrigin, vecAngles, pOwner ) );
	if ( !pRocket )
		return NULL;

	// Set firing weapon.
	pRocket->SetLauncher( pWeapon );
	pRocket->SetType( iType );

	// Spawn.
	DispatchSpawn( pRocket );

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	float flSpeed = pRocket->GetRocketSpeed();
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flSpeed, mult_projectile_speed );

	Vector vecVelocity = vecForward * flSpeed;

	// Setup potential gravity.
	float flGravity = 0.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flGravity, mod_rocket_gravity );
	if ( flGravity )
	{
		pRocket->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
		pRocket->SetGravity( flGravity );

		// Some upward force to approach grenade shooting behavior.
		// Players are used to their arcs, and we don't want to
		// require big mouse movements every shot.
		CTFPlayer *pPlayer = ToTFPlayer( pOwner );
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &vecForward, &vecRight, &vecUp );

		float flUpwardForce = 100.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pWeapon, flUpwardForce, mod_rocket_gravity_upward_force );

		vecVelocity = vecVelocity + vecUp * flUpwardForce + vecRight + vecUp;
	}

	pRocket->SetAbsVelocity( vecVelocity );
	pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	pRocket->SetAbsAngles( angles );

	// Set team.
	pRocket->ChangeTeam( pOwner->GetTeamNumber() );

	return pRocket;
}


//-----------------------------------------------------------------------------
// Purpose: Sweeps this projectile's collider and checks it against the hurt
// cylinder of the entity we just hit the hull of. Used for experimental new
// checks against angle-independent colliders.
//-----------------------------------------------------------------------------
bool CTFBaseRocket::HurtCylinderCheck( CBaseEntity *pOther )
{
	CTFPlayer *pVictimPlayer = dynamic_cast<CTFPlayer*>(pOther);
	if (pVictimPlayer) {
		// I think TestCollision *should* be calling CBaseAnimating::TestCollision and doing a full, proper sweep.
		//ICollideable *colliderToSweep = GetCollideable();
		//colliderToSweep->TestCollision()

		Vector vecCollisionMins, vecCollisionMaxs;
		pVictimPlayer->CollisionProp()->WorldSpaceSurroundingBounds(&vecCollisionMins, &vecCollisionMaxs);
		float flHeight = abs(vecCollisionMins.z - vecCollisionMaxs.z);

		// Set up the hurtcylinder the same way as we visualise it:
		HurtCylinder cylinder = HurtCylinder();
		cylinder.radius = pVictimPlayer->m_Shared.ProjetileHurtCylinderForClass(pVictimPlayer->GetPlayerClass()->GetClassIndex());
		cylinder.centre = pVictimPlayer->GetAbsOrigin() + Vector(0,0,flHeight / 2.0f);
		cylinder.height = flHeight;

		// Now check if the projectile's origin + velocity ray intersects or not:
		float intersection = cylinder.RayIntersect(GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity().Normalized());
		DevMsg("Cylinder intersect: %f \n", intersection);
		return intersection > 0.0f;
	}
	return false;
}
/*
bool CTFBaseRocket::TestCollision(const Ray_t &ray, unsigned int fContentsMask, trace_t &tr) {
	IPhysicsObject *pPhysObject = VPhysicsGetObject();
	Vector vecPosition;
	QAngle vecAngles;
	pPhysObject->GetPosition(&vecPosition, &vecAngles);
	const CPhysCollide *pScaledCollide = pPhysObject->GetCollide();
	physcollision->TraceBox(ray, pScaledCollide, vecPosition, vecAngles, &tr);

	return tr.DidHit();
}
*/

void CTFBaseRocket::RocketTouch( CBaseEntity *pOther )
{
	// Verify a correct "other".
	Assert( pOther );
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
		return;

	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();

	if ( tf2c_bouncing_rockets.GetBool() && !pOther->IsCombatCharacter() )
	{
		Vector vecDir = GetAbsVelocity();
		float flDot = DotProduct( vecDir, pTrace->plane.normal );
		Vector vecReflect = vecDir - 2.0f * flDot * pTrace->plane.normal;
		SetAbsVelocity( vecReflect );

		VectorNormalize( vecReflect );

		QAngle vecAngles;
		VectorAngles( vecReflect, vecAngles );
		SetAbsAngles( vecAngles );

		return;
	}

	// Handle hitting skybox (disappear).
	if ( pTrace->surface.flags & SURF_SKY )
	{
		// Don't remove RPG rockets on maps with low skybox ceilings.
		// We want mortar-like high angle shots to be possible.
		if ( pTrace->plane.normal[2] <= -1.0f )
		{
			float flGravity = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher, flGravity, mod_rocket_gravity );
			if ( flGravity )
				return;
		}

		UTIL_Remove( this );
		return;
	}
	
	if (tf2c_experimental_projectilehitdet.GetBool())
	{
		if (!HurtCylinderCheck(pOther))
			return;
	}

	HurtCylinderCheck(pOther);
	
	trace_t trace;
	memcpy(&trace, pTrace, sizeof(trace_t));
	Explode(&trace, pOther);
}


unsigned int CTFBaseRocket::PhysicsSolidMaskForEntity( void ) const
{
	int teamContents = 0;

	if ( !CanCollideWithTeammates() )
	{
		// Only collide with the other team

		switch ( GetTeamNumber() )
		{
		case TF_TEAM_RED:
			teamContents = CONTENTS_BLUETEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
			break;

		case TF_TEAM_BLUE:
			teamContents = CONTENTS_REDTEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
			break;

		case TF_TEAM_GREEN:
			teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_YELLOWTEAM;
			break;

		case TF_TEAM_YELLOW:
			teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_GREENTEAM;
			break;
		}
	}
	else
	{
		// Collide with all teams
		teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
	}

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}


void CTFBaseRocket::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	// Save this entity as enemy, they will take 100% damage.
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Figure out Econ ID.
	int iItemID = -1;
	if ( m_hLauncher.Get() )
	{
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( m_hLauncher.Get() );
		if ( pWeapon )
		{
			iItemID = pWeapon->GetItemID();
		}
	}

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CBaseEntity *pAttacker = GetAttacker();

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	bool bCrit = ( GetDamageType() & DMG_CRITICAL ) != 0;
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, GetWeaponID(), pOther->entindex(), ToBasePlayer( pAttacker ), GetTeamNumber(), bCrit, iItemID );
	CSoundEnt::InsertSound( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	// Damage.
	float flRadius = GetRadius();

	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info.Set( this, pAttacker, GetOriginalLauncher(), vec3_origin, vecOrigin, GetDamage(), GetDamageType() );
	radiusInfo.m_vecSrc = vecOrigin;
	radiusInfo.m_flRadius = flRadius;

	bool bIgnoreCheck = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), bIgnoreCheck, mod_explosion_no_owner_check);
	if( !bIgnoreCheck )
	{
		radiusInfo.m_flSelfDamageRadius = GetSelfDamageRadius();
		radiusInfo.m_bStockSelfDamage = UseStockSelfDamage();
	}

	TFGameRules()->RadiusDamage( radiusInfo );

	// Debug!
	if ( tf_rocket_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	// Don't decal players with scorch.
	if ( !pOther->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	// Remove the rocket.
	UTIL_Remove( this );
}


int	CTFBaseRocket::GetDamageType( void ) const
{
	int iDmgType = g_aWeaponDamageTypes[GetWeaponID()];
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}


float CTFBaseRocket::GetRadius( void )
{
	float flRadius = 146.0f;
	CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( m_hLauncher.Get(), flRadius, mult_explosion_radius );
	return flRadius;
}


float CTFBaseRocket::GetSelfDamageRadius( void )
{
	// Original rocket radius?
	return 121.0f;
}


void CTFBaseRocket::DrawRadius( float flRadius )
{
	Vector pos = GetAbsOrigin();
	int r = 255;
	int g = 0, b = 0;
	float flLifetime = 10.0f;
	bool bDepthTest = true;

	Vector edge, lastEdge;
	NDebugOverlay::Line( pos, pos + Vector( 0, 0, 50 ), r, g, b, !bDepthTest, flLifetime );

	lastEdge = Vector( flRadius + pos.x, pos.y, pos.z );
	float angle;
	for ( angle = 0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for ( angle = 0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = pos.x;
		edge.y = flRadius * cos( angle ) + pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for ( angle = 0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = flRadius * sin( angle ) + pos.y;
		edge.z = pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}
}


float CTFBaseRocket::GetRocketSpeed( void )
{
	return 1100.0f;
}


CBaseEntity *CTFBaseRocket::GetAttacker( void )
{
	return GetOwnerEntity();
}


void CTFBaseRocket::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	// Get rocket's speed.
	float flSpeed = GetAbsVelocity().Length();

	QAngle angForward;
	VectorAngles( vecDir, angForward );

	// Now change rocket's direction.
	SetAbsAngles( angForward );
	SetAbsVelocity( vecDir * flSpeed );

	if ( IsDeflectableSwapTeam() )
	{
		CTFPlayer *pTFOwner = ToTFPlayer( GetOwnerEntity() );
		if ( pTFOwner )
		{
			pTFOwner->SpeakConceptIfAllowed( MP_CONCEPT_DEFLECTED, "projectile:1,victim:1" );
		}

		if (pTFOwner && pTFOwner->IsEnemy(pDeflectedBy))
		{
			CTFPlayer* pDeflector = ToTFPlayer(pDeflectedBy);
			if (pDeflector)
			{
				if (m_bCritical)
				{
					CTF_GameStats.Event_PlayerBlockedDamage(pDeflector, GetDamage());
					CTF_GameStats.Event_PlayerAwardBonusPoints(pDeflector, this, m_iDeflected ? 2 : 1); // already deflected projectiles give even more bonus!
				}
				else
				{
					CTF_GameStats.Event_PlayerBlockedDamage(pDeflector, GetDamage() / 3.0f);
					if (m_iDeflected)
					{
						CTF_GameStats.Event_PlayerAwardBonusPoints(pDeflector, this, 1);
					}
				}
			}
		}

		// And change owner.
		SetOwnerEntity( pDeflectedBy );
		ChangeTeam( pDeflectedBy->GetTeamNumber() );

		CBaseCombatCharacter *pBCC = pDeflectedBy->MyCombatCharacterPointer();
		if ( pBCC )
		{
			SetLauncher( pBCC->GetActiveWeapon() );

			pTFOwner = ToTFPlayer(pDeflectedBy);
			if ( pTFOwner && pTFOwner->m_Shared.IsCritBoosted() )
			{
				SetCritical( true );
			}
		}
	}

	IncremenentDeflected();
	m_hDeflectedBy = pDeflectedBy;
}

//-----------------------------------------------------------------------------
// Purpose: Increment deflects counter
//-----------------------------------------------------------------------------
void CTFBaseRocket::IncremenentDeflected( void )
{
	m_iDeflected++;
}


void CTFBaseRocket::SetLauncher( CBaseEntity *pLauncher )
{
	m_hLauncher = pLauncher;

	BaseClass::SetLauncher( pLauncher );
}

void CTFBaseRocket::FlyThink( void )
{
	if ( tf2c_homing_rockets.GetBool() || ( tf2c_homing_deflected_rockets.GetBool() && m_iDeflected ) )
	{
		// Find the closest visible enemy player.
		CUtlVector<CTFPlayer *> vecPlayers;
		int count = CollectPlayers( &vecPlayers, TEAM_ANY, true );
		float flClosest = FLT_MAX;
		Vector vecClosestTarget = vec3_origin;

		for ( int i = 0; i < count; i++ )
		{
			CTFPlayer *pPlayer = vecPlayers[i];
			if ( pPlayer == GetOwnerEntity() )
				continue;

			if ( !pPlayer->IsEnemy( this ) )
				continue;

			Vector vecTarget;
			QAngle angTarget;
			if ( GetWeaponID() == TF_WEAPON_COMPOUND_BOW )
			{
				int iBone = pPlayer->LookupBone( "bip_head" );
				pPlayer->GetBonePosition( iBone, vecTarget, angTarget );
			}
			else
			{
				vecTarget = pPlayer->EyePosition();
			}

			if ( FVisible( vecTarget ) )
			{
				float flDistSqr = ( vecTarget - GetAbsOrigin() ).LengthSqr();
				if ( flDistSqr < flClosest )
				{
					flClosest = flDistSqr;
					vecClosestTarget = vecTarget;
				}
			}
		}

		// Head towards him.
		if ( vecClosestTarget != vec3_origin )
		{
			Vector vecTarget = vecClosestTarget;
			Vector vecDir = vecTarget - GetAbsOrigin();
			VectorNormalize( vecDir );

			float flSpeed = GetAbsVelocity().Length();
			QAngle angForward;
			VectorAngles( vecDir, angForward );
			SetAbsAngles( angForward );
			SetAbsVelocity( vecDir * flSpeed );
		}
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

void CTFBaseRocket::DetonateThink( void )
{
	// Putting this here just in case we're inside prediction to make sure all effects show up.
	CDisablePredictionFiltering disabler;

	trace_t		pTrace;
	Vector		vecSpot;

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &pTrace );

	// Explode without necessarily having a touched entity

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Figure out Econ ID.
	int iItemID = -1;
	if ( m_hLauncher.Get() )
	{
		CTFWeaponBase *pWeapon = dynamic_cast<CTFWeaponBase *>( m_hLauncher.Get() );
		if ( pWeapon )
		{
			iItemID = pWeapon->GetItemID();
		}
	}

	// Pull out a bit.
	if ( pTrace.fraction != 1.0 )
	{
		SetAbsOrigin( pTrace.endpos + ( pTrace.plane.normal * 1.0f ) );
	}

	CBaseEntity *pAttacker = GetAttacker();

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	bool bCrit = ( GetDamageType() & DMG_CRITICAL ) != 0;
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace.plane.normal, GetWeaponID(), GetAttacker()->entindex(), ToBasePlayer( pAttacker ), GetTeamNumber(), bCrit, iItemID );
	CSoundEnt::InsertSound( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	// Damage.
	float flRadius = GetRadius();

	CTFRadiusDamageInfo radiusInfo;
	radiusInfo.info.Set( this, pAttacker, GetOriginalLauncher(), vec3_origin, vecOrigin, GetDamage(), GetDamageType() );
	radiusInfo.m_vecSrc = vecOrigin;
	radiusInfo.m_flRadius = flRadius;
	bool bIgnoreCheck = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), bIgnoreCheck, mod_explosion_no_owner_check);
	if( !bIgnoreCheck )
	{
		radiusInfo.m_flSelfDamageRadius = GetSelfDamageRadius();
		radiusInfo.m_bStockSelfDamage = UseStockSelfDamage();
	}

	TFGameRules()->RadiusDamage( radiusInfo );

	// Debug!
	if ( tf_rocket_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	UTIL_DecalTrace( &pTrace, "Scorch" );

	// Remove the rocket.
	UTIL_Remove( this );
}

#endif
