//=============================================================================
//
// Purpose: Stub class for compatibility with item schema
//
//=============================================================================
#include "cbase.h"
#include "animation.h"
#include "tf_wearable.h"

#ifdef GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#include "props_shared.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearable, DT_TFWearable );

BEGIN_NETWORK_TABLE( CTFWearable, DT_TFWearable )
#if defined( GAME_DLL )
	SendPropBool( SENDINFO( m_bDisguiseWearable ) ),
	SendPropEHandle( SENDINFO( m_hWeaponAssociatedWith ) ),
	SendPropFloat( SENDINFO( m_flChargeMeter ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ),
#else
	RecvPropBool( RECVINFO( m_bDisguiseWearable ) ),
	RecvPropEHandle( RECVINFO( m_hWeaponAssociatedWith ) ),
	RecvPropFloat( RECVINFO( m_flChargeMeter ) ),
#endif // GAME_DLL
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_wearable, CTFWearable );
//PRECACHE_REGISTER( tf_wearable );

IMPLEMENT_NETWORKCLASS_ALIASED( TFWearableVM, DT_TFWearableVM );

BEGIN_NETWORK_TABLE( CTFWearableVM, DT_TFWearableVM )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_wearable_vm, CTFWearableVM );
PRECACHE_REGISTER( tf_wearable_vm );


CTFWearable::CTFWearable()
{
	m_bDisguiseWearable = false;
	m_hWeaponAssociatedWith = NULL;
	m_flChargeMeter = 1.0f;
}

#ifdef GAME_DLL

void CTFWearable::Equip( CBasePlayer *pPlayer )
{
	BaseClass::Equip( pPlayer );
	UpdateModelToClass();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWearable::UpdateModelToClass( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner )
	{
		const char *pszModel = GetItem()->GetPlayerDisplayModel( pOwner->GetPlayerClass()->GetClassIndex() );
		if ( pszModel && pszModel[0] != '\0' )
		{
			SetModel( pszModel );
		}
	}
}
#else
extern ConVar tf2c_legacy_invulnerability_material;

//-----------------------------------------------------------------------------
// Receive the BreakModel user message
//-----------------------------------------------------------------------------
void HandleBreakModel( bf_read &msg, bool bCheap )
{
	int nModelIndex = (int)msg.ReadShort();
	CUtlVector<breakmodel_t>	aGibs;
	BuildGibList( aGibs, nModelIndex, 1.0f, COLLISION_GROUP_NONE );
	if ( !aGibs.Count() )
		return;

	// Get the origin & angles
	Vector vecOrigin;
	QAngle vecAngles;
	int nSkin = 0;
	msg.ReadBitVec3Coord( vecOrigin );
	if ( !bCheap )
	{
		msg.ReadBitAngles( vecAngles );
		nSkin = (int)msg.ReadShort();
	}
	else
	{
		vecAngles = vec3_angle;
	}

	// Launch it straight up with some random spread
	Vector vecBreakVelocity = Vector( 0.0f, 0.0f, 200.0f );
	AngularImpulse angularImpulse( RandomFloat( 0.0f, 120.0f ), RandomFloat( 0.0f, 120.0f ), 0.0 );
	breakablepropparams_t breakParams( vecOrigin, vecAngles, vecBreakVelocity, angularImpulse );
	breakParams.impactEnergyScale = 1.0f;
	breakParams.nDefaultSkin = nSkin;

	CreateGibsFromList( aGibs, nModelIndex, NULL, breakParams, NULL, -1 , false, true );
}


int CTFWearable::GetSkin( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	return GetTeamSkin( pOwner && pOwner->IsDisguisedEnemy() ? pOwner->m_Shared.GetDisguiseTeam() : GetTeamNumber() );
}


int CTFWearable::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	bool bUseInvulnMaterial = ( tf2c_legacy_invulnerability_material.GetBool() && ( pOwner && pOwner->m_Shared.IsInvulnerable() && !pOwner->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) ) );
	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int iRet = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return iRet;
}


