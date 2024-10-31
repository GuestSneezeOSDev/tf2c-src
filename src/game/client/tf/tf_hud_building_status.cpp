//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "iclientmode.h"
#include "c_baseobject.h"
#include "c_tf_player.h"
#include "ienginevgui.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include <vgui/IVGui.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/AnimationController.h>
#include "game_controls/IconPanel.h"
#include "tf_round_timer.h"

#include "tf_hud_building_status.h"

#include "c_obj_sentrygun.h"
#include "c_obj_dispenser.h"
#include "c_obj_teleporter.h"
#include "c_obj_sapper.h"
#include "c_obj_jumppad.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_ObjectSentrygun;

extern CUtlVector<int> g_TeamRoundTimers;

using namespace vgui;

ConVar tf_hud_num_building_alert_beeps( "tf_hud_num_building_alert_beeps", "2", FCVAR_ARCHIVE, "Number of times to play warning sound when a new alert displays on building hud objects", true, 0, false, 0 );

ConVar tf2c_building_sapper_awareness( "tf2c_building_sapper_awareness", "0", FCVAR_ARCHIVE, "Makes the warning sound more obvious when a building is being sapped by applying the 'Engineer_Alert' soundmix." );
ConVar tf2c_building_sapper_awareness_cooldown( "tf2c_building_sapper_awareness_cooldown", "45.0", FCVAR_ARCHIVE, "The time it takes for the 'Engineer_Alert' soundmix to be re-applied." );
ConVar tf2c_building_sapper_awareness_timeout( "tf2c_building_sapper_awareness_timeout", "42.0", FCVAR_ARCHIVE, "The time it takes for the 'Engineer_Alert' soundmix to be removed (This value is subtracted from gpGlobals->curtime)." );

extern ConVar tf_radar_pulse_interval;

//============================================================================

DECLARE_BUILD_FACTORY( CBuildingHealthBar );


CBuildingHealthBar::CBuildingHealthBar( Panel *parent, const char *panelName ) : vgui::ProgressBar( parent, panelName )
{
}


void CBuildingHealthBar::Paint()
{
	if ( _progress < 0.5f )
	{
		SetFgColor( m_cLowHealthColor );
	}
	else
	{
		SetFgColor( m_cHealthColor );
	}

	BaseClass::Paint();
}


void CBuildingHealthBar::PaintBackground()
{
	// Save progress and real fg color.
	float flProgress = _progress;
	Color fgColor = GetFgColor();

	// Stuff our fake info.
	_progress = 1.0f;
	SetFgColor( GetBgColor() );

	BaseClass::Paint();

	// Restore actual progress / color.
	_progress = flProgress;
	SetFgColor( fgColor );
}


void CBuildingHealthBar::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetBgColor( GetSchemeColor( "BuildingHealthBar.BgColor", pScheme ) );
	m_cHealthColor = GetSchemeColor( "BuildingHealthBar.Health", pScheme );
	m_cLowHealthColor = GetSchemeColor( "BuildingHealthBar.LowHealth", pScheme );
	SetBorder( NULL );
}

//============================================================================


CBuildingStatusItem::CBuildingStatusItem( Panel *parent, const char *szLayout, int iObjectType, int iObjectMode ) :
	BaseClass( parent, "BuildingStatusItem" )
{
	SetProportional( true );

	// Save our layout file for re-loading.
	V_strcpy_safe( m_szLayout, szLayout );

	// Load control settings...
	LoadControlSettings( szLayout );

	SetPositioned( false );

	m_pObject = NULL;
	m_iObjectType = iObjectType;
	m_iObjectMode = iObjectMode;

	m_pBuiltPanel = new vgui::EditablePanel( this, "BuiltPanel" );
	m_pNotBuiltPanel = new vgui::EditablePanel( this, "NotBuiltPanel" );

	// Sub panels.
	m_pBuildingPanel = new vgui::EditablePanel( m_pBuiltPanel, "BuildingPanel" );
	m_pRunningPanel = new vgui::EditablePanel( m_pBuiltPanel, "RunningPanel" );

	// Shared between all sub panels.
	m_pBackground = new CIconPanel( this, "Background" );

	// Running and Building sub panels only.
	m_pHealthBar = new CBuildingHealthBar( m_pBuiltPanel, "Health" );
	m_pHealthBar->SetSegmentInfo( YRES( 1 ), YRES( 3 ) );
	m_pHealthBar->SetProgressDirection( ProgressBar::PROGRESS_NORTH );
	m_pHealthBar->SetBarInset( 0 );

	m_pBuildingProgress = new vgui::ContinuousProgressBar( m_pBuildingPanel, "BuildingProgress" ); 

	m_pLevelIcons[0] = new CIconPanel( m_pBuiltPanel, "Icon_Upgrade_1" );
	m_pLevelIcons[1] = new CIconPanel( m_pBuiltPanel, "Icon_Upgrade_2" );
	m_pLevelIcons[2] = new CIconPanel( m_pBuiltPanel, "Icon_Upgrade_3" );

	m_pJumppadModeIcons[JUMPPAD_TYPE_A] = new CIconPanel( m_pBuiltPanel, "Icon_Jumppad_Mode_A" );
	m_pJumppadModeIcons[JUMPPAD_TYPE_B] = new CIconPanel(m_pBuiltPanel, "Icon_Jumppad_Mode_B");

	m_pAlertTray = new CBuildingStatusAlertTray( m_pBuiltPanel, "AlertTray" );
	m_pWrenchIcon = new CIconPanel( m_pBuiltPanel, "WrenchIcon" );
	m_pSapperIcon = new CIconPanel( m_pBuiltPanel, "SapperIcon" );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}


