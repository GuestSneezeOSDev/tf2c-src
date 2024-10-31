//=============================================================================//
//
// Purpose: Mangages certain aspects of the Randomizer Mode.
//
//=============================================================================//
#include "cbase.h"
#include "filesystem.h"

#include "tf_randomizer_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL
void RandomizerCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	ConVarRef randomizermode( var );
	GetRandomizerManager()->SetRandomizerMode( randomizermode.GetInt() );
}
ConVar tf2c_randomizer( "tf2c_randomizer", "0", FCVAR_NOTIFY, "Enables Randomizer Mode, players will spawn with random classes and/or loadouts and/or attributes\n(0 = Disabled, 1 = Classes, 2 = Items, 3 = Classes + Items, 4 = Attributes, 5 = Classes + Attributes, 6 = Items + Attributes, 7 = Classes + Items + Attributes)", RandomizerCallback );

ConVar tf2c_randomizer_script( "tf2c_randomizer_script", "cfg/randomizer.cfg", FCVAR_NOTIFY, "Script file that Randomizer Mode will load its data from" );
#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRandomizerAttribute::CTFRandomizerAttribute()
{
	m_iAttributeID = -1;
	m_flValueChance[0] = 0.0f;
	m_flValueChance[1] = 0.0f;
}


bool CTFRandomizerAttribute::IsBlacklistedItem( int iItemID )
{
	// This item is blacklisted, that's NOT okay!
	if ( m_vecBlacklistedItems.Find( iItemID ) )
		return true;

	return false;
}


bool CTFRandomizerAttribute::IsWhitelistedItem( int iItemID )
{
	// This item is whitelisted, that's VERY okay!
	if ( m_vecBlacklistedItems.Find( iItemID ) )
		return true;

	return false;
}


bool CTFRandomizerAttribute::IsBlacklistedEntity( const char *pszEntity )
{
	// This entity is blacklisted, that's NOT okay!
	FOR_EACH_VEC( m_vecBlacklistedEntities, i )
	{
		if ( !V_stricmp( m_vecBlacklistedEntities[i], pszEntity ) )
			return true;
	}

	return false;
}


bool CTFRandomizerAttribute::IsWhitelistedEntity( const char *pszEntity )
{
	// This entity is whitelisted, that's VERY okay!
	FOR_EACH_VEC( m_vecWhitelistedEntities, i )
	{
		if ( !V_stricmp( m_vecWhitelistedEntities[i], pszEntity ) )
			return true;
	}

	return false;
}


bool CTFRandomizerAttribute::IsBlacklistedProjectile( int iProjectile )
{
	// This entity is blacklisted, that's NOT okay!
	if ( m_vecBlacklistedProjectiles.Find( iProjectile ) )
		return true;

	return false;
}


bool CTFRandomizerAttribute::IsWhitelistedProjectile( int iProjectile )
{
	// This entity is whitelisted, that's VERY okay!
	if ( m_vecWhitelistedProjectiles.Find( iProjectile ) )
		return true;

	return false;
}


bool CTFRandomizerAttribute::HasRequirement( const char *pszRequirement )
{
	// This entity has this requirement, that's VERY okay!
	FOR_EACH_VEC( m_vecRequirements, i )
	{
		if ( !V_stricmp( m_vecRequirements[i], pszRequirement ) )
			return true;
	}

	return false;
}
#endif

struct RandomizerProjectiles_t
{
	const char			*name;
	ProjectileType_t	projectile;
};

RandomizerProjectiles_t SupportedRandomizerProjectiles[] = 
{
	{ "none",			TF_PROJECTILE_NONE				},
	{ "bullet",			TF_PROJECTILE_BULLET			},
	{ "rocket",			TF_PROJECTILE_ROCKET			},
	{ "grenade",		TF_PROJECTILE_PIPEBOMB			},
	{ "stickybomb",		TF_PROJECTILE_PIPEBOMB_REMOTE	},
	{ "mirv",			TF_PROJECTILE_MIRV				},
	{ "arrow",			TF_PROJECTILE_ARROW				},
	// TODO poke Hogyn to make him add the heal grenades
};

// ------------------------------------------------------------------------------------ //
// CTFRandomizerManagerProxy implementation.
// ------------------------------------------------------------------------------------ //
CTFRandomizerManagerProxy *CTFRandomizerManagerProxy::s_pRandomizerManagerProxy = NULL;

IMPLEMENT_NETWORKCLASS_ALIASED( TFRandomizerManagerProxy, DT_TFRandomizerManagerProxy )

