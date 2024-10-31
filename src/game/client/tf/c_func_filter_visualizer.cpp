//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class C_FuncFilterVisualizer : public C_BaseEntity
{
	DECLARE_CLASS( C_FuncFilterVisualizer, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_FuncFilterVisualizer();
	virtual int DrawModel( int flags );
	virtual void NotifyShouldTransmit( ShouldTransmitState_t state );

	virtual bool ShouldCollide( int collisionGroup, int contentsMask, CBaseEntity *pEnt ) const;
private:
	bool m_bLocalPlayerPassesFilter;
};

IMPLEMENT_CLIENTCLASS_DT( C_FuncFilterVisualizer, DT_FuncFilterVisualizer, CFuncFilterVisualizer )
END_RECV_TABLE()

C_FuncFilterVisualizer::C_FuncFilterVisualizer()
{
	m_bLocalPlayerPassesFilter = false;
}

//-----------------------------------------------------------------------------
// Purpose: Don't draw for friendly players
//-----------------------------------------------------------------------------
int C_FuncFilterVisualizer::DrawModel( int flags )
{
	// Don't draw for anyone in endround
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		return 1;
	}
	
	if( !m_bLocalPlayerPassesFilter )
		return 1;

	return BaseClass::DrawModel( flags );
}

//-----------------------------------------------------------------------------
// Purpose: By default only forwards the call to should collide
// Input  : collisionGroup - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_FuncFilterVisualizer::ShouldCollide( int collisionGroup, int contentsMask, C_BaseEntity *pEnt ) const
{
	return m_bLocalPlayerPassesFilter;
}


void C_FuncFilterVisualizer::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	// Init should have been called before we get in here.
	Assert( CollisionProp()->GetPartitionHandle() != PARTITION_INVALID_HANDLE );
	if ( entindex() < 0 )
		return;
	
	switch( state )
	{
	case SHOULDTRANSMIT_START:
		m_bLocalPlayerPassesFilter = true;
		break;
	case SHOULDTRANSMIT_END:
		m_bLocalPlayerPassesFilter = false;
		break;
	}

	BaseClass::NotifyShouldTransmit( state );
}