void CBuildingStatusItem::ApplySchemeSettings( IScheme *pScheme )
{
	// This lets us use hud_reloadscheme to reload the status items.
	int x, y;
	GetPos( x, y );

	LoadControlSettings( m_szLayout );

	SetPos( x, y );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: Calc visibility of subpanels.
//-----------------------------------------------------------------------------
void CBuildingStatusItem::PerformLayout( void )
{
	BaseClass::PerformLayout();

	C_BaseObject *pObj = m_pObject.Get();
	if ( pObj )
	{
		m_bActive = true;

		// Redo the background.
		m_pBackground->SetIcon( GetBackgroundImage() );
		m_pLevelIcons[0]->SetVisible( false );
		m_pLevelIcons[1]->SetVisible( false );
		m_pLevelIcons[2]->SetVisible(false);

		if (GetRepresentativeObjectType() == OBJ_JUMPPAD)
		{
			if (GetRepresentativeObjectMode() == JUMPPAD_TYPE_A)
			{
				m_pJumppadModeIcons[JUMPPAD_TYPE_A]->SetVisible(true);
				m_pJumppadModeIcons[JUMPPAD_TYPE_B]->SetVisible(false);
			}
			else
			{
				m_pJumppadModeIcons[JUMPPAD_TYPE_A]->SetVisible(false);
				m_pJumppadModeIcons[JUMPPAD_TYPE_B]->SetVisible(true);
			}
		}

		if ( pObj->InRemoteConstruction() )
		{
			m_pBuildingPanel->SetVisible( false );
			m_pRunningPanel->SetVisible( false );
		}
		else if ( pObj->IsBuilding() )
		{
			m_pBuildingPanel->SetVisible( true );
			m_pRunningPanel->SetVisible( false );
		}
		else
		{
			m_pBuildingPanel->SetVisible( false );
			m_pRunningPanel->SetVisible( true );

			if ( IsUpgradable() )
			{
				m_pLevelIcons[pObj->GetUpgradeLevel() - 1]->SetVisible( true );
			}
		}
	}
	else
	{
		m_bActive = false;

		// Redo the background.
		m_pBackground->SetIcon( GetInactiveBackgroundImage() );

		if ( m_pAlertTray->IsTrayOut() )
		{
			m_pAlertTray->HideTray();
			m_pWrenchIcon->SetVisible( false );
			m_pSapperIcon->SetVisible( false );

			m_pLevelIcons[0]->SetVisible( false );
			m_pLevelIcons[1]->SetVisible( false );
			m_pLevelIcons[2]->SetVisible( false );
		}
	}

	m_pHealthBar->SetVisible( m_bActive );

	m_pNotBuiltPanel->SetVisible( !m_bActive );
	m_pBuiltPanel->SetVisible( m_bActive );
}


//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CBuildingStatusItem::LevelInit( void )
{
	if ( m_pAlertTray )
	{
		m_pAlertTray->LevelInit();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CBuildingStatusItem::SetObject( C_BaseObject *pObject )
{
	m_pObject = pObject;
	Assert( !pObject || ( pObject && !pObject->IsMarkedForDeletion() ) );
	if ( !pObject )
	{
		m_pAlertTray->HideTray();
		m_pWrenchIcon->SetVisible( false );
		m_pSapperIcon->SetVisible( false );
		m_pAlertTray->SetAlertType( BUILDING_HUD_ALERT_NONE );
	}
}


void CBuildingStatusItem::Paint( void )
{
}


void CBuildingStatusItem::PaintBackground( void )
{
}


const char *CBuildingStatusItem::GetBackgroundImage( void )
{
	const char *pResult = "obj_status_background_blue";

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		Assert( 0 );
		return pResult;
	}

	switch ( pLocalPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			pResult = "obj_status_background_red";
			break;
		case TF_TEAM_BLUE:
			pResult = "obj_status_background_blue";
			break;
		case TF_TEAM_GREEN:
			pResult = "obj_status_background_green";
			break;
		case TF_TEAM_YELLOW:
			pResult = "obj_status_background_yellow";
			break;
		default:
			break;
	}

	return pResult;
}


const char *CBuildingStatusItem::GetInactiveBackgroundImage( void )
{
	return "obj_status_background_disabled";
}


void CBuildingStatusItem::OnTick()
{
	// We only tick while active and with a valid built object.
	C_BaseObject *pObject = GetRepresentativeObject();
	if ( !pObject )
	{
		if ( m_bActive )
		{
			// We lost our object, force relayout to inactive mode.
			InvalidateLayout();

			// Tell our parent that we're gone.
			IGameEvent *event = gameeventmanager->CreateEvent( "building_info_changed" );
			if ( event )
			{
				event->SetInt( "building_type", GetRepresentativeObjectType() );
				event->SetInt( "object_mode", GetRepresentativeObjectType() );
				gameeventmanager->FireEventClientSide( event );
			}
		}

		// We don't want to tick while inactive regardless.
		return;
	}

	m_pHealthBar->SetProgress( (float)pObject->GetHealth() / (float)pObject->GetMaxHealth() );

	if ( pObject->InRemoteConstruction() )
	{
		m_pBuildingPanel->SetVisible( false );
		m_pRunningPanel->SetVisible( false );
	}
	else if ( pObject->IsBuilding() )
	{
		m_pBuildingPanel->SetVisible( true );
		m_pRunningPanel->SetVisible( false );

		m_pBuildingProgress->SetProgress( pObject->GetPercentageConstructed() );
	}
	else
	{
		m_pBuildingPanel->SetVisible( false );
		m_pRunningPanel->SetVisible( true );
	}

	// What is our current alert state?
	BuildingHudAlert_t alertLevel = pObject->GetBuildingAlertLevel();
	if ( !ShouldShowTray( alertLevel ) )
	{
		// If the tray is out, hide it.
		if ( m_pAlertTray->IsTrayOut() )
		{
			m_pAlertTray->HideTray();
			m_pWrenchIcon->SetVisible( false );
			m_pSapperIcon->SetVisible( false );
		}
	}
	else
	{
		if ( !m_pAlertTray->IsTrayOut() )
		{
			m_pAlertTray->ShowTray();
		}

		m_pAlertTray->SetAlertType( alertLevel );

		m_pWrenchIcon->SetVisible( false );
		m_pSapperIcon->SetVisible( false );

		if ( m_pAlertTray->GetAlertType() != BUILDING_HUD_ALERT_NONE &&
			m_pAlertTray->GetPercentDeployed() >= 1.0f )
		{
			switch ( m_pAlertTray->GetAlertType() )
			{
				case BUILDING_HUD_ALERT_LOW_AMMO:
				case BUILDING_HUD_ALERT_LOW_HEALTH:
				case BUILDING_HUD_ALERT_VERY_LOW_AMMO:
				case BUILDING_HUD_ALERT_VERY_LOW_HEALTH:
					m_pWrenchIcon->SetVisible( true );
					break;

				case BUILDING_HUD_ALERT_SAPPER:
					m_pSapperIcon->SetVisible( true );
					break;

				case BUILDING_HUD_ALERT_NONE:
				default:
					break;
			}
		}
	}	
}


C_BaseObject *CBuildingStatusItem::GetRepresentativeObject( void )
{
	if ( !m_bActive )
		return NULL;
	
	return m_pObject.Get();
}


int CBuildingStatusItem::GetRepresentativeObjectType( void )
{
	return m_iObjectType;
}


int CBuildingStatusItem::GetRepresentativeObjectMode( void )
{
	return m_iObjectMode;
}


int CBuildingStatusItem::GetObjectPriority( void )
{
	return GetObjectInfo( GetRepresentativeObjectType() )->m_iDisplayPriority;	
}


bool CBuildingStatusItem::IsUpgradable( void )
{
	C_BaseObject *pObject = GetRepresentativeObject();
	if ( pObject && pObject->GetMaxUpgradeLevel() > 1 )
		return true;

	return false;
}


bool CBuildingStatusItem::ShouldShowTray( BuildingHudAlert_t iAlertLevel )
{
	return iAlertLevel > BUILDING_HUD_ALERT_NONE;
}

//============================================================================


DECLARE_BUILD_FACTORY( CBuildingStatusAlertTray );


CBuildingStatusAlertTray::CBuildingStatusAlertTray(Panel *parent, const char *panelName) : BaseClass( parent, panelName )
{
	m_pAlertPanelMaterial = NULL;
	m_flAlertDeployedPercent = 0.0f;
	m_bIsTrayOut = false;

	m_pAlertPanelHudTexture = NULL;
	m_pAlertPanelMaterial = NULL;
}


void CBuildingStatusAlertTray::ApplySettings( KeyValues *inResourceData )
{
	m_pAlertPanelHudTexture = gHUD.GetIcon( inResourceData->GetString( "icon", "" ) );

	if ( m_pAlertPanelHudTexture )
	{
		m_pAlertPanelMaterial = materials->FindMaterial( m_pAlertPanelHudTexture->szTextureFile, TEXTURE_GROUP_VGUI );
	}

	BaseClass::ApplySettings( inResourceData );
}


void CBuildingStatusAlertTray::Paint( void )
{
	// Paint the alert tray.
	if ( !m_pAlertPanelMaterial || !m_pAlertPanelHudTexture )
		return;

	int x = 0;
	int y = 0;
	ipanel()->GetAbsPos(GetVPanel(), x,y );
	int iWidth = GetWide();
	int iHeight = GetTall();

	// Position the alert panel image based on the deployed percent.
	float flXa = m_pAlertPanelHudTexture->texCoords[0];
	float flXb = m_pAlertPanelHudTexture->texCoords[2];
	float flYa = m_pAlertPanelHudTexture->texCoords[1];
	float flYb = m_pAlertPanelHudTexture->texCoords[3];

	float flMaskXa = flXa;
	float flMaskXb = flXb;
	float flMaskYa = flYa;
	float flMaskYb = flYb;

	float flFrameDelta = ( flXb - flXa ) * ( 1.0f - m_flAlertDeployedPercent );

	flXa += flFrameDelta;
	flXb += flFrameDelta; 

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_pAlertPanelMaterial );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );

	int r, g, b, a;
	r = a = 255;

	switch ( m_lastAlertType )
	{
		case BUILDING_HUD_ALERT_VERY_LOW_AMMO:
		case BUILDING_HUD_ALERT_VERY_LOW_HEALTH:
			g = b = (int)( 127.0f + 127.0f * cos( gpGlobals->curtime * 2.0f * M_PI_F * 0.5f ) );
			break;

		case BUILDING_HUD_ALERT_SAPPER:
			g = b = (int)( 127.0f + 127.0f * cos( gpGlobals->curtime * 2.0f * M_PI_F * 1.5f ) );
			break;

		case BUILDING_HUD_ALERT_LOW_AMMO:
		case BUILDING_HUD_ALERT_LOW_HEALTH:
		case BUILDING_HUD_ALERT_NONE:
		default:
			g = b = 255;
			break;
	}

	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Position3f( x, y, 0.0f );
	meshBuilder.TexCoord2f( 0, flXa, flYa );
	meshBuilder.TexCoord2f( 1, flMaskXa, flMaskYa );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + iWidth, y, 0.0f );
	meshBuilder.TexCoord2f( 0, flXb, flYa );
	meshBuilder.TexCoord2f( 1, flMaskXb, flMaskYa );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x + iWidth, y + iHeight, 0.0f );
	meshBuilder.TexCoord2f( 0, flXb, flYb );
	meshBuilder.TexCoord2f( 1, flMaskXb, flMaskYb );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.Position3f( x, y + iHeight, 0.0f );
	meshBuilder.TexCoord2f( 0, flXa, flYb );
	meshBuilder.TexCoord2f( 1, flMaskXa, flMaskYb );
	meshBuilder.Color4ub( r, g, b, a );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void CBuildingStatusAlertTray::PaintBackground( void )
{
}

