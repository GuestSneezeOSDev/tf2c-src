//=============================================================================//
//
// Purpose: Arena team class composition
//
//=============================================================================//
#include "cbase.h"
#include "tf_hud_arena_class_layout.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudArenaClassLayout );

CHudArenaClassLayout::CHudArenaClassLayout( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudArenaClassLayout" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pBackground = new CTFImagePanel( this, "background" );
	m_pLocalPlayerBG = new CTFImagePanel( this, "localPlayerBG" );

	m_pTitleLabel = new CExLabel( this, "title", "" );
	m_pChangeLabel = new CExLabel( this, "changeLabel", "" );
	m_pChangeLabelShadow = new CExLabel( this, "changeLabelShadow", "" );

	for ( int i = 0; i < TF_HUD_ARENA_NUM_CLASS_IMAGES; i++ )
	{
		m_pClassImages[i] = new CTFImagePanel( this, VarArgs( "classImage%d", i ) );
	}

	memset( m_iClasses, 0, sizeof( m_iClasses ) );
	m_iLocalPlayerClassImage = -1;
	m_iNumClasses = -1;

	ivgui()->AddTickSignal( GetVPanel() );
	RegisterForRenderGroup( "mid" );
}


bool CHudArenaClassLayout::ShouldDraw( void )
{
	// Only in Arena mode.
	if ( !TFGameRules() || !TFGameRules()->IsInArenaMode() )
		return false;

	// Only if the player is actually playing.
	if ( GetLocalPlayerTeam() < FIRST_GAME_TEAM )
		return false;

	// Show it during preround phase.
	if ( TFGameRules()->State_Get() != GR_STATE_PREROUND )
		return false;

	return CHudElement::ShouldDraw();
}


void CHudArenaClassLayout::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudArenaClassLayout.res" );

	// Hide the change class labels, that feature was never made available in live TF2.
	m_pChangeLabel->SetVisible( false );
	m_pChangeLabelShadow->SetVisible( false );
}


void CHudArenaClassLayout::PerformLayout( void )
{
	int iTeam = GetLocalPlayerTeam();

	if ( iTeam < FIRST_GAME_TEAM )
		return;

	// Update BG width according to the amount of class images.
	int x, y, wide, tall;
	m_pBackground->GetBounds( x, y, wide, tall );

	int classX, classY, classWide, classTall;
	m_pClassImages[0]->GetBounds( classX, classY, classWide, classTall );
	int totalClassWide = ( classWide * m_iNumClasses );

	wide = totalClassWide + YRES( 20 ); // 10 units indent from each side.
	x = GetWide() / 2 - wide / 2;
	m_pBackground->SetBounds( x, y, wide, tall );

	m_pLocalPlayerBG->SetVisible( m_iLocalPlayerClassImage != -1 );

	int offset = GetWide() / 2 - totalClassWide / 2;

	for ( int i = 0; i < TF_HUD_ARENA_NUM_CLASS_IMAGES; i++ )
	{
		if ( i >= m_iNumClasses )
		{
			m_pClassImages[i]->SetVisible( false );
			continue;
		}

		m_pClassImages[i]->SetVisible( true );
		m_pClassImages[i]->SetPos( offset, classY );

		char szImage[128];
		V_sprintf_safe( szImage, "class_sel_sm_%s_%s", g_aRawPlayerClassNamesShort[m_iClasses[i]], g_aTeamNamesShort[iTeam] );
		m_pClassImages[i]->SetImage( szImage );

		if ( i == m_iLocalPlayerClassImage )
		{
			// That's me. Put the border in there.
			m_pLocalPlayerBG->SetPos( offset, m_pLocalPlayerBG->GetYPos() );
		}

		offset += classWide;
	}
}


void CHudArenaClassLayout::OnTick( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsInArenaMode() )
		return;

	if ( !g_TF_PR || GetLocalPlayerTeam() < FIRST_GAME_TEAM )
		return;

	// Collect all players on our team.
	int iLocalPlayerIndex = GetLocalPlayerIndex();
	int iLocalTeam = GetLocalPlayerTeam();
	int iNumClasses = 0;
	int iLocalPlayerClassImage = -1;
	bool bLayoutUpdated = false;

	for ( int i = 1; i <= gpGlobals->maxClients && iNumClasses < TF_HUD_ARENA_NUM_CLASS_IMAGES; i++ )
	{
		if ( !g_TF_PR->IsConnected( i ) )
			continue;

		// Teammates only.
		if ( g_TF_PR->GetTeam( i ) != iLocalTeam )
			continue;

		int iClass = g_TF_PR->GetPlayerClass( i );

		// Ignore players who haven't picked a class yet.
		if ( iClass == TF_CLASS_UNDEFINED )
			continue;

		if ( iClass != m_iClasses[iNumClasses] )
		{
			m_iClasses[iNumClasses] = iClass;
			bLayoutUpdated = true;
		}

		if ( i == iLocalPlayerIndex )
		{
			iLocalPlayerClassImage = iNumClasses;
		}

		iNumClasses++;
	}

	if ( iNumClasses != m_iNumClasses )
	{
		m_iNumClasses = iNumClasses;
		bLayoutUpdated = true;
	}

	if ( iLocalPlayerClassImage != m_iLocalPlayerClassImage )
	{
		m_iLocalPlayerClassImage = iLocalPlayerClassImage;
		bLayoutUpdated = true;
	}

	// Update the layout if the class layout has changed.
	if ( bLayoutUpdated )
	{
		InvalidateLayout( false );
	}
}
