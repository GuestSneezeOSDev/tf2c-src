//========= Copyright © 1996-2007, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

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
#include <vgui/IVGui.h>
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>
#include "view.h"

#include "c_playerresource.h"
#include "tf_round_timer.h"
#include "utlvector.h"
#include "entity_capture_flag.h"
#include "c_tf_player.h"
#include "c_team.h"
#include "c_tf_team.h"
#include "c_tf_objective_resource.h"
#include "tf_hud_objectivestatus.h"
#include "tf_spectatorgui.h"
#include "tf_gamerules.h"
#include "tf_hud_freezepanel.h"
#include "c_func_capture_zone.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFArrowPanel );
DECLARE_BUILD_FACTORY( CTFFlagStatus );

extern ConVar tf_flag_caps_per_round;


CTFArrowPanel::CTFArrowPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_RedMaterial.Init( "hud/objectives_flagpanel_compass_red", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterial.Init( "hud/objectives_flagpanel_compass_blue", TEXTURE_GROUP_VGUI ); 
	m_GreenMaterial.Init( "hud/objectives_flagpanel_compass_green", TEXTURE_GROUP_VGUI );
	m_YellowMaterial.Init( "hud/objectives_flagpanel_compass_yellow", TEXTURE_GROUP_VGUI );
	m_NeutralMaterial.Init( "hud/objectives_flagpanel_compass_grey", TEXTURE_GROUP_VGUI ); 

	m_RedMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_red_noArrow", TEXTURE_GROUP_VGUI ); 
	m_BlueMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_blue_noArrow", TEXTURE_GROUP_VGUI ); 
	m_GreenMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_green_noArrow", TEXTURE_GROUP_VGUI );
	m_YellowMaterialNoArrow.Init( "hud/objectives_flagpanel_compass_yellow_noArrow", TEXTURE_GROUP_VGUI );

	m_pMaterial = m_NeutralMaterial;

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}


float CTFArrowPanel::GetAngleRotation( void )
{
	float flRetVal = 0.0f;

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	C_BaseEntity *pEnt = m_hEntity.Get();
	if ( pPlayer && pEnt )
	{
		QAngle vangles = MainViewAngles();
		Vector eyeOrigin = MainViewOrigin();

		Vector vecFlag = pEnt->WorldSpaceCenter() - eyeOrigin;
		vecFlag.z = 0;
		vecFlag.NormalizeInPlace();

		Vector forward, right, up;
		AngleVectors( vangles, &forward, &right, &up );
		forward.z = 0;
		right.z = 0;
		forward.NormalizeInPlace();
		right.NormalizeInPlace();

		float dot = DotProduct( vecFlag, forward );
		float angleBetween = acos( clamp( dot, -1.0f, 1.0f ) );

		dot = DotProduct( vecFlag, right );
		if ( dot < 0.0f )
		{
			angleBetween *= -1;
		}

		flRetVal = RAD2DEG( angleBetween );
	}

	return flRetVal;
}


void CTFArrowPanel::OnTick( void )
{
	if ( !m_hEntity.Get() )
		return;

	C_TFPlayer *pPlayer = GetLocalObservedPlayer( false );
	if ( !pPlayer )
		return;

	C_BaseEntity *pEnt = m_hEntity.Get();
	m_pMaterial = m_NeutralMaterial;
	
	CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( m_hEntity.Get() );
	if ( pFlag && pFlag->GetGameType() == TF_FLAGTYPE_VIP )
	{
		// Don't draw the arrow panel if the player is on a different team than the flag and the flag is using the VIP gametype or if the flag is disabled.
		if ( pPlayer->GetTeamNumber() != pFlag->GetTeamNumber() || pFlag->IsDisabled() )
			return;
	}

	// Figure out what material we need to use.
	// Don't draw the arrow if watching the player who's carrying the flag.
	bool bCarried = ( pPlayer->GetTheFlag() == pEnt );

	switch ( pEnt->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			m_pMaterial = bCarried ? m_RedMaterialNoArrow : m_RedMaterial;
			break;
		case TF_TEAM_BLUE:
			m_pMaterial = bCarried ? m_BlueMaterialNoArrow : m_BlueMaterial;
			break;
		case TF_TEAM_GREEN:
			m_pMaterial = bCarried ? m_GreenMaterialNoArrow : m_GreenMaterial;
			break;
		case TF_TEAM_YELLOW:
			m_pMaterial = bCarried ? m_YellowMaterialNoArrow : m_YellowMaterial;
			break;
	}
}


