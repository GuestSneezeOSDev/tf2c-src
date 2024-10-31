//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_hud_target_id.h"
#include "c_tf_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "c_baseobject.h"
#include "c_team.h"
#include "tf_gamerules.h"
#include "tf_hud_statpanel.h"
#include "tf_hud_arena_class_layout.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CMainTargetID );
DECLARE_HUDELEMENT( CSpectatorTargetID );
DECLARE_HUDELEMENT( CSecondaryTargetID );

using namespace vgui;

extern ConVar tf2c_streamer_mode;
extern ConVar cl_hud_minmode;

vgui::IImage *GetDefaultAvatarImage( int iPlayerIndex );

ConVar tf_hud_target_id_alpha( "tf_hud_target_id_alpha", "100", FCVAR_ARCHIVE , "Alpha value of the Target ID background" );
ConVar tf_hud_target_id_show_avatars( "tf_hud_target_id_show_avatars", "1", FCVAR_ARCHIVE, "Display Steam avatars on TargetID, 1 = Everyone, 2 = Friends Only" );

void SpectatorTargetLocationCallback( IConVar *var, const char *pOldString, float flOldValue )
{
	CSpectatorTargetID *pTargetID = GET_HUDELEMENT( CSpectatorTargetID );
	if ( pTargetID )
	{
		pTargetID->InvalidateLayout();
	}
}
ConVar tf_spectator_target_location( "tf_spectator_target_location", "0", FCVAR_ARCHIVE, "Determines the location of the spectator Target ID panel", true, 0, true, 3, SpectatorTargetLocationCallback );


CTargetID::CTargetID( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_pBGPanel = NULL;
	m_pMedalImage = new vgui::ImagePanel( this, "MedalImage" );
	m_pAvatar = NULL;
	m_pTargetNameLabel = NULL;
	m_pTargetDataLabel = NULL;
	m_pMoveableIcon = NULL;
	m_pMoveableSymbolIcon = NULL;
	m_pMoveableIconBG = NULL;
	m_pMoveableKeyLabel = NULL;
	m_pTargetHealth = new CTFSpectatorGUIHealth( this, "SpectatorGUIHealth" );
	m_pAmmoIcon = NULL;
	m_pMoveableSubPanel = NULL;

	m_bLayoutOnUpdate = false;

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );

	m_iRenderPriority = 5;

	m_iTargetHealthOriginalX = 0;
	m_iBGPanelOriginalX = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CTargetID::Reset( void )
{
	m_pTargetHealth->Reset();
};


void CTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	LoadControlSettings( "resource/UI/TargetID.res" );

	BaseClass::ApplySchemeSettings( scheme );

	m_pBGPanel = dynamic_cast<CTFImagePanel *>( FindChildByName( "TargetIDBG" ) );
	m_pAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName( "AvatarImage" ) );
	m_pTargetNameLabel = dynamic_cast<Label *>( FindChildByName( "TargetNameLabel" ) );
	m_pTargetDataLabel = dynamic_cast<Label *>( FindChildByName( "TargetDataLabel" ) );
	m_pMoveableSubPanel = dynamic_cast<vgui::EditablePanel *>( FindChildByName( "MoveableSubPanel" ) );
	if ( m_pMoveableSubPanel )
	{
		m_pMoveableIcon = dynamic_cast<CIconPanel *>( m_pMoveableSubPanel->FindChildByName( "MoveableIcon" ) );
		m_pMoveableSymbolIcon = dynamic_cast<vgui::ImagePanel *>( m_pMoveableSubPanel->FindChildByName( "MoveableSymbolIcon" ) );
		m_pMoveableIconBG = dynamic_cast<CIconPanel *>( m_pMoveableSubPanel->FindChildByName( "MoveableIconBG" ) );
		m_pMoveableKeyLabel = dynamic_cast<Label *>( m_pMoveableSubPanel->FindChildByName( "MoveableKeyLabel" ) );
	}
	m_pAmmoIcon = dynamic_cast<ImagePanel *>( FindChildByName( "AmmoIcon" ) );
	m_pMoveableSubPanel = dynamic_cast<EditablePanel *>( FindChildByName( "MoveableSubPanel" ) );
	m_hFont = scheme->GetFont( "TargetID", true );

	int x, y;
	m_pBGPanel->GetPos( x, y );
	m_iBGPanelOriginalX = x;

	m_pTargetHealth->GetPos( x, y );
	m_iTargetHealthOriginalX = x;

	//SetPaintBackgroundEnabled( true );
	//SetBgColor( Color( 0, 0, 0, 90 ) );
}


