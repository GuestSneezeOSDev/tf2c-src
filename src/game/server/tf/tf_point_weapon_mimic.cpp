//=============================================================================
//
// Purpose:
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_rocket.h"
#include "tf_projectile_flare.h"
#include "tf_projectile_dart.h"
#include "tf_projectile_coil.h"
#include "tf_weapon_coilgun.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_grenade_stickybomb.h"
#include "tf_weapon_grenade_mirv.h"
#include "tf_projectile_arrow.h"
#include "particle_parse.h"

class CTFBaseGrenade;

class CTFPointWeaponMimic : public CPointEntity
{
public:
	DECLARE_CLASS( CTFPointWeaponMimic, CPointEntity );
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );

	void GetFiringAngles( QAngle &angShoot );
	void Fire( void );
	void DoFireEffects( void );
	void FireRocket( void );
	void FireGrenade( void );
	void FireStickyGrenade( void );
	void FireArrow( void );

	void InputFire( inputdata_t &inputdata );
	void InputFireMultiple( inputdata_t &inputdata );
	void InputDetonateStickies( inputdata_t &inputdata );

private:
	enum
	{
		TF_WEAPON_MIMIC_ROCKET,
		TF_WEAPON_MIMIC_GRENADE,
		TF_WEAPON_MIMIC_ARROW,
		TF_WEAPON_MIMIC_STICKY,
		TF_WEAPON_MIMIC_FLARE,
		TF_WEAPON_MIMIC_DART,
		TF_WEAPON_MIMIC_COIL,
		TF_WEAPON_MIMIC_MIRVLET
	};

	int m_iWeaponType;

	string_t m_iszFireSound;
	string_t m_iszFireEffect;
	string_t m_iszModelOverride;
	float m_flModelScale;

	float m_flMinSpeed;
	float m_flMaxSpeed;
	float m_flDamage;
	float m_flRadius;
	float m_flSpread;
	bool m_bCritical;

	CUtlVector<CHandle<CTFBaseGrenade>> m_Stickies;
};

BEGIN_DATADESC( CTFPointWeaponMimic )
DEFINE_KEYFIELD( m_iWeaponType, FIELD_INTEGER, "WeaponType" ),
DEFINE_KEYFIELD( m_iszFireSound, FIELD_STRING, "FireSound" ),
DEFINE_KEYFIELD( m_iszFireEffect, FIELD_STRING, "ParticleEffect" ),
DEFINE_KEYFIELD( m_iszModelOverride, FIELD_STRING, "ModelOverride" ),
DEFINE_KEYFIELD( m_flModelScale, FIELD_FLOAT, "ModelScale" ),
DEFINE_KEYFIELD( m_flMinSpeed, FIELD_FLOAT, "SpeedMin" ),
DEFINE_KEYFIELD( m_flMaxSpeed, FIELD_FLOAT, "SpeedMax" ),
DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "Damage" ),
DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "SplashRadius" ),
DEFINE_KEYFIELD( m_flSpread, FIELD_FLOAT, "SpreadAngle" ),
DEFINE_KEYFIELD( m_bCritical, FIELD_BOOLEAN, "Crits" ),
DEFINE_KEYFIELD( m_iTeamNum, FIELD_INTEGER, "TeamNum" ),

DEFINE_INPUTFUNC( FIELD_VOID, "FireOnce", InputFire ),
DEFINE_INPUTFUNC( FIELD_INTEGER, "FireMultiple", InputFireMultiple ),
DEFINE_INPUTFUNC( FIELD_VOID, "DetonateStickies", InputDetonateStickies ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_point_weapon_mimic, CTFPointWeaponMimic );


void CTFPointWeaponMimic::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	ChangeTeam( GetTeamNumber() );
}


void CTFPointWeaponMimic::Precache( void )
{
	BaseClass::Precache();

	if ( m_iszFireSound != NULL_STRING )
		PrecacheScriptSound( STRING( m_iszFireSound ) );

	if ( m_iszFireEffect != NULL_STRING )
		PrecacheParticleSystem( STRING( m_iszFireEffect ) );

	if ( m_iszModelOverride != NULL_STRING )
		PrecacheModel( STRING( m_iszModelOverride ) );
}