void CTFArrowPanel::Paint()
{
	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos( GetVPanel(), x, y );

	int nWidth = GetWide();
	int nHeight = GetTall();

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->MatrixMode( MATERIAL_MODEL );
	pRenderContext->PushMatrix(); 

	VMatrix panelRotation;
	panelRotation.Identity();
	MatrixBuildRotationAboutAxis( panelRotation, Vector( 0, 0, 1 ), GetAngleRotation() );
	//MatrixRotate( panelRotation, Vector( 1, 0, 0 ), 5 );
	panelRotation.SetTranslation( Vector( x + nWidth / 2, y + nHeight / 2, 0 ) );
	pRenderContext->LoadMatrix( panelRotation );

	IMesh *pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_pMaterial );

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );
		meshBuilder.TexCoord2f( 0, 0, 0 );
		meshBuilder.Position3f( -nWidth / 2, -nHeight / 2, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.TexCoord2f( 0, 1, 0 );
		meshBuilder.Position3f( nWidth / 2, -nHeight / 2, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.TexCoord2f( 0, 1, 1 );
		meshBuilder.Position3f( nWidth / 2, nHeight / 2, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();

		meshBuilder.TexCoord2f( 0, 0, 1 );
		meshBuilder.Position3f( -nWidth / 2, nHeight / 2, 0 );
		meshBuilder.Color4ub( 255, 255, 255, 255 );
		meshBuilder.AdvanceVertex();
	meshBuilder.End();

	pMesh->Draw();
	pRenderContext->PopMatrix();
}


bool CTFArrowPanel::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}


CTFFlagStatus::CTFFlagStatus( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pArrow = NULL;
	m_pStatusIcon = NULL;
	m_pBriefcase = NULL;
	m_hEntity = NULL;
	m_iNumFlags = 0;
}


void CTFFlagStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = new KeyValues( "conditions" );

	if ( TFGameRules() )
	{
		m_iNumFlags = CCaptureFlag::AutoList().Count();
		if ( m_iNumFlags == 1 )
		{
			for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
			{
				if ( pFlag->GetGameType() == TF_FLAGTYPE_VIP )
				{
					AddSubKeyNamed( pConditions, "if_vip" );
				}

				break;
			}
		}
	}

	// Load control settings...
	LoadControlSettings( "resource/UI/FlagStatus.res" , NULL, NULL, pConditions );

	m_pArrow = dynamic_cast<CTFArrowPanel *>( FindChildByName( "Arrow" ) );
	m_pStatusIcon = dynamic_cast<CTFImagePanel *>( FindChildByName( "StatusIcon" ) );
	m_pBriefcase = dynamic_cast<CTFImagePanel *>( FindChildByName( "Briefcase" ) );
}


bool CTFFlagStatus::IsVisible( void )
{
	if ( !m_hEntity.Get() )
		return false;

	if ( IsTakingAFreezecamScreenshot() )
		return false;

	// Don't draw the arrow panel for flags our team isn't allowed to pick up (again).
	if ( m_hEntity.Get() )
	{
		CCaptureFlag* pFlag = dynamic_cast<CCaptureFlag*>( m_hEntity.Get() );
		if ( pFlag && !pFlag->GetMyTeamCanPickup() )
		{
			return false;
		}
	}

	return BaseClass::IsVisible();
}


void CTFFlagStatus::UpdateStatus( void )
{
	if ( m_hEntity.Get() )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag *>( m_hEntity.Get() );
		if ( pFlag )
		{
			if ( pFlag->GetGameType() == TF_FLAGTYPE_VIP ) 
			{
				C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
				if ( !pPlayer )
					return;

				SetVisible( !( pFlag->GetTeamNumber() != pPlayer->GetTeamNumber() || pFlag->IsDisabled() ) );
			}
			
			const char *pszImage;
			if ( pFlag->IsDropped() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_dropped";
			}
			else if ( pFlag->IsStolen() )
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_moving";
			}
			else
			{
				pszImage = "../hud/objectives_flagpanel_ico_flag_home";
			}

			if ( m_pStatusIcon )
			{
				m_pStatusIcon->SetImage( pszImage );
			}
		}
	}
}


CTFHudFlagObjectives::CTFHudFlagObjectives( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pCarriedImage = NULL;
	m_pPlayingTo = NULL;
	m_bFlagAnimationPlayed = false;
	m_bCarryingFlag = false;
	m_pSpecCarriedImage = NULL;
	m_bShowPlayingTo = false;
	m_iNumFlags = 0;

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	ListenForGameEvent( "game_maploaded" );
}


