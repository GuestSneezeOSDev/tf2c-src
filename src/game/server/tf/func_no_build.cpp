//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"
#include "func_no_build.h"
#include "tf_team.h"
#include "ndebugoverlay.h"
#include "tf_obj.h"
#include "triggers.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_AUTO_LIST( IFuncNoBuildAutoList );

//-----------------------------------------------------------------------------
// Purpose: Defines an area where objects cannot be built
//-----------------------------------------------------------------------------
class CFuncNoBuild : public CBaseTrigger, public IFuncNoBuildAutoList
{
	DECLARE_CLASS( CFuncNoBuild, CBaseTrigger );
public:
	CFuncNoBuild();

	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Activate( void );

	// Inputs
	void	InputSetActive( inputdata_t &inputdata );
	void	InputSetInactive( inputdata_t &inputdata );
	void	InputToggleActive( inputdata_t &inputdata );

	void	SetActive( bool bActive );
	bool	GetActive() const;

	//bool	IsEmpty( void );
	bool	PointIsWithin( const Vector &vecPoint );
	bool	PreventsBuildOf( int iObjectType );

private:
	bool	m_bActive;

	bool	m_bAllowSentry;
	bool	m_bAllowDispenser;
	bool	m_bAllowTeleporters;
};

LINK_ENTITY_TO_CLASS( func_nobuild, CFuncNoBuild );

BEGIN_DATADESC( CFuncNoBuild )
	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "SetActive", InputSetActive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetInactive", InputSetInactive ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ToggleActive", InputToggleActive ),

	// Keyfields
	DEFINE_KEYFIELD( m_bAllowSentry, FIELD_BOOLEAN, "AllowSentry" ),
	DEFINE_KEYFIELD( m_bAllowDispenser, FIELD_BOOLEAN, "AllowDispenser" ),
	DEFINE_KEYFIELD( m_bAllowTeleporters, FIELD_BOOLEAN, "AllowTeleporters" ),
END_DATADESC()

PRECACHE_REGISTER( func_nobuild );

IMPLEMENT_AUTO_LIST( IFuncNoBuildAutoList );


CFuncNoBuild::CFuncNoBuild()
	: m_bAllowSentry( false )
	, m_bAllowDispenser( false )
	, m_bAllowTeleporters( false )
{
}

//-----------------------------------------------------------------------------
// Purpose: Initializes the resource zone
//-----------------------------------------------------------------------------
void CFuncNoBuild::Spawn( void )
{
	BaseClass::Spawn();
	InitTrigger();

	m_bActive = true;
}


void CFuncNoBuild::Precache( void )
{
}


void CFuncNoBuild::Activate( void )
{
	BaseClass::Activate();
	SetActive( true );
}



void CFuncNoBuild::InputSetActive( inputdata_t &inputdata )
{
	SetActive( true );
}


void CFuncNoBuild::InputSetInactive( inputdata_t &inputdata )
{
	SetActive( false );
}


void CFuncNoBuild::InputToggleActive( inputdata_t &inputdata )
{
	if ( m_bActive )
	{
		SetActive( false );
	}
	else
	{
		SetActive( true );
	}
}


void CFuncNoBuild::SetActive( bool bActive )
{
	m_bActive = bActive;
}


bool CFuncNoBuild::GetActive() const
{
	return m_bActive;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if the specified point is within this zone
//-----------------------------------------------------------------------------
bool CFuncNoBuild::PointIsWithin( const Vector &vecPoint )
{
	Ray_t ray;
	trace_t tr;
	ICollideable *pCollide = CollisionProp();
	ray.Init( vecPoint, vecPoint );
	enginetrace->ClipRayToCollideable( ray, MASK_ALL, pCollide, &tr );
	return ( tr.startsolid );
}


bool CFuncNoBuild::PreventsBuildOf( int iObjectType )
{
	switch ( iObjectType )
	{
		case OBJ_MINIDISPENSER:
		case OBJ_DISPENSER:
			return !m_bAllowDispenser;

		case OBJ_TELEPORTER:
		case OBJ_JUMPPAD:
			return !m_bAllowTeleporters;

		case OBJ_SENTRYGUN:
		case OBJ_FLAMESENTRY:
			return !m_bAllowSentry;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Does a nobuild zone prevent us from building?
//-----------------------------------------------------------------------------
bool PointInNoBuild( const Vector &vecBuildOrigin, const CBaseObject *pObj )
{
	Assert( pObj );
	if ( !pObj )
		return false;

	for ( int i = 0; i < IFuncNoBuildAutoList::AutoList().Count(); ++i )
	{
		CFuncNoBuild *pNoBuild = static_cast< CFuncNoBuild* >( IFuncNoBuildAutoList::AutoList()[i] );

		// Are we within this no build?
		if ( pNoBuild->GetActive()
			 && pNoBuild->PointIsWithin( vecBuildOrigin )
			 && pNoBuild->PreventsBuildOf( pObj->GetType() ) )
		{
			// See if this ent's on the team we're set to deny
			int nDenyTeam = pNoBuild->GetTeamNumber();
			if ( !nDenyTeam || nDenyTeam == pObj->GetTeamNumber() )
				return true;
		}
	}

	return false; // Building should be ok.
}
