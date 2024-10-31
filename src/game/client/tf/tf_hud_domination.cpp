//=============================================================================
//
// Purpose: Domination HUD
//
//=============================================================================
#include "cbase.h"
#include "tf_hud_domination.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include <vgui/IVGui.h>
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "tf_hud_freezepanel.h"
#include "tf_gamerules.h"
#include "c_tf_objective_resource.h"
#include "tf_announcer.h"
#include "hud_macros.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudDomination::CTFHudDomination( Panel *pParent, const char *pszName ) : EditablePanel( pParent, pszName )
{
	Reset();

#ifdef USE_POINT_DELTA_TEXT
	m_pScoreRedLabel = NULL;
	m_pScoreBlueLabel = NULL;
	m_pScoreGreenLabel = NULL;
	m_pScoreYellowLabel = NULL;
#endif

	ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "game_maploaded" );
	ListenForGameEvent( "teamplay_round_active" );
	ListenForGameEvent( "dom_score" );
}


void CTFHudDomination::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = new KeyValues( "conditions" );

	if ( TFGameRules() )
	{
		if ( TFGameRules()->IsFourTeamGame() )
		{
			if ( TFGameRules()->GetGameType() == TF_GAMETYPE_CP )
			{
				// CP gamemodes use the multiplier labels.
				AddSubKeyNamed( pConditions, "if_dom_4team" );
			}
			else
			{
				// Generic maps that just use dom as a scoring system in 4team.
				AddSubKeyNamed( pConditions, "if_generic_4team" ); 
			}
		}
		else if ( TFGameRules()->GetGameType() != TF_GAMETYPE_CP )
		{
			// Any gamemode other than CP does not use a multiplier so move the score labels to the center.
			AddSubKeyNamed( pConditions, "if_generic" );
		}
	}

	LoadControlSettings( "resource/UI/HudObjectiveDomination.res", NULL, NULL, pConditions );

	pConditions->deleteThis();

#ifdef USE_POINT_DELTA_TEXT
	m_pScoreRedLabel = dynamic_cast<vgui::Label *>( FindChildByName( "ScoreRedLabel" ) );
	m_pScoreBlueLabel = dynamic_cast<vgui::Label *>( FindChildByName( "ScoreBlueLabel" ) );
	m_pScoreGreenLabel = dynamic_cast<vgui::Label *>( FindChildByName( "ScoreGreenLabel" ) );
	m_pScoreYellowLabel = dynamic_cast<vgui::Label *>( FindChildByName( "ScoreYellowLabel" ) );
#endif

	// Initialize points
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		C_TFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		const char *pszName = g_aTeamLowerNames[iTeam];

		char szVar[16];
		V_sprintf_safe( szVar, "%sscore", pszName );
		SetDialogVariable( szVar, pTeam->GetRoundScore() );
	}
}


bool CTFHudDomination::IsVisible( void )
{
	if ( IsInFreezeCam() )
		return false;

	// Hide during waiting for players since we want to show the timer instead.
	if ( !TFGameRules() || TFGameRules()->IsInWaitingForPlayers() )
		return false;

	return BaseClass::IsVisible();
}


void CTFHudDomination::OnTick( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsInDominationMode() )
		return;

	SetDialogVariable( "rounds", TFGameRules()->GetPointLimit() );
	
	for ( int iTeam = FIRST_GAME_TEAM; iTeam < GetNumberOfTeams(); iTeam++ )
	{
		C_TFTeam *pTeam = GetGlobalTFTeam( iTeam );
		if ( !pTeam )
			continue;

		const char *pszName = g_aTeamLowerNames[iTeam];

		// Count the number of control points they own, it's the multiplier.
		if ( ObjectiveResource() )
		{
			int iCount = 0;
			for ( int iPoint = 0; iPoint < ObjectiveResource()->GetNumControlPoints(); iPoint++ )
			{
				if ( ObjectiveResource()->IsInMiniRound( iPoint ) &&
					ObjectiveResource()->GetOwningTeam( iPoint ) == iTeam &&
					!ObjectiveResource()->IsCPBlocked( iPoint ) &&
					ObjectiveResource()->GetNumPlayersInArea( iPoint, ObjectiveResource()->GetCappingTeam( iPoint ) ) <= 0 )
				{
					iCount += ObjectiveResource()->GetDominationRate( iPoint );
				}
			}

			// Don't show anything in case of x0.
			char szVar[16];
			V_sprintf_safe( szVar, "%smult", pszName );

			char szMult[8] = "";
			if ( iCount )
			{
				V_sprintf_safe( szMult, "x%d", iCount );
			}

			SetDialogVariable( szVar, szMult );
		}
	}
}


