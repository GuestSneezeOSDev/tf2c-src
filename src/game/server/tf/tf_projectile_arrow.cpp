//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// TF Arrow
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_arrow.h"
#include "soundent.h"
#include "tf_fx.h"
#include "props.h"
#include "baseobject_shared.h"
#include "SpriteTrail.h"
#include "IEffects.h"
#include "te_effect_dispatch.h"
#include "collisionutils.h"
#include "bone_setup.h"
#include "decals.h"
#include "tf_player.h"
#include "tf_gamestats.h"
#include "tf_weapon_shovel.h"
#include "tf_generic_bomb.h"

#include "tf_gamerules.h"
#include "bot/tf_bot.h"
#include "tf_weapon_medigun.h"
#include "soundenvelope.h"


//=============================================================================
//
// TF Arrow Projectile functions (Server specific).
//
#define ARROW_MODEL_GIB1			"models/weapons/w_models/w_arrow_gib1.mdl"
#define ARROW_MODEL_GIB2			"models/weapons/w_models/w_arrow_gib2.mdl"

#define ARROW_GRAVITY				0.3f

#define ARROW_BOX_RADIUS					1

#define ARROW_THINK_CONTEXT			"CTFProjectile_ArrowThink"
//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_projectile_arrow, CTFProjectile_Arrow );
PRECACHE_WEAPON_REGISTER( tf_projectile_arrow );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_Arrow, DT_TFProjectile_Arrow )

BEGIN_NETWORK_TABLE( CTFProjectile_Arrow, DT_TFProjectile_Arrow )
	SendPropBool( SENDINFO( m_bArrowAlight ) ),
	SendPropBool( SENDINFO( m_bCritical ) ),
	SendPropInt( SENDINFO( m_iProjectileType ) ),
END_NETWORK_TABLE()

BEGIN_DATADESC( CTFProjectile_Arrow )
DEFINE_THINKFUNC( ImpactThink ),
END_DATADESC()

extern ConVar tf2c_bouncing_rockets;

ConVar tf2c_experimental_huntsman_enable("tf2c_experimental_huntsman_enable", "0", FCVAR_NOTIFY, "Enables the experimental bounding box based collision for Huntsman headshots.");


CTFProjectile_Arrow::CTFProjectile_Arrow()
{
	m_flImpactTime = 0.0f;
	m_flTrailLife = 0.f;
	m_pTrail = NULL;
	m_bStruckEnemy = false;
	m_bArrowAlight = false;
	m_iDeflected = 0;
	m_bCritical = false;
	m_bPenetrate = false;
	m_iProjectileType = TF_PROJECTILE_ARROW;
	m_iWeaponId = TF_WEAPON_COMPOUND_BOW;
}


CTFProjectile_Arrow::~CTFProjectile_Arrow()
{
	m_HitEntities.Purge();
}

static const char *GetArrowEntityName( ProjectileType_t projectileType )
{
	return "tf_projectile_arrow";
}


CTFProjectile_Arrow *CTFProjectile_Arrow::Create( CBaseEntity *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, const float fSpeed, const float fGravity, ProjectileType_t projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	const char *pszArrowEntityName = GetArrowEntityName( projectileType );
	CTFProjectile_Arrow *pArrow = static_cast<CTFProjectile_Arrow *>( CBaseEntity::Create( pszArrowEntityName, vecOrigin, vecAngles, pOwner ) );
	if ( pArrow )
	{
		pArrow->InitArrow( pWeapon, vecAngles, fSpeed, fGravity, projectileType, pOwner, pScorer );
	}

	return pArrow;
}


void CTFProjectile_Arrow::InitArrow( CBaseEntity *pWeapon, const QAngle &vecAngles, const float fSpeed, const float fGravity, ProjectileType_t projectileType, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	// Initialize the owner.
	SetOwnerEntity( pOwner );

	// Set firing weapon.
	SetLauncher( pWeapon );

	// Set team.
	ChangeTeam( pOwner->GetTeamNumber() );

	// Must override projectile type before Spawn for proper model.
	m_iProjectileType = projectileType;

	// Spawn.
	Spawn();

	SetGravity( fGravity );

	SetCritical( m_bCritical );

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	Vector vecVelocity = vecForward * fSpeed;
	
	SetAbsVelocity( vecVelocity );	
	SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	SetAbsAngles( angles );

	// Save the scoring player.
	SetScorer( pScorer );

	// Create a trail.
	CreateTrail();

	// Add ourselves to the hit entities list so we dont shoot ourselves.
	m_HitEntities.AddToTail( pOwner->entindex() );

	m_bFiredWhileZoomed = false;
}