bool CTFHudFlagObjectives::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}


void CTFHudFlagObjectives::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = new KeyValues( "conditions" );

	if ( TFGameRules() )
	{
		m_bShowPlayingTo = true;

		m_iNumFlags = CCaptureFlag::AutoList().Count();
		if ( m_iNumFlags == 0 )
		{
			AddSubKeyNamed( pConditions, "if_no_flags" );	
		}
		else if ( TFGameRules()->IsInHybridCTF_CPMode() )
		{
			// Hybrid CTF/CP, can either have 1 or 2 flags.
			AddSubKeyNamed( pConditions, "if_hybrid" );

			if ( m_iNumFlags == 1 )
			{
				AddSubKeyNamed( pConditions, "if_hybrid_single" );
				
				for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
				{
					if ( pFlag->GetGameType() == TF_FLAGTYPE_VIP )
					{
						AddSubKeyNamed( pConditions, "if_vip" );
					}
					break;
				}
			}
			else
			{
				AddSubKeyNamed( pConditions, "if_hybrid_double" );
			}

			m_bShowPlayingTo = false;
		}
		else if ( TFGameRules()->IsInSpecialDeliveryMode() )
		{
			AddSubKeyNamed( pConditions, "if_specialdelivery" );
			AddSubKeyNamed( pConditions, "if_hybrid_single" );

			m_bShowPlayingTo = false;
		}
		else if ( TFGameRules()->IsMannVsMachineMode() )
		{
			AddSubKeyNamed( pConditions, "if_mvm" );
			AddSubKeyNamed( pConditions, "if_hybrid_single" );

			m_bShowPlayingTo = false;
		}
		else if ( m_iNumFlags == 1 )
		{
			// Normal CTF HUD but with one flag.
			AddSubKeyNamed( pConditions, "if_hybrid_single" );

			for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
			{
				if ( pFlag->GetGameType() == TF_FLAGTYPE_INVADE && pFlag->GetScoringType() == C_CaptureFlag::TF_FLAGSCORING_SCORE )
				{
					m_bShowPlayingTo = false;
				}
				// If only one flag is used in a vip map without any other logic such as control points.
				else if ( pFlag->GetGameType() == TF_FLAGTYPE_VIP )
				{
					AddSubKeyNamed( pConditions, "if_hybrid" );
					AddSubKeyNamed( pConditions, "if_vip" );

					m_bShowPlayingTo = false;
				}

				break;
			}
		}
		
	}

	// Load control settings...
	if ( TFGameRules() && TFGameRules()->IsFourTeamGame() )
	{
		LoadControlSettings( "resource/UI/HudObjectiveFourTeamFlagPanel.res", NULL, NULL, pConditions );
	}
	else 
	{
		LoadControlSettings( "resource/UI/HudObjectiveFlagPanel.res", NULL, NULL, pConditions );
	}

	pConditions->deleteThis();

	m_pCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "CarriedImage" ) );
	m_pPlayingTo = dynamic_cast<CExLabel *>( FindChildByName( "PlayingTo" ) );
	m_pPlayingToBG = dynamic_cast<CTFImagePanel *>( FindChildByName( "PlayingToBG" ) );
	
	m_pRedFlag = dynamic_cast<CTFFlagStatus *>( FindChildByName( "RedFlag" ) );
	m_pBlueFlag = dynamic_cast<CTFFlagStatus *>( FindChildByName( "BlueFlag" ) );
	m_pGreenFlag = dynamic_cast<CTFFlagStatus *>( FindChildByName( "GreenFlag" ) );
	m_pYellowFlag = dynamic_cast<CTFFlagStatus *>( FindChildByName( "YellowFlag" ) );

	m_pCapturePoint = dynamic_cast<CTFArrowPanel *>( FindChildByName( "CaptureFlag" ) );

	m_pSpecCarriedImage = dynamic_cast<ImagePanel *>( FindChildByName( "SpecCarriedImage" ) );

	// Outline is always on, so we need to init the alpha to 0.
	CTFImagePanel *pOutline = dynamic_cast<CTFImagePanel *>( FindChildByName( "OutlineImage" ) );
	if ( pOutline )
	{
		pOutline->SetAlpha( 0 );
	}
}


