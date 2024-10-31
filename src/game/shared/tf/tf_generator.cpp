//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========
//
// Purpose: Generator objective in the Power Siege game mode.
//
//=============================================================================

#include "cbase.h"
#include "tf_generator.h"
#include "IEffects.h"

#ifdef TF_GENERATOR

#define	GENERATOR_MODEL	"models/generator/generator/generator.mdl"
#define GENERATOR_HEALTH 5000
#define EXPLOSION_EFFECT "mvm_tank_destroy_bloom"
#define GENERATOR_DAMAGE_SOUND "Generator.Damage"

#define	SHIELD_MODEL "models/generator/forcefield/forcefield.mdl"
#define SHIELD_HEALTH 1000

#define	SHIELD_DEPLOY_SOUND "Generator.ShieldDeploy"
#define SHIELD_RETRACT_SOUND "Generator.ShieldRetract"
#define SHIELD_DAMAGE_SOUND "Generator.ShieldDamage"
#define SHIELD_RETRACT_SPEED 1.0f
#define SHIELD_STARTUP_TIME 10.0f

#define DMG_FALLOFF_MAX_RANGE 1024*1024
#define DMG_FALLOFF_MIN_RANGE 160*160	// Compensates for radius of entity some.

ConVar tf2c_powersiege_shield_regen_start_health("tf2c_powersiege_shield_regen_start_health", "200", FCVAR_NOTIFY | FCVAR_REPLICATED, "The shield health start value after respawning.");
ConVar tf2c_powersiege_shield_regen_rate("tf2c_powersiege_shield_regen_rate", "50", FCVAR_NOTIFY | FCVAR_REPLICATED, "The amount of hp added each second to the shield.");
ConVar tf2c_powersiege_shield_health_regain_time("tf2c_powersiege_shield_health_regain_time", "8", FCVAR_NOTIFY | FCVAR_REPLICATED, "The amount of seconds of non-damage it takes for a generator to start regaining health.");

IMPLEMENT_NETWORKCLASS_ALIASED(TeamShield, DT_Shield)

BEGIN_NETWORK_TABLE(CTeamShield, DT_Shield)
#ifdef GAME_DLL
SendPropFloat( SENDINFO( m_flHealthPercentage ) )
#else
RecvPropFloat( RECVINFO( m_flHealthPercentage ) )
#endif
END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( team_shield, CTeamShield );
#ifdef GAME_DLL

BEGIN_DATADESC( CTeamShield )
DEFINE_THINKFUNC( Enable ),
DEFINE_THINKFUNC( Disable ),
END_DATADESC()

CTeamShield::CTeamShield( void )
{

}

void CTeamShield::ChangeTeam( int iTeamNum )
{
	BaseClass::ChangeTeam( iTeamNum );
	
	m_nSkin = GetTeamSkin( iTeamNum );
}

void CTeamShield::Precache( void )
{
	PrecacheModel( SHIELD_MODEL );

	BaseClass::Precache();
}

void CTeamShield::Spawn( void )
{
	Precache();

	SetModel( SHIELD_MODEL );
	SetSolid( SOLID_VPHYSICS );
	AddEffects( EF_NOSHADOW );
	SetHealth( SHIELD_HEALTH );
	m_flHealthPercentage = ((float)m_iHealth) / SHIELD_HEALTH;

	PrecacheScriptSound( SHIELD_DEPLOY_SOUND );
	PrecacheScriptSound( SHIELD_RETRACT_SOUND );
	PrecacheScriptSound( "Player.ResistanceLight" );

	m_takedamage = DAMAGE_YES;
	m_bEnabled = true;
	
	RegisterThinkContext( "ShieldEnable" );
	RegisterThinkContext( "ShieldDisable" );
	// Next think time set to -1 since we don't need to think right now.
	SetContextThink( &CTeamShield::Enable, -1, "ShieldEnable" );
	SetContextThink( &CTeamShield::Disable, -1, "ShieldDisable" );
}

