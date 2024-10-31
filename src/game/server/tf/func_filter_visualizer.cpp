//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "func_filter_visualizer.h"
#include "func_no_build.h"
#include "tf_team.h"
#include "ndebugoverlay.h"
#include "tf_gamerules.h"
#include "entity_tfstart.h"
#include "filters.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//===========================================================================================================

LINK_ENTITY_TO_CLASS( func_filtervisualizer, CFuncFilterVisualizer);

BEGIN_DATADESC( CFuncFilterVisualizer )
	DEFINE_KEYFIELD( m_iFilterName,	FIELD_STRING,	"filtername" ),
	DEFINE_FIELD( m_hFilter,	FIELD_EHANDLE ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CFuncFilterVisualizer, DT_FuncFilterVisualizer )
END_SEND_TABLE()


void CFuncFilterVisualizer::Spawn( void )
{
	BaseClass::Spawn();

	SetActive( true );

	SetCollisionGroup( TFCOLLISION_GROUP_RESPAWNROOMS );
}

//------------------------------------------------------------------------------
// Activate
//------------------------------------------------------------------------------
void CFuncFilterVisualizer::Activate( void ) 
{ 
	// Get a handle to my filter entity if there is one
	if (m_iFilterName != NULL_STRING)
	{
		m_hFilter = dynamic_cast<CBaseFilter *>(gEntList.FindEntityByName( NULL, m_iFilterName ));
	}

	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this entity passes the filter criteria, false if not.
// Input  : pOther - The entity to be filtered.
//-----------------------------------------------------------------------------
bool CFuncFilterVisualizer::PassesTriggerFilters(CBaseEntity *pOther)
{
	CBaseFilter *pFilter = m_hFilter.Get();
	
	return (!pFilter) ? false : pFilter->PassesFilter( this, pOther );
}


int CFuncFilterVisualizer::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: Only transmit this entity to clients that aren't in our team
//-----------------------------------------------------------------------------
int CFuncFilterVisualizer::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	CBaseEntity *pRecipientEntity = CBaseEntity::Instance( pInfo->m_pClientEnt );

	return PassesTriggerFilters(pRecipientEntity) ? FL_EDICT_ALWAYS : FL_EDICT_DONTSEND;
}


void CFuncFilterVisualizer::SetActive( bool bActive )
{
	if ( bActive )
	{
		// We're a trigger, but we want to be solid. Out ShouldCollide() will make
		// us non-solid to members of the team that spawns here.
		RemoveSolidFlags( FSOLID_TRIGGER );
		RemoveSolidFlags( FSOLID_NOT_SOLID );	
	}
	else
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
		AddSolidFlags( FSOLID_TRIGGER );	
	}
}

//-----------------------------------------------------------------------------
// Purpose: By default only forwards the call to should collide
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFuncFilterVisualizer::ShouldCollide( int collisionGroup, int contentsMask,  const IHandleEntity *pEnt ) const
{
	if( !pEnt )
		return false;

	CBaseEntity *pPassEntity = const_cast<CBaseEntity*>( EntityFromEntityHandle( pEnt ) );
	return const_cast<CFuncFilterVisualizer*>(this)->PassesTriggerFilters(pPassEntity);
}