void CBuildingStatusAlertTray::ShowTray( void )
{
	if ( !m_bIsTrayOut )
	{
		m_flAlertDeployedPercent = 0.0f;
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "deployed", 1.0f, 0.0f, 0.3f, AnimationController::INTERPOLATOR_LINEAR );

		m_bIsTrayOut = true;
	}
}

void CBuildingStatusAlertTray::HideTray( void )
{
	if ( m_bIsTrayOut )
	{
		m_flAlertDeployedPercent = 1.0f;
		g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( this, "deployed", 0.0f, 0.0f, 0.3f, AnimationController::INTERPOLATOR_LINEAR );

		m_bIsTrayOut = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CBuildingStatusAlertTray::LevelInit( void )
{
	m_bIsTrayOut = false;
	m_flAlertDeployedPercent = 0.0f;
}

void CBuildingStatusAlertTray::SetAlertType( BuildingHudAlert_t alertLevel )
{	
	m_lastAlertType = alertLevel;
}

//============================================================================


CBuildingStatusItem_SentryGun::CBuildingStatusItem_SentryGun( Panel *parent ) :
	CBuildingStatusItem( parent, "resource/UI/hud_obj_sentrygun.res", OBJ_SENTRYGUN, OBJECT_MODE_NONE )
{
	m_pShellsProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Shells" );
	m_pRocketsProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Rockets" );
	m_pUpgradeProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Upgrade" );

	m_pRocketsIcon = new vgui::ImagePanel( GetRunningPanel(), "RocketIcon" );
	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );

	m_pKillsLabel = new CExLabel( GetRunningPanel(), "KillsLabel", "0" );

	m_pSentryIcons[0] = new CIconPanel( this, "Icon_Sentry_1" );
	m_pSentryIcons[1] = new CIconPanel( this, "Icon_Sentry_2" );
	m_pSentryIcons[2] = new CIconPanel( this, "Icon_Sentry_3" );

	m_iUpgradeLevel = 1;

	m_iKills = -1;
}



void CBuildingStatusItem_SentryGun::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_cLowAmmoColor = scheme->GetColor( "LowHealthRed", Color(255,0,0,255) );
	m_cNormalAmmoColor = scheme->GetColor( "ProgressOffWhite", Color(255,255,255,255) );
}