void CTeamShield::AdjustDamage( CTakeDamageInfo& info )
{
	// Shield takes 25% damage from minigun. Temp solution until proper systematical one.
	// Modifies whole damage as crits vulnerability is not implemented
	CTFWeaponBase* pWeapon = dynamic_cast<CTFWeaponBase*>(info.GetWeapon());
	if (pWeapon)
	{
		if (pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN)
		{
			info.ScaleDamage(0.25);
		}
		else if (pWeapon->GetWeaponID() == TF_WEAPON_NAILGUN || pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER)
		{
			info.ScaleDamage(0.5);
		}
	}

	// Lower damage at longer distances. Simple imitation of standard damage falloff.
	if ( info.GetAttacker() && info.GetDamageType() & DMG_USEDISTANCEMOD )
	{
		float fDamageMult = 1.0;
		Vector vAttackerPos = info.GetAttacker()->GetAbsOrigin();
		float fAttackerDistSqr = vAttackerPos.DistToSqr(GetAbsOrigin());
		DevMsg( "Attacker dist: %2.2f \n", sqrt(fAttackerDistSqr) );
		fDamageMult = RemapValClamped( fAttackerDistSqr, DMG_FALLOFF_MIN_RANGE, DMG_FALLOFF_MAX_RANGE, 1.0, 0.5 );
		info.ScaleDamage( fDamageMult );
	}
}

int CTeamShield::OnTakeDamage( const CTakeDamageInfo &info )
{
	// The owning team can't damage its own shield
	if( info.GetAttacker() )
	{
		if( GetTeamNumber() == info.GetAttacker()->GetTeamNumber() )
			return 0;
	}

	int iOldHealth = m_iHealth;

	auto infoNew = info;
	AdjustDamage( infoNew );

	DevMsg( "Shield damage HP: %i | DMG: %2.2f \n", m_iHealth.Get() , infoNew.GetDamage() );

	// Placeholder feedback.
	EmitSound( SHIELD_DAMAGE_SOUND );
	Vector vDir = GetAbsOrigin() - info.GetAttacker()->EyePosition();
	g_pEffects->EnergySplash( info.GetDamagePosition(), vDir.Normalized() );

	int iToReturn = BaseClass::OnTakeDamage(infoNew);

	if (iToReturn)
	{
		// TODO: Make it predict result instead of comparing actual health, like they do in other cases
		IGameEvent *event = gameeventmanager->CreateEvent("npc_hurt");
		if (event)
		{
			CTFPlayer *pTFAttacker = ToTFPlayer(infoNew.GetAttacker());
			CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase *>(infoNew.GetWeapon());

			event->SetInt("entindex", entindex());
			event->SetInt("attacker_player", pTFAttacker ? pTFAttacker->GetUserID() : 0);
			event->SetInt("weaponid", pTFWeapon ? pTFWeapon->GetWeaponID() : TF_WEAPON_NONE);
			event->SetInt("damageamount", iOldHealth - m_iHealth);
			event->SetInt("health", Max(0, m_iHealth.Get()));
			event->SetBool("crit", false);
			event->SetBool("boss", false);

			event->SetFloat("x", infoNew.GetDamagePosition().x);
			event->SetFloat("y", infoNew.GetDamagePosition().y);
			event->SetFloat("z", infoNew.GetDamagePosition().z);

			gameeventmanager->FireEvent(event);
		}
	}

	// If the shield is not dead yet it can regain health after x amount of seconds.
	if (m_iHealth > 0) 
	{
		SetNextThink(gpGlobals->curtime + tf2c_powersiege_shield_health_regain_time.GetInt(), "ShieldEnable");
	}

	m_flHealthPercentage = ((float)max(0, m_iHealth)) / SHIELD_HEALTH;

	return iToReturn;
}

// Never actually destroy the shield since it has to respawn.
void CTeamShield::Event_Killed( const CTakeDamageInfo &info )
{
	SetModelScale( 0.4f, SHIELD_RETRACT_SPEED );
	EmitSound( SHIELD_RETRACT_SOUND );
	m_takedamage = DAMAGE_NO;
	
	SetNextThink( gpGlobals->curtime + SHIELD_RETRACT_SPEED, "ShieldDisable" );
	
	// After being disabled it takes some time to start back up
	SetNextThink( gpGlobals->curtime + SHIELD_STARTUP_TIME, "ShieldEnable" );

	CTeamGenerator* pParentGenerator = dynamic_cast<CTeamGenerator*>(GetParent());
	if (pParentGenerator)
	{
		pParentGenerator->m_OnShieldDestroyed.FireOutput( info.GetAttacker(), this );
	}
}