void CTargetID::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_iRenderPriority = inResourceData->GetInt( "priority" );
}


int CTargetID::GetRenderGroupPriority( void )
{
	return m_iRenderPriority;
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CTargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
}


bool CTargetID::ShouldDraw( void )
{
	if ( !CHudElement::ShouldDraw() )
		return false;

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer || pLocalTFPlayer->GetObserverMode() == OBS_MODE_FREEZECAM )
		return false;

	// Get our target's ent index
	m_iTargetEntIndex = CalculateTargetIndex( pLocalTFPlayer );
	if ( !m_iTargetEntIndex )
	{
		// Check to see if we should clear our ID
		if ( m_flLastChangeTime && ( gpGlobals->curtime > m_flLastChangeTime ) )
		{
			m_flLastChangeTime = 0;
			m_iLastEntIndex = 0;
		}
		else
		{
			// Keep re-using the old one
			m_iTargetEntIndex = m_iLastEntIndex;
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;
	}

	CHudArenaClassLayout *pClassLayout = GET_HUDELEMENT( CHudArenaClassLayout );
	if ( pClassLayout && pClassLayout->IsVisible() )
		return false;

	// Hide Target ID while tranq'd.
	if ( pLocalTFPlayer->m_Shared.InCond( TF_COND_TRANQUILIZED ) )
		return false;

	bool bReturn = false;

	// Hide Target ID while tranq'd.
	if ( m_iTargetEntIndex && !pLocalTFPlayer->m_Shared.InCond( TF_COND_TRANQUILIZED ) )
	{
		C_BaseEntity *pEnt = cl_entitylist->GetEnt( m_iTargetEntIndex );
		if ( pEnt )
		{
			if ( IsPlayerIndex( m_iTargetEntIndex ) )
			{
				C_TFPlayer *pPlayer = static_cast<C_TFPlayer *>( pEnt );
				if ( !pPlayer->IsEnemyPlayer() )
				{
					bReturn = true;
				}
				else if ( pLocalTFPlayer->IsPlayerClass( TF_CLASS_SPY, true ) && !pPlayer->m_Shared.IsStealthed() ) // Spy can see enemy's health.
				{
					bReturn = true;
				}
				else if ( pPlayer->IsDisguisedEnemy( true ) && // they're disguised
					!pPlayer->m_Shared.InCond( TF_COND_DISGUISING ) && // they're not in the process of disguising
					!pPlayer->m_Shared.IsStealthed() // they're not cloaked
					)
				{
					bReturn = true;
				}
			}
			else if ( pEnt->IsBaseObject() &&
				( pEnt->InSameTeam( pLocalTFPlayer ) ||
				pLocalTFPlayer->GetTeamNumber() == TEAM_SPECTATOR ||
				pLocalTFPlayer->IsPlayerClass( TF_CLASS_SPY, true ) ) )
			{
				bReturn = true;
			}
		}
	}

	if ( bReturn )
	{
		if ( !IsVisible() || m_iTargetEntIndex != m_iLastEntIndex )
		{
			m_iLastEntIndex = m_iTargetEntIndex;
			m_bLayoutOnUpdate = true;
		}

		UpdateID();
	}

	return bReturn;
}


