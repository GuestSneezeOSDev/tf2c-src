//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_patienthealthbar.h"
#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "view.h"
#include "ivieweffects.h"
#include "viewrender.h"
#include "prediction.h"
#include "clienteffectprecachesystem.h"
#include "tf_spectatorgui.h"
#include "c_tf_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef TF2C_BETA

#define MEDICCALLER_WIDE		(YRES(56))
#define MEDICCALLER_TALL		(YRES(30))
#define MEDICCALLER_ARROW_WIDE	(YRES(16))
#define MEDICCALLER_ARROW_TALL	(YRES(24))

//CLIENTEFFECT_REGISTER_BEGIN( PrecacheMedicArrow )
//CLIENTEFFECT_MATERIAL( "hud/medic_arrow" )
//CLIENTEFFECT_REGISTER_END()

#ifdef CLIENT_DLL
ConVar tf2c_medicgl_show_patient_health( "tf2c_medicgl_show_patient_health", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Enables the displaying of health bars above targets recently healed by the player." );
ConVar tf2c_medicgl_recent_patient_timer( "tf2c_medicgl_recent_patient_timer", "4", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "The duration in seconds to display health bars above recently healed patients (if tf2c_medicgl_show_patient_health is enabled)." );
ConVar tf2c_medicgl_recent_patient_timer_fadeout( "tf2c_medicgl_recent_patient_timer_fadeout", "1.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "The duration in seconds of the fading out of the recent patient health bars." );
ConVar tf2c_medicgl_recent_patient_radius( "tf2c_medicgl_recent_patient_radius", "100", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "The halo radius of the offscreen recent patient health bars." );
ConVar tf2c_medicgl_recent_patient_offset( "tf2c_medicgl_recent_patient_offset", "18", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "The offset from the patient's head that the healthbar should show." );
ConVar tf2c_debug_patientbar("tf2c_debug_patientbar", "0", FCVAR_CLIENTDLL | FCVAR_HIDDEN);
ConVar tf2c_patientbar_falloff("tf2c_patientbar_falloff", "300", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "The numerator of the N/distance function used for scaling patient healthbars. Distances below this value will always have a scale of 1.");
#endif


CTFPatientHealthbarPanel::CTFPatientHealthbarPanel( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pPatientHealthbar = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_flHealthbarOpacity = 1.0f;
	m_iLastAlpha = -1;
	m_iWidthDefault = m_iHeightDefault = 1;
	m_pPatientHealthbar->GetSize( m_iWidthDefault, m_iHeightDefault );
	m_pPatientHealthbar->SetHealthTextVisibility(false);
}


CTFPatientHealthbarPanel::~CTFPatientHealthbarPanel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFPatientHealthbarPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/HUDPatientHealthBar.res" );
	//m_ArrowMaterial.Init( "hud/medic_arrow", TEXTURE_GROUP_VGUI );

	m_pPatientHealthbar->GetSize( m_iWidthDefault, m_iHeightDefault );

	m_pPatientHealthbar->SetHealthTextVisibility( false );
}


void CTFPatientHealthbarPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	m_pPatientHealthbar->SetPos( (GetWide() - m_pPatientHealthbar->GetWide()) * 0.5, (GetTall() - m_pPatientHealthbar->GetTall()) * 0.5 );
}


void CTFPatientHealthbarPanel::GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
{
	// Player Data
	Vector playerPosition = MainViewOrigin();
	QAngle playerAngles = MainViewAngles();

	Vector forward, right, up( 0, 0, 1 );
	AngleVectors( playerAngles, &forward, NULL, NULL );
	forward.z = 0;
	VectorNormalize( forward );
	CrossProduct( up, forward, right );
	float front = DotProduct( vecDelta, forward );
	float side = DotProduct( vecDelta, right );
	*xpos = flRadius * -side;
	*ypos = flRadius * -front;

	// Get the rotation (yaw)
	*flRotation = atan2( *xpos, *ypos ) + M_PI_F;
	*flRotation *= 180 / M_PI_F;

	float yawRadians = -DEG2RAD( *flRotation );
	float ca = cos( yawRadians );
	float sa = sin( yawRadians );

	// Rotate it around the circle
	*xpos = (int)( ( ScreenWidth() / 2 ) + ( flRadius * sa ) );
	*ypos = (int)( ( ScreenHeight() / 2 ) - ( flRadius * ca ) );
}