void CTFProjectile_Arrow::Spawn()
{
	SetModel( g_pszArrowModels[MODEL_ARROW_REGULAR] );

	SetSolid( SOLID_BBOX );	
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	AddEffects( EF_NOSHADOW );

	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS_NOTSOLID );

	UTIL_SetSize( this, Vector( -ARROW_BOX_RADIUS, -ARROW_BOX_RADIUS, -ARROW_BOX_RADIUS ), Vector( ARROW_BOX_RADIUS, ARROW_BOX_RADIUS, ARROW_BOX_RADIUS ) );

	AddFlag( FL_GRENADE );

	SetTouch( &CTFProjectile_Arrow::ArrowTouch );
	SetThink( &CTFProjectile_Arrow::FlyThink );
	SetNextThink( gpGlobals->curtime );

	// Set team.
	m_nSkin = GetTeamSkin( GetTeamNumber() );
}


void CTFProjectile_Arrow::Precache()
{
	PrecacheGibsForModel( PrecacheModel( g_pszArrowModels[MODEL_ARROW_REGULAR] ) );
	PrecacheModel( "effects/arrowtrail_red.vmt" );
	PrecacheModel( "effects/arrowtrail_blu.vmt" );
	PrecacheModel( "effects/arrowtrail_grn.vmt" );
	PrecacheModel( "effects/arrowtrail_ylw.vmt" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactFlesh" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactMetal" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactWood" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactConcrete" );
	PrecacheScriptSound( "Weapon_Arrow.Nearmiss" );
	PrecacheScriptSound( "Weapon_Arrow.ImpactFleshCrossbowHeal" );

	PrecacheParticleSystem( "flying_flaming_arrow" );

	BaseClass::Precache();
}


void CTFProjectile_Arrow::SetScorer( CBaseEntity *pScorer )
{
	m_Scorer = pScorer;
}


CBasePlayer *CTFProjectile_Arrow::GetScorer( void )
{
	return dynamic_cast<CBasePlayer *>( m_Scorer.Get() );
}

//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::CanHeadshot() 
{
	int iCanHeadshot = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), iCanHeadshot, can_headshot);
	if (iCanHeadshot)
		return true;

	int iNoHeadshot = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), iNoHeadshot, no_headshot);
	if (iNoHeadshot)
		return false;

	if ( m_iProjectileType == TF_PROJECTILE_BUILDING_REPAIR_BOLT 
		|| m_iProjectileType == TF_PROJECTILE_HEALING_BOLT 
		|| m_iProjectileType == TF_PROJECTILE_FESTIVE_HEALING_BOLT )
	{
		return false;
	}

	return true; 
}

//-----------------------------------------------------------------------------
// Purpose: Healing bolt damage.
//-----------------------------------------------------------------------------
float CTFProjectile_Arrow::GetDamage()
{
	return BaseClass::GetDamage();
}

//-----------------------------------------------------------------------------
// Purpose: Moves the arrow to a particular bbox.
//-----------------------------------------------------------------------------
bool CTFProjectile_Arrow::PositionArrowOnBone( mstudiobbox_t *pBox, CBaseAnimating *pAnimOther )
{
	CStudioHdr *pStudioHdr = pAnimOther->GetModelPtr();
	if ( !pStudioHdr )
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnimOther->GetHitboxSet() );
	if ( !set )
		return false;

	// Target must have hit boxes.
	if ( !set->numhitboxes )
		return false;

	// Bone index must be valid.
	if ( pBox->bone < 0 || pBox->bone >= pStudioHdr->numbones() )
		return false;

	CBoneCache *pCache = pAnimOther->GetBoneCache();
	if ( !pCache )
		return false;

	matrix3x4_t *bone_matrix = pCache->GetCachedBone( pBox->bone );
	if ( !bone_matrix )
		return false;

	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	TransformAABB( *bone_matrix, pBox->bbmin, pBox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs );

	// Adjust the arrow so it isn't exactly in the center of the box.
	Vector position;
	position.x = Lerp( RandomFloat( 0.4f, 0.6f ), vecBoxAbsMins.x, vecBoxAbsMaxs.x );
	position.y = Lerp( RandomFloat( 0.4f, 0.6f ), vecBoxAbsMins.y, vecBoxAbsMaxs.y );
	position.z = Lerp( RandomFloat( 0.4f, 0.6f ), vecBoxAbsMins.z, vecBoxAbsMaxs.z );
	SetAbsOrigin( position );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: This was written after PositionArrowOnBone, but the two might be mergable?
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::GetBoneAttachmentInfo( mstudiobbox_t *pBox, CBaseAnimating *pAnimOther, Vector &bonePosition, QAngle &boneAngles, int &boneIndexAttached, int &physicsBoneIndex )
{
	// Find a bone to stick to.
	matrix3x4_t arrowWorldSpace;
	MatrixCopy( EntityToWorldTransform(), arrowWorldSpace );

	// Get the bone info so we can follow the bone.
	boneIndexAttached = pBox->bone;
	physicsBoneIndex = pAnimOther->GetPhysicsBone( boneIndexAttached );
	matrix3x4_t boneToWorld;
	pAnimOther->GetBoneTransform( boneIndexAttached, boneToWorld );

	Vector attachedBonePos;
	QAngle attachedBoneAngles;
	pAnimOther->GetBonePosition( boneIndexAttached, attachedBonePos, attachedBoneAngles );

	// Transform my current position/orientation into the hit bone's space.
	matrix3x4_t worldToBone, localMatrix;
	MatrixInvert( boneToWorld, worldToBone );
	ConcatTransforms( worldToBone, arrowWorldSpace, localMatrix );
	MatrixAngles( localMatrix, boneAngles, bonePosition );
}