void CTargetID::PerformLayout( void )
{
	int iWidth = m_iTargetHealthOriginalX + m_pTargetHealth->GetWide() + YRES( 5 );
	int iBaseTextPos = iWidth;
	int iXOffset;

	if ( m_pMedalImage->IsVisible() )
	{
		// Here's the old 100% hardcoded way.
		/*int iHealthX, iHealthY;
		int iHealthW, iHealthH;
		m_pTargetHealth->GetPos( iHealthX, iHealthY );
		m_pTargetHealth->GetSize( iHealthW, iHealthH );

		m_pMedalImage->SetPos( 0, iHealthY + YRES( 2 ) );
		m_pMedalImage->SetZPos( m_pTargetHealth->GetZPos() + 1 );
		m_pMedalImage->SetSize( 64, iHealthH - YRES( 4 ) );
		m_pMedalImage->SetShouldScaleImage( true );
		iXOffset = XRES( 10 );*/
		iXOffset = m_iMedalXOffset;
	}
	else
	{
		iXOffset = 0;
	}

	int x, y;
	m_pTargetHealth->GetPos( x, y );
	m_pTargetHealth->SetPos( m_iTargetHealthOriginalX + iXOffset, y );

	if ( m_pAvatar && m_pAvatar->IsVisible() )
	{
		// Push the name further off to the right if avatar is on.
		m_pAvatar->SetPos( iBaseTextPos + iXOffset, m_pAvatar->GetYPos() );
		int iAvatarOffset = m_pAvatar->GetWide() + YRES( 2 );
		m_pTargetNameLabel->SetPos( ( iBaseTextPos + iAvatarOffset ) + iXOffset, m_pTargetNameLabel->GetYPos() );
		iWidth += iAvatarOffset;
	}
	else
	{
		// Place the name off to the right of health.
		m_pTargetNameLabel->SetPos( iBaseTextPos + iXOffset, m_pTargetNameLabel->GetYPos() );
	}

	m_pTargetDataLabel->SetPos( iBaseTextPos + iXOffset, m_pTargetDataLabel->GetYPos() );

	if ( m_pAmmoIcon )
	{
		m_pAmmoIcon->SetPos( iBaseTextPos + iXOffset, m_pAmmoIcon->GetYPos() );
	}

	int iTextW, iTextH;
	int iDataW, iDataH;
	m_pTargetNameLabel->GetContentSize( iTextW, iTextH );
	m_pTargetDataLabel->GetContentSize( iDataW, iDataH );
	iWidth += Max( iTextW, iDataW ) + YRES( 10 );

	// Put the moveable icon to the right hand of our panel
	if ( m_pMoveableSubPanel && m_pMoveableSubPanel->IsVisible() )
	{
		if ( m_pMoveableKeyLabel && m_pMoveableIcon && m_pMoveableSymbolIcon && m_pMoveableIconBG )
		{
			m_pMoveableKeyLabel->SizeToContents();

			int iIndent = XRES( 4 );
			int iMoveWide = MAX( XRES( 16 ) + m_pMoveableKeyLabel->GetWide() + iIndent, ( m_pMoveableIcon->GetWide() ) + iIndent + XRES( 8 ) );
			m_pMoveableKeyLabel->SetWide( iMoveWide );
			m_pMoveableSubPanel->SetSize( iMoveWide, GetTall() );
			m_pMoveableSubPanel->SetPos( ( iWidth - iIndent ) + iXOffset, 0 );

			m_pMoveableKeyLabel->GetPos( x, y );
			m_pMoveableSymbolIcon->SetPos( ( iMoveWide - m_pMoveableSymbolIcon->GetWide() ) * 0.5f, y - m_pMoveableSymbolIcon->GetTall() );
			m_pMoveableSymbolIcon->GetPos( x, y );
			m_pMoveableIcon->SetPos( ( iMoveWide - m_pMoveableIcon->GetWide() ) * 0.5f, y - m_pMoveableIcon->GetTall() );
			m_pMoveableIconBG->SetSize( m_pMoveableSubPanel->GetWide(), m_pMoveableSubPanel->GetTall() );
		}

		// Now add our extra width to the total size
		iWidth += m_pMoveableSubPanel->GetWide();
	}

	SetWide( iWidth + iXOffset );
	SetPos( ( ( ScreenWidth() - iWidth ) * 0.5f ) - iXOffset, GetYPos() );

	if ( m_pBGPanel )
	{
		m_pBGPanel->GetPos( x, y );
		m_pBGPanel->SetPos( m_iBGPanelOriginalX + iXOffset, y );
		m_pBGPanel->SetSize( iWidth - ( m_pMoveableSubPanel && m_pMoveableSubPanel->IsVisible() ? m_pMoveableSubPanel->GetWide() : 0 ), GetTall() );
		m_pBGPanel->SetAlpha( tf_hud_target_id_alpha.GetFloat() );
	}
}