bool CTFWearable::ShouldDraw()
{
	if ( !BaseClass::ShouldDraw() )
		return false;

	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	if ( pOwner && pOwner->m_Shared.IsDisguised() )
	{
		if ( m_bDisguiseWearable )
		{
			// This wearable is a part of our disguise, we might want to draw it.
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			int iLocalPlayerTeam = pLocalPlayer ? pLocalPlayer->GetTeamNumber() : TEAM_SPECTATOR;
			if ( iLocalPlayerTeam != pOwner->GetTeamNumber() && iLocalPlayerTeam != TEAM_SPECTATOR )
			{
				// This enemy Spy is disguised as a Spy on our team, but we can't see his disguise's wearable.
				if ( pOwner->m_Shared.GetDisguiseClass() == TF_CLASS_SPY && pOwner->m_Shared.GetDisguiseTeam() == iLocalPlayerTeam )
					return false;
				
				// The local player is an enemy, we can see his disguise's wearables.
				return true;
			}
		}

		// We are disguised, never draw any wearables.
		return false;
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Receive the BreakModel user message
//-----------------------------------------------------------------------------
void __MsgFunc_BreakModel( bf_read &msg )
{
	HandleBreakModel( msg, false );
}

//-----------------------------------------------------------------------------
// Receive the CheapBreakModel user message
//-----------------------------------------------------------------------------
void __MsgFunc_CheapBreakModel( bf_read &msg )
{
	HandleBreakModel( msg, true );
}
#endif // GAME_DLL


void CTFWearable::ReapplyProvision( void )
{
	// Disguise items never provide.
	if ( m_bDisguiseWearable )
	{
		CBaseEntity *pOwner = GetOwnerEntity();
		CBaseEntity *pOldOwner = m_hOldOwner.Get();
		
		if ( pOwner )
			GetAttributeManager()->StopProvidingTo( pOwner );

		if ( pOldOwner )
			GetAttributeManager()->StopProvidingTo( pOldOwner );
		return;
	}

	BaseClass::ReapplyProvision();
}

void CTFWearable::Reset()
{
	m_flChargeMeter.Set( 1.0f );
}

void CTFWearable::WearableFrame( void )
{
	if( IsDisguiseWearable() )
		return;

	if( UsesProgressBar() )
		WearableEffectThink();
}


bool CTFWearable::UpdateBodygroups( CBasePlayer *pOwner, bool bForce )
{
#ifdef CLIENT_DLL
	if ( !bForce && !ShouldDraw() )
		return false;
#endif

	if ( m_bDisguiseWearable )
	{
		CTFPlayer *pTFOwner = ToTFPlayer( pOwner );
		if ( !pTFOwner )
			return false;

		CTFPlayer *pDisguiseTarget = ToTFPlayer( pTFOwner->m_Shared.GetDisguiseTarget() );
		if ( !pDisguiseTarget )
			return false;

		CEconItemView *pItem = GetItem();
		if ( !pItem )
			return false;

		CEconItemDefinition *pStatic = pItem->GetStaticData();
		if ( !pStatic )
			return false;

		EconItemVisuals *pVisuals = pStatic->GetVisuals( pTFOwner->m_Shared.GetDisguiseTeam() );
		if ( !pVisuals )
			return false;

		int iDisguiseBody = pTFOwner->m_Shared.GetDisguiseBody();

		const char *pszBodyGroupName;
		int iBodygroup;
		for ( unsigned int i = 0, c = pVisuals->player_bodygroups.Count(); i < c; i++ )
		{
			pszBodyGroupName = pVisuals->player_bodygroups.GetElementName( i );
			if ( pszBodyGroupName )
			{
				iBodygroup = pDisguiseTarget->FindBodygroupByName( pszBodyGroupName );
				if ( iBodygroup == -1 )
					continue;

				::SetBodygroup( pDisguiseTarget->GetModelPtr(), iDisguiseBody, iBodygroup, pVisuals->player_bodygroups.Element( i ) );
			}
		}

		// Set our disguised bodygroups.
		pTFOwner->m_Shared.SetDisguiseBody( iDisguiseBody );
		return true;
	}

	return BaseClass::UpdateBodygroups( pOwner, bForce );
}

bool CTFWearable::SecondaryAttack( void )
{
	if( GetProgress() < 1.0f )
		return false;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerViaInterface() );
	if( !pOwner )
		return false;

	string_t strArgs = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( this, strArgs, add_wearable_cond_on_secondary_attack );
	if( strArgs != NULL_STRING )
	{
		float args[3];
#ifdef GAME_DLL
		UTIL_StringToFloatArray( args, 3, strArgs.ToCStr() );
#else
		UTIL_StringToFloatArray( args, 3, strArgs );
#endif

		int iCondition = Floor2Int( args[0] );

		if( pOwner->m_Shared.InCond( (ETFCond)iCondition ) )
			return false;

		m_flChargeMeter.Set( 1.0f );

		pOwner->m_Shared.AddCond( (ETFCond)iCondition, -1, this );
		return true;
	}

	return false;
}

bool CTFWearable::UsesProgressBar( void )
{
	CTFPlayer *pOwner = ToTFPlayer(GetOwnerViaInterface());
	if( !pOwner )
		return false;

	string_t strArgs = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( this, strArgs, add_wearable_cond_on_secondary_attack );
	return strArgs != NULL_STRING;
};

// This basically handles the charge meter
void CTFWearable::WearableEffectThink( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerViaInterface() );
	if( !pOwner )
		return;

	string_t strArgs = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( this, strArgs, add_wearable_cond_on_secondary_attack );
	float args[3];
#ifdef GAME_DLL
	UTIL_StringToFloatArray( args, 3, strArgs.ToCStr() );
#else
	UTIL_StringToFloatArray( args, 3, strArgs );
#endif

	ETFCond nCond = (ETFCond)Floor2Int(args[0]);
	float flEffectDuration = args[1];
	float flEffectRecharge = args[2];

	// If we're charging, take away charge meter
	if( pOwner->m_Shared.InCond( nCond ) )
	{
		float flChargeAmount = gpGlobals->frametime / flEffectDuration;
		float flNewLevel = max( m_flChargeMeter - flChargeAmount, 0.0 );
		m_flChargeMeter = flNewLevel;

		string_t strArgs2 = NULL_STRING;
		CALL_ATTRIB_HOOK_STRING_ON_OTHER( this, strArgs2, add_wearable_cond_on_effect_percent );
		if( strArgs2 != NULL_STRING )
		{
			float args2[2];
#ifdef GAME_DLL
			UTIL_StringToFloatArray( args2, 2, strArgs2.ToCStr() );
#else
			UTIL_StringToFloatArray( args2, 2, strArgs2 );
#endif
			ETFCond nCond2 = (ETFCond)Floor2Int(args2[0]);
			if ( m_flChargeMeter <= args2[1] && !pOwner->m_Shared.InCond( nCond2 ) )
				pOwner->m_Shared.AddCond( nCond2, -1, this );
		}
		// If we ran out of meter, stop charging
		if( m_flChargeMeter <= 0.0f )
			StopEffect();
	}
	// If we're out of meter and not charging, regenerate
	else if( m_flChargeMeter < 1.0f )
	{
		float flChargeAmount = gpGlobals->frametime / flEffectRecharge;
		float flNewLevel = min( m_flChargeMeter + flChargeAmount, 1 );
		m_flChargeMeter = flNewLevel;
	}
}

