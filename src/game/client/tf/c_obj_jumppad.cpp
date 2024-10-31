//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CObjectJumppad
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_baseobject.h"
#include "c_tf_player.h"
#include "vgui/ILocalize.h"
#include "c_obj_jumppad.h"
#include "soundenvelope.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define JUMPPAD_MINS			Vector( -24, -24, 0)
#define JUMPPAD_MAXS			Vector( 24, 24, 12)	

//-----------------------------------------------------------------------------
// Purpose: Jump Pad object
//-----------------------------------------------------------------------------

IMPLEMENT_CLIENTCLASS_DT(C_ObjectJumppad, DT_ObjectJumppad, CObjectJumppad)
	RecvPropInt( RECVINFO( m_iState ) ),
	RecvPropInt( RECVINFO( m_iTimesUsed ) ),
END_RECV_TABLE()


C_ObjectJumppad::C_ObjectJumppad()
{
	m_pFanSpinEffect = NULL;
	m_pDamageEffects = NULL;

	m_pSpinSound = NULL;
}


void C_ObjectJumppad::UpdateOnRemove( void )
{
	StopActiveEffects();

	if ( m_pSpinSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSpinSound );
	}

	BaseClass::UpdateOnRemove();
}

void C_ObjectJumppad::GetStatusText( wchar_t *pStatus, int iMaxStatusLen )
{
	float flHealthPercent = (float)GetHealth() / (float)GetMaxHealth();
	wchar_t wszHealthPercent[32];
	V_swprintf_safe( wszHealthPercent, L"%d%%", (int)( flHealthPercent * 100 ) );

	if ( IsBuilding() )
	{
		wchar_t *pszState = g_pVGuiLocalize->Find( "#TF_ObjStatus_JumpPad_Building" );
		if ( pszState )
		{
			g_pVGuiLocalize->ConstructString( pStatus, iMaxStatusLen, pszState,
				1,
				wszHealthPercent );
		}
	}
	else
	{
		const wchar_t *pszState = NULL;

		switch( m_iState )
		{
		case JUMPPAD_STATE_INACTIVE:
			pszState = g_pVGuiLocalize->Find( "#TF_Obj_JumpPad_State_Idle" );
			break;

		case JUMPPAD_STATE_UNDERWATER:
			pszState = g_pVGuiLocalize->Find("#TF_Obj_JumpPad_State_Underwater");
			break;

		case JUMPPAD_STATE_READY:
			pszState = g_pVGuiLocalize->Find( "#TF_Obj_JumpPad_State_Ready" );
			break;

		default:
			pszState = L"unknown";
			break;
		}

		if ( pszState )
		{
			wchar_t *pszTemplate = g_pVGuiLocalize->Find( "#TF_ObjStatus_JumpPad" );
			if ( pszTemplate )
			{
				g_pVGuiLocalize->ConstructString( pStatus, iMaxStatusLen, pszTemplate,
					2,
					wszHealthPercent,
					pszState );
			}
		}
	}
}



void C_ObjectJumppad::OnPreDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnPreDataChanged( updateType );

	m_iOldState = m_iState;
}

void C_ObjectJumppad::StartActiveEffects()
{
	char szEffect[128];
	V_sprintf_safe( szEffect, "jumppad_fan_air_%s" ,
		GetTeamSuffix( GetTeamNumber() ) );

	Assert(m_pFanSpinEffect == NULL);
	if (!m_pFanSpinEffect)
		m_pFanSpinEffect = ParticleProp()->Create(szEffect, PATTACH_ABSORIGIN);
	
	if ( !m_pSpinSound )
	{
		// Init the spin sound.
		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

		char szShootSound[128];
		V_sprintf_safe( szShootSound, "Building_Jumppad.Spin" );

		CLocalPlayerFilter filter;
		m_pSpinSound = controller.SoundCreate( filter, entindex(), szShootSound );
		controller.Play( m_pSpinSound, 1.0, 100 );
	}
}

void C_ObjectJumppad::StopActiveEffects()
{
	if (m_pFanSpinEffect)
	{
		ParticleProp()->StopEmission(m_pFanSpinEffect);
		m_pFanSpinEffect = NULL;
	}

	if ( m_pSpinSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pSpinSound );
		m_pSpinSound = NULL;
	}
}


void C_ObjectJumppad::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( m_iOldState != m_iState )
	{
		if (m_iState == JUMPPAD_STATE_READY && m_iOldState != JUMPPAD_STATE_READY)
		{
			StartActiveEffects();
		}
		else if (m_iState != JUMPPAD_STATE_READY && m_iOldState == JUMPPAD_STATE_READY)
		{
			StopActiveEffects();
		}

		m_iOldState = m_iState;
	}
}


int C_ObjectJumppad::GetTimesUsed( void )
{
	return m_iTimesUsed;
}


CStudioHdr *C_ObjectJumppad::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	SetNextClientThink( CLIENT_THINK_ALWAYS );

	return hdr;
}

void C_ObjectJumppad::GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes )
{
	sDataString[0] = '\0';

	wchar_t wszIDString[128];
	BaseClass::GetTargetIDDataString( wszIDString, iMaxLenInBytes );

	if ( m_iState == JUMPPAD_STATE_INACTIVE )
	{
		g_pVGuiLocalize->ConstructString( sDataString, MAX_ID_STRING, g_pVGuiLocalize->Find("#TF_playerid_jumppad_idle" ), 0 );
	}
	if (m_iState == JUMPPAD_STATE_UNDERWATER)
	{
		g_pVGuiLocalize->ConstructString(sDataString, MAX_ID_STRING, g_pVGuiLocalize->Find("#TF_playerid_jumppad_underwater"), 0);
	}

	V_wcsncat( sDataString, L" ", iMaxLenInBytes / 2 );

	V_wcsncat( sDataString, wszIDString, iMaxLenInBytes / 2 );
}

//-----------------------------------------------------------------------------
// Purpose: Damage level has changed, update our effects
//-----------------------------------------------------------------------------
void C_ObjectJumppad::UpdateDamageEffects( BuildingDamageLevel_t damageLevel )
{
	if ( m_pDamageEffects )
	{
		ParticleProp()->StopEmission( m_pDamageEffects );
		m_pDamageEffects = NULL;
	}

	if ( IsPlacing() )
		return;

	const char *pszEffect = "";

	switch ( damageLevel )
	{
		case BUILDING_DAMAGE_LEVEL_LIGHT:
			pszEffect = "tpdamage_1";
			break;
		case BUILDING_DAMAGE_LEVEL_MEDIUM:
			pszEffect = "tpdamage_2";
			break;
		case BUILDING_DAMAGE_LEVEL_HEAVY:
			pszEffect = "tpdamage_3";
			break;
		case BUILDING_DAMAGE_LEVEL_CRITICAL:
			pszEffect = "tpdamage_4";
			break;
		default:
			break;
	}

	if ( Q_strlen( pszEffect ) > 0 )
	{
		m_pDamageEffects = ParticleProp()->Create( pszEffect, PATTACH_ABSORIGIN );
	}
}