void CTFHudFlagObjectives::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );

	if ( m_pCarriedImage )
	{
		m_pCarriedImage->SetVisible( false );
	}

	if ( m_pBlueFlag )
	{
		m_pBlueFlag->SetVisible( true );
	}

	if ( m_pRedFlag )
	{
		m_pRedFlag->SetVisible( true );
	}

	if ( m_pGreenFlag )
	{
		m_pGreenFlag->SetVisible( true );
	}

	if ( m_pYellowFlag )
	{
		m_pYellowFlag->SetVisible( true );
	}

	if ( m_pSpecCarriedImage )
	{
		m_pSpecCarriedImage->SetVisible( false );
	}
}


void CTFHudFlagObjectives::SetPlayingToLabelVisible( bool bVisible )
{
	if ( m_pPlayingTo && m_pPlayingToBG )
	{
		m_pPlayingTo->SetVisible( bVisible );
		m_pPlayingToBG->SetVisible( bVisible );
	}
}


void CTFHudFlagObjectives::OnTick()
{
	if ( !TFGameRules() )
		return;

	if ( m_iNumFlags == 0 )
	{
		if ( m_pRedFlag )
		{
			m_pRedFlag->SetEntity( NULL );
		}

		if ( m_pBlueFlag )
		{
			m_pBlueFlag->SetEntity( NULL );
		}

		if ( m_pGreenFlag )
		{
			m_pGreenFlag->SetEntity( NULL );
		}

		if ( m_pYellowFlag )
		{
			m_pYellowFlag->SetEntity( NULL );
		}
	}
	else if ( m_iNumFlags == 1 )
	{
		// If there's only one flag, just set the red panel up to track it.
		for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
		{
			if ( m_pRedFlag )
			{
				m_pRedFlag->SetEntity( pFlag );
			}
			break;
		}

		if ( m_pBlueFlag )
		{
			m_pBlueFlag->SetEntity( NULL );
		}

		if ( m_pGreenFlag )
		{
			m_pGreenFlag->SetEntity( NULL );
		}

		if ( m_pYellowFlag )
		{
			m_pYellowFlag->SetEntity( NULL );
		}
	}
	else
	{
		// Iterate through the flags to set their position in our HUD.
		for ( CCaptureFlag *pFlag : CCaptureFlag::AutoList() )
		{
			if ( !pFlag->GetMyTeamCanPickup() )
				continue;

			if ( m_pRedFlag && pFlag->GetTeamNumber() == TF_TEAM_RED )
			{
				m_pRedFlag->SetEntity( pFlag );
			}
			else if ( m_pBlueFlag && pFlag->GetTeamNumber() == TF_TEAM_BLUE )
			{
				m_pBlueFlag->SetEntity( pFlag );
			}
			else if ( m_pGreenFlag && pFlag->GetTeamNumber() == TF_TEAM_GREEN )
			{
				m_pGreenFlag->SetEntity( pFlag );
			}
			else if ( m_pYellowFlag && pFlag->GetTeamNumber() == TF_TEAM_YELLOW )
			{
				m_pYellowFlag->SetEntity( pFlag );
			}
		}
	}

	if ( m_bShowPlayingTo )
	{
		// Are we playing captures for rounds?
		if ( tf_flag_caps_per_round.GetInt() > 0 )
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pTeam )
			{
				SetDialogVariable( "bluescore", pTeam->GetFlagCaptures() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pTeam )
			{
				SetDialogVariable( "redscore", pTeam->GetFlagCaptures() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
			if ( pTeam )
			{
				SetDialogVariable( "greenscore", pTeam->GetFlagCaptures() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
			if ( pTeam )
			{
				SetDialogVariable( "yellowscore", pTeam->GetFlagCaptures() );
			}

			SetPlayingToLabelVisible( true );
			SetDialogVariable( "rounds", tf_flag_caps_per_round.GetInt() );
		}
		// We're just playing straight score.
		else
		{
			C_TFTeam *pTeam = GetGlobalTFTeam( TF_TEAM_BLUE );
			if ( pTeam )
			{
				SetDialogVariable( "bluescore", pTeam->Get_Score() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_RED );
			if ( pTeam )
			{
				SetDialogVariable( "redscore", pTeam->Get_Score() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_GREEN );
			if ( pTeam )
			{
				SetDialogVariable( "greenscore", pTeam->Get_Score() );
			}

			pTeam = GetGlobalTFTeam( TF_TEAM_YELLOW );
			if ( pTeam )
			{
				SetDialogVariable( "yellowscore", pTeam->Get_Score() );
			}

			SetPlayingToLabelVisible( false );
		}
	}

	// Update the carried flag status.
	UpdateStatus();

	// Check the local player to see if they're spectating, OBS_MODE_IN_EYE, and the target entity is carrying the flag.
	bool bSpecCarriedImage = false;
	C_TFPlayer *pPlayer = GetLocalObservedPlayer( true );
	if ( pPlayer && !pPlayer->IsLocalPlayer() )
	{
		CCaptureFlag *pPlayerFlag = pPlayer->GetTheFlag();
		if ( pPlayerFlag )
		{
			bSpecCarriedImage = true;

			if ( m_pSpecCarriedImage )
			{
				char szImage[MAX_PATH];
				pPlayerFlag->GetHudIcon( pPlayerFlag->GetTeamNumber(), szImage, MAX_PATH );
				m_pSpecCarriedImage->SetImage( szImage );
			}
		}
	}

	if ( bSpecCarriedImage )
	{
		if ( m_pSpecCarriedImage )
		{
			m_pSpecCarriedImage->SetVisible( true );
		}
	}
	else if ( m_pSpecCarriedImage )
	{
		m_pSpecCarriedImage->SetVisible( false );
	}
}


void CTFHudFlagObjectives::UpdateStatus( void )
{
	// Are we carrying a flag?
	C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	CCaptureFlag *pPlayerFlag = NULL;
	if ( pLocalPlayer )
	{
		pPlayerFlag = pLocalPlayer->GetTheFlag();
	}

	if ( pPlayerFlag )
	{
		m_bCarryingFlag = true;

		// Make sure the panels are on, set the initial alpha values, 
		// set the color of the flag we're carrying, and start the animations.
		if ( m_pCarriedImage && !m_bFlagAnimationPlayed )
		{
			m_bFlagAnimationPlayed = true;

			// Set the correct flag image depending on the flag we're holding.
			char szImage[MAX_PATH];
			pPlayerFlag->GetHudIcon( pPlayerFlag->GetTeamNumber(), szImage, MAX_PATH );
			m_pCarriedImage->SetImage( szImage );

			if ( m_pRedFlag )
			{
				m_pRedFlag->SetVisible( false );
			}

			if ( m_pBlueFlag )
			{
				m_pBlueFlag->SetVisible( false );
			}

			if ( m_pGreenFlag )
			{
				m_pGreenFlag->SetVisible( false );
			}

			if ( m_pYellowFlag )
			{
				m_pYellowFlag->SetVisible( false );
			}

			m_pCarriedImage->SetVisible( true );

			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );

			if ( m_pCapturePoint )
			{
				m_pCapturePoint->SetVisible( true );

				if ( pLocalPlayer )
				{
					// Go through all the capture zones and find ours.
					for ( C_CaptureZone *pZone : C_CaptureZone::AutoList() )
					{
						if ( !pZone->IsDormant() 
							&& ( pZone->GetTeamNumber() == pLocalPlayer->GetTeamNumber() ) || pZone->GetTeamNumber() == TEAM_UNASSIGNED
							&& !pZone->IsDisabled() )
						{
							m_pCapturePoint->SetEntity( pZone );
						}
					}
				}
			}
		}
	}
	else
	{
		// Were we carrying the flag?
		if ( m_bCarryingFlag )
		{
			m_bCarryingFlag = false;
			m_bFlagAnimationPlayed = false;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutline" );
		}

		if ( m_pCarriedImage )
		{
			m_pCarriedImage->SetVisible( false );
		}

		if ( m_pCapturePoint )
		{
			m_pCapturePoint->SetVisible( false );
		}

		if ( m_pBlueFlag )
		{
			m_pBlueFlag->SetVisible( true );
			m_pBlueFlag->UpdateStatus();
		}

		if ( m_pRedFlag )
		{
			m_pRedFlag->SetVisible( true );
			m_pRedFlag->UpdateStatus();
		}

		if ( m_pGreenFlag )
		{
			m_pGreenFlag->SetVisible( true );
			m_pGreenFlag->UpdateStatus();
		}

		if ( m_pYellowFlag )
		{
			m_pYellowFlag->SetVisible( true );
			m_pYellowFlag->UpdateStatus();
		}
	}
}


void CTFHudFlagObjectives::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( !V_strcmp( pszEventName, "game_maploaded" ) )
	{
		InvalidateLayout( false, true );
	}
}