void CTFPointWeaponMimic::GetFiringAngles( QAngle &angShoot )
{
	if ( m_flSpread == 0.0f )
	{
		angShoot = GetAbsAngles();
		return;
	}

	Vector vecForward, vecRight, vecUp;
	AngleVectors( GetAbsAngles(), &vecForward, &vecRight, &vecUp );

	float flSpreadMult = RandomFloat( -0.5, 0.5 ) + RandomFloat( -0.5, 0.5 );
	VMatrix mat1 = SetupMatrixAxisRot( vecForward, RandomFloat( -180.0f, 180.0f ) );
	VMatrix mat2 = SetupMatrixAxisRot( vecUp, m_flSpread * flSpreadMult );

	VMatrix matResult;
	MatrixMultiply( mat1, mat2, matResult );
	MatrixToAngles( matResult, angShoot );
}


void CTFPointWeaponMimic::Fire( void )
{
	switch ( m_iWeaponType )
	{
	case TF_WEAPON_MIMIC_ROCKET:
	case TF_WEAPON_MIMIC_FLARE:
	case TF_WEAPON_MIMIC_DART:
	case TF_WEAPON_MIMIC_COIL:
		FireRocket();
		break;
	case TF_WEAPON_MIMIC_GRENADE:
	case TF_WEAPON_MIMIC_MIRVLET:
		FireGrenade();
		break;
	case TF_WEAPON_MIMIC_ARROW:
		FireArrow();
		break;
	case TF_WEAPON_MIMIC_STICKY:
		FireStickyGrenade();
		break;
	}
}


void CTFPointWeaponMimic::DoFireEffects( void )
{
	if ( m_iszFireSound != NULL_STRING )
		EmitSound( STRING( m_iszFireSound ) );

	if ( m_iszFireEffect != NULL_STRING )
		DispatchParticleEffect( STRING( m_iszFireEffect ), GetAbsOrigin(), GetAbsAngles() );
}


void CTFPointWeaponMimic::FireRocket( void )
{
	Vector vecForward;
	QAngle angForward;
	GetFiringAngles( angForward );

	CTFBaseRocket *pRocket = NULL;

	switch ( m_iWeaponType )
	{
	default:
	case TF_WEAPON_MIMIC_ROCKET:
		pRocket = CTFProjectile_Rocket::Create( NULL, GetAbsOrigin(), angForward, this, TF_PROJECTILE_ROCKET );
		break;
	case TF_WEAPON_MIMIC_FLARE:
		pRocket = CTFProjectile_Flare::Create( NULL, GetAbsOrigin(), angForward, this );
		break;
	case TF_WEAPON_MIMIC_DART:
		pRocket = CTFProjectile_Dart::Create( NULL, GetAbsOrigin(), angForward, this );
		break;
	case TF_WEAPON_MIMIC_COIL:
		pRocket = CTFProjectile_Coil::Create( NULL, GetAbsOrigin(), angForward, RandomFloat( m_flMinSpeed, m_flMaxSpeed ), this );
		break;
	}

	if ( pRocket )
	{
		if ( m_iszModelOverride != NULL_STRING )
			pRocket->SetModel( STRING( m_iszModelOverride ) );

		pRocket->SetModelScale( m_flModelScale );
		pRocket->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS_NOTSOLID );

		Vector vecForward, vecRight, vecUp;
		AngleVectors( angForward, &vecForward );

		float flSpeed = RandomFloat( m_flMinSpeed, m_flMaxSpeed );
		Vector vecVelocity = vecForward * flSpeed;
		pRocket->SetAbsVelocity( vecVelocity );
		pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

		pRocket->SetDamage( m_flDamage );
		pRocket->SetCritical( m_bCritical );
	}
}


