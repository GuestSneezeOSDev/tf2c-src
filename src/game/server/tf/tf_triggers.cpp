//=============================================================================
//
// Purpose:
//
//=============================================================================
#include "cbase.h"
#include "tf_triggers.h"
#include "tf_player.h"
#include "tf_bot.h"
#include "saverestore_utlvector.h"

#include "tf_weapon_compound_bow.h"
#include "tf_projectile_arrow.h"
#include "tf_weapon_beacon.h"	// !!! foxysen beacon

BEGIN_DATADESC( CTriggerAddTFPlayerCondition )
DEFINE_KEYFIELD( m_nCond, FIELD_INTEGER, "condition" ),
DEFINE_KEYFIELD( m_flDuration, FIELD_FLOAT, "duration" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_add_tf_player_condition, CTriggerAddTFPlayerCondition );


void CTriggerAddTFPlayerCondition::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}


void CTriggerAddTFPlayerCondition::StartTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	if ( m_nCond == TF_COND_INVALID )
	{
		Warning( "Invalid Condition ID [%d] in trigger %s\n", TF_COND_INVALID, GetDebugName() );
	}
	else
	{
		pPlayer->m_Shared.AddCond( m_nCond, m_flDuration );
		BaseClass::StartTouch( pOther );
	}
}


void CTriggerAddTFPlayerCondition::EndTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	if ( m_flDuration != -1.0f )
		return;

	if ( m_nCond == TF_COND_INVALID )
	{
		Warning( "Invalid Condition ID [%d] in trigger %s\n", TF_COND_INVALID, GetDebugName() );
	}
	else
	{
		pPlayer->m_Shared.RemoveCond( m_nCond );
		BaseClass::EndTouch( pOther );
	}
}

BEGIN_DATADESC( CTriggerRemoveTFPlayerCondition )
DEFINE_KEYFIELD( m_nCond, FIELD_INTEGER, "condition" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_remove_tf_player_condition, CTriggerRemoveTFPlayerCondition );


void CTriggerRemoveTFPlayerCondition::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}


void CTriggerRemoveTFPlayerCondition::StartTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	if ( m_nCond == TF_COND_INVALID )
	{
		pPlayer->m_Shared.RemoveAllCond();
	}
	else
	{
		pPlayer->m_Shared.RemoveCond( m_nCond );

		CTFBot *pBot = ToTFBot( pPlayer );
		if ( pBot )
		{
			pBot->ClearLastKnownArea();
		}

		BaseClass::StartTouch( pOther );
	}
}

BEGIN_DATADESC( CFuncJumpPad )
DEFINE_KEYFIELD( m_angPush, FIELD_VECTOR, "impulse_dir" ),
DEFINE_KEYFIELD( m_flPushForce, FIELD_FLOAT, "force" ),
DEFINE_KEYFIELD( m_iszJumpSound, FIELD_SOUNDNAME, "jumpsound" ),

DEFINE_OUTPUT( m_OnJump, "OnJump" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_jumppad, CFuncJumpPad );


void CFuncJumpPad::Precache( void )
{
	BaseClass::Precache();

	if ( m_iszJumpSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING( m_iszJumpSound ) );
	}
}


void CFuncJumpPad::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	InitTrigger();

	// Convert push angle to vector and transform it into entity space.
	Vector vecDir;
	AngleVectors( m_angPush, &vecDir );
	VectorIRotate( vecDir, EntityToWorldTransform(), m_vecPushDir );
}


void CFuncJumpPad::StartTouch( CBaseEntity *pOther )
{
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	// Transform the vector back into world space.
	Vector vecAbsDir;
	VectorRotate( m_vecPushDir, EntityToWorldTransform(), vecAbsDir );
	pPlayer->SetAbsVelocity( vecAbsDir * m_flPushForce );

	if ( m_iszJumpSound != NULL_STRING )
	{
		EmitSound( STRING( m_iszJumpSound ) );
	}

	m_OnJump.FireOutput( pPlayer, this );
}