void CTeamShield::Enable( void )
{
	// Shield enable regen phase.
	if ( m_bEnabled && m_iHealth < SHIELD_HEALTH )
	{		
		SetHealth( min( m_iHealth + tf2c_powersiege_shield_regen_rate.GetInt(), SHIELD_HEALTH ) );
		m_flHealthPercentage = ((float)m_iHealth) / SHIELD_HEALTH;
		DevMsg("Shield health %i\n", m_iHealth.Get() );

		if(m_iHealth != SHIELD_HEALTH)
			SetNextThink(gpGlobals->curtime + 1, "ShieldEnable");

		return;
	}

	// When the shield has reduced to its minmum size make it invisible and non solid.
	RemoveEffects( EF_NODRAW );
	SetSolid( SOLID_VPHYSICS );

	// Allow the shield to take damage again and play the deploy sound to alert people that the shield is back up.
	m_takedamage = DAMAGE_YES;
	EmitSound( SHIELD_DEPLOY_SOUND );
	SetHealth(tf2c_powersiege_shield_regen_start_health.GetInt());
	m_flHealthPercentage = ((float)m_iHealth) / SHIELD_HEALTH;
	SetModelScale(1.0f, SHIELD_RETRACT_SPEED);

	m_bEnabled = true;

	CTeamGenerator* pParentGenerator = dynamic_cast<CTeamGenerator*>(GetParent());
	if (pParentGenerator)
	{
		pParentGenerator->m_OnShieldRespawned.FireOutput( this, this );
	}

	SetNextThink(gpGlobals->curtime + 1, "ShieldEnable");
}

void CTeamShield::Disable( void )
{
	// When the shield has reduced to its minimum size make it invisible and non solid.
	AddEffects( EF_NODRAW );
	SetSolid( SOLID_NONE );

	m_bEnabled = false;
}

//-----------------------------------------------------------------------------
// Debug infos
//-----------------------------------------------------------------------------
int CTeamShield::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		char tempstr[512];

		V_sprintf_safe( tempstr, "TeamNumber: %d", GetTeamNumber() );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		V_sprintf_safe( tempstr, "Shield HP: %d / %d", GetHealth(), SHIELD_HEALTH );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		V_sprintf_safe( tempstr, "Recharge: %i", GetNextThinkTick( "ShieldEnable" ) );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;
	}

	return text_offset;
}

int CTeamShield::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TeamGenerator, DT_Generator )

BEGIN_NETWORK_TABLE( CTeamGenerator, DT_Generator )
#ifdef GAME_DLL
SendPropFloat( SENDINFO(m_flHealthPercentage) ),
SendPropEHandle( SENDINFO(m_hShield ) )
#else
RecvPropFloat( RECVINFO( m_flHealthPercentage ) ),
RecvPropEHandle( RECVINFO( m_hShield ) )
#endif
END_NETWORK_TABLE()

#ifdef GAME_DLL
BEGIN_DATADESC( CTeamGenerator )
DEFINE_KEYFIELD( m_szModel, FIELD_STRING, "generator_model" ),
DEFINE_OUTPUT(m_OnDestroyed, "OnDestroyed"),
DEFINE_OUTPUT(m_OnHealthBelow90Percent, "OnHealthBelow90Percent"), //These would actually mean below or equal
DEFINE_OUTPUT(m_OnHealthBelow80Percent, "OnHealthBelow80Percent"),
DEFINE_OUTPUT(m_OnHealthBelow70Percent, "OnHealthBelow70Percent"),
DEFINE_OUTPUT(m_OnHealthBelow60Percent, "OnHealthBelow60Percent"),
DEFINE_OUTPUT(m_OnHealthBelow50Percent, "OnHealthBelow50Percent"),
DEFINE_OUTPUT(m_OnHealthBelow40Percent, "OnHealthBelow40Percent"),
DEFINE_OUTPUT(m_OnHealthBelow30Percent, "OnHealthBelow30Percent"),
DEFINE_OUTPUT(m_OnHealthBelow20Percent, "OnHealthBelow20Percent"),
DEFINE_OUTPUT(m_OnHealthBelow10Percent, "OnHealthBelow10Percent"),
DEFINE_OUTPUT(m_OnShieldRespawned, "OnShieldRespawned"),
DEFINE_OUTPUT(m_OnShieldDestroyed, "OnShieldDestroyed"),
DEFINE_INPUTFUNC(FIELD_VOID, "ShieldDisable", InputShieldDisable),
DEFINE_INPUTFUNC(FIELD_VOID, "ShieldEnable", InputShieldEnable)
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS( team_generator, CTeamGenerator );