//-----------------------------------------------------------------------------
// Purpose: Calc visibility of subpanels
//-----------------------------------------------------------------------------
void CBuildingStatusItem_SentryGun::PerformLayout( void )
{
	BaseClass::PerformLayout();

	C_ObjectSentrygun *pSentrygun = dynamic_cast<C_ObjectSentrygun *>( GetRepresentativeObject() );
	if ( !pSentrygun )
		return;

	GetRunningPanel()->SetDialogVariable( "numkills", pSentrygun->GetKills() );
	GetRunningPanel()->SetDialogVariable( "numassists", pSentrygun->GetAssists() );

	int iShells, iMaxShells;
	int iRockets, iMaxRockets;
	pSentrygun->GetAmmoCount( iShells, iMaxShells, iRockets, iMaxRockets );

	// Shells label.
	float flShells = (float)iShells / (float)iMaxShells;
	m_pShellsProgress->SetProgress( flShells );

	if ( flShells < 0.25f )
	{
		m_pShellsProgress->SetFgColor( m_cLowAmmoColor );
	}
	else
	{
		m_pShellsProgress->SetFgColor( m_cNormalAmmoColor );
	}

	// Rockets label.
	float flRockets = (float)iRockets / (float)SENTRYGUN_MAX_ROCKETS;
	m_pRocketsProgress->SetProgress( flRockets );

	if ( flRockets < 0.25f )
	{
		m_pRocketsProgress->SetFgColor( m_cLowAmmoColor );
	}
	else
	{
		m_pRocketsProgress->SetFgColor( m_cNormalAmmoColor );
	}

	int iUpgradeLevel = pSentrygun->GetUpgradeLevel();
	Assert( iUpgradeLevel >= 1 && iUpgradeLevel <= 3 );

	// Show the correct icon.
	m_pSentryIcons[0]->SetVisible( false );
	m_pSentryIcons[1]->SetVisible( false );
	m_pSentryIcons[2]->SetVisible( false );
	m_pSentryIcons[iUpgradeLevel-1]->SetVisible( true );

	// Upgrade progress.
	int iMetal = pSentrygun->GetUpgradeMetal();
	int iMetalRequired = pSentrygun->GetUpgradeMetalRequired();
	float flUpgrade = (float)iMetal / (float)iMetalRequired;
	m_pUpgradeProgress->SetProgress( flUpgrade );

	// Upgrade label only in 1 or 2.
	bool bUpgradeable = iUpgradeLevel < 3;
	m_pUpgradeIcon->SetVisible( bUpgradeable );
	m_pUpgradeProgress->SetVisible( bUpgradeable );

	// Rockets label only in 3.
	bool bMaxUpgrade = iUpgradeLevel == 3;
	m_pRocketsIcon->SetVisible( bMaxUpgrade );
	m_pRocketsProgress->SetVisible( bMaxUpgrade );
}


void CBuildingStatusItem_SentryGun::OnTick()
{
	BaseClass::OnTick();
}


const char *CBuildingStatusItem_SentryGun::GetBackgroundImage( void )
{
	const char *pResult = "obj_status_background_tall_blue";

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return pResult;

	switch ( pLocalPlayer->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			pResult = "obj_status_background_tall_red";
			break;
		default:
		case TF_TEAM_BLUE:
			pResult = "obj_status_background_tall_blue";
			break;
		case TF_TEAM_GREEN:
			pResult = "obj_status_background_tall_green";
			break;
		case TF_TEAM_YELLOW:
			pResult = "obj_status_background_tall_yellow";
			break;
	}

	return pResult;
}


const char *CBuildingStatusItem_SentryGun::GetInactiveBackgroundImage( void )
{
	return "obj_status_background_tall_disabled";
}

//--------------------------------------------------------------------------------------------------------
// FLAME SENTRY
//--------------------------------------------------------------------------------------------------------


CBuildingStatusItem_FlameSentry::CBuildingStatusItem_FlameSentry(Panel* parent) :
	CBuildingStatusItem(parent, "resource/UI/hud_obj_sentrygun.res", OBJ_FLAMESENTRY, OBJECT_MODE_NONE)
{
	m_pShellsProgress = new vgui::ContinuousProgressBar(GetRunningPanel(), "Shells");
	m_pRocketsProgress = new vgui::ContinuousProgressBar(GetRunningPanel(), "Rockets");
	m_pUpgradeProgress = new vgui::ContinuousProgressBar(GetRunningPanel(), "Upgrade");

	m_pRocketsIcon = new vgui::ImagePanel(GetRunningPanel(), "RocketIcon");
	m_pUpgradeIcon = new CIconPanel(GetRunningPanel(), "UpgradeIcon");

	m_pKillsLabel = new CExLabel(GetRunningPanel(), "KillsLabel", "0");

	m_pSentryIcons[0] = new CIconPanel(this, "Icon_Sentry_1");
	m_pSentryIcons[1] = new CIconPanel(this, "Icon_Sentry_2");
	m_pSentryIcons[2] = new CIconPanel(this, "Icon_Sentry_3");

	m_iUpgradeLevel = 1;

	m_iKills = -1;
}

void CBuildingStatusItem_FlameSentry::ApplySchemeSettings(vgui::IScheme* scheme)
{
	BaseClass::ApplySchemeSettings(scheme);

	m_cLowAmmoColor = scheme->GetColor("LowHealthRed", Color(255, 0, 0, 255));
	m_cNormalAmmoColor = scheme->GetColor("ProgressOffWhite", Color(255, 255, 255, 255));
}