//-----------------------------------------------------------------------------
int	CTFProjectile_Arrow::GetProjectileType( void ) const	
{ 
	return m_iProjectileType;
}


bool CTFProjectile_Arrow::StrikeTarget(mstudiobbox_t *pBox, CBaseEntity *pOther)
{
	if (!pOther)
		return false;

	const Vector &vecOriginBeforeRepos = GetAbsOrigin();

	// Different path for arrows that would heal friendly buildings.
	if (pOther->IsBaseObject() && OnArrowImpactObject(pOther))
		return false;

	// Block and break on invulnerable players.
	CTFPlayer *pTFPlayerOther = ToTFPlayer(pOther);
	if (pTFPlayerOther && pTFPlayerOther->m_Shared.IsInvulnerable() && pTFPlayerOther->m_Shared.InCond(TF_COND_INVULNERABLE_SMOKE_BOMB))
		return false;

	CBaseAnimating *pAnimOther = dynamic_cast<CBaseAnimating *>(pOther);
	if (!pAnimOther)
		return false;

	// Replace 'false' with entity checks here for what you want it to break against. -> EX: dynamic_cast<CBaseEntity *>( pOther ) != NULL )
	bool bBreakArrow = IsBreakable() && (dynamic_cast<CTFPumpkinBomb*>(pOther) || pOther->IsBaseObject());
	// Are we a headshot?
	bool bHeadshot = false;

	// Experimental headshot detection: Skip if there's no player (it's the world or a building!)
	if (tf2c_experimental_huntsman_enable.GetBool() && pTFPlayerOther)
	{
		// Surrounding boxes are axially aligned, so ignore angles
		Vector vecSurroundMins, vecSurroundMaxs;
		pOther->CollisionProp()->WorldSpaceSurroundingBounds(&vecSurroundMins, &vecSurroundMaxs);	

		float fHeadshotFraction = CTFPlayerShared::HeadshotFractionForClass( pTFPlayerOther->GetPlayerClass()->GetClassIndex() );

		// Headshot will be if the z-position of the arrow is greater than the box bottom + some fraction of the height
		float flHeight = abs(vecSurroundMins.z - vecSurroundMaxs.z);
		float flMinZForHead = vecSurroundMins.z + ((1.0f - clamp(fHeadshotFraction, 0.0f, 1.0f)) * flHeight);

		bHeadshot = vecOriginBeforeRepos.z >= flMinZForHead;

		bool bRepositionedSuccessfully = false;

		// If the arrow is headshotting, forcibly move it to the position of the head hurtbox so it gets positioned to the victim's head bone.
		if (bHeadshot)
		{
			// Get the hurtbox set for pOther's model
			mstudiohitboxset_t *set = NULL;
			CStudioHdr *pStudioHdr = NULL;
			if (pAnimOther)
			{
				pStudioHdr = pAnimOther->GetModelPtr();
				if (pStudioHdr)
				{
					set = pStudioHdr->pHitboxSet(pAnimOther->GetHitboxSet());

					for (int i = 0; i < set->numhitboxes; i++)
					{
						mstudiobbox_t *pHurtbox = set->pHitbox(i);

						if (pHurtbox->group == HITGROUP_HEAD)
						{
							// Found the head, move us there.
							Vector position;
							QAngle angles;
							pAnimOther->GetBonePosition(pHurtbox->bone, position, angles);

							if (!PositionArrowOnBone(pHurtbox, pAnimOther))
								return false;

							bRepositionedSuccessfully = true;
							break;
						}
					}
				}
			}

			// Something went wrong; just position it on whatever bone was closest.
			if (!bRepositionedSuccessfully)
			{
				if (!m_bPenetrate && !bBreakArrow)
				{
					if (!PositionArrowOnBone(pBox, pAnimOther))
						return false;
				}
			}
		}
		else
		{
			// Position the arrow so its on the bone, within a reasonable region defined by the bbox.
			if (!m_bPenetrate && !bBreakArrow)
			{
				if (!PositionArrowOnBone(pBox, pAnimOther))
					return false;
			}
		}

		//DevMsg("flMinZForHead: %f, arrow origin.z: %f, bbox bottom/top: %f/%f\n", flMinZForHead, vecOriginBeforeRepos.z, vecSurroundMins.z, vecSurroundMaxs.z);
	}
	// Vanilla headshot detection:
	else
	{
		// Position the arrow so its on the bone, within a reasonable region defined by the bbox.
		if (!m_bPenetrate && !bBreakArrow)
		{
			if (!PositionArrowOnBone(pBox, pAnimOther))
				return false;
		}

		if (pBox->group == HITGROUP_HEAD && CanHeadshot())
		{
			bHeadshot = true;
		}
	}

	const Vector &vecOrigin = GetAbsOrigin();
	Vector vecVelocity = GetAbsVelocity();
	int nDamageCustom = 0;
	bool bApplyEffect = true;
	int nDamageType = GetDamageType();

	// Damage the entity we struck.
	/*CBaseEntity *pAttacker = GetScorer();
	if ( !pAttacker )
	{
		// Likely not launched by a player.
		pAttacker = GetOwnerEntity();
	}*/
	// Now, why do we need to remember the scorer, whoever that is? - Foxysen
	CBaseEntity *pAttacker = GetOwnerEntity();
	
	if ( pAttacker )
	{
		// Check if we have the penetrate attribute.  We don't want
		// to strike the same target multiple times.
		if ( m_bPenetrate )
		{
			// Don't strike the same target again.
			if ( m_HitEntities.Find( pOther->entindex() ) != m_HitEntities.InvalidIndex() )
			{
				bApplyEffect = false;
			}
			else
			{
				m_HitEntities.AddToTail( pOther->entindex() );
			}
		}

		if ( !InSameTeam( pOther ) )
		{
			IScorer *pScorerInterface = dynamic_cast<IScorer *>( pAttacker );
			if ( pScorerInterface )
			{
				pAttacker = pScorerInterface->GetScorer();
			}

			if ( m_bArrowAlight )
			{
				nDamageType |= DMG_IGNITE;
				nDamageCustom = TF_DMG_CUSTOM_FLYINGBURN;
			}

			if ( bHeadshot )
			{
				int iHeadshotIsMinicrit = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(GetOriginalLauncher(), iHeadshotIsMinicrit, headshot_is_minicrit);
				if (iHeadshotIsMinicrit)
				{
					auto pTFAttacker = ToTFPlayer(pAttacker);
					if (pTFAttacker)
					{
						pTFAttacker->SetNextAttackMinicrit(true);
					}
				}
				else
				{
					nDamageType |= DMG_CRITICAL;
				}
				nDamageCustom = TF_DMG_CUSTOM_HEADSHOT;
			}

			if ( m_bCritical )
			{
				nDamageType |= DMG_CRITICAL;
			}

			// Damage.
			if ( bApplyEffect )
			{
				CTakeDamageInfo info( this, pAttacker, m_hLauncher, vecVelocity, vecOrigin, GetDamage(), nDamageType, nDamageCustom );
				pOther->TakeDamage( info );

				// Play an impact sound.
				ImpactSound( "Weapon_Arrow.ImpactFlesh", true );
			}
		}
	}

	if ( !m_bPenetrate && !bBreakArrow )
	{
		OnArrowImpact( pBox, pOther, pAttacker );
	}

	// Perform a blood mesh decal trace.
	trace_t tr;
	Vector start = vecOrigin - vecVelocity * gpGlobals->frametime;
	Vector end = vecOrigin + vecVelocity * gpGlobals->frametime;
	CTraceFilterCollisionArrows filter( this, GetOwnerEntity() );
	UTIL_TraceLine( start, end, CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, &filter, &tr );
	UTIL_ImpactTrace( &tr, 0 );

	// Break it?
	if ( bBreakArrow )
		return false;

	return true;
}



