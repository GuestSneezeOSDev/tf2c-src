//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_mediccallers.h"
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

#define MEDICCALLER_WIDE		(YRES(56))
#define MEDICCALLER_TALL		(YRES(30))
#define MEDICCALLER_ARROW_WIDE	(YRES(16))
#define MEDICCALLER_ARROW_TALL	(YRES(24))

CLIENTEFFECT_REGISTER_BEGIN( PrecacheMedicArrow )
CLIENTEFFECT_MATERIAL( "hud/medic_arrow" )
CLIENTEFFECT_REGISTER_END()


CTFMedicCallerPanel::CTFMedicCallerPanel( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pCallerBG = new CTFImagePanel( this, "CallerBG" );
	m_pCallerBurning = new CTFImagePanel( this, "CallerBurning" );
	m_pCallerBleeding = new CTFImagePanel( this, "CallerBleeding" );
	m_pCallerHurt = new CTFImagePanel( this, "CallerHealth" );
	m_pCallerAuto = new CTFImagePanel( this, "CallerAuto" );

	m_iDrawArrow = DRAW_ARROW_UP;
	m_bOnscreen = false;
	m_bAuto = false;
}


CTFMedicCallerPanel::~CTFMedicCallerPanel( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFMedicCallerPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/MedicCallerPanel.res" );
	m_ArrowMaterial.Init( "hud/medic_arrow", TEXTURE_GROUP_VGUI );
}


void CTFMedicCallerPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	m_pCallerBG->SetPos( (GetWide() - m_pCallerBG->GetWide()) * 0.5, (GetTall() - m_pCallerBG->GetTall()) * 0.5 );
	m_pCallerBurning->SetPos( (GetWide() - m_pCallerBurning->GetWide()) * 0.5, (GetTall() - m_pCallerBurning->GetTall()) * 0.5 );
	m_pCallerBleeding->SetPos( (GetWide() - m_pCallerBleeding->GetWide()) * 0.5, (GetTall() - m_pCallerBleeding->GetTall()) * 0.5 );
	m_pCallerHurt->SetPos( (GetWide() - m_pCallerHurt->GetWide()) * 0.5, (GetTall() - m_pCallerHurt->GetTall()) * 0.5 );
	m_pCallerAuto->SetPos( (GetWide() - m_pCallerAuto->GetWide()) * 0.5, (GetTall() - m_pCallerAuto->GetTall()) * 0.5 );
}


void CTFMedicCallerPanel::GetCallerPosition( const Vector &vecDelta, float flRadius, float *xpos, float *ypos, float *flRotation )
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
	*xpos = (int)((ScreenWidth() / 2) + (flRadius * sa));
	*ypos = (int)((ScreenHeight() / 2) - (flRadius * ca));
}


void CTFMedicCallerPanel::OnTick( void )
{
	if ( !m_hPlayer || !m_hPlayer->IsAlive() || gpGlobals->curtime > m_flRemoveAt )
	{
		MarkForDeletion();
		return;
	}

	// If the local player has started healing this guy, remove it too.
	//Also don't draw it if we're dead.
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalTFPlayer )
	{
		CBaseEntity *pHealTarget = pLocalTFPlayer->MedicGetHealTarget();
		if ( (pHealTarget && pHealTarget == m_hPlayer) || pLocalTFPlayer->IsAlive() == false )
		{
			MarkForDeletion();
			return;
		}

		// If we're pointing to an enemy Spy and they're no longer disguised, remove ourselves.
		if ( m_hPlayer->IsPlayerClass( TF_CLASS_SPY, true ) && !m_hPlayer->IsDisguisedEnemy( true ) )
		{
			MarkForDeletion();
			return;
		}

		if ( !m_bAuto )
		{
			// Show the burning and bleeding background depending on the player condition.
			m_pCallerBurning->SetVisible( m_hPlayer->m_Shared.InCond( TF_COND_BURNING ) );
			m_pCallerBleeding->SetVisible( m_hPlayer->m_Shared.InCond( TF_COND_BLEEDING ) );

			// Scale the icon redness based on player's health.
			m_pCallerHurt->SetVisible( true );
			m_pCallerHurt->SetAlpha( (int)RemapValClamped( m_hPlayer->HealthFraction(), 0.0f, 1.0f, 255.0f, 0.0f ) );
		}
		else
		{
			m_pCallerBurning->SetVisible( false );
			m_pCallerBleeding->SetVisible( false );
			m_pCallerHurt->SetVisible( false );
		}

	}
}


