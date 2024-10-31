//=============================================================================
//
// Purpose: VIP HUD
//
//=============================================================================
#include "cbase.h"
#include "tf_hud_vip.h"
#include <vgui/IVGui.h>
#include "c_tf_objective_resource.h"
#include "tf_hud_freezepanel.h"
#include "c_tf_player.h"
#include "c_tf_team.h"

using namespace vgui;

CTFHudVIP::CTFHudVIP( Panel *pParent, const char *pszName ) : EditablePanel( pParent, pszName )
{
	m_pLevelBar = new ImagePanel( this, "LevelBar" );

	m_pEscortItemPanel = new EditablePanel( this, "EscortItemPanel" );

	ivgui()->AddTickSignal( GetVPanel() );
}


void CTFHudVIP::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/HudObjectiveVIP.res" );
}


bool CTFHudVIP::IsVisible( void )
{
	if ( IsInFreezeCam() )
		return false;

	if ( ObjectiveResource() && !ObjectiveResource()->GetShouldDisplayObjectiveHUD() )
		return false;

	return BaseClass::IsVisible();
}


void CTFHudVIP::OnTick( void )
{
	// Position Civ icon based on his progress.
	float flProgress = ObjectiveResource() ? ObjectiveResource()->GetVIPProgress( TF_TEAM_BLUE ) : 0.0f;

	// Simple linear interpolation (0.1% per tick)
	const float flTrackingSpeed = 0.001f;
	if ( abs( flProgress - m_flPreviousProgress ) > flTrackingSpeed )
	{
		flProgress = m_flPreviousProgress + ( flProgress > m_flPreviousProgress ? flTrackingSpeed : -flTrackingSpeed );
	}

	m_flPreviousProgress = flProgress;

	int x, y, wide, tall, pos;
	m_pLevelBar->GetBounds( x, y, wide, tall );

	pos = (int)( wide * flProgress ) - m_pEscortItemPanel->GetWide() / 2;

	m_pEscortItemPanel->SetPos( x + pos, m_pEscortItemPanel->GetYPos() );
}