void CTFHudDomination::Announce( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsInDominationMode() )
		return;

	// Announcer lines near end of round
	int iLimit = TFGameRules()->GetPointLimit();

	// At 80% or, at very low limit, 6 points left.
	if ( m_iHighestScore >= Min<float>( iLimit * 0.8f, iLimit - 6.0f ) )
	{
		if ( m_bFire80Pct )
		{
			int iLocalTeam = GetLocalPlayerTeam();

			if ( GetGlobalTFTeam( iLocalTeam ) == m_pTeamInFirst )
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_DOM_TEAMGETTINGCLOSE );
			}
			else
			{
				g_TFAnnouncer.Speak( TF_ANNOUNCER_DOM_ENEMYGETTINGCLOSE );
			}

			m_bFire80Pct = false;
			
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudDOMLeadAlert" );
		}
		else if ( m_iHighestScore >= iLimit - 5 && m_bFire5Pts )
		{
			g_TFAnnouncer.Speak( TF_ANNOUNCER_ROUNDENDS_5SEC );
			m_bFire5Pts = false;
		}
		else if ( m_iHighestScore >= iLimit - 4 && m_bFire4Pts )
		{
			g_TFAnnouncer.Speak( TF_ANNOUNCER_ROUNDENDS_4SEC );
			m_bFire4Pts = false;
		}
		else if ( m_iHighestScore >= iLimit - 3 && m_bFire3Pts )
		{
			g_TFAnnouncer.Speak( TF_ANNOUNCER_ROUNDENDS_3SEC );
			m_bFire3Pts = false;
		}
		else if ( m_iHighestScore >= iLimit - 2 && m_bFire2Pts )
		{
			g_TFAnnouncer.Speak( TF_ANNOUNCER_ROUNDENDS_2SEC );
			m_bFire2Pts = false;
		}
		else if ( m_iHighestScore >= iLimit - 1 && m_bFire1Pts )
		{
			g_TFAnnouncer.Speak( TF_ANNOUNCER_ROUNDENDS_1SEC );
			m_bFire1Pts = false;
		}
	}
}


void CTFHudDomination::UpdateActiveTeam( int iTeam /*= TEAM_UNASSIGNED*/ )
{
	if ( iTeam < FIRST_GAME_TEAM )
		return;

	// Don't play at start.
	if ( m_pTeamInFirst != NULL )
	{
		int iLocalTeam = GetLocalPlayerTeam();

		if ( iLocalTeam == iTeam )
		{
			g_TFAnnouncer.Speak( TF_ANNOUNCER_DOM_LEADGAINED );
		}
		else if ( m_pTeamInFirst == GetGlobalTFTeam( iLocalTeam ) )
		{
			g_TFAnnouncer.Speak( TF_ANNOUNCER_DOM_LEADLOST );
		}

		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "Hud.EndRoundScored" );
	}

	C_TFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( pTeam )
	{
		m_pTeamInFirst = pTeam;

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, VarArgs( "HudDOMLead%s", g_aTeamNames[iTeam] ) );

		// We check this here as well because otherwise teams overtaking the current lead don't get the Red coloring
		if(	m_iHighestScore >= Min<float>( TFGameRules()->GetPointLimit() * 0.8f, TFGameRules()->GetPointLimit() - 6.0f ) )
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudDOMLeadAlert" );
	}
}


void CTFHudDomination::UpdateScore( int iTeam /*= TEAM_UNASSIGNED*/, int iPoints /*= 0*/, int iAmount /*= 0*/ )
{
	if ( !TFGameRules() || !TFGameRules()->IsInDominationMode() )
		return;

	if ( iTeam < FIRST_GAME_TEAM )
		return;

	C_TFTeam *pTeam = GetGlobalTFTeam( iTeam );
	if ( pTeam )
	{
		const char *pszName = g_aTeamLowerNames[iTeam];
		int iTeamScore = iPoints;

		char szVar[16];
		V_sprintf_safe( szVar, "%sscore", pszName );
		SetDialogVariable( szVar, iTeamScore );

#ifdef USE_POINT_DELTA_TEXT
		if ( iAmount > 0 || iAmount < 0 )
		{
			point_account_delta_t *pDelta = &m_AccountDeltaItems[m_AccountDeltaItems.AddToTail()];
			pDelta->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
			pDelta->m_iAmount = iAmount;

			switch ( iTeam )
			{
				case TF_TEAM_RED:
					pDelta->m_pPanel = m_pScoreRedLabel;
					break;
				case TF_TEAM_BLUE:
					pDelta->m_pPanel = m_pScoreBlueLabel;
					break;
				case TF_TEAM_GREEN:
					pDelta->m_pPanel = m_pScoreGreenLabel;
					break;
				case TF_TEAM_YELLOW:
					pDelta->m_pPanel = m_pScoreYellowLabel;
					break;
				default:
					pDelta->m_pPanel = NULL;
					break;
			}
		}
#endif

		if ( pTeam == m_pTeamInFirst )
		{
			m_iHighestScore = iTeamScore;
		}
		else if ( iTeamScore > m_iHighestScore )
		{
			UpdateActiveTeam( iTeam );
			m_iHighestScore = iTeamScore;
		}

		// Count the number if control points they own, it's the multiplier.
		if ( ObjectiveResource() )
		{
			int iCount = 0;
			for ( int iPoint = 0; iPoint < ObjectiveResource()->GetNumControlPoints(); iPoint++ )
			{
				if ( ObjectiveResource()->IsInMiniRound( iPoint ) &&
					ObjectiveResource()->GetOwningTeam( iPoint ) == iTeam &&
					ObjectiveResource()->GetNumPlayersInArea( iPoint, ObjectiveResource()->GetCappingTeam( iPoint ) ) <= 0 )
				{
					iCount += ObjectiveResource()->GetDominationRate( iPoint );
				}
			}

			// Don't show anything in case of x0.
			V_sprintf_safe( szVar, "%smult", pszName );

			char szMult[8] = "";
			if ( iCount )
			{
				V_sprintf_safe( szMult, "x%d", iCount );
			}

			SetDialogVariable( szVar, szMult );
		}

		Announce();
	}
}