void CBuildingStatusItem_FlameSentry::PerformLayout(void)
{
	BaseClass::PerformLayout();

	C_ObjectSentrygun* pSentrygun = dynamic_cast<C_ObjectSentrygun*>(GetRepresentativeObject());
	if (!pSentrygun)
		return;

	GetRunningPanel()->SetDialogVariable("numkills", pSentrygun->GetKills());
	GetRunningPanel()->SetDialogVariable("numassists", pSentrygun->GetAssists());

	int iShells, iMaxShells;
	int iRockets, iMaxRockets;
	pSentrygun->GetAmmoCount(iShells, iMaxShells, iRockets, iMaxRockets);

	// Shells label.
	float flShells = (float)iShells / (float)iMaxShells;
	m_pShellsProgress->SetProgress(flShells);

	if (flShells < 0.25f)
	{
		m_pShellsProgress->SetFgColor(m_cLowAmmoColor);
	}
	else
	{
		m_pShellsProgress->SetFgColor(m_cNormalAmmoColor);
	}

	// Rockets label.
	float flRockets = (float)iRockets / (float)SENTRYGUN_MAX_ROCKETS;
	m_pRocketsProgress->SetProgress(flRockets);

	if (flRockets < 0.25f)
	{
		m_pRocketsProgress->SetFgColor(m_cLowAmmoColor);
	}
	else
	{
		m_pRocketsProgress->SetFgColor(m_cNormalAmmoColor);
	}

	int iUpgradeLevel = pSentrygun->GetUpgradeLevel();
	Assert(iUpgradeLevel >= 1 && iUpgradeLevel <= 3);

	// Show the correct icon.
	m_pSentryIcons[0]->SetVisible(false);
	m_pSentryIcons[1]->SetVisible(false);
	m_pSentryIcons[2]->SetVisible(false);
	m_pSentryIcons[iUpgradeLevel - 1]->SetVisible(true);

	// Upgrade progress.
	int iMetal = pSentrygun->GetUpgradeMetal();
	int iMetalRequired = pSentrygun->GetUpgradeMetalRequired();
	float flUpgrade = (float)iMetal / (float)iMetalRequired;
	m_pUpgradeProgress->SetProgress(flUpgrade);

	// Upgrade label only in 1 or 2.
	bool bUpgradeable = iUpgradeLevel < 3;
	m_pUpgradeIcon->SetVisible(bUpgradeable);
	m_pUpgradeProgress->SetVisible(bUpgradeable);

	// Rockets label only in 3.
	bool bMaxUpgrade = iUpgradeLevel == 3;
	m_pRocketsIcon->SetVisible(bMaxUpgrade);
	m_pRocketsProgress->SetVisible(bMaxUpgrade);
}

void CBuildingStatusItem_FlameSentry::OnTick()
{
	BaseClass::OnTick();
}


const char* CBuildingStatusItem_FlameSentry::GetBackgroundImage(void)
{
	const char* pResult = "obj_status_background_tall_blue";

	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pLocalPlayer)
		return pResult;

	switch (pLocalPlayer->GetTeamNumber())
	{
	case TF_TEAM_RED:
		pResult = "obj_status_background_tall_red";
		break;
	default:
	case TF_TEAM_BLUE:
		pResult = "obj_status_background_tall_blue";
		break;
	case TF_TEAM_GREEN:
		pResult = "obj_status_background_tall_green";
		break;
	case TF_TEAM_YELLOW:
		pResult = "obj_status_background_tall_yellow";
		break;
	}

	return pResult;
}


const char* CBuildingStatusItem_FlameSentry::GetInactiveBackgroundImage(void)
{
	return "obj_status_background_tall_disabled";
}

//============================================================================


CBuildingStatusItem_Dispenser::CBuildingStatusItem_Dispenser( Panel *parent ) :
	CBuildingStatusItem( parent, "resource/UI/hud_obj_dispenser.res", OBJ_DISPENSER, OBJECT_MODE_NONE )
{
	m_pAmmoProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Ammo" );
	m_pUpgradeProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Upgrade" );
	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );
}

//-----------------------------------------------------------------------------
// Purpose: Calc visibility of subpanels
//-----------------------------------------------------------------------------
void CBuildingStatusItem_Dispenser::PerformLayout( void )
{
	BaseClass::PerformLayout();

	C_ObjectDispenser *pDispenser = dynamic_cast<C_ObjectDispenser *>( GetRepresentativeObject() );
	if ( !pDispenser )
		return;

	int iAmmo = pDispenser->GetMetalAmmoCount();

	float flProgress = (float)iAmmo / (float)DISPENSER_MAX_METAL_AMMO;
	m_pAmmoProgress->SetProgress( flProgress );

	int iUpgradeLevel = pDispenser->GetUpgradeLevel();

	// Upgrade progress.
	int iMetal = pDispenser->GetUpgradeMetal();
	int iMetalRequired = pDispenser->GetUpgradeMetalRequired();
	float flUpgrade = (float)iMetal / (float)iMetalRequired;
	m_pUpgradeProgress->SetProgress(flUpgrade);

	// Upgrade label only in 1 or 2.
	bool bUpgradeable = iUpgradeLevel < 3 && IsUpgradable();
	m_pUpgradeIcon->SetVisible( bUpgradeable );
	m_pUpgradeProgress->SetVisible( bUpgradeable );

}

//============================================================================


CBuildingStatusItem_MiniDispenser::CBuildingStatusItem_MiniDispenser(Panel* parent) :
	CBuildingStatusItem(parent, "resource/UI/hud_obj_minidispenser.res", OBJ_MINIDISPENSER, OBJECT_MODE_NONE)
{
	m_pAmmoProgress = new vgui::ContinuousProgressBar(GetRunningPanel(), "Ammo");
	m_pUpgradeProgress = new vgui::ContinuousProgressBar(GetRunningPanel(), "Upgrade");
	m_pUpgradeIcon = new CIconPanel(GetRunningPanel(), "UpgradeIcon");
}

//-----------------------------------------------------------------------------
// Purpose: Calc visibility of subpanels
//-----------------------------------------------------------------------------
void CBuildingStatusItem_MiniDispenser::PerformLayout(void)
{
	BaseClass::PerformLayout();

	m_pAmmoProgress->SetVisible(false);
	m_pUpgradeIcon->SetVisible(false);
	m_pUpgradeProgress->SetVisible(false);
}

//============================================================================


CBuildingStatusItem_TeleporterEntrance::CBuildingStatusItem_TeleporterEntrance( Panel *parent ) :
CBuildingStatusItem( parent, "resource/UI/hud_obj_tele_entrance.res", OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE )
{
	// Panel and children when we are charging.
	m_pChargingPanel = new vgui::EditablePanel( GetRunningPanel(), "ChargingPanel" );
	m_pRechargeTimer = new vgui::ContinuousProgressBar( m_pChargingPanel, "Recharge" );

	// Panel and children when we are fully charged.
	m_pFullyChargedPanel = new vgui::EditablePanel( GetRunningPanel(), "FullyChargedPanel" );

	m_pUpgradeProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Upgrade" );

	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );

	m_iTimesUsed = -1; // Force first update of 0.
	m_iTeleporterState = -1;

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}


