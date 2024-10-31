//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//=============================================================================

#include "cbase.h"
#include "KeyValues.h"
#include "tf_playerclass_shared.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "tier2/tier2.h"
#include "baseplayer_shared.h"

#ifdef CLIENT_DLL
bool UseHWMorphModels();
#endif

const char *s_aPlayerClassFiles[TF_CLASS_COUNT_ALL] =
{
	"",
	"scripts/playerclasses/scout",
	"scripts/playerclasses/sniper",
	"scripts/playerclasses/soldier",
	"scripts/playerclasses/demoman",
	"scripts/playerclasses/medic",
	"scripts/playerclasses/heavyweapons",
	"scripts/playerclasses/pyro",
	"scripts/playerclasses/spy",
	"scripts/playerclasses/engineer",
	"scripts/playerclasses/civilian",
};

CTFPlayerClassDataMgr s_TFPlayerClassDataMgr;
CTFPlayerClassDataMgr *g_pTFPlayerClassDataMgr = &s_TFPlayerClassDataMgr; 

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TFPlayerClassData_t::TFPlayerClassData_t()
{
	m_szClassName[0] = '\0';
	m_szModelName[0] = '\0';
	m_szHWMModelName[0] = '\0';
	m_szModelHandsName[0] = '\0';
	m_szLocalizableName[0] = '\0';
	m_flMaxSpeed = 0.0f;
	m_flFlagMaxSpeed = 0.0f;
	m_nMaxHealth = 0;
	m_nMaxArmor = 0;

#ifdef GAME_DLL
	m_szDeathSound[0] = '\0';
	m_szCritDeathSound[0] = '\0';
	m_szMeleeDeathSound[0] = '\0';
	m_szExplosionDeathSound[0] = '\0';
#endif

	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		m_aWeapons[iWeapon] = TF_WEAPON_NONE;
	}

	for ( int iGrenade = 0; iGrenade < TF_PLAYER_GRENADE_COUNT; ++iGrenade )
	{
		m_aGrenades[iGrenade] = TF_WEAPON_NONE;
	}

	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		m_aAmmoMax[iAmmo] = TF_AMMO_DUMMY;
	}

	for ( int iBuildable = 0; iBuildable < TF_PLAYER_BUILDABLE_COUNT; ++iBuildable )
	{
		m_aBuildable[iBuildable] = OBJ_LAST;
	}

	m_bParsed = false;
}


void TFPlayerClassData_t::Parse( const char *szName )
{
	// Have we parsed this file already?
	if ( m_bParsed )
		return;

	// Parse class file.
	KeyValues *pKV = ReadEncryptedKVFile( filesystem, szName, GetTFEncryptionKey() );
	if ( pKV )
	{
		ParseData( pKV );
		pKV->deleteThis();
	}
}


const char *TFPlayerClassData_t::GetModelName() const
{
#ifdef CLIENT_DLL
	if ( UseHWMorphModels() )
	{
		if ( m_szHWMModelName[0] != '\0' )
		{
			return m_szHWMModelName;
		}
	}

	return m_szModelName;
#else
	return m_szModelName;
#endif
}