void CTFWearable::StopEffect( void )
{
	m_flChargeMeter = 0.0f;

	CTFPlayer *pOwner = ToTFPlayer( GetOwnerViaInterface() );
	if( !pOwner )
		return;

	string_t strArgs = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( this, strArgs, add_wearable_cond_on_secondary_attack );
	float args[3];
#ifdef GAME_DLL
	UTIL_StringToFloatArray( args, 3, strArgs.ToCStr() );
#else
	UTIL_StringToFloatArray( args, 3, strArgs );
#endif

	ETFCond nCond = (ETFCond)Floor2Int(args[0]);

	pOwner->m_Shared.RemoveCond( nCond );

	string_t strArgs2 = NULL_STRING;
	CALL_ATTRIB_HOOK_STRING_ON_OTHER( this, strArgs2, add_wearable_cond_on_effect_percent );
	if( strArgs2 != NULL_STRING )
	{
		float args2[2];
#ifdef GAME_DLL
		UTIL_StringToFloatArray( args2, 2, strArgs2.ToCStr() );
#else
		UTIL_StringToFloatArray( args2, 2, strArgs2 );
#endif

		ETFCond nCond2 = (ETFCond)Floor2Int(args2[0]);
		pOwner->m_Shared.RemoveCond( nCond2 );
	}
}