void CBuildingStatusItem_TeleporterEntrance::OnTick( void )
{
	// We only tick while active and with a valid built object.
	C_ObjectTeleporter *pTeleporter = dynamic_cast<C_ObjectTeleporter *>( GetRepresentativeObject() );
	if ( pTeleporter && IsActive() )
	{
		int iUpgradeLevel = pTeleporter->GetUpgradeLevel();

		if ( pTeleporter->GetState() == TELEPORTER_STATE_RECHARGING )
		{
			// Update the recharge.
			float flMaxRecharge = 0.5f + g_flTeleporterRechargeTimes[iUpgradeLevel - 1];
			float flChargeTime = pTeleporter->GetChargeTime();
			m_pRechargeTimer->SetProgress( 1.0f - ( flChargeTime / flMaxRecharge ) );
		}

		// Upgrade progress.
		int iMetal = pTeleporter->GetUpgradeMetal();
		int iMetalRequired = pTeleporter->GetUpgradeMetalRequired();
		float flUpgrade = (float)iMetal / (float)iMetalRequired;
		m_pUpgradeProgress->SetProgress( flUpgrade );

		// Upgrade label only in 1 or 2.
		bool bUpgradeable = iUpgradeLevel < 3 && IsUpgradable();
		m_pUpgradeIcon->SetVisible( bUpgradeable );
		m_pUpgradeProgress->SetVisible( bUpgradeable );
	}

	BaseClass::OnTick();
}


void CBuildingStatusItem_TeleporterEntrance::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// We only tick while active and with a valid built object
	C_ObjectTeleporter *pTeleporter = static_cast<C_ObjectTeleporter *>( GetRepresentativeObject() );
	if ( !IsActive() || !pTeleporter )
		return;

	bool bRecharging = pTeleporter->GetState() == TELEPORTER_STATE_RECHARGING;
	m_pChargingPanel->SetVisible( bRecharging );
	m_pFullyChargedPanel->SetVisible( !bRecharging );

	// How many times has this teleporter been used?
	m_pFullyChargedPanel->SetDialogVariable( "timesused", pTeleporter->GetTimesUsed() );		
}

//============================================================================

CBuildingStatusItem_TeleporterExit::CBuildingStatusItem_TeleporterExit( Panel *parent ) :
	CBuildingStatusItem( parent, "resource/UI/hud_obj_tele_exit.res", OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT )
{
	m_pUpgradeProgress = new vgui::ContinuousProgressBar( GetRunningPanel(), "Upgrade" );
	m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );
}


void CBuildingStatusItem_TeleporterExit::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// We only tick while active and with a valid built object
	C_ObjectTeleporter *pTeleporter = dynamic_cast<C_ObjectTeleporter *>( GetRepresentativeObject() );
	if ( !IsActive() || !pTeleporter )
		return;

	// Upgrade progress.
	int iMetal = pTeleporter->GetUpgradeMetal();
	int iMetalRequired = pTeleporter->GetUpgradeMetalRequired();
	float flUpgrade = (float)iMetal / (float)iMetalRequired;

	bool bUpgradeable = pTeleporter->GetUpgradeLevel() < 3 && IsUpgradable();
	if ( m_pUpgradeProgress )
	{ 
		m_pUpgradeProgress->SetProgress( flUpgrade );
		m_pUpgradeProgress->SetVisible( bUpgradeable );
	}
		
	if ( m_pUpgradeIcon )
	{
		m_pUpgradeIcon->SetVisible( bUpgradeable );
	}
}

//============================================================================


CBuildingStatusItem_JumpPad::CBuildingStatusItem_JumpPad( Panel* parent, int iMode ) :
	CBuildingStatusItem( parent, "resource/UI/hud_obj_jumppad.res", OBJ_JUMPPAD, iMode )
{
	// Panel and children when we are charging.
	m_pChargingPanel = new vgui::EditablePanel( GetRunningPanel(), "ChargingPanel" );
	m_pRechargeTimer = new vgui::ContinuousProgressBar( m_pChargingPanel, "Recharge" );

	// Panel and children when we are fully charged.
	m_pFullyChargedPanel = new vgui::EditablePanel( GetRunningPanel(), "FullyChargedPanel" );

	//m_pUpgradeIcon = new CIconPanel( GetRunningPanel(), "UpgradeIcon" );

	m_iTimesUsed = -1; // Force first update of 0.
	m_iJumpPadState = -1;

	vgui::ivgui()->AddTickSignal(GetVPanel());
}


void CBuildingStatusItem_JumpPad::PerformLayout(void)
{
	BaseClass::PerformLayout();

	// We only tick while active and with a valid built object
	C_ObjectJumppad* pJumpPad = static_cast<C_ObjectJumppad*>( GetRepresentativeObject() );
	if ( !IsActive() || !pJumpPad )
		return;

	// How many times has this teleporter been used?
	m_pFullyChargedPanel->SetVisible(true);
	m_pFullyChargedPanel->SetDialogVariable("timesused", pJumpPad->GetTimesUsed() );
}

//============================================================================


CBuildingStatusItem_Sapper::CBuildingStatusItem_Sapper( Panel *parent ) :
	CBuildingStatusItem( parent, "resource/UI/hud_obj_sapper.res", OBJ_ATTACHMENT_SAPPER, OBJECT_MODE_NONE )
{
	// Health of target building.
	m_pTargetHealthBar = new ContinuousProgressBar( GetRunningPanel(), "TargetHealth" );

	// Image of target building.
	m_pTargetIcon = new CIconPanel( GetRunningPanel(), "TargetIcon" );

	// Force first think to set the icon.
	m_iTargetType = -1;
}


void CBuildingStatusItem_Sapper::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// We only tick while active and with a valid built object.
	C_ObjectSapper *pSapper = dynamic_cast<C_ObjectSapper *>( GetRepresentativeObject() );

	// Only visible.
	SetVisible( pSapper != NULL );

	if ( !IsActive() || !pSapper )
		return;

	C_BaseObject *pTarget = pSapper->GetParentObject();
	if ( pTarget )
	{
		m_pTargetHealthBar->SetProgress( (float)pTarget->GetHealth() / (float)pTarget->GetMaxHealth() );

		int iTargetType = pTarget->GetType();
		if ( m_iTargetType != iTargetType )
		{
			m_pTargetIcon->SetIcon( pTarget->GetHudStatusIcon() );

			m_iTargetType = iTargetType;
		}
	}
	else
	{
		m_pTargetHealthBar->SetProgress( 0.0f );
	}
}


bool CBuildingStatusItem_Sapper::ShouldShowTray( BuildingHudAlert_t iAlertLevel )
{
	return false;
}


//============================================================================


DECLARE_HUDELEMENT( CHudBuildingStatusContainer_Spy );


CHudBuildingStatusContainer_Spy::CHudBuildingStatusContainer_Spy( const char *pElementName ) :
	BaseClass( "BuildingStatus_Spy" )
{
	AddBuildingPanel(OBJ_ATTACHMENT_SAPPER, OBJECT_MODE_NONE);
}


bool CHudBuildingStatusContainer_Spy::ShouldDraw( void )
{
	// Don't draw in freezecam.
	if ( IsInFreezeCam() )
		return false;

	// Don't draw in freezecam.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_SPY, true ) || pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	if ( pPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
		return false;

	return CHudElement::ShouldDraw();
}

//============================================================================

