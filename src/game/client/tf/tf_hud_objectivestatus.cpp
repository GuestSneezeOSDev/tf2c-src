//========= Copyright  1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=====================================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>

#include "c_playerresource.h"
#include "tf_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_tf_objective_resource.h"
#include "tf_hud_flagstatus.h"
#include "tf_hud_objectivestatus.h"
#include "tf_gamerules.h"
#include "tf_hud_arena_player_count.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFProgressBar );


CTFProgressBar::CTFProgressBar( vgui::Panel *parent, const char *name ) : vgui::ImagePanel( parent, name )
{
	m_flPercent = 0.0f;

	SetIcon( "hud/objectives_timepanel_progressbar" );
}

void CTFProgressBar::SetIcon( const char* szIcon )
{
	m_iTexture = vgui::surface()->DrawGetTextureId( szIcon );
	if ( m_iTexture == -1 ) // we didn't find it, so create a new one
	{
		m_iTexture = vgui::surface()->CreateNewTextureID();
	}

	vgui::surface()->DrawSetTextureFile( m_iTexture, szIcon, true, false );
}


void CTFProgressBar::Paint()
{
	int wide, tall;
	GetSize( wide, tall );

	float uv1 = 0.0f, uv2 = 1.0f;
	Vector2D uv11( uv1, uv1 );
	Vector2D uv21( uv2, uv1 );
	Vector2D uv22( uv2, uv2 );
	Vector2D uv12( uv1, uv2 );

	vgui::Vertex_t verts[4];
	verts[0].Init( Vector2D( 0, 0 ), uv11 );
	verts[1].Init( Vector2D( wide, 0 ), uv21 );
	verts[2].Init( Vector2D( wide, tall ), uv22 );
	verts[3].Init( Vector2D( 0, tall ), uv12 );

	// first, just draw the whole thing inactive.
	vgui::surface()->DrawSetTexture( m_iTexture );
	vgui::surface()->DrawSetColor( m_clrInActive );
	vgui::surface()->DrawTexturedPolygon( 4, verts );

	// now, let's calculate the "active" part of the progress bar
	if ( m_flPercent < m_flPercentWarning )
	{
		vgui::surface()->DrawSetColor( m_clrActive );
	}
	else
	{
		vgui::surface()->DrawSetColor( m_clrWarning );
	}

	// we're going to do this using quadrants
	//  -------------------------
	//  |           |           |
	//  |           |           |
	//  |     4     |     1     |
	//  |           |           |
	//  |           |           |
	//  -------------------------
	//  |           |           |
	//  |           |           |
	//  |     3     |     2     |
	//  |           |           |
	//  |           |           |
	//  -------------------------

	float flCompleteCircle = ( 2.0f * M_PI_F );
	float fl90degrees = flCompleteCircle / 4.0f;

	float flEndAngle = flCompleteCircle * ( 1.0f - m_flPercent ); // count DOWN (counter-clockwise)
	//float flEndAngle = flCompleteCircle * m_flPercent; // count UP (clockwise)

	float flHalfWide = (float)wide / 2.0f;
	float flHalfTall = (float)tall / 2.0f;

	if ( flEndAngle >= fl90degrees * 3.0f ) // >= 270 degrees
	{
		// draw the first and second quadrants
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 1.0f );
		uv12.Init( 0.5, 1.0f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, tall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, tall ), uv12 );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the third quadrant
		uv11.Init( 0.0f, 0.5f );
		uv21.Init( 0.5f, 0.5f );
		uv22.Init( 0.5f, 1.0f );
		uv12.Init( 0.0f, 1.0f );

		verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
		verts[1].Init( Vector2D( flHalfWide, flHalfTall ), uv21 );
		verts[2].Init( Vector2D( flHalfWide, tall ), uv22 );
		verts[3].Init( Vector2D( 0.0f, tall ), uv12 );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial fourth quadrant
		if ( flEndAngle > fl90degrees * 3.5f ) // > 315 degrees
		{
			uv11.Init( 0.0f, 0.0f );
			uv21.Init( 0.5f - ( tan( fl90degrees * 4.0f - flEndAngle ) * 0.5 ), 0.0f );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.0f, 0.5f );

			verts[0].Init( Vector2D( 0.0f, 0.0f ), uv11 );
			verts[1].Init( Vector2D( flHalfWide - ( tan( fl90degrees * 4.0f - flEndAngle ) * flHalfTall ), 0.0f ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 315 degrees
		{
			uv11.Init( 0.0f, 0.5f );
			uv21.Init( 0.0f, 0.5f - ( tan( flEndAngle - fl90degrees * 3.0f ) * 0.5 ) );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.0f, 0.5f );

			verts[0].Init( Vector2D( 0.0f, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( 0.0f, flHalfTall - ( tan( flEndAngle - fl90degrees * 3.0f ) * flHalfWide ) ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else if ( flEndAngle >= fl90degrees * 2.0f ) // >= 180 degrees
	{
		// draw the first and second quadrants
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 1.0f );
		uv12.Init( 0.5, 1.0f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, tall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, tall ), uv12 );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial third quadrant
		if ( flEndAngle > fl90degrees * 2.5f ) // > 225 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 0.5f, 1.0f );
			uv22.Init( 0.0f, 1.0f );
			uv12.Init( 0.0f, 0.5f + ( tan( fl90degrees * 3.0f - flEndAngle ) * 0.5 ) );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( flHalfWide, tall ), uv21 );
			verts[2].Init( Vector2D( 0.0f, tall ), uv22 );
			verts[3].Init( Vector2D( 0.0f, flHalfTall + ( tan( fl90degrees * 3.0f - flEndAngle ) * flHalfWide ) ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 225 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 0.5f, 1.0f );
			uv22.Init( 0.5f - ( tan( flEndAngle - fl90degrees * 2.0f ) * 0.5 ), 1.0f );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( flHalfWide, tall ), uv21 );
			verts[2].Init( Vector2D( flHalfWide - ( tan( flEndAngle - fl90degrees * 2.0f ) * flHalfTall ), tall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else if ( flEndAngle >= fl90degrees ) // >= 90 degrees
	{
		// draw the first quadrant
		uv11.Init( 0.5f, 0.0f );
		uv21.Init( 1.0f, 0.0f );
		uv22.Init( 1.0f, 0.5f );
		uv12.Init( 0.5f, 0.5f );

		verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
		verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
		verts[2].Init( Vector2D( wide, flHalfTall ), uv22 );
		verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

		vgui::surface()->DrawTexturedPolygon( 4, verts );

		// draw the partial second quadrant
		if ( flEndAngle > fl90degrees * 1.5f ) // > 135 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 1.0f, 0.5f );
			uv22.Init( 1.0f, 1.0f );
			uv12.Init( 0.5f + ( tan( fl90degrees * 2.0f - flEndAngle ) * 0.5f ), 1.0f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( wide, flHalfTall ), uv21 );
			verts[2].Init( Vector2D( wide, tall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide + ( tan( fl90degrees * 2.0f - flEndAngle ) * flHalfTall ), tall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 135 degrees
		{
			uv11.Init( 0.5f, 0.5f );
			uv21.Init( 1.0f, 0.5f );
			uv22.Init( 1.0f, 0.5f + ( tan( flEndAngle - fl90degrees ) * 0.5f ) );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, flHalfTall ), uv11 );
			verts[1].Init( Vector2D( wide, flHalfTall ), uv21 );
			verts[2].Init( Vector2D( wide, flHalfTall + ( tan( flEndAngle - fl90degrees ) * flHalfWide ) ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
	else // > 0 degrees
	{
		if ( flEndAngle > fl90degrees / 2.0f ) // > 45 degrees
		{
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 1.0f, 0.0f );
			uv22.Init( 1.0f, 0.5f - ( tan( fl90degrees - flEndAngle ) * 0.5 ) );
			uv12.Init( 0.5f, 0.5f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( wide, 0.0f ), uv21 );
			verts[2].Init( Vector2D( wide, flHalfTall - ( tan( fl90degrees - flEndAngle ) * flHalfWide ) ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, flHalfTall ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
		else // <= 45 degrees
		{
			uv11.Init( 0.5f, 0.0f );
			uv21.Init( 0.5 + ( tan( flEndAngle ) * 0.5 ), 0.0f );
			uv22.Init( 0.5f, 0.5f );
			uv12.Init( 0.5f, 0.0f );

			verts[0].Init( Vector2D( flHalfWide, 0.0f ), uv11 );
			verts[1].Init( Vector2D( flHalfWide + ( tan( flEndAngle ) * flHalfTall ), 0.0f ), uv21 );
			verts[2].Init( Vector2D( flHalfWide, flHalfTall ), uv22 );
			verts[3].Init( Vector2D( flHalfWide, 0.0f ), uv12 );

			vgui::surface()->DrawTexturedPolygon( 4, verts );
		}
	}
}


CTFHudTimeStatus::CTFHudTimeStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_iTeamIndex = -1;
	m_pTimeValue = new CExLabel( this, "TimePanelValue", "" );
	m_pProgressBar = NULL;
	m_pOvertimeLabel = NULL;
	m_pOvertimeBG = NULL;
	m_pSuddenDeathLabel = NULL;
	m_pSuddenDeathBG = NULL;
	m_pWaitingForPlayersBG = NULL;
	m_pWaitingForPlayersLabel = NULL;
	m_pSetupLabel = NULL;
	m_pSetupBG = NULL;
	m_pTimePanelBG = NULL;

	m_flNextThink = 0.0f;
	m_iTimerIndex = 0;
	m_bOvertime = false;

	m_iTimerDeltaHead = 0;
	for ( int i = 0; i < NUM_TIMER_DELTA_ITEMS; i++ )
	{
		m_TimerDeltaItems[i].m_flDieTime = 0.0f;
	}

	ListenForGameEvent( "teamplay_update_timer" );
	ListenForGameEvent( "teamplay_timer_time_added" );
	ListenForGameEvent( "localplayer_changeteam" );
}


void CTFHudTimeStatus::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( !Q_strcmp( pszEventName, "teamplay_update_timer" ) )
	{
		SetExtraTimePanels();
	}
	else if ( !Q_strcmp( pszEventName, "teamplay_timer_time_added" ) )
	{
		int iIndex = event->GetInt( "timer", -1 );
		int nSeconds = event->GetInt( "seconds_added", 0 );

		SetTimeAdded( iIndex, nSeconds );
	}
	else if ( !Q_strcmp( pszEventName, "localplayer_changeteam" ) )
	{
		SetTeamBackground();
	}
}


void CTFHudTimeStatus::SetTeamBackground( void )
{
	if ( !TFGameRules() )
		return;

	if ( m_pTimePanelBG )
	{
		const char *pszImage;
		int iTeamNumber = GetLocalPlayerTeam();

		if ( m_iTeamIndex > -1 )
		{
			iTeamNumber = m_iTeamIndex;
		}

		switch ( iTeamNumber )
		{
			case TF_TEAM_RED:
				pszImage = "../hud/objectives_timepanel_red_bg";
				break;
			case TF_TEAM_BLUE:
				pszImage = "../hud/objectives_timepanel_blue_bg";
				break;
			case TF_TEAM_GREEN:
				pszImage = "../hud/objectives_timepanel_green_bg";
				break;
			case TF_TEAM_YELLOW:
				pszImage = "../hud/objectives_timepanel_yellow_bg";
				break;
			default:
				pszImage = "../hud/objectives_timepanel_black_bg";
				break;
		}

		m_pTimePanelBG->SetImage( pszImage );
	}
}


void CTFHudTimeStatus::SetTimeAdded( int iIndex, int nSeconds )
{
	// Make sure this is the timer we're displaying in the HUD.
	if ( m_iTimerIndex != iIndex )
		return;

	if ( nSeconds != 0 )
	{
		// Create a delta item that floats off the top.
		timer_delta_t *pNewDeltaItem = &m_TimerDeltaItems[m_iTimerDeltaHead];
		m_iTimerDeltaHead++;
		m_iTimerDeltaHead %= NUM_TIMER_DELTA_ITEMS;
		pNewDeltaItem->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
		pNewDeltaItem->m_nAmount = nSeconds;
	}
}


void CTFHudTimeStatus::CheckClockLabelLength( CExLabel *pLabel, CTFImagePanel *pBG )
{
	if ( !pLabel || !pBG )
		return;

	int textWide, textTall;
	pLabel->GetContentSize( textWide, textTall );

	// Make sure our string isn't longer than the label it's in.
	if ( textWide > pLabel->GetWide() )
	{
		int xStart, yStart, wideStart, tallStart;
		pLabel->GetBounds( xStart, yStart, wideStart, tallStart );

		int newXPos = xStart + ( wideStart / 2.0f ) - ( textWide / 2.0f );
		pLabel->SetBounds( newXPos, yStart, textWide, tallStart );
	}

	// Turn off the background if our text label is wider than it is.
	if ( pLabel->GetWide() > pBG->GetWide() )
	{
		pBG->SetVisible( false );
	}
}


void CTFHudTimeStatus::SetExtraTimePanels()
{
	if ( !TFGameRules() )
		return;

	C_TeamRoundTimer *pTimer = dynamic_cast<C_TeamRoundTimer *>( ClientEntityList().GetEnt( m_iTimerIndex ) );
	if ( !pTimer )
		return;

	if ( m_pSetupLabel )
	{
		bool bInSetup = TFGameRules()->InSetup();
		if ( m_pSetupBG )
		{
			m_pSetupBG->SetVisible( bInSetup );
		}

		m_pSetupLabel->SetVisible( bInSetup );
	}

	// Set the Sudden Death panels to be visible.
	if ( m_pSuddenDeathLabel )
	{
		bool bInSD = TFGameRules()->InStalemate();
		if ( m_pSuddenDeathBG )
		{
			m_pSuddenDeathBG->SetVisible( bInSD );
		}

		m_pSuddenDeathLabel->SetVisible( bInSD );
	}

	if ( m_pOvertimeLabel )
	{
		if ( TFGameRules()->IsInKothMode() )
		{
			// In KOTH, CTFGameRules::m_bInOvertime is reset when the active timer changes
			// so we need to rememeber overtime status and only reset it if the time is no longer 0.
			// We also want to only show Overtime panel if the time is 0.
			if ( pTimer->GetTimeRemaining() != 0.0f )
			{
				m_bOvertime = false;
			}
			else if ( TFGameRules()->InOvertime() )
			{
				m_bOvertime = true;
			}		
		}
		else
		{
			m_bOvertime = TFGameRules()->InOvertime();
		}

		if ( m_bOvertime )
		{
			if ( m_pOvertimeBG && !m_pOvertimeBG->IsVisible() )
			{
				m_pOvertimeLabel->SetAlpha( 0 );
				m_pOvertimeBG->SetAlpha( 0 );

				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "OvertimeShow" );

				// Need to turn off the SuddenDeath images if they're on.
				if ( m_pSuddenDeathBG )
					m_pSuddenDeathBG->SetVisible( false );

				m_pSuddenDeathLabel->SetVisible( false );
			}

			if ( m_pOvertimeBG )
			{
				m_pOvertimeBG->SetVisible( true );
			}

			m_pOvertimeLabel->SetVisible( true );

			CheckClockLabelLength( m_pOvertimeLabel, m_pOvertimeBG );
		}
		else
		{
			if ( m_pOvertimeBG )
			{
				m_pOvertimeBG->SetVisible( false );
			}

			m_pOvertimeLabel->SetVisible( false );
		}
	}

	if ( m_pWaitingForPlayersLabel )
	{
		if ( TFGameRules()->IsInWaitingForPlayers() )
		{
			m_pWaitingForPlayersLabel->SetVisible( true );

			if ( m_pWaitingForPlayersBG )
			{
				m_pWaitingForPlayersBG->SetVisible( true );
			}

			// Can't be waiting for players *AND* in setup at the same time.
			if ( m_pSetupLabel )
			{
				m_pSetupLabel->SetVisible( false );
			}

			if ( m_pSetupBG )
			{
				m_pSetupBG->SetVisible( false );
			}

			CheckClockLabelLength( m_pWaitingForPlayersLabel, m_pWaitingForPlayersBG );
		}
		else
		{
			m_pWaitingForPlayersLabel->SetVisible( false );

			if ( m_pWaitingForPlayersBG )
			{
				m_pWaitingForPlayersBG->SetVisible( false );
			}
		}
	}
}


void CTFHudTimeStatus::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
	m_iTimerIndex = 0;

	m_iTimerDeltaHead = 0;
	for ( int i = 0; i < NUM_TIMER_DELTA_ITEMS; i++ )
	{
		m_TimerDeltaItems[i].m_flDieTime = 0.0f;
	}
}

void CTFHudTimeStatus::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_iBaseYPos = GetYPos();
}


void CTFHudTimeStatus::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudObjectiveTimePanel.res" );

	//m_pTimeValue = dynamic_cast<CExLabel *>(FindChildByName("TimePanelValue"));
	m_pProgressBar = dynamic_cast<CTFProgressBar *>( FindChildByName( "TimePanelProgressBar" ) );

	m_pOvertimeLabel = dynamic_cast<CExLabel *>( FindChildByName( "OvertimeLabel" ) );
	m_pOvertimeBG = dynamic_cast<CTFImagePanel *>( FindChildByName( "OvertimeBG" ) );

	m_pSuddenDeathLabel = dynamic_cast<CExLabel *>( FindChildByName( "SuddenDeathLabel" ) );
	m_pSuddenDeathBG = dynamic_cast<CTFImagePanel *>( FindChildByName( "SuddenDeathBG" ) );

	m_pWaitingForPlayersLabel = dynamic_cast<CExLabel *>( FindChildByName( "WaitingForPlayersLabel" ) );
	m_pWaitingForPlayersBG = dynamic_cast<CTFImagePanel *>( FindChildByName( "WaitingForPlayersBG" ) );

	m_pSetupLabel = dynamic_cast<CExLabel *>( FindChildByName( "SetupLabel" ) );
	m_pSetupBG = dynamic_cast<CTFImagePanel *>( FindChildByName( "SetupBG" ) );

	m_pTimePanelBG = dynamic_cast<ScalableImagePanel *>( FindChildByName( "TimePanelBG" ) );

	m_flNextThink = 0.0f;
	m_iTimerIndex = 0;

	SetTeamBackground();
	SetExtraTimePanels();

	BaseClass::ApplySchemeSettings( pScheme );
}


void CTFHudTimeStatus::OnThink()
{
	CHudArenaPlayerCount *pPlayerCount = GET_HUDELEMENT( CHudArenaPlayerCount );
	CTFHudObjectiveStatus *pObjective = GET_HUDELEMENT( CTFHudObjectiveStatus );

	// We use "ShouldDraw" instead of "IsVisible" here because the player count was setup to use that function instead - Kay
	if( pPlayerCount && pPlayerCount->ShouldDraw() )
	{
		int x, y, wide, tall;
		pPlayerCount->GetBounds( x, y, wide, tall );

		SetPos( GetXPos(), y + tall + m_iTimerOffset );
	}
	// We assume the domination panel exists here because its created in the Objective Status Constructor
	// If for some reason a crash occurs here, I'd first look into how the domination panel was even destroyed
	// rather than adding another null pointer check - Kay
	else if( pObjective && pObjective->m_pDominationPanel->IsVisible() )
	{
		int x, y, wide, tall;
		pObjective->m_pDominationPanel->GetBounds(x, y, wide, tall);

		SetPos( GetXPos(), y + tall + m_iTimerOffset );
	}
	// If we don't need to move down, Wake up, get up, get out there 
	else
	{
		SetPos( GetXPos(), m_iBaseYPos );
	}

	if ( m_flNextThink < gpGlobals->curtime )
	{
		CTeamRoundTimer *pTimer = dynamic_cast<CTeamRoundTimer*>( ClientEntityList().GetEnt( m_iTimerIndex ) );
		// get the time remaining (in seconds)
		if ( pTimer )
		{
			int nTotalTime = pTimer->GetTimerMaxLength();
			int nTimeRemaining = (int)ceil( pTimer->GetTimeRemaining() );

			// Always show 0 if we're in Overtime, workaround for rounding issues, etc.
			if ( m_bOvertime )
			{
				nTimeRemaining = 0;
			}

			if ( !pTimer->ShowTimeRemaining() )
			{
				nTimeRemaining = nTotalTime - nTimeRemaining;
			}

			if ( m_pTimeValue && m_pTimeValue->IsVisible() )
			{
				// set our label
				int nMinutes = 0;
				int nSeconds = 0;
				char temp[256];

				if ( nTimeRemaining <= 0 )
				{
					nMinutes = 0;
					nSeconds = 0;
				}
				else
				{
					nMinutes = nTimeRemaining / 60;
					nSeconds = nTimeRemaining % 60;
				}
				V_sprintf_safe( temp, "%d:%02d", nMinutes, nSeconds );
				m_pTimeValue->SetText( temp );
			}
			// let the progress bar know the percentage of time that's passed ( 0.0 -> 1.0 )
			if ( m_pProgressBar && m_pProgressBar->IsVisible() )
			{
				m_pProgressBar->SetPercentage( ( (float)nTotalTime - nTimeRemaining ) / (float)nTotalTime );
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}

	// Push the timer down below the player counts in Arena mode.
	/*CHudArenaPlayerCount *pPlayerCount = GET_HUDELEMENT( CHudArenaPlayerCount );

	if ( pPlayerCount && pPlayerCount->IsVisible() )
	{
		int x, y, wide, tall;
		pPlayerCount->GetBounds( x, y, wide, tall );
		SetPos( GetXPos(), y + tall - YRES( 12 ) );
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Paint the deltas
//-----------------------------------------------------------------------------
void CTFHudTimeStatus::Paint( void )
{
	BaseClass::Paint();

	for ( int i = 0; i < NUM_TIMER_DELTA_ITEMS; i++ )
	{
		// update all the valid delta items
		if ( m_TimerDeltaItems[i].m_flDieTime > gpGlobals->curtime )
		{
			// position and alpha are determined from the lifetime
			// color is determined by the delta - green for positive, red for negative

			Color c = ( m_TimerDeltaItems[i].m_nAmount > 0 ) ? m_DeltaPositiveColor : m_DeltaNegativeColor;

			float flLifetimePercent = ( m_TimerDeltaItems[i].m_flDieTime - gpGlobals->curtime ) / m_flDeltaLifetime;

			// fade out after half our lifetime
			if ( flLifetimePercent < 0.5 )
			{
				c[3] = (int)( 255.0f * ( flLifetimePercent / 0.5 ) );
			}

			float flHeight = ( m_flDeltaItemStartPos - m_flDeltaItemEndPos );
			float flYPos = m_flDeltaItemEndPos + flLifetimePercent * flHeight;

			vgui::surface()->DrawSetTextFont( m_hDeltaItemFont );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawSetTextPos( m_flDeltaItemX, (int)flYPos );

			wchar_t wBuf[20];
			int nMinutes, nSeconds;
			int nClockTime = ( m_TimerDeltaItems[i].m_nAmount > 0 ) ? m_TimerDeltaItems[i].m_nAmount : ( m_TimerDeltaItems[i].m_nAmount * -1 );
			nMinutes = nClockTime / 60;
			nSeconds = nClockTime % 60;

			if ( m_TimerDeltaItems[i].m_nAmount > 0 )
			{
				V_swprintf_safe( wBuf, L"+%d:%02d", nMinutes, nSeconds );
			}
			else
			{
				V_swprintf_safe( wBuf, L"-%d:%02d", nMinutes, nSeconds );
			}

			vgui::surface()->DrawPrintText( wBuf, wcslen( wBuf ), FONT_DRAW_NONADDITIVE );
		}
	}
}


DECLARE_HUDELEMENT( CTFHudObjectiveStatus );


CTFHudObjectiveStatus::CTFHudObjectiveStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudObjectiveStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pFlagPanel = new CTFHudFlagObjectives( this, "ObjectiveStatusFlagPanel" );
	m_pControlPointIconsPanel = NULL;
	m_pControlPointProgressBar = new CControlPointProgressBar( this );
	m_pEscortPanel = new CTFHudEscort( this, "ObjectiveStatusEscort", TF_TEAM_BLUE, false );
	m_pEscortRacePanel = new CTFHudMultipleEscort( this, "ObjectiveStatusMultipleEscort" );
	//m_pTrainingPanel = new CTFHudTraining( this, "ObjectiveStatusTraining" );
	//m_pRobotDestructionPanel = new CTFHUDRobotDestruction( this, "ObjectiveStatusRobotDestruction" );
	m_pVIPPanel = new CTFHudVIP( this, "ObjectiveStatusVIP" );
	m_pDominationPanel = new CTFHudDomination( this, "ObjectiveStatusDomination" );
	m_pPowerSiegePanel = new CTFHudPowerSiege( this, "ObjectiveStatusPowerSiege" );

	SetHiddenBits( 0 );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
}


void CTFHudObjectiveStatus::ApplySchemeSettings( IScheme *pScheme )
{
	LoadControlSettings( "resource/UI/HudObjectiveStatus.res" );

	if ( !m_pControlPointIconsPanel )
	{
		m_pControlPointIconsPanel = GET_HUDELEMENT( CHudControlPointIcons );
		m_pControlPointIconsPanel->SetParent( this );
	}

	m_pControlPointProgressBar->InvalidateLayout( true, true );

	BaseClass::ApplySchemeSettings( pScheme );
}


void CTFHudObjectiveStatus::Reset()
{
	m_pFlagPanel->Reset();
	m_pEscortPanel->Reset();
	m_pEscortRacePanel->Reset();
}


CControlPointProgressBar *CTFHudObjectiveStatus::GetControlPointProgressBar( void )
{
	return m_pControlPointProgressBar;
}


void CTFHudObjectiveStatus::SetVisiblePanels( void )
{
	if ( !TFGameRules() )
		return;

	TurnOffPanels();

	int iGameType = TFGameRules()->GetGameType();
	if ( TFGameRules()->GetHudType() )
	{
		iGameType = TFGameRules()->GetHudType();
	}

	switch ( iGameType )
	{
		case TF_GAMETYPE_CTF:
			// turn on the flag panel
			m_pFlagPanel->SetVisible( true );

			if ( TFGameRules()->IsInHybridCTF_CPMode() )
			{
				if ( m_pControlPointIconsPanel )
				{
					m_pControlPointIconsPanel->SetVisible( true );
				}
			}
			break;

		case TF_GAMETYPE_ARENA:
		case TF_GAMETYPE_CP:
			// turn on the control point icons
			if ( m_pControlPointIconsPanel )
			{
				m_pControlPointIconsPanel->SetVisible( true );
			}

			if ( TFGameRules()->IsInHybridCTF_CPMode() )
			{
				// turn on the flag panel
				m_pFlagPanel->SetVisible( true );
			}

			if ( TFGameRules()->IsInDominationMode() )
			{
				m_pDominationPanel->SetVisible( true );
			}
			break;

		case TF_GAMETYPE_ESCORT:
			// turn on the payload panel
			if ( TFGameRules()->HasMultipleTrains() )
			{
				m_pEscortRacePanel->SetVisible( true );
			}
			else
			{
				m_pEscortPanel->SetVisible( true );
			}
			break;

		case TF_GAMETYPE_VIP:
			m_pVIPPanel->SetVisible( true );

			if ( m_pControlPointIconsPanel )
			{
				m_pControlPointIconsPanel->SetVisible( true );
			}
			if ( TFGameRules()->IsInHybridCTF_CPMode() )
			{
				// turn on the flag panel
				m_pFlagPanel->SetVisible( true );
			}
	
			break;
		case TF_GAMETYPE_POWERSIEGE:
			m_pPowerSiegePanel->SetVisible( true );
			break;
		case TF_GAMETYPE_INFILTRATION:
			// We are using the domination hud for now, we will replace this with it's own hud class later on.
			m_pDominationPanel->SetVisible( true );
			break;
	}
}


void CTFHudObjectiveStatus::TurnOffPanels()
{
	// turn off the flag panel
	m_pFlagPanel->SetVisible( false );

	// turn off the control point icons
	m_pControlPointIconsPanel->SetVisible( false );

	m_pEscortPanel->SetVisible( false );
	m_pEscortRacePanel->SetVisible( false );

	m_pDominationPanel->SetVisible( false );

	m_pVIPPanel->SetVisible( false );

	m_pPowerSiegePanel->SetVisible( false );
}


void CTFHudObjectiveStatus::Think()
{
	SetVisiblePanels();
}

DECLARE_HUDELEMENT( CTFHudKothTimeStatus );


CTFHudKothTimeStatus::CTFHudKothTimeStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudKothTimeStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pActiveKothTimerPanel = NULL;
	m_pBlueKothTimer = new CTFHudTimeStatus( this, "BlueTimer" );
	m_pBlueKothTimer->m_iTeamIndex = TF_TEAM_BLUE;
	m_pRedKothTimer = new CTFHudTimeStatus( this, "RedTimer" );
	m_pRedKothTimer->m_iTeamIndex = TF_TEAM_RED;
	m_pGreenKothTimer = new CTFHudTimeStatus( this, "GreenTimer" );
	m_pGreenKothTimer->m_iTeamIndex = TF_TEAM_GREEN;
	m_pYellowKothTimer = new CTFHudTimeStatus( this, "YellowTimer" );
	m_pYellowKothTimer->m_iTeamIndex = TF_TEAM_YELLOW;

	m_pActiveTimerBG = new ImagePanel( this, "ActiveTimerBG" );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );
	
	ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "game_maploaded" );
}


void CTFHudKothTimeStatus::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = new KeyValues( "conditions" );

	if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
	{
		AddSubKeyNamed( pConditions, "if_fourteams" );
	}

	LoadControlSettings( "resource/UI/HudObjectiveKothTimePanel.res", NULL, NULL, pConditions );
}


void CTFHudKothTimeStatus::Reset( void )
{
	for ( int i = FIRST_GAME_TEAM; i < TF_TEAM_COUNT; i++ )
	{
		CTFHudTimeStatus *pTimerStatus = GetTimer( i );
		if ( pTimerStatus )
		{
			pTimerStatus->Reset();
		}
	}

	UpdateActiveTeam();
}


bool CTFHudKothTimeStatus::ShouldDraw( void )
{
	if ( !TFGameRules() )
		return false;

	if ( !TFGameRules()->IsInKothMode() )
		return false;

	if ( TFGameRules()->IsInWaitingForPlayers() )
		return false;

	if ( IsInFreezeCam() )
		return false;

	return CHudElement::ShouldDraw();
}


void CTFHudKothTimeStatus::UpdateActiveTeam( void )
{
	if ( m_pActiveTimerBG )
	{
		if ( m_pActiveKothTimerPanel )
		{
			m_pActiveTimerBG->SetVisible( true );

			int x, y;
			m_pActiveTimerBG->GetPos( x, y );

			if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
			{
				switch ( m_pActiveKothTimerPanel->m_iTeamIndex )
				{
					case TF_TEAM_RED:
						x = m_n4RedActiveXPos;
						break;
					case TF_TEAM_BLUE:
						x = m_n4BlueActiveXPos;
						break;
					case TF_TEAM_GREEN:
						x = m_n4GreenActiveXPos;
						break;
					case TF_TEAM_YELLOW:
						x = m_n4YellowActiveXPos;
						break;
				}
			}
			else
			{
				switch ( m_pActiveKothTimerPanel->m_iTeamIndex )
				{
					case TF_TEAM_RED:
						x = m_nRedActiveXPos;
						break;
					case TF_TEAM_BLUE:
						x = m_nBlueActiveXPos;
						break;
				}
			}

			m_pActiveTimerBG->SetPos( x, y );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "ActiveTimerBGPulse" );
		}
		else
		{
			m_pActiveTimerBG->SetVisible( false );
		}
	}
}



void CTFHudKothTimeStatus::OnTick( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsInKothMode() )
		return;

	CTFHudTimeStatus *pActiveKothTimerPanel = NULL;

	for ( int i = FIRST_GAME_TEAM; i < TF_TEAM_COUNT; i++ )
	{
		CTFHudTimeStatus *pTimerStatus = GetTimer( i );
		if ( !pTimerStatus )
			continue;

		bool bShowTimer = false;

		// Make sure the time panel still pointing at the proper timer.
		C_TeamRoundTimer *pTimer = TFGameRules()->GetKothTimer( i );

		// Check for the current active timer (used for the pulsating HUD animation)
		if ( pTimer && !pTimer->IsDormant() && !pTimer->IsDisabled() )
		{
			pTimerStatus->SetTimerIndex( pTimer->index );

			// the current timer is fine, make sure the panel is visible
			bShowTimer = true;

			if ( !pTimer->IsTimerPaused() )
				pActiveKothTimerPanel = pTimerStatus;
		}

		if ( pTimerStatus->IsVisible() != bShowTimer )
		{
			pTimerStatus->SetVisible( bShowTimer );
		}
	}

	// Set overtime panels active on our active panel (if needed)
	if ( m_pActiveKothTimerPanel )
		m_pActiveKothTimerPanel->SetExtraTimePanels();

	// Do NOT put a null check here, otherwise the white active timer BG will linger around after a round end
	if ( pActiveKothTimerPanel != m_pActiveKothTimerPanel )
	{
		m_pActiveKothTimerPanel = pActiveKothTimerPanel;
		UpdateActiveTeam();
	}
}


void CTFHudKothTimeStatus::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "game_maploaded" ) == 0 )
	{
		InvalidateLayout( false, true );
	}
}


CTFHudTimeStatus *CTFHudKothTimeStatus::GetTimer( int iTeam )
{
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			return m_pRedKothTimer;

		case TF_TEAM_BLUE:
			return m_pBlueKothTimer;

		case TF_TEAM_GREEN:
			return m_pGreenKothTimer;

		case TF_TEAM_YELLOW:
			return m_pYellowKothTimer;
	}

	return NULL;
}