void TFPlayerClassData_t::ParseData( KeyValues *pKeyValuesData )
{
	// Attributes.
	V_strcpy_safe( m_szClassName, pKeyValuesData->GetString( "name" ) );

	// Load the high res model or the lower res model.
	V_strcpy_safe( m_szHWMModelName, pKeyValuesData->GetString( "model_hwm" ) );
	V_strcpy_safe( m_szModelName, pKeyValuesData->GetString( "model" ) );
	V_strcpy_safe( m_szModelHandsName, pKeyValuesData->GetString("model_hands") );
	V_strcpy_safe( m_szLocalizableName, pKeyValuesData->GetString( "localize_name" ) );

	m_flMaxSpeed = pKeyValuesData->GetFloat( "speed_max" );
	m_flFlagMaxSpeed = pKeyValuesData->GetFloat( "flag_speed_max", m_flMaxSpeed );
	m_nMaxHealth = pKeyValuesData->GetInt( "health_max" );
	m_nMaxArmor = pKeyValuesData->GetInt( "armor_max" );

	// Weapons.
	int i;
	char buf[32];
	for ( i = 0; i < TF_PLAYER_WEAPON_COUNT; i++ )
	{
		V_sprintf_safe( buf, "weapon%d", i+1 );		
		m_aWeapons[i] = GetWeaponId( pKeyValuesData->GetString( buf ) );
	}

	// Grenades.
	m_aGrenades[0] = GetWeaponId( pKeyValuesData->GetString( "grenade1" ) );
	m_aGrenades[1] = GetWeaponId( pKeyValuesData->GetString( "grenade2" ) );

	// Ammo Max.
	KeyValues *pAmmoKeyValuesData = pKeyValuesData->FindKey( "AmmoMax" );
	if ( pAmmoKeyValuesData )
	{
		for ( int iAmmo = 1; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			m_aAmmoMax[iAmmo] = pAmmoKeyValuesData->GetInt( g_aAmmoNames[iAmmo], 0 );
		}
	}

	// Buildables
	for ( i = 0; i < TF_PLAYER_BUILDABLE_COUNT; i++ )
	{
		V_sprintf_safe( buf, "buildable%d", i+1 );		
		m_aBuildable[i] = GetBuildableId( pKeyValuesData->GetString( buf ) );		
	}

//#ifdef GAME_DLL		// right now we only emit these sounds from server. if that changes we can do this in both dlls

	// Death Sounds
	V_strcpy_safe( m_szDeathSound, pKeyValuesData->GetString( "sound_death", "Player.Death" ) );
	V_strcpy_safe( m_szCritDeathSound, pKeyValuesData->GetString( "sound_crit_death", "TFPlayer.CritDeath" ) );
	V_strcpy_safe( m_szMeleeDeathSound, pKeyValuesData->GetString( "sound_melee_death", "Player.MeleeDeath" ) );
	V_strcpy_safe( m_szExplosionDeathSound, pKeyValuesData->GetString( "sound_explosion_death", "Player.ExplosionDeath" ) );

//#endif

	// The file has been parsed.
	m_bParsed = true;
}


CTFPlayerClassDataMgr::CTFPlayerClassDataMgr()
{

}


bool CTFPlayerClassDataMgr::Init( void )
{
	// Special case the undefined class.
	TFPlayerClassData_t *pClassData = &m_aTFPlayerClassData[TF_CLASS_UNDEFINED];
	Assert( pClassData );
	Q_strncpy( pClassData->m_szClassName, "undefined", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szModelName, "models/player/scout.mdl", TF_NAME_LENGTH );	// Undefined players still need a model
	Q_strncpy( pClassData->m_szModelHandsName, "models/weapons/c_models/c_scout_arms.mdl", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szLocalizableName, "undefined", TF_NAME_LENGTH );

	// Initialize the classes.
	for ( int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		COMPILE_TIME_ASSERT( ARRAYSIZE( s_aPlayerClassFiles ) == TF_CLASS_COUNT_ALL );
		pClassData = &m_aTFPlayerClassData[iClass];
		Assert( pClassData );
		pClassData->Parse( s_aPlayerClassFiles[iClass] );
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to get player class data.
//-----------------------------------------------------------------------------
TFPlayerClassData_t *CTFPlayerClassDataMgr::Get( unsigned int iClass )
{
	Assert( iClass < TF_CLASS_COUNT_ALL );
	return &m_aTFPlayerClassData[iClass];
}


TFPlayerClassData_t *GetPlayerClassData( unsigned int iClass )
{
	return g_pTFPlayerClassDataMgr->Get( iClass );
}

//=============================================================================
//
// Shared player class data.
//

//=============================================================================
//
// Tables.
//

#define CLASSMODEL_PARITY_BITS		3
#define CLASSMODEL_PARITY_MASK		( ( 1 << CLASSMODEL_PARITY_BITS ) - 1 )

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTFPlayerClassShared, DT_TFPlayerClassShared )
	RecvPropInt( RECVINFO( m_iClass ) ),

	// SetCustomModel
	RecvPropString( RECVINFO( m_iszCustomModel ) ),
	RecvPropVector( RECVINFO( m_vecCustomModelOffset ) ),
	RecvPropQAngles( RECVINFO( m_angCustomModelRotation ) ),
	RecvPropBool( RECVINFO( m_bCustomModelRotates ) ),
	RecvPropBool( RECVINFO( m_bCustomModelRotationSet ) ),
	RecvPropBool( RECVINFO( m_bCustomModelVisibleToSelf ) ),
	RecvPropBool( RECVINFO( m_bUseClassAnimations ) ),
	RecvPropInt( RECVINFO( m_iClassModelParity ) ),
END_RECV_TABLE()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTFPlayerClassShared, DT_TFPlayerClassShared )
	SendPropInt( SENDINFO( m_iClass ), Q_log2( TF_CLASS_COUNT_ALL ) + 1, SPROP_UNSIGNED ),

	// SetCustomModel
	SendPropStringT( SENDINFO( m_iszCustomModel ) ),
	SendPropVector( SENDINFO( m_vecCustomModelOffset ) ),
	SendPropQAngles( SENDINFO( m_angCustomModelRotation ) ),
	SendPropBool( SENDINFO( m_bCustomModelRotates ) ),
	SendPropBool( SENDINFO( m_bCustomModelRotationSet ) ),
	SendPropBool( SENDINFO( m_bCustomModelVisibleToSelf ) ),
	SendPropBool( SENDINFO( m_bUseClassAnimations ) ),
	SendPropInt( SENDINFO( m_iClassModelParity ), CLASSMODEL_PARITY_BITS, SPROP_UNSIGNED ),
