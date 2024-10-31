//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "damagemodifier.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CDamageModifier::CDamageModifier()
{
	m_flModifier = 1;
	m_bDoneToMe = false;
}


void CDamageModifier::AddModifierToEntity( CBaseEntity *pEntity )
{
	RemoveModifier();

	pEntity->m_DamageModifiers.AddToTail( this );
	m_hEnt = pEntity;
}


void CDamageModifier::RemoveModifier()
{
	if ( m_hEnt.Get() )
	{
		m_hEnt->m_DamageModifiers.FindAndRemove( this );
		m_hEnt = 0;
	}
}


void CDamageModifier::SetModifier( float flScale )
{
	m_flModifier = flScale;
}


float CDamageModifier::GetModifier() const
{
	return m_flModifier;
}


CBaseEntity* CDamageModifier::GetCharacter() const
{
	return m_hEnt.Get();
}


void CDamageModifier::SetDoneToMe( bool bDoneToMe )
{
	m_bDoneToMe = bDoneToMe;
}


bool CDamageModifier::IsDamageDoneToMe() const
{
	return m_bDoneToMe;
}