void CTFPointWeaponMimic::FireGrenade( void )
{
	QAngle angForward;
	GetFiringAngles( angForward );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( angForward, &vecForward, &vecRight, &vecUp );

	Vector vecVelocity = ( vecForward * RandomFloat( m_flMinSpeed, m_flMaxSpeed ) ) +
		( vecUp * 200.0f ) +
		( RandomFloat( -10.0f, 10.0f ) * vecRight ) +
		( RandomFloat( -10.0f, 10.0f ) * vecUp );

	AngularImpulse angVelocity( 600, RandomInt( -1200, 1200 ), 0 );

	CTFBaseGrenade *pProjectile = NULL;

	switch ( m_iWeaponType )
	{
	default:
	case TF_WEAPON_MIMIC_GRENADE:
		pProjectile = CTFGrenadePipebombProjectile::Create( GetAbsOrigin(), angForward,
			vecVelocity, angVelocity,
			this, NULL, TF_PROJECTILE_PIPEBOMB );
		break;
	case TF_WEAPON_MIMIC_MIRVLET:
		pProjectile = CTFGrenadeMirvBomb::Create( GetAbsOrigin(), angForward,
			vecVelocity, angVelocity,
			ToBaseCombatCharacter( this ), RandomFloat( 1.0f, 2.0f ) );
		break;
	}

	if ( pProjectile )
	{
		if ( m_iszModelOverride != NULL_STRING )
		{
			pProjectile->VPhysicsDestroyObject();
			pProjectile->SetModel( STRING( m_iszModelOverride ) );
			pProjectile->VPhysicsInitNormal( SOLID_BBOX, FSOLID_TRIGGER, false );
		}

		pProjectile->SetModelScale( m_flModelScale );
		pProjectile->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );

		pProjectile->SetDamage( m_flDamage );
		pProjectile->SetCritical( m_bCritical );
		pProjectile->SetDamageRadius( m_flRadius );
	}
}


void CTFPointWeaponMimic::FireStickyGrenade( void )
{
	QAngle angForward;
	GetFiringAngles( angForward );

	Vector vecForward, vecRight, vecUp;
	AngleVectors( angForward, &vecForward, &vecRight, &vecUp );

	Vector vecVelocity = ( vecForward * RandomFloat( m_flMinSpeed, m_flMaxSpeed ) ) +
		( vecUp * 200.0f ) +
		( RandomFloat( -10.0f, 10.0f ) * vecRight ) +
		( RandomFloat( -10.0f, 10.0f ) * vecUp );

	AngularImpulse angVelocity( 600, RandomInt( -1200, 1200 ), 0 );

	CTFBaseGrenade *pProjectile = CTFGrenadeStickybombProjectile::Create( GetAbsOrigin(), angForward,
		vecVelocity, angVelocity,
		this, NULL );

	if ( pProjectile )
	{
		if ( m_iszModelOverride != NULL_STRING )
		{
			pProjectile->VPhysicsDestroyObject();
			pProjectile->SetModel( STRING( m_iszModelOverride ) );
			pProjectile->VPhysicsInitNormal( SOLID_BBOX, 0, false );
		}

		pProjectile->SetModelScale( m_flModelScale );
		pProjectile->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );

		pProjectile->SetDamage( m_flDamage );
		pProjectile->SetCritical( m_bCritical );
		pProjectile->SetDamageRadius( m_flRadius );

		m_Stickies.AddToTail( pProjectile );
	}
}


void CTFPointWeaponMimic::FireArrow( void )
{
	QAngle angForward;
	GetFiringAngles( angForward );

	CTFBaseRocket *pArrow = CTFProjectile_Arrow::Create( NULL, GetAbsOrigin(), angForward,
		RandomFloat( m_flMinSpeed, m_flMaxSpeed ), 0.7f,
		TF_PROJECTILE_ARROW,
		this );

	if ( pArrow )
	{
		if ( m_iszModelOverride != NULL_STRING )
			pArrow->SetModel( STRING( m_iszModelOverride ) );

		pArrow->SetModelScale( m_flModelScale );
		pArrow->SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS_NOTSOLID );

		pArrow->SetDamage( m_flDamage );
		pArrow->SetCritical( m_bCritical );
	}
}


void CTFPointWeaponMimic::InputFire( inputdata_t &inputdata )
{
	Fire();
	DoFireEffects();
}


void CTFPointWeaponMimic::InputFireMultiple( inputdata_t &inputdata )
{
	int count = inputdata.value.Int();
	for ( int i = 0; i < count; i++ )
	{
		Fire();
	}

	DoFireEffects();
}


void CTFPointWeaponMimic::InputDetonateStickies( inputdata_t &inputdata )
{
	if ( m_Stickies.Count() == 0 )
		return;

	for ( int i = 0; i < m_Stickies.Count(); i++ )
	{
		CTFBaseGrenade *pBomb = m_Stickies[i];
		if ( pBomb )
		{
			pBomb->Detonate();
		}
	}

	m_Stickies.Purge();
}