void CTFPatientHealthbarPanel::OnTick( void )
{
	if ( (!m_hPlayer || !m_hPlayer->IsAlive() || gpGlobals->curtime > m_flRemoveAt) && !tf2c_debug_patientbar.GetBool() )
	{
		DevMsg("Patient died or icon expired, marking for deletion.\n");
		MarkForDeletion();
		return;
	}

	// If the local player has started healing this guy, remove it too.
	//Also don't draw it if we're dead.
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalTFPlayer )
	{
		if ( pLocalTFPlayer->IsAlive() == false && !tf2c_debug_patientbar.GetBool())
		{
			DevMsg( "You died, marking for deletion.\n" );
			MarkForDeletion();
			return;
		}

		// If we're pointing to an enemy Spy and they're no longer disguised, remove ourselves.
		if ( m_hPlayer->IsPlayerClass( TF_CLASS_SPY, true ) && (!m_hPlayer->IsDisguisedEnemy( true ) || m_hPlayer->GetPercentInvisible() > 0.5f) && !tf2c_debug_patientbar.GetBool() )
		{
			DevMsg( "Spy lost disguise or went invisible, marking for deletion.\n" );
			MarkForDeletion();
			return;
		}

		float flOpacity;
		// Update opacity (fadeout)
		if ( !tf2c_debug_patientbar.GetBool() )
			flOpacity = RemapValClamped( m_flFadeOutTime - (m_flRemoveAt - gpGlobals->curtime), 0, m_flFadeOutTime, 1, 0 );
		else
			flOpacity = 1;
		/*if ( flOpacity <= 0.0f ) {
			DevMsg( "Opacity hit 0, deleting.\n" );
			MarkForDeletion();
		}*/

		int iAlpha = (int)(flOpacity * 255);
		if ( iAlpha != m_iLastAlpha )
		{
			SetAlphaRecursive( this, iAlpha );
			m_pPatientHealthbar->m_iAlpha = iAlpha;
			m_pPatientHealthbar->SetChildrenAlpha( iAlpha );
			m_iLastAlpha = iAlpha;
		}
		
		UpdatePatientHealth();
		m_pPatientHealthbar->SetVisible( true );
	}
}


void CTFPatientHealthbarPanel::PaintBackground( void )
{
	// If the local player has started healing this guy, remove it too.
	//Also don't draw it if we're dead.
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	if ( !m_hPlayer || m_hPlayer->IsDormant() )
	{
		SetAlpha( 0 );
		return;
	}

	// Reposition the callout based on our target's position
	int iX, iY;
	Vector vecTarget = ( m_hPlayer->GetAbsOrigin() + m_vecOffset );
	Vector vecDelta = vecTarget - MainViewOrigin();
	//float flDist = vecDelta.LengthSqr();
	bool bOnscreen = GetVectorInScreenSpace( vecTarget, iX, iY );

	int halfWidth = GetWide() / 2;
	if ( !bOnscreen || iX < halfWidth || iX > ScreenWidth() - halfWidth )
	{
		// It's off the screen. Position the callout.
		VectorNormalize( vecDelta );
		float xpos, ypos;
		float flRotation;
		float flRadius = YRES( tf2c_medicgl_recent_patient_radius.GetFloat() );
		GetCallerPosition( vecDelta, flRadius, &xpos, &ypos, &flRotation );

		iX = xpos;
		iY = ypos;

		// Move the icon there
		SetPos( iX - halfWidth, iY - ( GetTall() / 2 ) );

		m_pPatientHealthbar->SetHealthbarScale( 1.0f );
	}
	else
	{
		// On screen
		// If our target isn't visible, we draw transparently
		/*trace_t	tr;
		UTIL_TraceLine( vecTarget, MainViewOrigin(), MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction >= 1.0f )
		{
			SetAlpha( 150 );
			return;
		}*/


		iX = clamp( iX, 0, ScreenWidth() );
		iY = clamp( iY, 0, ScreenHeight() );
		SetPos( iX - halfWidth, iY - (GetTall() / 2) );

		//m_pPatientHealthbar->SetHealthbarScale( (sinf(gpGlobals->curtime) + 1)/2.0f );
		float flDist = vecDelta.Length();
		float flScale = clamp( tf2c_patientbar_falloff.GetFloat() / flDist, 0.0f, 1.0f );
		m_pPatientHealthbar->SetHealthbarScale( flScale );
		//DevMsg( "SCALE: %2.2f \n", flScale );

		//SetSizeRecursive( this, 64, 64 );
	 }

	BaseClass::PaintBackground();
}