#ifdef CLIENT_DLL

void RecvProxy_TFRandomizerManager( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
	CTFRandomizerManager *pManager = GetRandomizerManager();
	Assert( pManager );
	*pOut = pManager;
}

BEGIN_RECV_TABLE( CTFRandomizerManagerProxy, DT_TFRandomizerManagerProxy )
	RecvPropDataTable( "tf_randomizer_manager_data", 0, 0, &REFERENCE_RECV_TABLE( DT_TFRandomizerManager ), RecvProxy_TFRandomizerManager )
END_RECV_TABLE()
#else

void *SendProxy_TFRandomizerManager( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
	CTFRandomizerManager *pManager = GetRandomizerManager();
	Assert( pManager );
	pRecipients->SetAllRecipients();
	return pManager;
}

BEGIN_SEND_TABLE( CTFRandomizerManagerProxy, DT_TFRandomizerManagerProxy )
	SendPropDataTable( "tf_randomizer_manager_data", 0, &REFERENCE_SEND_TABLE( DT_TFRandomizerManager ), SendProxy_TFRandomizerManager ),
END_SEND_TABLE()
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRandomizerManagerProxy::CTFRandomizerManagerProxy()
{
	// Allow map placed proxy entities to overwrite the static one.
	if ( s_pRandomizerManagerProxy )
	{
#ifndef CLIENT_DLL
		UTIL_Remove( s_pRandomizerManagerProxy );
#endif
		s_pRandomizerManagerProxy = NULL;
	}

	s_pRandomizerManagerProxy = this;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFRandomizerManagerProxy::~CTFRandomizerManagerProxy()
{
	if ( s_pRandomizerManagerProxy == this )
	{
		s_pRandomizerManagerProxy = NULL;
	}
}


int CTFRandomizerManagerProxy::UpdateTransmitState()
{
#ifndef CLIENT_DLL
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
#else
	return 0;
#endif

}


void CTFRandomizerManagerProxy::NotifyNetworkStateChanged()
{
	if ( s_pRandomizerManagerProxy )
	{
		s_pRandomizerManagerProxy->NetworkStateChanged();
		DevMsg( "CTFRandomizerManagerProxy::NotifyNetworkStateChanged\n" );
	}
	else
	{
		DevWarning( "CTFRandomizerManagerProxy::NotifyNetworkStateChanged: s_pRandomizerManagerProxy is NULL!\n" );
	}
}
LINK_ENTITY_TO_CLASS( tf_randomizer_manager, CTFRandomizerManagerProxy );

BEGIN_NETWORK_TABLE_NOBASE( CTFRandomizerManager, DT_TFRandomizerManager )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iRandomizerFlags ) ),
	RecvPropBool( RECVINFO( m_bRandomizerWasOn ) ),
#else
	SendPropInt( SENDINFO( m_iRandomizerFlags ) ),
	SendPropBool( SENDINFO( m_bRandomizerWasOn ) ),
#endif
END_NETWORK_TABLE()

