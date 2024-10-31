//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectSentrygun
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_baseobject.h"
#include "c_tf_player.h"
#include "vgui/ILocalize.h"
#include "c_obj_dispenser.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: RecvProxy that converts the Team's player UtlVector to entindexes.
//-----------------------------------------------------------------------------
void RecvProxy_HealingList( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	C_ObjectDispenser *pDispenser = (C_ObjectDispenser *)pStruct;

	CBaseHandle *pHandle = (CBaseHandle *)( &pDispenser->m_hHealingTargets[pData->m_iElement] ); 
	RecvProxy_IntToEHandle( pData, pStruct, pHandle );

	// Update the heal beams.
	pDispenser->UpdateHealingTargets();
}

void RecvProxyArrayLength_HealingArray( void *pStruct, int objectID, int currentArrayLength )
{
	C_ObjectDispenser *pDispenser = (C_ObjectDispenser *)pStruct;
	if ( pDispenser->m_hHealingTargets.Size() != currentArrayLength )
	{
		pDispenser->m_hHealingTargets.SetSize( currentArrayLength );
	}

	// Update the heal beams.
	pDispenser->UpdateHealingTargets();
}

//-----------------------------------------------------------------------------
// Purpose: Dispenser object
//-----------------------------------------------------------------------------

IMPLEMENT_CLIENTCLASS_DT( C_ObjectDispenser, DT_ObjectDispenser, CObjectDispenser )
	RecvPropBool( RECVINFO( m_bHealingTargetsParity ) ),
	RecvPropInt( RECVINFO( m_iAmmoMetal ) ),
	RecvPropArray2( 
		RecvProxyArrayLength_HealingArray,
		RecvPropInt( "healing_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_HealingList ), 
		MAX_PLAYERS, 
		0, 
		"healing_array"
	)
END_RECV_TABLE()


C_ObjectDispenser::C_ObjectDispenser()
{
	m_bUpdateHealingTargets = m_bHealingTargetsParity = m_bOldHealingTargetsParity = false;
	m_bPlayingSound = false;

	m_pDamageEffects = NULL;
}

C_ObjectDispenser::~C_ObjectDispenser()
{
	StopSound( "Building_Dispenser.Heal" );
}

void C_ObjectDispenser::GetStatusText( wchar_t *pStatus, int iMaxStatusLen )
{
	float flHealthPercent = (float)GetHealth() / (float)GetMaxHealth();
	wchar_t wszHealthPercent[32];
	V_swprintf_safe( wszHealthPercent, L"%d%%", (int)( flHealthPercent * 100 ) );

	wchar_t *pszTemplate;

	if ( IsBuilding() )
	{
		pszTemplate = g_pVGuiLocalize->Find( "#TF_ObjStatus_Dispenser_Building" );
	}
	else
	{
		pszTemplate = g_pVGuiLocalize->Find( "#TF_ObjStatus_Dispenser" );
	}

	if ( pszTemplate )
	{
		g_pVGuiLocalize->ConstructString( pStatus, iMaxStatusLen, pszTemplate,
			1,
			wszHealthPercent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_ObjectDispenser::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_bOldHealingTargetsParity = m_bHealingTargetsParity;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_ObjectDispenser::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_bHealingTargetsParity != m_bOldHealingTargetsParity )
	{
		m_bUpdateHealingTargets = true;
	}

	if ( m_bUpdateHealingTargets )
	{
		UpdateEffects();
		m_bUpdateHealingTargets = false;
	}
}


void C_ObjectDispenser::SetDormant( bool bDormant )
{
	if ( !IsDormant() && bDormant )
	{
		m_bPlayingSound = false;
		StopSound( "Building_Dispenser.Heal" );
	}

	BaseClass::SetDormant( bDormant );
}


bool C_ObjectDispenser::ShouldShowHealingEffectForPlayer( C_TFPlayer *pPlayer )
{
	// Don't give away cloaked spies.
	// FIXME: Is the latter part of this check necessary?
	if ( pPlayer->m_Shared.IsStealthed() || pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) )
		return false;
		
	// Don't show the effect for disguised spies unless they're the same color.
	if ( GetLocalPlayerTeam() >= FIRST_GAME_TEAM && pPlayer->m_Shared.InCond(TF_COND_DISGUISED) && ( !pPlayer->m_Shared.DisguiseFoolsTeam( GetTeamNumber() ) ) )
		return false;

	return true;
}


void C_ObjectDispenser::UpdateEffects( void )
{
	// Find all the targets we've stopped healing.
	int i, j, c, x;
	bool bStillHealing[MAX_PLAYERS];
	for ( i = 0, c = m_hHealingTargetEffects.Count(); i < c; i++ )
	{
		bStillHealing[i] = false;

		// Are we still healing this target?
		for ( j = 0, x = m_hHealingTargets.Count(); j < x; j++ )
		{
			if ( m_hHealingTargets[j] && m_hHealingTargets[j] == m_hHealingTargetEffects[i].pTarget &&
				ShouldShowHealingEffectForPlayer( m_hHealingTargets[j] ) )
			{
				bStillHealing[i] = true;
				break;
			}
		}
	}

	// Now remove all the dead effects.
	for ( i = m_hHealingTargetEffects.Count() - 1; i >= 0; i-- )
	{
		if ( !bStillHealing[i] )
		{
			ParticleProp()->StopEmission( m_hHealingTargetEffects[i].pEffect );
			m_hHealingTargetEffects.Remove( i );
		}
	}

	// Now add any new targets.
	for ( i = 0, c = m_hHealingTargets.Count(); i < c; i++ )
	{
		C_TFPlayer *pTarget = m_hHealingTargets[i].Get();
		if ( !pTarget || !ShouldShowHealingEffectForPlayer( pTarget ) )
			continue;

		// Loops through the healing targets, and make sure we have an effect for each of them.
		bool bHaveEffect = false;
		for ( j = 0, x = m_hHealingTargetEffects.Count(); j < x; j++ )
		{
			if ( m_hHealingTargetEffects[j].pTarget == pTarget )
			{
				bHaveEffect = true;
				break;
			}
		}

		if ( bHaveEffect )
			continue;

		const char *pszEffectName = ConstructTeamParticle( "dispenser_heal_%s", GetTeamNumber() );
		CNewParticleEffect *pEffect = ( GetObjectFlags() & OF_IS_CART_OBJECT ) ?
			ParticleProp()->Create( pszEffectName, PATTACH_ABSORIGIN_FOLLOW ) :
			ParticleProp()->Create( pszEffectName, PATTACH_POINT_FOLLOW, "heal_origin" );

		ParticleProp()->AddControlPoint( pEffect, 1, pTarget, PATTACH_ABSORIGIN_FOLLOW, NULL, Vector( 0, 0, 50 ) );

		int iIndex = m_hHealingTargetEffects.AddToTail();
		m_hHealingTargetEffects[iIndex].pTarget = pTarget;
		m_hHealingTargetEffects[iIndex].pEffect = pEffect;

		// Start the sound over again every time we start a new beam.
		StopSound( "Building_Dispenser.Heal" );

		CLocalPlayerFilter filter;
		EmitSound( filter, entindex(), "Building_Dispenser.Heal" );

		m_bPlayingSound = true;
	}

	// Stop the sound if we're not healing anyone.
	if ( m_bPlayingSound && m_hHealingTargets.Count() == 0 )
	{
		m_bPlayingSound = false;

		// Stop the sound.
		StopSound( "Building_Dispenser.Heal" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Damage level has changed, update our effects.
//-----------------------------------------------------------------------------
void C_ObjectDispenser::UpdateDamageEffects( BuildingDamageLevel_t damageLevel )
{
	if ( m_pDamageEffects )
	{
		ParticleProp()->StopEmission( m_pDamageEffects );
		m_pDamageEffects = NULL;
	}

	if ( IsPlacing() )
		return;

	const char *pszEffect;
	switch ( damageLevel )
	{
		case BUILDING_DAMAGE_LEVEL_LIGHT:
			pszEffect = "dispenserdamage_1";
			break;
		case BUILDING_DAMAGE_LEVEL_MEDIUM:
			pszEffect = "dispenserdamage_2";
			break;
		case BUILDING_DAMAGE_LEVEL_HEAVY:
			pszEffect = "dispenserdamage_3";
			break;
		case BUILDING_DAMAGE_LEVEL_CRITICAL:
			pszEffect = "dispenserdamage_4";
			break;

		default:
			pszEffect = "";
			break;
	}

	if ( V_strlen( pszEffect ) > 0 )
	{
		m_pDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN );
	}
}

//-----------------------------------------------------------------------------
// Control screen 
//-----------------------------------------------------------------------------

DECLARE_VGUI_SCREEN_FACTORY( CDispenserControlPanel, "screen_obj_dispenser_red" );
DECLARE_VGUI_SCREEN_FACTORY( CDispenserControlPanel_Blue, "screen_obj_dispenser_blue" );
DECLARE_VGUI_SCREEN_FACTORY( CDispenserControlPanel_Green, "screen_obj_dispenser_green" );
DECLARE_VGUI_SCREEN_FACTORY( CDispenserControlPanel_Yellow, "screen_obj_dispenser_yellow" );


//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CDispenserControlPanel::CDispenserControlPanel( vgui::Panel *parent, const char *panelName )
: BaseClass( parent, "CDispenserControlPanel" ) 
{
	m_pAmmoProgress = new RotatingProgressBar( this, "MeterArrow" );
}

//-----------------------------------------------------------------------------
// Deactivates buttons we can't afford
//-----------------------------------------------------------------------------
void CDispenserControlPanel::OnTickActive( C_BaseObject *pObj, C_TFPlayer *pLocalPlayer )
{
	BaseClass::OnTickActive( pObj, pLocalPlayer );

	m_pAmmoProgress->SetProgress( assert_cast<C_ObjectDispenser *>( pObj )->GetMetalAmmoCount() / (float)DISPENSER_MAX_METAL_AMMO );
}


IMPLEMENT_CLIENTCLASS_DT( C_ObjectCartDispenser, DT_ObjectCartDispenser, CObjectCartDispenser )
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_ObjectMiniDispenser, DT_ObjectMiniDispenser, CObjectMiniDispenser)
RecvPropBool(RECVINFO(m_bHealingTargetsParity)),
RecvPropInt(RECVINFO(m_iAmmoMetal)),
RecvPropArray2(
	RecvProxyArrayLength_HealingArray,
	RecvPropInt("healing_array_element", 0, SIZEOF_IGNORE, 0, RecvProxy_HealingList),
	MAX_PLAYERS,
	0,
	"healing_array"
)
END_RECV_TABLE()

C_ObjectMiniDispenser::C_ObjectMiniDispenser()
{
	m_bUpdateHealingTargets = m_bHealingTargetsParity = m_bOldHealingTargetsParity = false;
	m_bPlayingSound = false;

	m_pDamageEffects = NULL;
}

C_ObjectMiniDispenser::~C_ObjectMiniDispenser()
{
	StopSound("Building_Dispenser.Heal");
}