#ifdef GAME_DLL
CTeamGenerator::CTeamGenerator( void )
{
	m_szModel = MAKE_STRING( GENERATOR_MODEL );
	m_hShield = NULL;
}


void CTeamGenerator::ChangeTeam( int iTeamNum )
{
	BaseClass::ChangeTeam( iTeamNum );

	if( m_hShield->Get() )
		m_hShield.Get()->ChangeTeam( GetTeamNumber() );

	m_nSkin = GetTeamSkin( iTeamNum );
}

void CTeamGenerator::Precache( void )
{
	PrecacheModel( STRING(m_szModel) );
	PrecacheScriptSound( "Generator.DamageWarning" );
	PrecacheScriptSound( "Generator.Explode" );
	PrecacheParticleSystem( EXPLOSION_EFFECT );

	BaseClass::Precache();
}


void CTeamGenerator::Spawn( void )
{
	Precache();

	SetModel( STRING(m_szModel) );
	SetSolid( SOLID_VPHYSICS );
	SetHealth( GENERATOR_HEALTH );
	m_flHealthPercentage = 1.0f;

	m_takedamage = DAMAGE_YES;

	m_hShield = CBaseEntity::Create( "team_shield", GetAbsOrigin(), GetAbsAngles(), this );
	m_hShield.Get()->SetParent( this );
}

void CTeamGenerator::AdjustDamage( CTakeDamageInfo& info )
{
	// Generator takes 25% damage from minigun. Temp solution until proper systematical one.
	// Modifies whole damage as crits vulnerability is not implemented
	CTFWeaponBase* pWeapon = dynamic_cast<CTFWeaponBase*>(info.GetWeapon());
	if (pWeapon)
	{
		if (pWeapon->GetWeaponID() == TF_WEAPON_MINIGUN)
		{
			info.ScaleDamage(0.25);
		}
		else if (pWeapon->GetWeaponID() == TF_WEAPON_NAILGUN || pWeapon->GetWeaponID() == TF_WEAPON_FLAMETHROWER)
		{
			info.ScaleDamage(0.5);
		}
	}

	// Lower damage at longer distances. Simple imitation of standard damage falloff.
	if ( info.GetAttacker() && info.GetDamageType() & DMG_USEDISTANCEMOD )
	{
		float fDamageMult = 1.0;
		Vector vAttackerPos = info.GetAttacker()->GetAbsOrigin();
		float fAttackerDistSqr = vAttackerPos.DistToSqr(GetAbsOrigin());
		DevMsg( "Attacker dist: %2.2f \n", sqrt(fAttackerDistSqr) );
		fDamageMult = RemapValClamped( fAttackerDistSqr, DMG_FALLOFF_MIN_RANGE, DMG_FALLOFF_MAX_RANGE, 1.0, 0.5 );
		info.ScaleDamage( fDamageMult );
	}
}