int	CTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer ) 
{ 
	int iIndex = pLocalTFPlayer->GetIDTarget();

	// If our target entity is already in our secondary ID, don't show it in primary.
	CSecondaryTargetID *pSecondaryID = GET_HUDELEMENT( CSecondaryTargetID );
	if ( pSecondaryID && pSecondaryID != this &&
		pSecondaryID->IsVisible() &&
		pSecondaryID->GetTargetIndex() == iIndex )
		return 0;

	return iIndex;
}


void CTargetID::UpdateID( void )
{
	wchar_t sIDString[MAX_ID_STRING] = L"";
	wchar_t sDataString[MAX_ID_STRING] = L"";

	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return;

	if ( !g_PR )
		return;

	// Get our target's ent index
	Assert( m_iTargetEntIndex );

	// Is this an entindex sent by the server?
	if ( !m_iTargetEntIndex )
		return;

	C_BaseEntity *pEnt = ClientEntityList().GetBaseEntity( m_iTargetEntIndex );
	if ( !pEnt )
		return;

	int iHealth = 0;
	int iMaxHealth = 1;
	int iMaxBuffedHealth = 0;
	int iColorNum = TEAM_UNASSIGNED;
	bool bShowAmmo = false;
	const char *pszActionCommand = NULL;
	const char *pszActionIcon = NULL;
	int iTargetEntIndex = m_iTargetEntIndex;

	// Some entities we always want to check, cause the text may change
	// even while we're looking at it
	// Is it a player?
	if ( IsPlayerIndex( iTargetEntIndex ) )
	{
		C_TFPlayer *pPlayer = static_cast<C_TFPlayer *>( pEnt );
		if ( !pPlayer )
			return;

		const char *printFormatString = NULL;
		wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
		bool bDisguisedEnemy = false;

		// If this is a disguised enemy spy, change their identity and color.
		if ( pPlayer->IsDisguisedEnemy() )
		{
			bDisguisedEnemy = true;

			iTargetEntIndex = pPlayer->m_Shared.GetDisguiseTargetIndex();
			g_pVGuiLocalize->ConvertANSIToUnicode( g_PR->GetPlayerName( iTargetEntIndex ), wszPlayerName, sizeof( wszPlayerName ) );
			iColorNum = pPlayer->m_Shared.GetDisguiseTeam();
		}
		else
		{
			// Get their name, team color and avatar.
			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(), wszPlayerName, sizeof( wszPlayerName ) );

			iColorNum = pPlayer->GetTeamNumber();
		}

		pPlayer->GetTargetIDDataString( sDataString, sizeof( sDataString ), bShowAmmo );

		if ( !pPlayer->IsEnemyPlayer() || pPlayer->IsDisguisedEnemy( true ) )
		{
			printFormatString = "#TF_playerid_sameteam";
		}
		else
		{
			// Spy can see enemy's health.
			printFormatString = "#TF_playerid_diffteam";
		}

		if ( bDisguisedEnemy )
		{
			iHealth = pPlayer->m_Shared.GetDisguiseHealth();
			iMaxHealth = pPlayer->m_Shared.GetDisguiseMaxHealth();
			iMaxBuffedHealth = pPlayer->m_Shared.GetDisguiseMaxBuffedHealth();
		}
		else
		{
			iHealth = pPlayer->GetHealth();
			iMaxHealth = g_TF_PR->GetMaxHealth( iTargetEntIndex );
			iMaxBuffedHealth = pPlayer->m_Shared.GetMaxBuffedHealth();
		}

		const wchar_t *pszPrepend = GetPrepend();
		if ( !pszPrepend || !pszPrepend[0] )
		{
			pszPrepend = L"";
		}
		g_pVGuiLocalize->ConstructString( sIDString, sizeof( sIDString ), g_pVGuiLocalize->Find( printFormatString ), 2, pszPrepend, wszPlayerName );
	}
	else if ( pEnt->IsBaseObject() ) // see if it is an object
	{
		C_BaseObject *pObj = assert_cast<C_BaseObject *>( pEnt );
		pObj->GetTargetIDString( sIDString, sizeof( sIDString ) );
		pObj->GetTargetIDDataString( sDataString, sizeof( sDataString ) );
		iHealth = pObj->GetHealth();
		iMaxHealth = pObj->GetMaxHealth();

		iTargetEntIndex = pObj->GetBuilderIndex();
		iColorNum = IsPlayerIndex( iTargetEntIndex ) ? g_PR->GetTeam( iTargetEntIndex ) : pObj->GetTeamNumber();

		// Switch the icon to the right object
		if ( pObj->GetBuilder() == pLocalTFPlayer )
		{
			int iObj = pObj->GetType();
			if ((iObj >= OBJ_DISPENSER && iObj <= OBJ_SENTRYGUN) || iObj == OBJ_JUMPPAD || iObj == OBJ_MINIDISPENSER || iObj == OBJ_FLAMESENTRY)
			{
				if ( pLocalTFPlayer->CanPickupBuilding( pObj ) && pLocalTFPlayer->IsAlive() )
				{
					pszActionCommand = "+attack2";
				}
						
				switch ( iObj )
				{
					default:
					case OBJ_MINIDISPENSER:
					case OBJ_DISPENSER:
						pszActionIcon = "obj_status_dispenser";
						break;
					case OBJ_TELEPORTER:
					{
						pszActionIcon = pObj->GetObjectMode() == TELEPORTER_TYPE_ENTRANCE ? "obj_status_tele_entrance" : "obj_status_tele_exit";
						break;
					}
					case OBJ_FLAMESENTRY:
					case OBJ_SENTRYGUN:
					{
						int iLevel = pObj->GetUpgradeLevel();
						pszActionIcon = iLevel == 3 ? "obj_status_sentrygun_3" : iLevel == 2 ? "obj_status_sentrygun_2" : "obj_status_sentrygun_1";
						break;
					}
					case OBJ_JUMPPAD:
						pszActionIcon = "obj_status_jumppad";
						break;
				}
			}
		}
	}

	// Setup health icon
	if ( !pEnt->IsAlive() )
	{
		iHealth = 0;	// fixup for health being 1 when dead
	}

	bool bStreamerMode = tf2c_streamer_mode.GetBool();
	bool bShowMedal = m_pMedalImage->IsVisible();

	const char *pszMedalImage = GetTeamMedalString( iColorNum, g_TF_PR->GetPlayerRank( iTargetEntIndex ) );
	if ( pszMedalImage && !bStreamerMode )
	{
		m_pMedalImage->SetImage( pszMedalImage );
		m_pMedalImage->SetVisible( true );
	}
	else
	{
		m_pMedalImage->SetVisible( false );
	}

	if ( m_pMedalImage->IsVisible() != bShowMedal )
	{
		m_bLayoutOnUpdate = true;
	}

	m_pTargetHealth->SetHealth( iHealth, iMaxHealth, iMaxBuffedHealth );
	m_pTargetHealth->ShowBuildingBG( pEnt->IsBaseObject() );
	m_pTargetHealth->SetVisible( true );

	if ( m_pMoveableSubPanel )
	{
		bool bShowActionKey = pszActionCommand != NULL;
		if ( m_pMoveableSubPanel->IsVisible() != bShowActionKey )
		{
			m_pMoveableSubPanel->SetVisible( bShowActionKey );
			m_bLayoutOnUpdate = true;
		}

		if ( m_pMoveableSubPanel->IsVisible() )
		{
			const char *pszBoundKey = engine->Key_LookupBinding( pszActionCommand );
			m_pMoveableSubPanel->SetDialogVariable( "movekey", pszBoundKey );
		}

		if ( m_pMoveableIcon )
		{
			if ( pszActionIcon )
			{
				m_pMoveableIcon->SetIcon( pszActionIcon );
			}

			m_pMoveableIcon->SetVisible( pszActionIcon != NULL );
		}
	}

	m_pBGPanel->SetBGImage( iColorNum );

	if ( m_pAvatar )
	{
		bool bShowAvatar = !cl_hud_minmode.GetBool() && ( tf_hud_target_id_show_avatars.GetBool() && iTargetEntIndex );
		if ( !bStreamerMode && bShowAvatar && tf_hud_target_id_show_avatars.GetInt() == 2 )
		{
			CSteamID steamID;
			if ( g_TF_PR->GetSteamID( iTargetEntIndex, &steamID ) )
			{
				bShowAvatar = steamapicontext->SteamFriends()->GetFriendRelationship( steamID ) == k_EFriendRelationshipFriend;
			}
			else
			{
				bShowAvatar = false;
			}
		}

		if ( m_pAvatar->IsVisible() != bShowAvatar )
		{
			m_pAvatar->SetVisible( bShowAvatar );
			m_bLayoutOnUpdate = true;
		}

		// Only update avatar if we're switching to a different player.
		if ( m_pAvatar->IsVisible() && m_bLayoutOnUpdate )
		{
			// Edge case where we're using global disguises and someone has streamer mode/is a bot
			// Normally it could show "The Green" or "The Red" in the avatar due to disguising as another team
			// Here we fix that by using the local player's default avatar just in case - Kay
			m_pAvatar->SetDefaultAvatar( 
			GetDefaultAvatarImage( iColorNum == GetLocalPlayerTeam() ? C_TFPlayer::GetLocalTFPlayer()->entindex() : iTargetEntIndex ) );
			m_pAvatar->SetPlayer( !bStreamerMode ? iTargetEntIndex : 0 );
			m_pAvatar->SetShouldDrawFriendIcon( false );
		}
	}

	if ( m_pAmmoIcon )
	{
		m_pAmmoIcon->SetVisible( bShowAmmo );
	}

	int iNameW, iDataW, iIgnored;
	m_pTargetNameLabel->GetContentSize( iNameW, iIgnored );
	m_pTargetDataLabel->GetContentSize( iDataW, iIgnored );

	// Target name
	if ( sIDString[0] )
	{
		sIDString[ARRAYSIZE( sIDString ) - 1] = '\0';
		m_pTargetNameLabel->SetVisible( true );

		// TODO: Support	if( hud_centerid.GetInt() == 0 )
		SetDialogVariable( "targetname", sIDString );
	}
	else
	{
		m_pTargetNameLabel->SetVisible( false );
		m_pTargetNameLabel->SetText( "" );
	}

	// Extra target data
	if ( sDataString[0] )
	{
		sDataString[ARRAYSIZE( sDataString ) - 1] = '\0';
		m_pTargetDataLabel->SetVisible( true );
		SetDialogVariable( "targetdata", sDataString );
	}
	else
	{
		m_pTargetDataLabel->SetVisible( false );
		m_pTargetDataLabel->SetText( "" );
	}

	int iPostNameW, iPostDataW;
	m_pTargetNameLabel->GetContentSize( iPostNameW, iIgnored );
	m_pTargetDataLabel->GetContentSize( iPostDataW, iIgnored );

	if ( m_bLayoutOnUpdate || iPostDataW != iDataW || iPostNameW != iNameW )
	{
		InvalidateLayout( true );
		m_bLayoutOnUpdate = false;
	}
}