DECLARE_HUDELEMENT( CHudBuildingStatusContainer_Engineer );


CHudBuildingStatusContainer_Engineer::CHudBuildingStatusContainer_Engineer( const char *pElementName ) :
	BaseClass( "BuildingStatus_Engineer" )
{
	AddBuildingPanel( OBJ_SENTRYGUN, OBJECT_MODE_NONE );
	AddBuildingPanel( OBJ_FLAMESENTRY, OBJECT_MODE_NONE);
	AddBuildingPanel( OBJ_DISPENSER, OBJECT_MODE_NONE );
	AddBuildingPanel( OBJ_MINIDISPENSER, OBJECT_MODE_NONE );
	AddBuildingPanel( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE );
	AddBuildingPanel( OBJ_JUMPPAD, JUMPPAD_TYPE_A );
	AddBuildingPanel( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT );
	AddBuildingPanel( OBJ_JUMPPAD, JUMPPAD_TYPE_B );
}


bool CHudBuildingStatusContainer_Engineer::ShouldDraw( void )
{
	// Don't draw in freezecam.
	if ( IsInFreezeCam() )
		return false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsPlayerClass( TF_CLASS_ENGINEER, true ) )
		return false;

	if ( pPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
		return false;

	return CHudElement::ShouldDraw();
}

//============================================================================

// Order the buildings in our m_BuildingsList by their object priority.
typedef CBuildingStatusItem *BUILDINGSTATUSITEM_PTR;
static bool BuildingOrderLessFunc( const BUILDINGSTATUSITEM_PTR &left, const BUILDINGSTATUSITEM_PTR &right )
{
	return ( left->GetObjectPriority() < right->GetObjectPriority() );
}


CHudBuildingStatusContainer::CHudBuildingStatusContainer( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	SetProportional( true );

	ListenForGameEvent("building_info_changed");
	ListenForGameEvent("post_inventory_application");

	m_AlertLevel = BUILDING_HUD_ALERT_NONE;
	m_flNextBeep = 0;
	m_iNumBeepsToBeep = 0;

	// For beeping.
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}


bool CHudBuildingStatusContainer::ShouldDraw( void )
{
	// Don't draw in freezecam.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	return CHudElement::ShouldDraw();
}