int CTeamGenerator::OnTakeDamage( const CTakeDamageInfo &info )
{
	// The owning team can't damage its own generator
	if( info.GetAttacker() )
	{
		if( GetTeamNumber() == info.GetAttacker()->GetTeamNumber() )
			return 0;
	}

	// We can't damage generator while shield is up
	// Temp solution
	if (m_hShield.Get() && m_hShield.Get()->m_takedamage != DAMAGE_NO)
		return 0;

	int iHealth = GetHealth();
	int iNewHealth = GetHealth() - ( int ) info.GetDamage();

	// Play a warning sound for every 1000 hp taken away from the generator
	if( ( iHealth / 1000 > iNewHealth / 1000 ) && iNewHealth <= GENERATOR_HEALTH - 1000 )
	{
		if( TFGameRules() )
		{
			TFGameRules()->BroadcastSound( 255, "mvm/mvm_bomb_warning.wav" ); // 255 makes the sound play for all teams.
		}
	}

	if ( m_hShield.Get() )
	{
		CTeamShield* shield = dynamic_cast<CTeamShield*> (m_hShield.Get().Get());
		shield->SetNextThink(gpGlobals->curtime + SHIELD_STARTUP_TIME, "ShieldEnable");
	}

	int iOldHealth = m_iHealth;

	auto infoNew = info;
	AdjustDamage( infoNew );

	DevMsg( "Generator damage HP: %i | DMG: %2.2f \n", iNewHealth, infoNew.GetDamage() );

	// Placeholder feedback.
	EmitSound( GENERATOR_DAMAGE_SOUND );
	Vector vDir = GetAbsOrigin() - info.GetDamagePosition();
	g_pEffects->MetalSparks( info.GetDamagePosition(), vDir.Normalized() );

	int iToReturn = BaseClass::OnTakeDamage(infoNew);

	if (iToReturn)
	{
		// TODO: Make it predict result instead of comparing actual health, like they do in other cases
		IGameEvent *event = gameeventmanager->CreateEvent("npc_hurt");
		if (event)
		{
			CTFPlayer *pTFAttacker = ToTFPlayer(infoNew.GetAttacker());
			CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase *>(infoNew.GetWeapon());

			event->SetInt("entindex", entindex());
			event->SetInt("attacker_player", pTFAttacker ? pTFAttacker->GetUserID() : 0);
			event->SetInt("weaponid", pTFWeapon ? pTFWeapon->GetWeaponID() : TF_WEAPON_NONE);
			event->SetInt("damageamount", iOldHealth - m_iHealth);
			event->SetInt("health", Max(0, m_iHealth.Get()));
			event->SetBool("crit", false);
			event->SetBool("boss", false);

			event->SetFloat("x", infoNew.GetDamagePosition().x);
			event->SetFloat("y", infoNew.GetDamagePosition().y);
			event->SetFloat("z", infoNew.GetDamagePosition().z);

			gameeventmanager->FireEvent(event);
		}

		//Oh BOY! It's the stairs!
		if (m_iHealth > 0 && iOldHealth > GENERATOR_HEALTH * 0.1 && m_iHealth <= GENERATOR_HEALTH * 0.1)
		{
			m_OnHealthBelow10Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		else if (iOldHealth > GENERATOR_HEALTH * 0.2 && m_iHealth <= GENERATOR_HEALTH * 0.2)
		{
			m_OnHealthBelow20Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		else if (iOldHealth > GENERATOR_HEALTH * 0.3 && m_iHealth <= GENERATOR_HEALTH * 0.3)
		{
			m_OnHealthBelow30Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		else if (iOldHealth > GENERATOR_HEALTH * 0.4 && m_iHealth <= GENERATOR_HEALTH * 0.4)
		{
			m_OnHealthBelow40Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		else if (iOldHealth > GENERATOR_HEALTH * 0.5 && m_iHealth <= GENERATOR_HEALTH * 0.5)
		{
			m_OnHealthBelow50Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		else if (iOldHealth > GENERATOR_HEALTH * 0.6 && m_iHealth <= GENERATOR_HEALTH * 0.6)
		{
			m_OnHealthBelow60Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		else if (iOldHealth > GENERATOR_HEALTH * 0.7 && m_iHealth <= GENERATOR_HEALTH * 0.7)
		{
			m_OnHealthBelow70Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		else if (iOldHealth > GENERATOR_HEALTH * 0.8 && m_iHealth <= GENERATOR_HEALTH * 0.8)
		{
			m_OnHealthBelow80Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		else if (iOldHealth > GENERATOR_HEALTH * 0.9 && m_iHealth <= GENERATOR_HEALTH * 0.9)
		{
			m_OnHealthBelow90Percent.FireOutput(infoNew.GetAttacker(), this);
		}
		// You made it!
	}

	m_flHealthPercentage = ((float) m_iHealth)/GENERATOR_HEALTH;

	return iToReturn;
}

void CTeamGenerator::Event_Killed( const CTakeDamageInfo &info )
{
	EmitSound( "Generator.Explode" );
	CPVSFilter filter( GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0.0, EXPLOSION_EFFECT, WorldSpaceCenter(), GetAbsAngles() );

	m_OnDestroyed.FireOutput(info.GetAttacker(), this);

	BaseClass::Event_Killed( info );
}

//-----------------------------------------------------------------------------
// Debug infos
//-----------------------------------------------------------------------------
int CTeamGenerator::DrawDebugTextOverlays( void )
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		char tempstr[512];

		V_sprintf_safe( tempstr, "TeamNumber: %d", GetTeamNumber() );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;

		V_sprintf_safe( tempstr, "Health: %d / %d", GetHealth(), GENERATOR_HEALTH );
		EntityText( text_offset, tempstr, 0 );
		text_offset++;
	}

	return text_offset;
}

void CTeamGenerator::InputShieldDisable( inputdata_t& inputData ) 
{
	if ( m_hShield->Get() )
		UTIL_Remove(m_hShield->Get());
}

void CTeamGenerator::InputShieldEnable(inputdata_t& inputData)
{
	if (m_hShield->Get())
		return;

	m_hShield = CBaseEntity::Create("team_shield", GetAbsOrigin(), GetAbsAngles(), this);
	m_hShield.Get()->SetParent(this);
}

int CTeamGenerator::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}
#endif
#endif

#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS( item_bonuspack, CBonusPack );

BEGIN_DATADESC( CBonusPack )
DEFINE_KEYFIELD( m_szModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_szPickupSoundPos, FIELD_STRING, "pickup_sound_pos" ),
DEFINE_KEYFIELD( m_szPickupSoundNeg, FIELD_STRING, "pickup_sound_neg" ),
DEFINE_OUTPUT( m_OnSpawn, "OnSpawn" ),
DEFINE_OUTPUT( m_OnPickup, "OnPickup" ),
DEFINE_OUTPUT( m_OnPickupTeam1, "OnPickupTeam1" ),
DEFINE_OUTPUT( m_OnPickupTeam2, "OnPickupTeam2" ),
DEFINE_OUTPUT( m_OnPickupTeam3, "OnPickupTeam3" ),
DEFINE_OUTPUT( m_OnPickupTeam4, "OnPickupTeam4" ),
END_DATADESC()

CBonusPack::CBonusPack( void )
{
	m_szModel = MAKE_STRING( "models/bots/bot_worker/bot_worker_powercore.mdl" );
	m_szPickupSoundPos = MAKE_STRING( "ui/chime_rd_2base_pos.wav" );
	m_szPickupSoundNeg = MAKE_STRING( "ui/chime_rd_2base_neg.wav" );
}

void CBonusPack::Spawn( void )
{
	Precache();

	SetModel( STRING( m_szModel ) );
	SetSequence( LookupSequence( "idle" ) );
	SetPlaybackRate( 1.0f );
	UseClientSideAnimation();

	SetSolid( SOLID_BBOX );
	SetSolidFlags( FSOLID_NOT_SOLID | FSOLID_TRIGGER );
	SetCollisionBounds( Vector( -19.5f, -22.5f, -6.5f ), Vector( 19.5f, 22.5f, 6.5f ) );

	SetTouch( &CBonusPack::OnTouch );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	
	m_OnSpawn.FireOutput( this, this );
}

void CBonusPack::Precache( void )
{
	PrecacheModel( STRING( m_szModel ) );
	PrecacheScriptSound( STRING( m_szPickupSoundPos) );
	PrecacheScriptSound( STRING( m_szPickupSoundNeg ) );

	BaseClass::Precache();
}

void CBonusPack::ChangeTeam( int iTeamNum )
{
	BaseClass::ChangeTeam( iTeamNum );
	m_nSkin = GetTeamSkin( iTeamNum );
}

void CBonusPack::OnTouch( CBaseEntity *pOther )
{
	// Only players can pickup the pack to score points.
	if( !pOther->IsPlayer() )
		return;

	// You can only pickup cores dropped from the other teams not your own.
	if( GetTeamNumber() != 0 && GetTeamNumber() == pOther->GetTeamNumber() )
		return;

	m_OnPickup.FireOutput( pOther, this );

	switch( pOther->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			m_OnPickupTeam1.FireOutput( pOther, this );
			break;

		case TF_TEAM_BLUE:
			m_OnPickupTeam2.FireOutput( pOther, this );
			break;

		case TF_TEAM_GREEN:
			m_OnPickupTeam3.FireOutput( pOther, this );
			break;

		case TF_TEAM_YELLOW:
			m_OnPickupTeam4.FireOutput( pOther, this );
			break;
	}

	if( TFGameRules() ) 
	{
		
		for( int iTeam = LAST_SHARED_TEAM + 1; iTeam < GetNumberOfTeams(); iTeam++ )
		{
			// Play positive sounds for the team that picked the pack up
			if( iTeam == pOther->GetTeamNumber() )
			{
				TFGameRules()->BroadcastSound( iTeam, STRING( m_szPickupSoundPos ) );
			}
			// Play negative sounds to the other teams.
			else
			{
				TFGameRules()->BroadcastSound( iTeam, STRING( m_szPickupSoundNeg ) );
			}
			
		}
	}
		
	Remove();
}
#endif