void CTFMedicCallerPanel::PaintBackground( void )
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
	Vector vecTarget = (m_hPlayer->GetAbsOrigin() + m_vecOffset);
	Vector vecDelta = vecTarget - MainViewOrigin();
	bool bOnscreen = GetVectorInScreenSpace( vecTarget, iX, iY );

	int halfWidth = GetWide() / 2;
	if ( !bOnscreen || iX < halfWidth || iX > ScreenWidth() - halfWidth )
	{
		// It's off the screen. Position the callout.
		VectorNormalize( vecDelta );
		float xpos, ypos;
		float flRotation;
		float flRadius = YRES( 100 );
		GetCallerPosition( vecDelta, flRadius, &xpos, &ypos, &flRotation );

		iX = xpos;
		iY = ypos;

		Vector vCenter = m_hPlayer->WorldSpaceCenter();
		if ( MainViewRight().Dot( vCenter - MainViewOrigin() ) > 0 )
		{
			m_iDrawArrow = DRAW_ARROW_RIGHT;
		}
		else
		{
			m_iDrawArrow = DRAW_ARROW_LEFT;
		}

		// Move the icon there
		SetPos( iX - halfWidth, iY - (GetTall() / 2) );
		SetAlpha( 255 );
	}
	else
	{
		// On screen
		// If our target isn't visible, we draw transparently
		trace_t	tr;
		UTIL_TraceLine( vecTarget, MainViewOrigin(), MASK_OPAQUE, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction >= 1.0f )
		{
			m_bOnscreen = true;
			SetAlpha( 0 );
			return;
		}

		m_iDrawArrow = DRAW_ARROW_UP;
		SetAlpha( 92 );
		SetPos( iX - halfWidth, iY - (GetTall() / 2) );
	}

	m_bOnscreen = false;
	BaseClass::PaintBackground();
}


void CTFMedicCallerPanel::Paint( void )
{
	// Don't draw if our target is visible. The particle effect will be doing it for us.
	if ( m_bOnscreen )
		return;

	BaseClass::Paint();

	if ( m_iDrawArrow == DRAW_ARROW_UP )
		return;

	float uA, uB, yA, yB;
	int x, y;
	GetPos( x, y );
	if ( m_iDrawArrow == DRAW_ARROW_LEFT )
	{
		uA = 1.0;
		uB = 0.0;
		yA = 0.0;
		yB = 1.0;
	}
	else
	{
		uA = 0.0;
		uB = 1.0;
		yA = 0.0;
		yB = 1.0;
		x += GetWide() - MEDICCALLER_ARROW_WIDE;
	}

	int iyindent = (GetTall() - MEDICCALLER_ARROW_TALL) * 0.5;
	y += iyindent;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_ArrowMaterial );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, uA, yA );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + MEDICCALLER_ARROW_WIDE, y, 0.0f );
	meshBuilder.TexCoord2f( 0, uB, yA );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + MEDICCALLER_ARROW_WIDE, y + MEDICCALLER_ARROW_TALL, 0.0f );
	meshBuilder.TexCoord2f( 0, uB, yB );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x, y + MEDICCALLER_ARROW_TALL, 0.0f );
	meshBuilder.TexCoord2f( 0, uA, yB );
	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}


void CTFMedicCallerPanel::SetPlayer( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset, bool bAuto )
{
	m_hPlayer = pPlayer;
	m_flRemoveAt = gpGlobals->curtime + flDuration;
	m_vecOffset = vecOffset;
	m_bAuto = bAuto;

	m_pCallerBG->SetVisible( !bAuto );
	m_pCallerAuto->SetVisible( bAuto );
}


void CTFMedicCallerPanel::AddMedicCaller( C_TFPlayer *pPlayer, float flDuration, Vector &vecOffset, bool bAuto )
{
	CTFMedicCallerPanel *pCaller = NULL;

	// See if we can re-use the existing caller panel.
	for ( CTFMedicCallerPanel *pCallerList : CTFMedicCallerPanel::AutoList() )
	{
		if ( pCallerList->m_hPlayer == pPlayer )
		{
			pCaller = pCallerList;
		}
	}

	if ( !pCaller )
	{
		pCaller = new CTFMedicCallerPanel( g_pClientMode->GetViewport(), "MedicCallerPanel" );

		vgui::SETUP_PANEL( pCaller );
		pCaller->SetBounds( 0, 0, MEDICCALLER_WIDE, MEDICCALLER_TALL );
		pCaller->SetVisible( true );
		vgui::ivgui()->AddTickSignal( pCaller->GetVPanel() );
	}

	pCaller->SetPlayer( pPlayer, flDuration, vecOffset, bAuto );
}