void CTFHudDomination::Reset( void )
{
	// Should this really be a pointer?
	m_pTeamInFirst = NULL;
	m_iHighestScore = 0;

	m_bFire5Pts = true;
	m_bFire4Pts = true;
	m_bFire3Pts = true;
	m_bFire2Pts = true;
	m_bFire1Pts = true;
	m_bFire80Pct = true;

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudDOMLeadUnassigned" );
}

#ifdef USE_POINT_DELTA_TEXT
//-----------------------------------------------------------------------------
// Purpose: Paint the deltas
//-----------------------------------------------------------------------------
void CTFHudDomination::Paint( void )
{
	BaseClass::Paint();

	for ( int i = m_AccountDeltaItems.Count() - 1; i >= 0; i-- )
	{
		// Update all the valid delta items.
		if ( m_AccountDeltaItems[i].m_pPanel && m_AccountDeltaItems[i].m_flDieTime > gpGlobals->curtime )
		{
			// Position and alpha are determined from the lifetime
			// Color is determined by the delta - green for positive, red for negative.
			Color c = m_AccountDeltaItems[i].m_iAmount > 0 ? m_DeltaPositiveColor : Color( 255, 255, 255, 255 );

			float flLifetimePercent = ( m_AccountDeltaItems[i].m_flDieTime - gpGlobals->curtime ) / m_flDeltaLifetime;

			// Fade out after half our lifetime.
			if ( flLifetimePercent < 0.5f )
			{
				c[3] = (int)( 255.0f * ( flLifetimePercent / 0.5f ) );
			}

			int x, y;
			m_AccountDeltaItems[i].m_pPanel->GetPos( x, y );
			x += m_AccountDeltaItems[i].m_pPanel->GetWide() / 2;

			y += (int)( ( 1.0f - flLifetimePercent ) * m_flDeltaItemEndPos );

			wchar_t wBuf[16];
			if ( m_AccountDeltaItems[i].m_iAmount > 0 )
			{
				V_swprintf_safe( wBuf, L"+%d", m_AccountDeltaItems[i].m_iAmount );
			}
			else if ( m_AccountDeltaItems[i].m_iAmount < 0 )
			{
				V_swprintf_safe( wBuf, L"%d", m_AccountDeltaItems[i].m_iAmount );
			}

			// Offset x pos so the text is centered.
			x -= UTIL_ComputeStringWidth( m_hDeltaItemFont, wBuf ) / 2;

			vgui::surface()->DrawSetTextFont( m_hDeltaItemFont );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawSetTextPos( x, y );

			vgui::surface()->DrawPrintText( wBuf, wcslen( wBuf ), FONT_DRAW_NONADDITIVE );
		}
		else
		{
			// It's time for it to go peacefully.
			m_AccountDeltaItems.Remove( i );
		}
	}
}
#endif


void CTFHudDomination::FireGameEvent( IGameEvent *event )
{
	if ( V_strcmp( event->GetName(), "game_maploaded" ) == 0 )
	{
		Reset();
		InvalidateLayout( false, true );
		return;
	}
	else if ( V_strcmp( event->GetName(), "teamplay_round_active" ) == 0 )
	{
		Reset();
		return;
	}
	else if ( V_strcmp( event->GetName(), "dom_score" ) == 0 )
	{
		UpdateScore( event->GetInt( "team" ), event->GetInt( "points" ), event->GetInt( "amount" ) );
		InvalidateLayout( true, false );
		return;
	}
}