END_SEND_TABLE()

#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPlayerClassShared::CTFPlayerClassShared()
{
	Reset();
}


void CTFPlayerClassShared::Reset( void )
{
	m_iClass.Set( TF_CLASS_UNDEFINED );
#ifdef CLIENT_DLL
	m_iszCustomModel[0] = '\0';
#else
	m_iszCustomModel.Set( NULL_STRING );
#endif
	m_vecCustomModelOffset = vec3_origin;
	m_angCustomModelRotation = vec3_angle;
	m_bCustomModelRotates = true;
	m_bCustomModelRotationSet = false;
	m_bCustomModelVisibleToSelf = true;
	m_bUseClassAnimations = false;
	m_iClassModelParity = 0;
}

#ifndef CLIENT_DLL

void CTFPlayerClassShared::SetCustomModel( const char *pszModelName, bool isUsingClassAnimations )
{
	if ( pszModelName && pszModelName[0] )
	{
		bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
		CBaseEntity::SetAllowPrecache( true );
		CBaseEntity::PrecacheModel( pszModelName );
		CBaseEntity::SetAllowPrecache( bAllowPrecache );

		m_iszCustomModel.Set( AllocPooledString( pszModelName ) );

		m_bUseClassAnimations = isUsingClassAnimations;
	}
	else
	{
		m_iszCustomModel.Set( NULL_STRING );
		m_vecCustomModelOffset = vec3_origin;
		m_angCustomModelRotation = vec3_angle;
	}

	m_iClassModelParity = (m_iClassModelParity + 1) & CLASSMODEL_PARITY_MASK;
}
#endif // #ifndef CLIENT_DLL


bool CTFPlayerClassShared::CustomModelHasChanged( void )
{
	if ( m_iClassModelParity != m_iOldClassModelParity )
	{
		m_iOldClassModelParity = m_iClassModelParity.Get();
		return true;
	}

	return false;
}


const char *CTFPlayerClassShared::GetModelName( void ) const						
{ 
	// Does this play have an overridden model?
#ifdef CLIENT_DLL
	if ( m_iszCustomModel[0] )
		return m_iszCustomModel;
#else
	if ( m_iszCustomModel.Get() != NULL_STRING )
		return ( STRING( m_iszCustomModel.Get() ) );
#endif

	#define MAX_MODEL_FILENAME_LENGTH 256
	static char modelFilename[ MAX_MODEL_FILENAME_LENGTH ];

	Q_strncpy( modelFilename, GetPlayerClassData( m_iClass )->GetModelName(), sizeof( modelFilename ) );

	return modelFilename;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class.
//-----------------------------------------------------------------------------
bool CTFPlayerClassShared::Init( int iClass )
{
	Assert ( ( iClass >= TF_FIRST_NORMAL_CLASS ) && ( iClass <= TF_CLASS_COUNT ) );
	m_iClass = iClass;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: If needed, put this into playerclass scripts
//-----------------------------------------------------------------------------
bool CTFPlayerClassShared::CanBuildObject( int iObjectType ) const
{
	bool bFound = false;

	TFPlayerClassData_t  *pData = GetData();

	for ( int i = 0; i < TF_PLAYER_BUILDABLE_COUNT; i++ )
	{
		if ( iObjectType == pData->m_aBuildable[i] )
		{
			bFound = true;
			break;
		}
	}

	return bFound;
}