void CTFPatientHealthbarPanel::SetPlayer( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset, bool bAuto )
{
	m_hPlayer = pPlayer;
	m_flRemoveAt = gpGlobals->curtime + flDuration;
	m_vecOffset = vecOffset + Vector(0, 0, tf2c_medicgl_recent_patient_offset.GetFloat());
}


void CTFPatientHealthbarPanel::AddPatientHealthbar( C_TFPlayer *pPlayer, Vector &vecOffset )
{
	CTFPatientHealthbarPanel *pCaller = NULL;

	// See if we can re-use the existing caller panel.
	for ( CTFPatientHealthbarPanel *pCallerList : CTFPatientHealthbarPanel::AutoList() )
	{
		if ( pCallerList->m_hPlayer == pPlayer )
		{
			pCaller = pCallerList;
			//DevMsg("Player already has a medic caller panel! Refreshing now.\n");
		}
	}

	if ( !pCaller )
	{
		pCaller = new CTFPatientHealthbarPanel( g_pClientMode->GetViewport(), "MedicCallerPanel" );

		vgui::SETUP_PANEL( pCaller );
		pCaller->SetBounds( 0, 0, MEDICCALLER_WIDE, MEDICCALLER_TALL );
		pCaller->SetSize( MEDICCALLER_WIDE, MEDICCALLER_TALL );
		pCaller->SetVisible( true );
		vgui::ivgui()->AddTickSignal( pCaller->GetVPanel() );
	}

	pCaller->m_flFadeOutTime = min(tf2c_medicgl_recent_patient_timer_fadeout.GetFloat(), tf2c_medicgl_recent_patient_timer.GetFloat());
	pCaller->SetPlayer( pPlayer, tf2c_medicgl_recent_patient_timer.GetFloat(), vecOffset, false );
	pCaller->m_flHealthbarOpacity = 1.0f;
	pCaller->m_iLastAlpha = -1;
	pCaller->m_pPatientHealthbar->SetVisible( true );
}

//-----------------------------------------------------------------------------
// Purpose: Refreshes the remaining duration on the UI element
//-----------------------------------------------------------------------------
/*void CTFPatientHealthbarPanel::SetDurationRemaining( float flDurationNew )
{
	m_flRemoveAt = gpGlobals->curtime + flDurationNew;
	m_flDurationTotal = flDurationNew;
}*/

void CTFPatientHealthbarPanel::UpdatePatientHealth()
{
	int iTargetEntIndex = m_hPlayer->index;

	if ( !g_PR )
		return;

	int iHealth = 0;
	int iMaxHealth = 1;
	int iMaxBuffedHealth = 0;
	bool bDisguisedEnemy = false;

	// If this is a disguised enemy spy, change their identity and color.
	if ( m_hPlayer->IsDisguisedEnemy() )
	{
		bDisguisedEnemy = true;

		iTargetEntIndex = m_hPlayer->m_Shared.GetDisguiseTargetIndex();
	}


	if ( bDisguisedEnemy )
	{
		iHealth = m_hPlayer->m_Shared.GetDisguiseHealth();
		iMaxHealth = m_hPlayer->m_Shared.GetDisguiseMaxHealth();
		iMaxBuffedHealth = m_hPlayer->m_Shared.GetDisguiseMaxBuffedHealth();
	}
	else
	{
		iHealth = m_hPlayer->GetHealth();
		iMaxHealth = g_TF_PR->GetMaxHealth( iTargetEntIndex );
		iMaxBuffedHealth = m_hPlayer->m_Shared.GetMaxBuffedHealth();
	}

	m_pPatientHealthbar->SetHealth( iHealth, iMaxHealth, iMaxBuffedHealth );
}

void CTFPatientHealthbarPanel::SetAlphaRecursive( Panel *pPanel, int iAlpha )
{
	pPanel->SetAlpha( iAlpha );
	FOR_EACH_VEC( pPanel->GetChildren(), i )
	{
		Panel *pChild = pPanel->GetChild( i );
		SetAlphaRecursive( pChild, iAlpha );
	}
}

void CTFPatientHealthbarPanel::SetSizeRecursive( Panel *pPanel, int iWidth, int iHeight)
{
	pPanel->SetSize( iWidth, iHeight );
	pPanel->SetBounds( 0, 0, iWidth, iHeight );
	FOR_EACH_VEC( pPanel->GetChildren(), i )
	{
		Panel *pChild = pPanel->GetChild( i );
		SetSizeRecursive( pChild, iWidth, iHeight);
	}
}

#endif // TF2C_BETA