void CTFProjectile_Arrow::OnArrowImpact( mstudiobbox_t *pBox, CBaseEntity *pOther, CBaseEntity *pAttacker )
{
	CBaseAnimating *pAnimOther = dynamic_cast<CBaseAnimating *>( pOther );
	if ( !pAnimOther )
		return;

	const Vector &vecOrigin = GetAbsOrigin();
	Vector vecVelocity = GetAbsVelocity();

	Vector bonePosition = vec3_origin;
	QAngle boneAngles = QAngle( 0, 0, 0 );
	int boneIndexAttached = -1;
	int physicsBoneIndex = -1;
	GetBoneAttachmentInfo( pBox, pAnimOther, bonePosition, boneAngles, boneIndexAttached, physicsBoneIndex );
	bool bSendImpactMessage = true;

	// Did we kill the target?
	if ( !pOther->IsAlive() && pOther->IsPlayer() )
	{
		CTFPlayer *pTFPlayerOther = dynamic_cast<CTFPlayer *>( pOther );
		if ( pTFPlayerOther && pTFPlayerOther->m_hRagdoll )
		{
			VectorNormalize( vecVelocity );
			if ( CheckRagdollPinned( vecOrigin, vecVelocity, boneIndexAttached, physicsBoneIndex, pTFPlayerOther->m_hRagdoll, pBox->group, pTFPlayerOther->entindex() ) )
			{
				pTFPlayerOther->StopRagdollDeathAnim();
				bSendImpactMessage = false;
			}
		}
	}

	// Notify relevant clients of an arrow impact.
	if ( bSendImpactMessage )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "arrow_impact" );
		if ( event )
		{
			event->SetInt( "attachedEntity", pOther->entindex() );
			event->SetInt( "shooter", pAttacker ? pAttacker->entindex() : 0 );
			event->SetInt( "attachedEntity", pOther->entindex() );
			event->SetInt( "boneIndexAttached", boneIndexAttached );
			event->SetFloat( "bonePositionX", bonePosition.x );
			event->SetFloat( "bonePositionY", bonePosition.y );
			event->SetFloat( "bonePositionZ", bonePosition.z );
			event->SetFloat( "boneAnglesX", boneAngles.x );
			event->SetFloat( "boneAnglesY", boneAngles.y );
			event->SetFloat( "boneAnglesZ", boneAngles.z );
			event->SetInt( "projectileType", GetProjectileType() );
			gameeventmanager->FireEvent( event );
		}
	}

	FadeOut( 3.0f );
}



