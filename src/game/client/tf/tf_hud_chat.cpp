//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_hud_chat.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "vgui/ILocalize.h"
#include "engine/IEngineSound.h"
#include "c_tf_team.h"
#include "c_playerresource.h"
#include "c_tf_playerresource.h"
#include "c_tf_player.h"
#include "tf_gamerules.h"
// #include "ihudlcd.h"
#include "tf_hud_freezepanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudChat );
DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUD_MESSAGE( CHudChat, SayText2 );
DECLARE_HUD_MESSAGE( CHudChat, TextMsg );
DECLARE_HUD_MESSAGE( CHudChat, VoiceSubtitle );


//=====================
//CHudChatLine
//=====================

void CHudChatLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_hFont = pScheme->GetFont( "ChatFont" );
	SetBorder( NULL );
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetFgColor( Color( 0, 0, 0, 0 ) );

	SetFont( m_hFont );
}

CHudChatLine::CHudChatLine( vgui::Panel *parent, const char *panelName ) : CBaseHudChatLine( parent, panelName )
{
	m_text = NULL;
}


//=====================
//CHudChatInputLine
//=====================

void CHudChatInputLine::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
}


//=====================
//CHudChat
//=====================

static CHudChat *g_pTFChatHud = NULL;
CHudChat *GetTFChatHud( void )
{
	return g_pTFChatHud;
}

CHudChat::CHudChat( const char *pElementName ) : BaseClass( pElementName )
{
#if defined ( _X360 )
	RegisterForRenderGroup( "mid" );
#endif

	g_pTFChatHud = this;
}

void CHudChat::CreateChatInputLine( void )
{
	m_pChatInput = new CHudChatInputLine( this, "ChatInputLine" );
	m_pChatInput->SetVisible( false );
}

void CHudChat::CreateChatLines( void )
{
#ifndef _XBOX
	m_ChatLine = new CHudChatLine( this, "ChatLine1" );
	m_ChatLine->SetVisible( false );		

#endif
}

void CHudChat::Init( void )
{
	BaseClass::Init();

	HOOK_HUD_MESSAGE( CHudChat, SayText );
	HOOK_HUD_MESSAGE( CHudChat, SayText2 );
	HOOK_HUD_MESSAGE( CHudChat, TextMsg );
	HOOK_HUD_MESSAGE( CHudChat, VoiceSubtitle );
}


void CHudChat::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_clrRedTeam = pScheme->GetColor( "TFColors.ChatTextTeamRed", g_ColorRed );
	m_clrBlueTeam = pScheme->GetColor( "TFColors.ChatTextTeamBlue", g_ColorBlue );
	m_clrGreenTeam = pScheme->GetColor( "TFColors.ChatTextTeamGreen", g_ColorGreen );
	m_clrYellowTeam = pScheme->GetColor( "TFColors.ChatTextTeamYellow", g_ColorYellow );
}

//-----------------------------------------------------------------------------
// Purpose: Hides us when our render group is hidden
// move render group to base chat when its safer
//-----------------------------------------------------------------------------
bool CHudChat::ShouldDraw( void )
{
	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Overrides base reset to not cancel chat at round restart
//-----------------------------------------------------------------------------
void CHudChat::Reset( void )
{
}


int CHudChat::GetChatInputOffset( void )
{
	if ( m_pChatInput->IsVisible() )
	{
		return m_iFontHeight;
	}
	else
	{
		return 0;
	}
}


int CHudChat::GetFilterForString( const char *pString )
{
	int iFilter = BaseClass::GetFilterForString( pString );

	if ( iFilter == CHAT_FILTER_NONE )
	{
		if ( !Q_stricmp( pString, "#TF_Name_Change" ) ) 
		{
			return CHAT_FILTER_NAMECHANGE;
		}
	}

	return iFilter;
}


Color CHudChat::GetClientColor( int clientIndex )
{
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	if ( pScheme == NULL )
		return Color( 255, 255, 255, 255 );

	if ( clientIndex == 0 ) // console msg
	{
		return g_ColorGreen;
	}
	else if ( g_TF_PR )
	{
		int iTeam = g_TF_PR->GetTeam( clientIndex );

		if ( IsVoiceSubtitle() )
		{
			// If this player is on the other team but disguised as my own team, show his disguised color.
			C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( clientIndex ) );
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pPlayer && pLocalPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) &&
				pPlayer->m_Shared.GetDisguiseTeam() == pLocalPlayer->GetTeamNumber() )
			{
				iTeam = pPlayer->m_Shared.GetDisguiseTeam();
			}
		}

		return GetTeamColor( iTeam );
	}

	return g_ColorYellow;
}


Color CHudChat::GetTeamColor( int iTeam )
{
	switch ( iTeam )
	{
		case TF_TEAM_RED:
			return m_clrRedTeam;
		case TF_TEAM_BLUE:
			return m_clrBlueTeam;
		case TF_TEAM_GREEN:
			return m_clrGreenTeam;
		case TF_TEAM_YELLOW:
			return m_clrYellowTeam;
	}

	return g_ColorGrey;
}


bool CHudChat::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}


const char *CHudChat::GetDisplayedSubtitlePlayerName( int clientIndex )
{
	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( clientIndex ) );
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || !pLocalPlayer )
		return BaseClass::GetDisplayedSubtitlePlayerName( clientIndex );

	// If they are disguised as the enemy, and not on our team
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) &&
		pPlayer->m_Shared.GetDisguiseTeam() != pPlayer->GetTeamNumber() && 
		!pLocalPlayer->InSameTeam( pPlayer ) )
	{
		C_TFPlayer *pDisguiseTarget = ToTFPlayer( pPlayer->m_Shared.GetDisguiseTarget() );
		if ( !pDisguiseTarget )
			return BaseClass::GetDisplayedSubtitlePlayerName( clientIndex );

		return pDisguiseTarget->GetPlayerName();
	}

	return BaseClass::GetDisplayedSubtitlePlayerName( clientIndex );
}


Color CHudChat::GetTextColorForClient( TextColor colorNum, int clientIndex )
{
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	if ( pScheme == NULL )
		return Color( 255, 255, 255, 255 );

	Color c;
	switch ( colorNum )
	{
	case COLOR_CUSTOM:
		c = m_ColorCustom;
		break;

	case COLOR_PLAYERNAME:
		c = GetClientColor( clientIndex );
		break;

	case COLOR_LOCATION:
		c = g_ColorDarkGreen;
		break;

	case COLOR_ACHIEVEMENT:
		{
			IScheme *pSourceScheme = scheme()->GetIScheme( scheme()->GetScheme( ClientSchemesArray[SCHEME_SOURCE_STRING] ) ); 
			if ( pSourceScheme )
			{
				c = pSourceScheme->GetColor( "SteamLightGreen", GetBgColor() );
			}
			else
			{
				c = pScheme->GetColor( "TFColors.ChatTextYellow", GetBgColor() );
			}
		}
		break;

	default:
		c = pScheme->GetColor( "TFColors.ChatTextYellow", GetBgColor() );
	}

	return Color( c[0], c[1], c[2], 255 );
}