CSecondaryTargetID::CSecondaryTargetID( const char *pElementName ) : CTargetID( pElementName )
{
	m_wszPrepend[0] = '\0';

	RegisterForRenderGroup( "mid" );

	m_bWasHidingLowerElements = false;
}


bool CSecondaryTargetID::ShouldDraw( void )
{
	bool bDraw = BaseClass::ShouldDraw();
	if ( bDraw )
	{
		if ( !m_bWasHidingLowerElements )
		{
			HideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = true;
		}
	}
	else 
	{
		if ( m_bWasHidingLowerElements )
		{
			UnhideLowerPriorityHudElementsInGroup( "mid" );
			m_bWasHidingLowerElements = false;
		}
	}

	return bDraw;
}


int CSecondaryTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer )
{
	C_TFPlayer *pTargetTFPlayer = pLocalTFPlayer;

	// If we're spectating, get info from them instead.
	int iObserverMode = pLocalTFPlayer->GetObserverMode();
	if ( iObserverMode == OBS_MODE_DEATHCAM || 
		iObserverMode == OBS_MODE_IN_EYE || 
		iObserverMode == OBS_MODE_CHASE )
	{
		C_BaseEntity *pObserverTarget = pLocalTFPlayer->GetObserverTarget();
		if ( pObserverTarget && pObserverTarget != pLocalTFPlayer )
		{
			pTargetTFPlayer = dynamic_cast<C_TFPlayer *>( pObserverTarget );
		}
	}

	if ( pTargetTFPlayer )
	{
		// If we're a Medic & we're healing someone, target him.
		CBaseEntity *pHealTarget = pTargetTFPlayer->MedicGetHealTarget();
		if ( pHealTarget )
		{
			if ( pHealTarget->entindex() != m_iTargetEntIndex )
			{
				g_pVGuiLocalize->ConstructString( m_wszPrepend, sizeof( m_wszPrepend ), g_pVGuiLocalize->Find( "#TF_playerid_healtarget" ), 0 );
			}

			return pHealTarget->entindex();
		}

		// If we have a healer, target him.
		C_TFPlayer *pHealer;
		float flHealerChargeLevel;
		pTargetTFPlayer->GetHealer( &pHealer, &flHealerChargeLevel );
		if ( pHealer )
		{
			if ( pHealer->entindex() != m_iTargetEntIndex )
			{
				g_pVGuiLocalize->ConstructString( m_wszPrepend, sizeof( m_wszPrepend ), g_pVGuiLocalize->Find( "#TF_playerid_healer" ), 0 );
			}

			return pHealer->entindex();
		}
	}

	if ( m_iTargetEntIndex )
	{
		m_wszPrepend[0] = '\0';
	}

	return 0;
}