bool CTFProjectile_Arrow::OnArrowImpactObject( CBaseEntity *pOther )
{
	return false;
}



void CTFProjectile_Arrow::ImpactThink( void )
{
}

void CTFProjectile_Arrow::FlyThink( void )
{
	BaseClass::FlyThink();

	if ( GetWaterLevel() > WL_NotInWater )
	{
		m_bArrowAlight = false;
	}
}


void CTFProjectile_Arrow::OnArrowMissAllPlayers()
{
}


void CTFProjectile_Arrow::ArrowTouch( CBaseEntity *pOther )
{
	if ( m_bStruckEnemy || GetMoveType() == MOVETYPE_NONE )
		return;

	if ( !pOther )
		return;

	bool bShield = pOther->IsCombatItem() && !InSameTeam( pOther );
	bool bPumpkpin = (dynamic_cast<CTFPumpkinBomb*>(pOther) != nullptr);	// Come up with better solution than dynamic_cast?
	if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) && !bShield )
		return;

	// Test against combat characters, which include players, Engineer buildings, and NPCs,
	CBaseCombatCharacter *pOtherCombatCharacter = dynamic_cast<CBaseCombatCharacter *>( pOther );
	if ( !pOtherCombatCharacter )
	{
		if ( tf2c_bouncing_rockets.GetBool() )
		{
			const trace_t *pTrace = &CBaseEntity::GetTouchTrace();

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
	}

	if (pOther->IsWorld() || (!pOtherCombatCharacter && !bShield && !bPumpkpin))
	{
		// Check to see if we struck the skybox.
		CheckSkyboxImpact( pOther );

		// If we've only got 1 entity in the hit list (the attacker by default) and we've not been deflected
		// then we can consider this arrow to have completely missed all players.
		if ( m_HitEntities.Count() == 1 && m_iDeflected == 0 )
		{
			OnArrowMissAllPlayers();
		}

		return;
	}

	CBaseAnimating *pAnimOther = dynamic_cast<CBaseAnimating *>( pOther );
	CStudioHdr *pStudioHdr = NULL;
	mstudiohitboxset_t *set = NULL;
	if ( pAnimOther )
	{
		pStudioHdr = pAnimOther->GetModelPtr();
		if ( pStudioHdr )
		{
			set = pStudioHdr->pHitboxSet( pAnimOther->GetHitboxSet() );
		}
	}

	if ( !pAnimOther || !pStudioHdr || !set )
	{
		// Whatever we hit doesn't have hitboxes. Ignore it.
		UTIL_Remove( this );
		return;
	}

	// We struck the collision box of a player or a buildable object.
	// Trace forward to see if we struck a hitbox.
	CTraceFilterCollisionArrows filter( this, GetOwnerEntity() );
	Vector start = GetAbsOrigin();
	Vector vel = GetAbsVelocity();
	trace_t tr;
	
	// Tracing a line is a just plain wrong if the projectile is a box...
	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );
	
	Vector mins, maxs;
	mins = Vector( -ARROW_BOX_RADIUS );
	maxs = Vector( ARROW_BOX_RADIUS );
	// Tracehull is fucking broken.
	//UTIL_TraceHull( start, start + vel * gpGlobals->frametime, mins, maxs, CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, &filter, &tr );

	// Instead, we'll do a raycast from each of the 8 corners of the box hull and take the shortest distance. Almost the same thing but it'll actually work.
	int i, j, k;
	Vector vecEnd;
	float distance = (tr.endpos - start).Length();
	Vector minmaxs[2] = { mins, maxs };
	Vector projectedPos = start + vel * gpGlobals->frametime;
	Vector startCorner = start;
	
	trace_t tr_temp;
	tr_temp = tr;

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = minmaxs[i][0];
				vecEnd.y = minmaxs[j][1];
				vecEnd.z = minmaxs[k][2];
				vecEnd += projectedPos;

				startCorner = start;
				startCorner.x += minmaxs[i][0];
				startCorner.y += minmaxs[j][0];
				startCorner.z += minmaxs[k][0];

				UTIL_TraceLine( startCorner, vecEnd, CONTENTS_HITBOX | CONTENTS_MONSTER | CONTENTS_SOLID, &filter, &tr_temp );
				//NDebugOverlay::Line( startCorner, vecEnd, 255, 255, 255, true, 5.0f );
				if ( tr_temp.fraction < 1.0 )
				{
					float thisDistance = (tr_temp.endpos - start).Length();
					if ( thisDistance < distance )
					{
						tr = tr_temp;
						distance = thisDistance;
					}
				}
			}
		}
	}

	// If we hit a hitbox, stop tracing.
	mstudiobbox_t *closest_box = NULL;
	if ( tr.m_pEnt && tr.m_pEnt->GetTeamNumber() != GetTeamNumber() )
	{
		// This means the arrow was true and was flying directly at a hitbox on the target.
		// We'll attach to that hitbox.
		closest_box = set->pHitbox( tr.hitbox );
	}

	if ( !closest_box )
	{
		// Locate the hitbox closest to our point of impact on the collision box.
		Vector position, start, forward;
		QAngle angles;
		float closest_dist = 99999;

		// Intense, but extremely accurate:
		AngleVectors( GetAbsAngles(), &forward );
		start = GetAbsOrigin() + forward * 16;
		for ( int i = 0; i < set->numhitboxes; i++ )
		{
			mstudiobbox_t *pbox = set->pHitbox( i );

			pAnimOther->GetBonePosition( pbox->bone, position, angles );

			Ray_t ray;
			ray.Init( start, position );
			trace_t tr;
			IntersectRayWithBox( ray, position+pbox->bbmin, position+pbox->bbmax, 0.0f, &tr );
			float dist = tr.endpos.DistTo( start );
			if ( dist < closest_dist )
			{
				closest_dist = dist;
				closest_box = pbox;
			}
		}
	}

	if ( closest_box )
	{
		// See if we're supposed to stick in the target.
		bool bStrike = StrikeTarget( closest_box, pOther );
		if ( bStrike && !m_bPenetrate )
		{
			// If we're here, it means StrikeTarget() called FadeOut( 3.0f ).
			SetAbsOrigin( start );
		}

		if ( !bStrike || bShield )
		{
			BreakArrow();
		}

		// Slightly confusing. If we're here, the arrow stopped at the
		// target and will fade or break. Setting this prevents the
		// touch code from re-running during the delay.
		if ( !m_bPenetrate )
		{
			m_bStruckEnemy = true;
		}
	}
}