BEGIN_DATADESC( CTriggerStun )

	// Function Pointers
	DEFINE_FUNCTION( StunThink ),

	// Fields

	DEFINE_KEYFIELD( m_flTriggerDelay, FIELD_FLOAT, "trigger_delay" ),
	DEFINE_KEYFIELD( m_flStunDuration, FIELD_FLOAT, "stun_duration" ),
	DEFINE_KEYFIELD( m_flMoveSpeedReduction, FIELD_FLOAT, "move_speed_reduction" ),
	DEFINE_KEYFIELD( m_iStunType, FIELD_INTEGER, "stun_type"  ),
	DEFINE_KEYFIELD( m_bStunEffects, FIELD_INTEGER, "stun_effects"  ),

	DEFINE_UTLVECTOR( m_stunEntities, FIELD_EHANDLE ),

	// Outputs
	DEFINE_OUTPUT( m_OnStunPlayer, "OnStunPlayer" ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( trigger_stun, CTriggerStun );


//-----------------------------------------------------------------------------
// Purpose: Called when spawning, after keyvalues have been handled.
//-----------------------------------------------------------------------------
void CTriggerStun::Spawn( void )
{
	BaseClass::Spawn();

	InitTrigger();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}

//-----------------------------------------------------------------------------
// Purpose: When touched, a stun trigger applies its stunflags to the other for a duration.
// Input  : pOther - The entity that is touching us.
//-----------------------------------------------------------------------------
bool CTriggerStun::StunEntity( CBaseEntity *pOther )
{
	if ( !pOther->m_takedamage || !PassesTriggerFilters( pOther ) )
		return false;

	CTFPlayer* pTFPlayer = ToTFPlayer( pOther );
	if ( !pTFPlayer )
		return false;

	int iStunFlags = TF_STUN_MOVEMENT;
	switch ( m_iStunType )
	{
		case 0:
			// Movement Only
			break;
		case 1:
			// Controls
			iStunFlags |= TF_STUN_CONTROLS;
			break;
		case 2:
			// Loser State
			iStunFlags |= TF_STUN_LOSER_STATE;
			break;
	}

	if ( !m_bStunEffects )
	{
		iStunFlags |= TF_STUN_NO_EFFECTS;
	}

	iStunFlags |= TF_STUN_BY_TRIGGER;

	pTFPlayer->m_Shared.StunPlayer( m_flStunDuration, m_flMoveSpeedReduction, iStunFlags, NULL );

	m_OnStunPlayer.FireOutput( pOther, this );
	m_stunEntities.AddToTail( (EHANDLE)pOther );

	return true;
}

void CTriggerStun::StunThink()
{
	// If I stun anyone, think again.
	if ( StunAllTouchers( 0.5f ) <= 0 )
	{
		SetThink( NULL );
	}
	else
	{
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
}

void CTriggerStun::EndTouch( CBaseEntity *pOther )
{
	if ( PassesTriggerFilters( pOther ) )
	{
		EHANDLE hOther;
		hOther = pOther;

		// If this guy has never been stunned...
		if ( !m_stunEntities.HasElement( hOther ) )
		{
			StunEntity( pOther );
		}
	}

	BaseClass::EndTouch( pOther );
}

int CTriggerStun::StunAllTouchers( float dt )
{
	m_flLastStunTime = gpGlobals->curtime;
	m_stunEntities.RemoveAll();

	int stunCount = 0;
	touchlink_t *root = (touchlink_t *)GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			CBaseEntity *pTouch = link->entityTouched;
			if ( pTouch )
			{
				if ( StunEntity( pTouch ) )
				{
					stunCount++;
				}
			}
		}
	}

	return stunCount;
}

void CTriggerStun::Touch( CBaseEntity *pOther )
{
	if ( m_pfnThink == NULL )
	{
		SetThink( &CTriggerStun::StunThink );
		SetNextThink( gpGlobals->curtime + m_flTriggerDelay );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Ignites the arrows of any bow carried by a player who touches this trigger
//-----------------------------------------------------------------------------
class CTriggerIgniteArrows : public CBaseTrigger
{
public:
	DECLARE_CLASS( CTriggerIgniteArrows, CBaseTrigger );

	void Spawn( void );
	void Touch( CBaseEntity *pOther );

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CTriggerIgniteArrows )
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_ignite_arrows, CTriggerIgniteArrows );



void CTriggerIgniteArrows::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();
}

//-----------------------------------------------------------------------------
// Purpose: Ignites the arrows of any bow carried by a player who touches this trigger
//-----------------------------------------------------------------------------
void CTriggerIgniteArrows::Touch( CBaseEntity *pOther )
{
	// Purposefully ignore the trigger filters.

	if (V_strcmp(pOther->GetClassname(), "tf_projectile_arrow") == 0)
	{
		// Light any arrows that fly by.
		CTFProjectile_Arrow* pArrow = assert_cast<CTFProjectile_Arrow*>(pOther);
		pArrow->SetArrowAlight(true);
		return;
	}

	if ( !PassesTriggerFilters( pOther ) )
		return;

	if ( !pOther->IsPlayer() )
		return;

	// Ignore non-Snipers
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if ( !pPlayer )
		return;

	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if (pWpn)
	{
		if (pWpn->GetWeaponID() == TF_WEAPON_COMPOUND_BOW)
		{
			// Make sure they're looking at the origin
			Vector vecPos, vecForward, vecUp, vecRight;
			pPlayer->EyePositionAndVectors(&vecPos, &vecForward, &vecUp, &vecRight);
			Vector vTargetDir = GetAbsOrigin() - vecPos;
			VectorNormalize(vTargetDir);
			float fDotPr = DotProduct(vecForward, vTargetDir);
			if (fDotPr >= 0.95)
			{
				CTFCompoundBow *pBow = static_cast<CTFCompoundBow *>(pWpn);
				pBow->SetArrowAlight(true);
			}
		}
	}
}