CTFRandomizerManager g_RandomizerManager( "CTFRandomizerManager" );
CTFRandomizerManager *GetRandomizerManager()
{
	return &g_RandomizerManager;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFRandomizerManager::CTFRandomizerManager( const char *pszName ) : CAutoGameSystem( pszName )
{
	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFRandomizerManager::~CTFRandomizerManager()
{
}


bool CTFRandomizerManager::Init( void )
{
	return true;
}


void CTFRandomizerManager::Shutdown( void )
{
}


void CTFRandomizerManager::LevelInitPreEntity( void )
{
	Reset();

	if ( m_iRandomizerFlags > 0 )
	{
#ifdef GAME_DLL
		if ( RandomizerMode( TF_RANDOMIZER_ATTRIBUTES ) )
		{
			ParseRandomizerAttributes();
		}
#endif
		m_bRandomizerWasOn = true;
	}
}


void CTFRandomizerManager::LevelShutdownPostEntity( void )
{
	Reset();
}


void CTFRandomizerManager::Reset( void )
{
#ifdef GAME_DLL
	m_iMaxRandomizerAttributes = 0;
	m_iMaxRandomizerRetries = 0;
#endif

	m_bRandomizerWasOn = false;

#ifdef GAME_DLL
	ClearRandomizerAttributes();
#endif
}

#ifdef GAME_DLL
// List of generic usable attributes for Randomizer
enum RandomizerAttributesImplemented_t
{
	TF2C_ATTRIB_DAMAGE = 1,
	TF2C_ATTRIB_CLIP_SIZE = 3,
	TF2C_ATTRIB_FIRE_RATE = 5,
	TF2C_ATTRIB_HEAL_RATE = 7,
	TF2C_ATTRIB_UBER_RATE = 9,
	TF2C_ATTRIB_ADD_HEALTH = 16,
	TF2C_ATTRIB_MAX_HEALTH = 26,
	TF2C_ATTRIB_CRIT_CHANCE = 28,
	TF2C_ATTRIB_CRIT_ON_KILL = 31,
	TF2C_ATTRIB_SPREAD = 36,
	TF2C_ATTRIB_BULLETS_PER_SHOT = 45,
	TF2C_ATTRIB_MOVE_SPEED = 54,
	TF2C_ATTRIB_BURN_DAMAGE = 71,
	TF2C_ATTRIB_BURN_TIME = 73,
	TF2C_ATTRIB_MAX_AMMO_PRIMARY = 76,
	TF2C_ATTRIB_MAX_AMMO_SECONDARY = 78,
	TF2C_ATTRIB_CLOAK_CONSUME_RATE = 82,
	TF2C_ATTRIB_CLOAK_REGEN_RATE = 84,
	TF2C_ATTRIB_MINIGUN_SPINUP_TIME = 86,
	TF2C_ATTRIB_RELOAD_TIME = 96,
	TF2C_ATTRIB_EXPLOSION_RADIUS = 99,
	TF2C_ATTRIB_PROJECTILE_SPEED = 103,
	TF2C_ATTRIB_BLEED_DURATION = 149,
	TF2C_ATTRIB_AIRBLAST_COST = 170,
	TF2C_ATTRIB_DEPLOY_TIME = 177,
	TF2C_ATTRIB_DMGTYPE_IGNITE = 208,
	TF2C_ATTRIB_AIRBLAST_COOLDOWN = 256,
	TF2C_ATTRIB_PROJECTILE_PENETRATION = 266,
	TF2C_ATTRIB_EFFECT_BAR_RECHARGE = 278,
	TF2C_ATTRIB_PROJECTILE_TYPE_OVERRIDE = 280,
	TF2C_ATTRIB_CLOAK_MOVE_SPEED = 10027
};

// Usable for projectile override
int RandomizerProjectilesArray[] =
{
	TF_PROJECTILE_BULLET, TF_PROJECTILE_ROCKET, TF_PROJECTILE_PIPEBOMB, TF_PROJECTILE_SYRINGE, TF_PROJECTILE_FLARE, TF_PROJECTILE_ARROW,
	TF_PROJECTILE_NAIL, TF_PROJECTILE_DART, TF_PROJECTILE_COIL
};

//-----------------------------------------------------------------------------
// Purpose: Handle attributes that require special values
//-----------------------------------------------------------------------------
float CTFRandomizerManager::OverrideValue( int iAttributeID, float flValue, CEconItemView *pItem, CTFWeaponInfo *pWeaponInfo )
{
	switch ( iAttributeID )
	{
		case TF2C_ATTRIB_FIRE_RATE:

			DevMsg( "\nHandling special attribute %i...\n", iAttributeID );

			// Fire rate penalties can get some damage compensation
			if ( flValue > 1.0 )
			{
				float flDamageMult = RandomFloat( 1.0, flValue );
				CEconItemAttribute econAttributeDamage( 2, flDamageMult );
				DevMsg( "\nAdding damage multiplier %f...\n", flDamageMult );

				econAttributeDamage.m_strAttributeClass = AllocPooledString( econAttributeDamage.attribute_class );
				pItem->AddAttribute( &econAttributeDamage );
			}

			break;

		case TF2C_ATTRIB_ADD_HEALTH:

			DevMsg( "\nHandling special attribute %i...\n", iAttributeID );

			// Scale by base fire rate to avoid infinite hp flamethrowers
			DevMsg( "Overriding Value: %f...", flValue );
			flValue = ceil( flValue * pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flTimeFireDelay );
			DevMsg( " - overridden: %f!\n\n", flValue );

			break;

		case TF2C_ATTRIB_PROJECTILE_TYPE_OVERRIDE:

			DevMsg( "\nHandling special attribute %i...\n", iAttributeID );

			DevMsg( "Overriding Value: %f...\n", flValue );
			flValue = RandomizerProjectilesArray[RandomInt( 0, 8 )];
			DevMsg( " - overridden: %f!\n\n", flValue );

			// Compensate for guns whose base damage assumes multiple bullets (e.g. shotguns)
			int iBulletsPerShot = pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nBulletsPerShot;
			if ( iBulletsPerShot > 1 )
			{
				DevMsg( "\nAdding damage multiplier %i...\n", iBulletsPerShot );

				CEconItemAttribute econAttributeDamage( 2, iBulletsPerShot );

				econAttributeDamage.m_strAttributeClass = AllocPooledString( econAttributeDamage.attribute_class );
				pItem->AddAttribute( &econAttributeDamage );
			}

			break;


	}

	return flValue;
}


void CTFRandomizerManager::ApplyRandomizerAttributes( CEconItemView *pItem, ETFLoadoutSlot iSlot, int iClass )
{
	if ( !pItem )
		return;

	CEconItemDefinition *pItemDef = pItem->GetStaticData();
	if ( !pItemDef )
		return;

	CTFWeaponInfo *pWeaponInfo = GetTFWeaponInfoForItem( pItem, iClass );
	if ( !pWeaponInfo )
		return;

	int iItemID = pItem->GetItemDefIndex();
	const char *pszEntityName = pItemDef->item_class;
	int iProjectile = pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile;

	int iAttributeID = -1;
	int iTotalAttributes = 0;

	DevMsg( "[CTFRandomizerManager] Item entity '%s'", pszEntityName );

	bool bRetry = false;
	int iTries = m_iMaxRandomizerRetries;

	// TODO: This looks kind of scary, clean this up if possible.
	for ( int i = 0; i < m_iMaxRandomizerAttributes; i++ )
	{
		CTFRandomizerAttribute *pRandomAttribute = NULL;
		do
		{
			bRetry = false;

			pRandomAttribute = m_vecAttributes.Random();
			if ( !pRandomAttribute )
				goto Retry;

			iAttributeID = pRandomAttribute->GetAttributeID();
			DevMsg( "\n[CTFRandomizerManager] Random attribute: %d", iAttributeID );

			//// Blacklists/Whitelists
			if ( pRandomAttribute->HasItemWhitelists() && !pRandomAttribute->IsWhitelistedItem( iItemID ) )
			{
				DevWarning( " - Item ID %d not whitelisted", iItemID );
				goto Retry;
			}

			if ( pRandomAttribute->HasEntityWhitelists() && !pRandomAttribute->IsWhitelistedEntity( pszEntityName ) )
			{
				DevWarning( " - Entity name '%s' not whitelisted", pszEntityName );
				goto Retry;
			}

			if ( pRandomAttribute->HasProjectileWhitelists() && !pRandomAttribute->IsWhitelistedProjectile( iProjectile ) )
			{
				DevWarning( " - Projectile %d not whitelisted", iProjectile );
				goto Retry;
			}

			if ( pRandomAttribute->HasItemBlacklists() && pRandomAttribute->IsBlacklistedItem( iItemID ) )
			{
				DevWarning( " - Item ID %d blacklisted", iItemID );
				goto Retry;
			}

			if ( pRandomAttribute->HasEntityBlacklists() && pRandomAttribute->IsBlacklistedEntity( pszEntityName ) )
			{
				DevWarning( " - Entity name '%s' blacklisted", pszEntityName );
				goto Retry;
			}

			if ( pRandomAttribute->HasProjectileBlacklists() && pRandomAttribute->IsBlacklistedProjectile( iProjectile ) )
			{
				DevWarning( " - Projectile %d blacklisted", iProjectile );
				goto Retry;
			}

			//// Requirements
			if ( pRandomAttribute->HasRequirement( "damage" ) && pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flDamage <= 0 )
			{
				DevWarning( " - Failed 'damage' requirement" );
				goto Retry;
			}
			
			if ( pRandomAttribute->HasRequirement( "clipsize" ) && pWeaponInfo->iMaxClip1 <= 0 )
			{
				DevWarning( " - Failed 'clipsize' requirement" );
				goto Retry;
			}
			
			if ( pRandomAttribute->HasRequirement( "firerate" ) && pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flTimeFireDelay <= 0 )
			{
				DevWarning( " - Failed 'firerate' requirement" );
				goto Retry;
			}
			
			if ( pRandomAttribute->HasRequirement( "reloadtime" ) && pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flTimeReload <= 0.0f )
			{
				DevWarning( " - Failed 'reloadtime' requirement" );
				goto Retry;
			}

			continue;
			
			Retry:
			{
				bRetry = true;
				iTries--;
				DevMsg( "\n[CTFRandomizerManager] Retrying random attribute application for Item ID: %d\n", iItemID );
			}
		}
		while ( iTries > 0 && bRetry );

		if ( iTries > 0 && pRandomAttribute )
		{
			iAttributeID = pRandomAttribute->GetAttributeID();
			float flValue = pRandomAttribute->GetRandomValue();

			flValue = OverrideValue( iAttributeID, flValue, pItem, pWeaponInfo );

			CEconItemAttribute econAttribute( iAttributeID, flValue );
			econAttribute.m_strAttributeClass = AllocPooledString( econAttribute.attribute_class );
			if ( pItem->AddAttribute( &econAttribute ) )
			{
				DevMsg( " - successfully applied attribute to Item ID: %d, Value: %f!\n", iItemID, flValue );

				iTotalAttributes++;
			}
		}
	}

	if ( iTotalAttributes != 0 )
	{
		pItem->SkipBaseAttributes( false );
		DevMsg( "[CTFRandomizerManager] Total of random attributes applied to Item ID: %d, %d\n", iItemID, iTotalAttributes );
	}

	/*int iRandomAttrib = 0;

	int iGivenAttributes = 0;
	int iAttempts = 0;

	const char *pszWeaponClassname = pWeaponInfo->szClassName;
	DevMsg( "\nWeapon: %s\n", pszWeaponClassname );

	float flMaxHP = GetMaxHealth();
	float flBaseSpeed = pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flProjectileSpeed;
	int iBulletsPerShot = pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nBulletsPerShot;

	int iSize = sizeof( RandomizerAttributesArray ) / sizeof( RandomizerAttributesArray[0] );

	// Try to give 3 random attributes
	do
	{
		iAttempts++;
		iRandomAttrib = RandomizerAttributesArray[ RandomInt( 0, iSize-1 ) ];
		DevMsg( "Try: %i, Attrib: %i\n", iAttempts, iRandomAttrib );
		if ( iRandomAttrib == 0 ) continue;

		switch ( iRandomAttrib )
		{
		case TF2C_ATTRIB_DAMAGE:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flDamage <= 0 || !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				DevMsg( "Not giving damage\n" );
				continue;
			}
			flValue = RandomFloat( 0.4, 2.0 );
			break;

		case TF2C_ATTRIB_CLIP_SIZE:
			if ( pWeaponInfo->iMaxClip1 <= 0 )
			{
				DevMsg( "Not giving clip size\n" );
				continue;
			}
			flValue = RandomFloat( 0.2, 4.0 );
			break;

		case TF2C_ATTRIB_FIRE_RATE:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flTimeFireDelay <= 0 || !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				DevMsg( "Not giving fire rate\n" );
				continue;
			}
			flValue = RandomFloat( 0.25, 2.0 );
			// Fire rate penalties can get some damage compensation
			if ( flValue > 1.0 )
			{
				CEconItemAttribute econAttributeDamage( 2, RandomFloat( 1.0, flValue ) );

				econAttributeDamage.m_strAttributeClass = AllocPooledString( econAttributeDamage.attribute_class );
				pItem->AddAttribute( &econAttributeDamage );
			}
			break;

		case TF2C_ATTRIB_HEAL_RATE:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				flValue = RandomFloat( 0.5, 3.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_UBER_RATE:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				flValue = RandomFloat( 1.0, 5.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_ADD_HEALTH:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flDamage <= 0 || !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				DevMsg( "Not giving on hit\n" );
				continue;
			}
			flValue = ceil( RandomFloat( 0.0f, flMaxHP * 0.5f ) );
			// Scale by base fire rate to avoid infinite hp flamethrowers
			flValue = ceil( flValue * pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flTimeFireDelay );
			break;

		case TF2C_ATTRIB_MAX_HEALTH:
			flValue = ceil( RandomFloat( -flMaxHP * 0.5f, flMaxHP * 2.0f ) );
			break;

		case TF2C_ATTRIB_CRIT_CHANCE:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flDamage <= 0 || !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				DevMsg( "Not giving crit chance\n" );
				continue;
			}
			flValue = RandomFloat( 0.0, 8.0 );
			break;

		case TF2C_ATTRIB_CRIT_ON_KILL:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flDamage <= 0 || !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				DevMsg( "Not giving on kill\n" );
				continue;
			}
			flValue = RandomInt( 1, 10 );
			break;

		case TF2C_ATTRIB_SPREAD:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_NONE )
			{
				DevMsg( "Not giving spread\n" );
				continue;
			}
			flValue = RandomFloat( 0.0, 4.0 );
			break;

		case TF2C_ATTRIB_BULLETS_PER_SHOT:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile != TF_PROJECTILE_BULLET )
			{
				DevMsg( "Not giving bullets per shot\n" );
				continue;
			}
			flValue = RandomFloat( 1 / iBulletsPerShot, 4.0 );
			break;

		case TF2C_ATTRIB_MOVE_SPEED:
			flValue = RandomFloat( 0.75, 1.5 );
			break;

		case TF2C_ATTRIB_BURN_DAMAGE:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_flamethrower" ) )
			{
				flValue = RandomFloat( 0.0, 3.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_BURN_TIME:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_flamethrower" ) )
			{
				flValue = RandomFloat( 0.0, 3.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_MAX_AMMO_PRIMARY:
			flValue = RandomFloat( 0.25, 3.0 );
			break;

		case TF2C_ATTRIB_MAX_AMMO_SECONDARY:
			flValue = RandomFloat( 0.25, 3.0 );
			break;

		case TF2C_ATTRIB_CLOAK_CONSUME_RATE:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_invis" ) )
			{
				flValue = RandomFloat( 0.0, 3.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_CLOAK_REGEN_RATE:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_invis" ) )
			{
				flValue = RandomFloat( -1.0, 5.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_MINIGUN_SPINUP_TIME:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_minigun" ) )
			{
				flValue = RandomFloat( 0.25, 2.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_RELOAD_TIME:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flTimeReload <= 0.0f || !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				DevMsg( "Not giving reload time\n" );
				continue;
			}
			flValue = RandomFloat( 0.25, 3.0 );
			break;

		case TF2C_ATTRIB_EXPLOSION_RADIUS:
			if (	pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_ROCKET || 
					pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_PIPEBOMB ||
					pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_PIPEBOMB_REMOTE ||
					pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_MIRV 
				)
			{
				flValue = RandomFloat( 0.25, 3.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_PROJECTILE_SPEED:
			if (	pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_NONE || 
					pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_BULLET
				)
			{
				DevMsg( "Not giving proj speed\n" );
				continue;
			}
			flValue = min( RandomFloat( 0.25, 3.0 ), 3500.0f / flBaseSpeed );
			break;

		case TF2C_ATTRIB_BLEED_DURATION:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flDamage <= 0 || !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) )
			{
				DevMsg( "Not giving bleed on hit\n" );
				continue;
			}
			flValue = RandomInt( 1, 5 );
			break;

		case TF2C_ATTRIB_AIRBLAST_COST:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_flamethrower" ) )
			{
				flValue = RandomFloat( 0.0, 2.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_DEPLOY_TIME:
			flValue = RandomFloat( 0.25, 2.0 );
			break;

		case TF2C_ATTRIB_DMGTYPE_IGNITE:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_flDamage <= 0 || !V_stricmp( pszWeaponClassname, "tf_weapon_medigun" ) || !V_stricmp( pszWeaponClassname, "tf_weapon_flamethrower" ) || !V_stricmp( pszWeaponClassname, "tf_weapon_flaregun" ) )
			{
				DevMsg( "Not giving ignite on hit\n" );
				continue;
			}
			flValue = 1.0f;
			break;

		case TF2C_ATTRIB_AIRBLAST_COOLDOWN:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_flamethrower" ) )
			{
				flValue = RandomFloat( 0.1, 2.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_EFFECT_BAR_RECHARGE:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_lunchbox" ) || !V_stricmp( pszWeaponClassname, "tf_weapon_grenade_mirv" ) )
			{
				flValue = RandomFloat( 0.0, 2.0 );
			}
			else continue;
			break;

		case TF2C_ATTRIB_PROJECTILE_PENETRATION:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile != TF_PROJECTILE_ARROW )
			{
				DevMsg( "Not giving arrow penetration\n" );
				continue;
			}
			flValue = 1;
			break;

		case TF2C_ATTRIB_PROJECTILE_TYPE_OVERRIDE:
			if ( pWeaponInfo->GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_iProjectile == TF_PROJECTILE_NONE )
			{
				DevMsg( "Not giving proj type\n" );
				continue;
			}
			flValue = RandomizerProjectilesArray[ RandomInt( 0, 8 ) ];
			// Compensate for guns whose base damage assumes multiple bullets (e.g. shotguns)
			if ( flValue != 1 && iBulletsPerShot > 1 )
			{
				CEconItemAttribute econAttributeDamage( 2, iBulletsPerShot );

				econAttributeDamage.m_strAttributeClass = AllocPooledString( econAttributeDamage.attribute_class );
				pItem->AddAttribute( &econAttributeDamage );
			}
			break;

		case TF2C_ATTRIB_CLOAK_MOVE_SPEED:
			if ( !V_stricmp( pszWeaponClassname, "tf_weapon_invis" ) )
			{
				flValue = RandomFloat( 0.75, 1.5 );
			}
			else continue;
			break;

		default:
			flValue = 1.0f;
			break;
		}

		CEconItemAttribute econAttribute( iRandomAttrib, flValue );

		econAttribute.m_strAttributeClass = AllocPooledString( econAttribute.attribute_class );
		if ( pItem->AddAttribute( &econAttribute ) ) iGivenAttributes++;

		const char *pszAttribName = econAttribute.GetStaticData()->attribute_class;
		DevMsg( "Attribute: %s, %2.2f, player %i\n", pszAttribName, flValue, GetUserID() );
		// TODO: Tell players their random attributes

	} while ( iAttempts < 50 && iGivenAttributes < 3 );

	pItem->SkipBaseAttributes( false );*/
}


void CTFRandomizerManager::SetRandomizerMode( int iMode )
{
	// No changes were made.
	if ( iMode == m_iRandomizerFlags )
		return;

	if ( iMode > 0 || m_iRandomizerFlags > 0 )
	{
		m_bRandomizerWasOn = true;
	}

	if ( iMode & TF_RANDOMIZER_ATTRIBUTES )
	{
		if ( !RandomizerMode( TF_RANDOMIZER_ATTRIBUTES ) )
		{
			if ( !ParseRandomizerAttributes() )
			{
				DevWarning( "[CTFRandomizerManager] Failed to parse Randomizer Mode attributes, disabling random attributes!" );
				iMode &= TF_RANDOMIZER_ATTRIBUTES;
			}
		}
	}
	else if ( RandomizerMode( TF_RANDOMIZER_ATTRIBUTES ) )
	{
		ClearRandomizerAttributes();
	}

	m_iRandomizerFlags = iMode;
}

//-----------------------------------------------------------------------------
// Purpose: Right now we only parse attributes.
//-----------------------------------------------------------------------------
bool CTFRandomizerManager::ParseRandomizerAttributes( void )
{
	KeyValues *pkvFile = new KeyValues( "Randomizer" );
	if ( pkvFile->LoadFromFile( filesystem, tf2c_randomizer_script.GetString(), "MOD" ) )
	{
		KeyValues *pkvSettings = pkvFile->FindKey( "settings" );
		if ( pkvSettings )
		{
			// Parse the requested setting changes.
			FOR_EACH_SUBKEY( pkvSettings, pkvSetting )
			{
				const char *pszSettingName = pkvSetting->GetName();
				if ( !V_stricmp( pszSettingName, "max_randomized_attributes" ) )
				{
					m_iMaxRandomizerAttributes = pkvSetting->GetInt();
				}
				else if ( !V_stricmp( pszSettingName, "max_randomized_retries" ) )
				{
					m_iMaxRandomizerRetries = pkvSetting->GetInt();
				}
			}
		}

		KeyValues *pkvAttributes = pkvFile->FindKey( "attributes" );
		if ( pkvAttributes )
		{
			// Parse each attribute that's requested to be put in the Randomizer Manager.
			FOR_EACH_SUBKEY( pkvAttributes, pkvAttribute )
			{
				int iAttributeID = atoi( pkvAttribute->GetName() );

				CTFRandomizerAttribute *pRandomizerAttribute = new CTFRandomizerAttribute();
				pRandomizerAttribute->SetAttributeID( iAttributeID );

				DevMsg( "\n[CTFRandomizerManager] Adding attribute %d", iAttributeID );

				FOR_EACH_SUBKEY( pkvAttribute, pkvAttributeData )
				{
					const char *pszAttributeDataName = pkvAttributeData->GetName();
					if ( !V_stricmp( pszAttributeDataName, "value_chance" ) )
					{
						int iLimit = 2;
						float flChance[2] = { 0.0f, 0.0f };

						// How powerful is this Jack in the Box?
						char *pszToken = strtok( (char *)pkvAttributeData->GetString(), "," );
						while ( pszToken != NULL && !iLimit == 0 )
						{
							flChance[abs( iLimit - 2 )] = atof( pszToken );
							iLimit--;
							pszToken = strtok( NULL, "," );
						}

						pRandomizerAttribute->SetValueChance( flChance[0], flChance[1] );
						DevMsg( ", Chance: %f - %f", flChance[0], flChance[1] );
					}
					else if ( !V_stricmp( pszAttributeDataName, "item_blacklist" ) )
					{
						// Blacklist this item ID.
						char *pszToken = strtok( (char *)pkvAttributeData->GetString(), "," );
						while ( pszToken != NULL )
						{
							pRandomizerAttribute->AddBlacklistedItem( atoi( pszToken ) );
							DevMsg( ", Blacklisted Item ID: %d", atoi( pszToken ) );

							pszToken = strtok( NULL, "," );
						}
					}
					else if ( !V_stricmp( pszAttributeDataName, "item_whitelist" ) )
					{
						// Whitelist this item ID.
						char *pszToken = strtok( (char *)pkvAttributeData->GetString(), "," );
						while ( pszToken != NULL )
						{
							pRandomizerAttribute->AddWhitelistedItem( atoi( pszToken ) );
							DevMsg( ", Whitelisted Item ID: %d", atoi( pszToken ) );

							pszToken = strtok( NULL, "," );
						}
					}
					else if ( !V_stricmp( pszAttributeDataName, "entity_blacklist" ) )
					{
						// Blacklist this entity name.
						char *pszToken = strtok( (char *)pkvAttributeData->GetString(), "," );
						while ( pszToken != NULL )
						{
							pRandomizerAttribute->AddBlacklistedEntity( pszToken );
							DevMsg( ", Blacklisted Entity: %s", pszToken );

							pszToken = strtok( NULL, "," );
						}
					}
					else if ( !V_stricmp( pszAttributeDataName, "entity_whitelist" ) )
					{
						// Whitelist this entity name.
						char *pszToken = strtok( (char *)pkvAttributeData->GetString(), "," );
						while ( pszToken != NULL )
						{
							pRandomizerAttribute->AddWhitelistedEntity( pszToken );
							DevMsg( ", Whitelisted Entity: %s", pszToken );

							pszToken = strtok( NULL, "," );
						}
					}
					else if ( !V_stricmp( pszAttributeDataName, "projectile_blacklist" ) )
					{
						// Blacklist this projectile.
						char *pszToken = strtok( (char *)pkvAttributeData->GetString(), "," );
						while ( pszToken != NULL )
						{
							for ( int i = 0; i < ARRAYSIZE( SupportedRandomizerProjectiles ); i++ )
							{
								if ( !V_stricmp( pszToken, SupportedRandomizerProjectiles[i].name ) )
								{
									pRandomizerAttribute->AddBlacklistedProjectile( SupportedRandomizerProjectiles[i].projectile );
									DevMsg( ", Blacklisted Projectile: %d", SupportedRandomizerProjectiles[i].projectile );
								}
							}
							pszToken = strtok( NULL, "," );
						}
					}
					else if ( !V_stricmp( pszAttributeDataName, "projectile_whitelist" ) )
					{
						// Whitelist this projectile.
						char *pszToken = strtok( (char *)pkvAttributeData->GetString(), "," );
						while ( pszToken != NULL )
						{
							for ( int i = 0; i < ARRAYSIZE( SupportedRandomizerProjectiles ); i++ )
							{
								if ( !V_stricmp( pszToken, SupportedRandomizerProjectiles[i].name ) )
								{
									pRandomizerAttribute->AddWhitelistedProjectile( SupportedRandomizerProjectiles[i].projectile );
									DevMsg( ", Whitelisted Projectile: %d", SupportedRandomizerProjectiles[i].projectile );

								}
							}
							pszToken = strtok( NULL, "," );
						}
					}
				}

				KeyValues *pkvAttributeRequirements = pkvAttribute->FindKey( "requirements" );
				if ( pkvAttributeRequirements )
				{
					FOR_EACH_SUBKEY( pkvAttributeRequirements, pkvRequirements )
					{
						pRandomizerAttribute->AddRequirement( pkvRequirements->GetName() );
					}
				}

				m_vecAttributes.AddToTail( pRandomizerAttribute );
			}
		}

		DevMsg( "\n[CTFRandomizerManager] Successfully parsed Randomizer attributes\n" );

		return true;
	}
	
	return false;
}


void CTFRandomizerManager::ClearRandomizerAttributes( void )
{
	m_vecAttributes.PurgeAndDeleteElements();
}
#endif