unsigned int CTFProjectile_Arrow::PhysicsSolidMaskForEntity( void ) const
{
	int teamContents = 0;

	if ( !CanCollideWithTeammates() )
	{
		// Only collide with the other team.
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
		// Collide with all teams.
		teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM | CONTENTS_GREENTEAM | CONTENTS_YELLOWTEAM;
	}

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}


void CTFProjectile_Arrow::CheckSkyboxImpact( CBaseEntity *pOther )
{
	trace_t tr;
	Vector velDir = GetAbsVelocity();
	VectorNormalize( velDir );
	Vector vecSpot = GetAbsOrigin() - velDir * 32;
	UTIL_TraceLine( vecSpot, vecSpot + velDir * 64, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );
	if ( tr.fraction < 1.0 && tr.surface.flags & SURF_SKY )
	{
		// We hit the skybox, go away soon.
		FadeOut( 3.0f );
		return;
	}

	if ( !pOther->IsWorld() )
	{
		BreakArrow();
	}
	else
	{
		CEffectData	data;
		data.m_vOrigin = tr.endpos;
		data.m_vNormal = velDir;
		data.m_nEntIndex = 0;
		data.m_nAttachmentIndex = 0;
		data.m_nMaterial = 0;
		data.m_fFlags = GetProjectileType();
		data.m_nColor = GetTeamSkin( GetTeamNumber() );

		DispatchEffect( "TFBoltImpact", data );

		FadeOut( 3.0f );

		// Play an impact sound.
		surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );

		const char *pszSoundName;
		switch ( psurf ? psurf->game.material : CHAR_TEX_METAL )
		{
			case CHAR_TEX_CONCRETE:
				pszSoundName = "Weapon_Arrow.ImpactConcrete";
				break;
			case CHAR_TEX_WOOD:
				pszSoundName = "Weapon_Arrow.ImpactWood";
				break;
			default:
				pszSoundName = "Weapon_Arrow.ImpactMetal";
				break;
		}
		ImpactSound( pszSoundName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Plays an impact sound. Louder for the attacker.
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::ImpactSound( const char *pszSoundName, bool bLoudForAttacker )
{
	CTFPlayer *pAttacker = ToTFPlayer( GetScorer() );
	if ( !pAttacker )
		return;

	if ( bLoudForAttacker )
	{
		float soundlen = 0;
		EmitSound_t params;
		params.m_flSoundTime = 0;
		params.m_pSoundName = pszSoundName;
		params.m_pflSoundDuration = &soundlen;
		CPASFilter filter( GetAbsOrigin() );
		filter.RemoveRecipient( pAttacker );
		EmitSound( filter, entindex(), params );

		CSingleUserRecipientFilter attackerFilter( pAttacker );
		EmitSound( attackerFilter, pAttacker->entindex(), params );
	}
	else
	{
		EmitSound( pszSoundName );
	}
}


void CTFProjectile_Arrow::BreakArrow()
{
	FadeOut( 3.0f );
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();
}


bool CTFProjectile_Arrow::CheckRagdollPinned( const Vector &start, const Vector &vel, int boneIndexAttached, int physicsBoneIndex, CBaseEntity *pOther, int iHitGroup, int iVictim )
{
	// Pin to the wall.
	trace_t tr;
	UTIL_TraceLine( start, start + vel * 125, MASK_BLOCKLOS, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0f && tr.DidHitWorld() )
	{
		CEffectData	data;
		data.m_vOrigin = tr.endpos;
		data.m_vNormal = vel;
		data.m_nEntIndex = pOther->entindex();
		data.m_nAttachmentIndex = boneIndexAttached;
		data.m_nMaterial = physicsBoneIndex;
		data.m_nDamageType = iHitGroup;
		data.m_nSurfaceProp = iVictim;
		data.m_fFlags = GetProjectileType();
		data.m_nColor = GetTeamSkin( GetTeamNumber() );

		if ( GetScorer() )
		{
			data.m_nHitBox = GetScorer()->entindex();
		}

		DispatchEffect( "TFBoltImpact", data );

		return true;
	}

	return false;
}


void CTFProjectile_Arrow::FadeOut( int iTime )
{
	SetMoveType( MOVETYPE_NONE );
	SetAbsVelocity( vec3_origin	);
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );

	// Start remove timer.
	SetContextThink( &CTFProjectile_Arrow::RemoveThink, gpGlobals->curtime + iTime, "ARROW_REMOVE_THINK" );
}


void CTFProjectile_Arrow::RemoveThink( void )
{
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
const char *CTFProjectile_Arrow::GetTrailParticleName( void )
{
	// Would use this, but an invalid texture would likely crash the game if it's shot from a non-normal team.
	//ConstructTeamParticle( "effects/arrowtrail_%s.vmt", GetTeamNumber(), g_aTeamNamesShort );
	const char *szTrailTexture;
	switch ( GetTeamNumber() )
	{
		case TF_TEAM_RED:
			szTrailTexture = "effects/arrowtrail_red.vmt";
			break;
		case TF_TEAM_BLUE:
			szTrailTexture = "effects/arrowtrail_blu.vmt";
			break;
		case TF_TEAM_GREEN:
			szTrailTexture = "effects/arrowtrail_grn.vmt";
			break;
		case TF_TEAM_YELLOW:
			szTrailTexture = "effects/arrowtrail_ylw.vmt";
			break;
		default:
			szTrailTexture = "effects/arrowtrail_red.vmt";
			break;
	}

	return szTrailTexture;
}


void CTFProjectile_Arrow::CreateTrail( void )
{
	if ( IsDormant() )
		return;

	if ( !m_pTrail )
	{
		CSpriteTrail *pTempTrail = CSpriteTrail::SpriteTrailCreate( GetTrailParticleName(), GetAbsOrigin(), true );
		pTempTrail->FollowEntity( this );
		pTempTrail->SetTransparency( kRenderTransAlpha, 255, 255, 255, 255, kRenderFxNone );
		pTempTrail->SetStartWidth( 3 );
		pTempTrail->SetTextureResolution( 1.0f / ( 96.0f * 1.0f ) );
		pTempTrail->SetLifeTime( 0.3f );
		pTempTrail->TurnOn();
		pTempTrail->SetAttachment( this, 0 );
		m_pTrail = pTempTrail;
		SetContextThink( &CTFProjectile_Arrow::RemoveTrail, gpGlobals->curtime + 3.0f, "FadeTrail" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fade and kill the trail
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::RemoveTrail( void )
{
	if ( m_pTrail )
	{
		if ( m_flTrailLife <= 0.0f )
		{
			UTIL_Remove( m_pTrail );
			m_flTrailLife = 1.0f;
		}
		else	
		{
			CSpriteTrail *pTempTrail = dynamic_cast<CSpriteTrail *>( m_pTrail.Get() );
			if ( pTempTrail )
			{
				pTempTrail->SetBrightness( (int)( 128 * m_flTrailLife ) );
			}

			m_flTrailLife = m_flTrailLife - 0.1f;
			SetContextThink( &CTFProjectile_Arrow::RemoveTrail, gpGlobals->curtime + 0.05f, "FadeTrail" );
		}
	}
}


void CTFProjectile_Arrow::AdjustDamageDirection( const CTakeDamageInfo &info, Vector &dir, CBaseEntity *pEnt )
{
	if ( pEnt )
	{
		dir = info.GetDamagePosition() - info.GetDamageForce() - pEnt->WorldSpaceCenter();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Arrow was deflected.
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::IncrementDeflected( void )
{
	m_iDeflected++; 

	// Change trail color.
	if ( m_pTrail )
	{
		UTIL_Remove( m_pTrail );
		m_pTrail = NULL;
		m_flTrailLife = 1.0f;
	}
	CreateTrail();
}

//-----------------------------------------------------------------------------
// Purpose: Arrow was deflected.
//-----------------------------------------------------------------------------
void CTFProjectile_Arrow::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	BaseClass::Deflected( pDeflectedBy, vecDir );
	IncrementDeflected();

	if ( IsDeflectableSwapTeam() )
	{
		// Purge our hit list so we can hit everyone again.
		m_HitEntities.Purge();

		// Add ourselves so we don't hit ourselves.
		m_HitEntities.AddToTail( pDeflectedBy->entindex() );
	}
}


bool CTFProjectile_Arrow::IsDeflectable( void )
{
	return !IsSolidFlagSet( FSOLID_NOT_SOLID );
}