void CHudBuildingStatusContainer::LevelInit( void )
{
	CHudElement::LevelInit();

	for ( int i = 0, c = m_BuildingPanels.Count(); i < c; i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element( i );
		if ( pItem )
		{
			pItem->LevelInit();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Create the appropriate info panel for the object
//-----------------------------------------------------------------------------
CBuildingStatusItem *CHudBuildingStatusContainer::CreateItemPanel( int iObjectType, int iObjectMode )
{
	CBuildingStatusItem *pBuildingItem;
	
	switch ( iObjectType )
	{
		case OBJ_SENTRYGUN:
			pBuildingItem = new CBuildingStatusItem_SentryGun( this );
			break;
		case OBJ_FLAMESENTRY:
			pBuildingItem = new CBuildingStatusItem_FlameSentry(this);
			break;
		case OBJ_DISPENSER:
			pBuildingItem = new CBuildingStatusItem_Dispenser( this );
			break;
		case OBJ_MINIDISPENSER:
			pBuildingItem = new CBuildingStatusItem_MiniDispenser(this);
			break;
		case OBJ_TELEPORTER:
			if ( iObjectMode == TELEPORTER_TYPE_ENTRANCE )
			{
				pBuildingItem = new CBuildingStatusItem_TeleporterEntrance( this );
			}
			else /*if ( iObjectMode == TELEPORTER_TYPE_EXIT )*/
			{
				pBuildingItem = new CBuildingStatusItem_TeleporterExit( this );
			}
			break;
		case OBJ_ATTACHMENT_SAPPER:
			pBuildingItem = new CBuildingStatusItem_Sapper( this );
			break;
		case OBJ_JUMPPAD:
			if ( iObjectMode == JUMPPAD_TYPE_A )
			{
				pBuildingItem = new CBuildingStatusItem_JumpPad( this, JUMPPAD_TYPE_A );
			}
			else /*if ( iObjectMode == JUMPPAD_TYPE_B )*/
			{
				pBuildingItem = new CBuildingStatusItem_JumpPad( this, JUMPPAD_TYPE_B );
			}
			break;
		default:
			pBuildingItem = NULL;
			break;
	}
	
	Assert( pBuildingItem );
	return pBuildingItem;
}


void CHudBuildingStatusContainer::AddBuildingPanel( int iBuildingType, int iBuildingMode )
{
	CBuildingStatusItem *pBuildingItem = CreateItemPanel( iBuildingType, iBuildingMode );
	Assert( pBuildingItem );
	pBuildingItem->SetPos( 0, 0 );
	pBuildingItem->InvalidateLayout();
	m_BuildingPanels.AddToTail( pBuildingItem );

	RepositionObjectPanels();
}


void CHudBuildingStatusContainer::UpdateAllBuildings( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return;

	for ( int i = 0, c = m_BuildingPanels.Count(); i < c; i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element( i );
		if ( pItem )
		{
			// Find the item that represents this building type and find the object.
			pItem->SetObject( pLocalPlayer->GetObjectOfType( pItem->GetRepresentativeObjectType(), pItem->GetRepresentativeObjectMode() ) );
			pItem->InvalidateLayout( true );

			RecalculateAlertState();
		}
	}
}


void CHudBuildingStatusContainer::OnBuildingChanged( int iBuildingType, int iObjectMode )
{
	bool bFound = false;
	for ( int i = 0, c = m_BuildingPanels.Count(); i < c && !bFound; i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element( i );
		if ( pItem && pItem->GetRepresentativeObjectType() == iBuildingType && pItem->GetRepresentativeObjectMode() == iObjectMode )
		{
			// Find the item that represents this building type and get the object.
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				pItem->SetObject( pLocalPlayer->GetObjectOfType( iBuildingType, iObjectMode ) );
			}

			pItem->InvalidateLayout( true );
			bFound = true;

			RecalculateAlertState();
		}
	}
}


void CHudBuildingStatusContainer::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	SetPaintBackgroundEnabled( false );

	RepositionObjectPanels();
}

//-----------------------------------------------------------------------------
// Purpose: Contents of object list has changed, reposition the panels
//-----------------------------------------------------------------------------
void CHudBuildingStatusContainer::RepositionObjectPanels( void )
{
	float flXPos = XRES( 9 );
	float flYPos = YRES( 9 );

	for ( int i = 0, c = m_BuildingPanels.Count(); i < c; i++ )
	{
		CBuildingStatusItem *pItem = m_BuildingPanels.Element( i );
		if ( pItem )
		{

			// Set position directly.
			pItem->SetPos(flXPos, flYPos);
			// Don't let Teleport panels to push Y further, as we are putting Jump Pads on top of them
			if (pItem->GetRepresentativeObjectType() != OBJ_SENTRYGUN && 
				pItem->GetRepresentativeObjectType() != OBJ_TELEPORTER && 
				pItem->GetRepresentativeObjectType() != OBJ_DISPENSER )
			{
				// The fade around the panels gives a gap.
				flYPos += pItem->GetTall();
			}
		}
	}
}


void CHudBuildingStatusContainer::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	//if (FStrEq("post_inventory_application", event->GetName()))
	//{
	//	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	//	if (!pLocalPlayer)
	//		return;
	//
	//	bHasJumpPads = false;
	//	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, bHasJumpPads, set_teleporter_mode);
	////}
	//
	////else
	if ( !Q_strcmp( pszEventName, "building_info_changed" ) )
	{
		int iBuildingType = event->GetInt( "building_type" );
		int iObjectMode = event->GetInt( "object_mode" );
		if ( iBuildingType >= 0 )
		{
			OnBuildingChanged( iBuildingType, iObjectMode );
		}
		else
		{
			UpdateAllBuildings();
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

void CHudBuildingStatusContainer::RecalculateAlertState( void )
{
	BuildingHudAlert_t maxAlertLevel = BUILDING_HUD_ALERT_NONE;

	// Find our highest warning level.
	for ( int i = 0, c = m_BuildingPanels.Count(); i < c; i++ )
	{
		C_BaseObject *pObject = m_BuildingPanels.Element( i )->GetRepresentativeObject();
		if ( pObject )
		{
			BuildingHudAlert_t alertLevel = pObject->GetBuildingAlertLevel();
			if ( alertLevel > maxAlertLevel )
			{
				maxAlertLevel = alertLevel;
			}
		}
	}

	if ( maxAlertLevel != m_AlertLevel )
	{
		if ( maxAlertLevel >= BUILDING_HUD_ALERT_VERY_LOW_AMMO )
		{
			m_flNextBeep = gpGlobals->curtime;
			m_iNumBeepsToBeep = tf_hud_num_building_alert_beeps.GetInt();
		}

		m_AlertLevel = maxAlertLevel;
	}
}

void CHudBuildingStatusContainer::PostInit()
{
	pSndMixer = (ConVar*)cvar->FindVar("snd_soundmixer");
}

void CHudBuildingStatusContainer::OnTick( void )
{
	C_TFPlayer* pLocalPlayer = C_TFPlayer::GetLocalTFPlayer( );
	if ( !pLocalPlayer )
		return;

	// this needs to leave this function and not be so fucking hot
	// -sappho
	bHasJumpPads = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, bHasJumpPads, set_teleporter_mode);

	bHasMiniDispenser = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, bHasMiniDispenser, set_dispenser_mode);

	bHasFlameSentry = false;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer, bHasFlameSentry, set_sentry_mode);

	for ( int i = 0; i < m_BuildingPanels.Count( ); i++ )
	{
		CBuildingStatusItem* pItem = m_BuildingPanels.Element( i );

		if ( pItem && ( pItem->GetRepresentativeObjectType( ) == OBJ_TELEPORTER ) )
		{
			if ( pItem->IsVisible( ) != !bHasJumpPads )
			{
				pItem->SetVisible( !bHasJumpPads );
			}
		}

		if ( pItem && ( pItem->GetRepresentativeObjectType( ) == OBJ_JUMPPAD ) )
		{
			if ( pItem->IsVisible() != bHasJumpPads )
			{
				pItem->SetVisible( bHasJumpPads );
			}
		}

		if ( pItem && (pItem->GetRepresentativeObjectType() == OBJ_DISPENSER) )
		{
			if (pItem->IsVisible() != !bHasMiniDispenser)
			{
				pItem->SetVisible(!bHasMiniDispenser);
			}
		}

		if (pItem && (pItem->GetRepresentativeObjectType() == OBJ_MINIDISPENSER))
		{
			if (pItem->IsVisible() != bHasMiniDispenser)
			{
				pItem->SetVisible(bHasMiniDispenser);
			}
		}

		if (pItem && (pItem->GetRepresentativeObjectType() == OBJ_SENTRYGUN))
		{
			if (pItem->IsVisible() != !bHasFlameSentry)
			{
				pItem->SetVisible(!bHasFlameSentry);
			}
		}

		if (pItem && (pItem->GetRepresentativeObjectType() == OBJ_FLAMESENTRY))
		{
			if (pItem->IsVisible() != bHasFlameSentry)
			{
				pItem->SetVisible(bHasFlameSentry);
			}
		}
	}
	
	// Sounds

	if ( m_AlertLevel >= BUILDING_HUD_ALERT_VERY_LOW_AMMO &&
		gpGlobals->curtime >= m_flNextBeep && 
		m_iNumBeepsToBeep > 0 )
	{
		pLocalPlayer->EmitSound( "Hud.Warning" );

		if ( tf2c_building_sapper_awareness.GetBool() )
		{
			if ( m_AlertLevel == BUILDING_HUD_ALERT_SAPPER && gpGlobals->curtime >= m_flNextSoundMix && pSndMixer )
			{
				pSndMixer->SetValue( "Engineer_Alert" );

				m_bRevertSoundMixer = true;
				m_flNextSoundMix = gpGlobals->curtime + tf2c_building_sapper_awareness_cooldown.GetFloat();
			}
		}
			
		switch ( m_AlertLevel )
		{
			case BUILDING_HUD_ALERT_VERY_LOW_AMMO:
			case BUILDING_HUD_ALERT_VERY_LOW_HEALTH:
				m_flNextBeep = gpGlobals->curtime + 2.0f;
				m_iNumBeepsToBeep--;
				break;

			case BUILDING_HUD_ALERT_SAPPER:
				m_flNextBeep = gpGlobals->curtime + 1.0f;
				// Don't decrement beeps, we want them to go on forever.
				break;
		}
	}
	
	if ( tf2c_building_sapper_awareness.GetBool() )
	{
		if ( m_AlertLevel < BUILDING_HUD_ALERT_SAPPER || gpGlobals->curtime >= m_flNextSoundMix - tf2c_building_sapper_awareness_timeout.GetFloat() )
		{
			if ( m_bRevertSoundMixer && pSndMixer )
			{
				pSndMixer->Revert();
				m_bRevertSoundMixer = false;

				if ( gpGlobals->curtime >= m_flNextSoundMix )
				{
					m_flNextSoundMix = gpGlobals->curtime;
				}
			}
		}
	}
}