// Separately declared versions of the hud element for alive and dead so they
// can have different positions

bool CMainTargetID::ShouldDraw( void )
{
	return BaseClass::ShouldDraw();
}

void CSpectatorTargetID::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_iOriginalY = GetYPos();
}

bool CSpectatorTargetID::ShouldDraw( void )
{
	C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalTFPlayer )
		return false;

	if ( pLocalTFPlayer->GetObserverMode() <= OBS_MODE_FREEZECAM )
		return false;

	return BaseClass::ShouldDraw();
}

int	CSpectatorTargetID::CalculateTargetIndex( C_TFPlayer *pLocalTFPlayer ) 
{
	// If we're spectating someone, then ID them.
	int iObserverMode = pLocalTFPlayer->GetObserverMode();
	if ( iObserverMode == OBS_MODE_DEATHCAM || 
		iObserverMode == OBS_MODE_IN_EYE || 
		iObserverMode == OBS_MODE_CHASE )
	{
		C_BaseEntity *pObserverTarget = pLocalTFPlayer->GetObserverTarget();
		if ( pObserverTarget && pObserverTarget != pLocalTFPlayer )
		{
			return pObserverTarget->entindex();
		}
	}

	return 0;
}

void CSpectatorTargetID::PerformLayout( void )
{
	BaseClass::PerformLayout();

	if ( g_pSpectatorGUI )
	{
		switch ( tf_spectator_target_location.GetInt() )
		{
			case 1:
				// Left
				SetPos( m_iXOffset, ScreenHeight() - g_pSpectatorGUI->GetBottomBarHeight() - m_iYOffset - GetTall() );
				break;
			case 2:
				// Center
				SetPos( ( ScreenWidth() - GetWide() ) * 0.5, ScreenHeight() - g_pSpectatorGUI->GetBottomBarHeight() - m_iYOffset - GetTall() );
				break;
			case 3:
				// Right
				SetPos( ScreenWidth() - GetWide() - m_iXOffset, ScreenHeight() - g_pSpectatorGUI->GetBottomBarHeight() - m_iYOffset - GetTall() );
				break;
			default:
				SetPos( ( ScreenWidth() - GetWide() ) * 0.5, m_iOriginalY );
				break;
		}
	}
}
