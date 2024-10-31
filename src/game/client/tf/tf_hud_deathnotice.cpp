//=============================================================================//
//
// Purpose: TF2 Killfeed HUD
//
//=============================================================================//
#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "c_playerresource.h"
#include "iclientmode.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <KeyValues.h>
#include "c_baseplayer.h"
#include "c_team.h"
#include "c_tf_team.h"

#include "tf_shareddefs.h"
#include "clientmode_tf.h"
#include "c_tf_player.h"
#include "c_tf_playerresource.h"
#include "tf_hud_freezepanel.h"
#include "engine/IEngineSound.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Player entries in a death notice
struct DeathNoticePlayer
{
	DeathNoticePlayer()
	{
		szName[0] = 0;
		iTeam = TEAM_UNASSIGNED;
	}

	char		szName[MAX_PLAYER_NAME_LENGTH * 2];	// big enough for player name and additional information
	int			iTeam;								// team #	
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem
{
	DeathNoticeItem()
	{
		szIcon[0] = 0;
		wzInfoText[0] = 0;
		iconDeath = NULL;
		bSelfInflicted = false;
		flCreationTime = 0;
		bLocalPlayerInvolved = false;
		iCritType = CTakeDamageInfo::CRIT_NONE;
	}

	float GetExpiryTime();

	DeathNoticePlayer	Killer;
	DeathNoticePlayer   Victim;
	DeathNoticePlayer   Assister;
	char		szIcon[32];		// name of icon to display
	wchar_t		wzInfoText[64];	// any additional text to display next to icon
	CHudTexture *iconDeath;
	CHudTexture *iconCritDeath;	// crit background icon

	bool		bSelfInflicted;
	float		flCreationTime;
	bool		bLocalPlayerInvolved;
	int			iCritType;
};

// Must match resource/tf_objects.txt!!!
const char *szLocalizedObjectNames[OBJ_LAST] =
{
	"#TF_Object_Dispenser",
	"#TF_Object_Tele",
	"#TF_Object_Sentry",
	"#TF_object_sapper",
	"#TF_Object_Jump",
	"#TF_Object_Dispenser",
};

#define NUM_CORNER_COORD 10
#define NUM_BACKGROUND_COORD NUM_CORNER_COORD*4

class CTFHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTFHudDeathNotice, vgui::Panel );
public:
	CTFHudDeathNotice( const char *pElementName );

	void VidInit( void );
	virtual void Init( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool IsVisible( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );

	void RetireExpiredDeathNotices( void );

	virtual void FireGameEvent( IGameEvent *event );

	void PlayRivalrySounds( int iKillerIndex, int iVictimIndex, int iType );
	virtual Color GetInfoTextColor( int iDeathNoticeMsg, bool bLocalPlayerInvolved ) { return bLocalPlayerInvolved ? COLOR_BLACK : COLOR_WHITE; }

protected:
	virtual Color GetPlayerColor( const DeathNoticePlayer &playerItem, bool bLocalPlayerInvolved = false );
	void DrawText( int x, int y, vgui::HFont hFont, Color clr, const wchar_t *szText );
	int AddDeathNoticeItem();
	void GetBackgroundPolygonVerts( int x0, int y0, int x1, int y1, int iVerts, vgui::Vertex_t vert[] );
	void CalcRoundedCorners();
	CHudTexture *GetIcon( const char *szIcon, bool bInvert );

	void GetLocalizedControlPointName( IGameEvent *event, char *namebuf, int namelen );

private:
	void AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey, bool bDomination = true );

	CHudTexture		*m_iconDomination;

	CPanelAnimationVar( Color, m_clrBlueText, "TeamBlue", "153 204 255 255" );
	CPanelAnimationVar( Color, m_clrRedText, "TeamRed", "255 64 64 255" );
	CPanelAnimationVar( Color, m_clrGreenText, "TeamGreen", "8 174 0 255" );
	CPanelAnimationVar( Color, m_clrYellowText, "TeamYellow", "255 160 0 255" );

	CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLineSpacing, "LineSpacing", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flCornerRadius, "CornerRadius", "3", "proportional_float" );
	CPanelAnimationVar( float, m_flMaxDeathNotices, "MaxDeathNotices", "4" );
	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( Color, m_clrIcon, "IconColor", "255 80 0 255" );
	CPanelAnimationVar( Color, m_clrBaseBGColor, "BaseBackgroundColor", "46 43 42 220" );
	CPanelAnimationVar( Color, m_clrLocalBGColor, "LocalBackgroundColor", "245 229 196 200" );

	CUtlVector<DeathNoticeItem> m_DeathNotices;

	Vector2D	m_CornerCoord[NUM_CORNER_COORD];
};

using namespace vgui;

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6" );
extern ConVar tf2c_show_nemesis_relationships;

DECLARE_HUDELEMENT( CTFHudDeathNotice );


CTFHudDeathNotice::CTFHudDeathNotice( const char *pElementName ) :
CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	// This is needed for custom colors.
	SetScheme( vgui::scheme()->LoadSchemeFromFile( ClientSchemesArray[SCHEME_CLIENT_PATHSTRINGTF2C], ClientSchemesArray[SCHEME_CLIENT_STRINGTF2C] ) );
}


void CTFHudDeathNotice::Init( void )
{
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "object_destroyed" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );
	ListenForGameEvent( "teamplay_flag_event" );
	ListenForGameEvent( "mirv_defused" );
	ListenForGameEvent( "vip_death" );
}


void CTFHudDeathNotice::VidInit( void )
{
	m_DeathNotices.RemoveAll();
}


void CTFHudDeathNotice::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );

	CalcRoundedCorners();

	m_iconDomination = gHUD.GetIcon( "leaderboard_dominated" );
}


bool CTFHudDeathNotice::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CTFHudDeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}


void CTFHudDeathNotice::Paint()
{
	// Retire any death notices that have expired
	RetireExpiredDeathNotices();

	CBaseViewport *pViewport = dynamic_cast<CBaseViewport *>( GetClientModeNormal()->GetViewport() );
	int yStart = pViewport->GetDeathMessageStartHeight();

	surface()->DrawSetTextFont( m_hTextFont );

	int xMargin = YRES( 10 );
	int xSpacing = UTIL_ComputeStringWidth( m_hTextFont, L" " );

	int iCount = m_DeathNotices.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		const DeathNoticeItem &msg = m_DeathNotices[i];

		CHudTexture *icon = msg.iconDeath;

		wchar_t victim[256] = L"";
		wchar_t killer[256] = L"";
		wchar_t assister[256] = L"";

		// TEMP - print the death icon name if we don't have a material for it

		g_pVGuiLocalize->ConvertANSIToUnicode( msg.Victim.szName, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( msg.Killer.szName, killer, sizeof( killer ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( msg.Assister.szName, assister, sizeof( assister ) );

		int iVictimTextWide = UTIL_ComputeStringWidth( m_hTextFont, victim ) + xSpacing;
		int iDeathInfoTextWide = msg.wzInfoText[0] ? UTIL_ComputeStringWidth( m_hTextFont, msg.wzInfoText ) + xSpacing : 0;
		int iKillerTextWide = killer[0] ? UTIL_ComputeStringWidth( m_hTextFont, killer ) + xSpacing : 0;
		int iPlusIconWide = assister[0] ? UTIL_ComputeStringWidth( m_hTextFont, "+" ) + xSpacing : 0;
		int iAssisterTextWide = assister[0] ? UTIL_ComputeStringWidth( m_hTextFont, assister ) + xSpacing : 0;
		int iLineTall = m_flLineHeight;
		int iTextTall = surface()->GetFontTall( m_hTextFont );
		int iconWide = 0, iconTall = 0, iDeathInfoOffset = 0, iVictimTextOffset = 0, iconActualWide = 0;

		// Get the local position for this notice
		if ( icon )
		{
			iconActualWide = icon->EffectiveWidth( 1.0f );
			iconWide = iconActualWide + xSpacing;
			iconTall = icon->EffectiveHeight( 1.0f );

			int iconTallDesired = iLineTall - YRES( 2 );
			Assert( 0 != iconTallDesired );
			float flScale = (float)iconTallDesired / (float)iconTall;

			iconActualWide *= flScale;
			iconTall *= flScale;
			iconWide *= flScale;
		}
		int iTotalWide = iKillerTextWide + iPlusIconWide + iAssisterTextWide + iconWide + iVictimTextWide + iDeathInfoTextWide + ( xMargin * 2 );
		int y = yStart + ( ( iLineTall + m_flLineSpacing ) * i );
		int yText = y + ( ( iLineTall - iTextTall ) / 2 );
		int yIcon = y + ( ( iLineTall - iconTall ) / 2 );

		int x = 0;
		if ( m_bRightJustify )
		{
			x = GetWide() - iTotalWide;
		}

		// draw a background panel for the message
		Vertex_t vert[NUM_BACKGROUND_COORD];
		GetBackgroundPolygonVerts( x, y + 1, x + iTotalWide, y + iLineTall - 1, ARRAYSIZE( vert ), vert );
		surface()->DrawSetTexture( -1 );
		surface()->DrawSetColor( msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor );
		surface()->DrawTexturedPolygon( ARRAYSIZE( vert ), vert );

		x += xMargin;

		if ( killer[0] )
		{
			// Draw killer's name
			DrawText( x, yText, m_hTextFont, GetPlayerColor( msg.Killer, msg.bLocalPlayerInvolved ), killer );
			x += iKillerTextWide;
		}

		if ( assister[0] )
		{
			// Draw a + between the names
			DrawText( x, yText, m_hTextFont, GetInfoTextColor( i, msg.bLocalPlayerInvolved ), L"+" );
			x += iPlusIconWide;

			// Draw assister's name
			DrawText( x, yText, m_hTextFont, GetPlayerColor( msg.Assister, msg.bLocalPlayerInvolved ), assister );
			x += iAssisterTextWide;
		}

		// Draw glow behind weapon icon to show it was a crit death
		if ( msg.iCritType > CTakeDamageInfo::CRIT_NONE && msg.iconCritDeath )
		{
			msg.iconCritDeath->DrawSelf( x, yIcon, iconActualWide, iconTall, m_clrIcon );
		}

		// Draw death icon
		if ( icon )
		{
			icon->DrawSelf( x, yIcon, iconActualWide, iconTall, m_clrIcon );
			x += iconWide;
		}

		// Draw additional info text next to death icon 
		if ( msg.wzInfoText[0] )
		{
			if ( msg.bSelfInflicted )
			{
				iDeathInfoOffset += iVictimTextWide;
				iVictimTextOffset -= iDeathInfoTextWide;
			}

			DrawText( x + iDeathInfoOffset, yText, m_hTextFont, GetInfoTextColor( i, msg.bLocalPlayerInvolved ), msg.wzInfoText );
			x += iDeathInfoTextWide;
		}

		// Draw victims name
		DrawText( x + iVictimTextOffset, yText, m_hTextFont, GetPlayerColor( msg.Victim, msg.bLocalPlayerInvolved ), victim );
		x += iVictimTextWide;
	}
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::RetireExpiredDeathNotices()
{
	// Remove any expired death notices.  Loop backwards because we might remove one
	int iCount = m_DeathNotices.Count();
	for ( int i = iCount - 1; i >= 0; i-- )
	{
		if ( gpGlobals->curtime > m_DeathNotices[i].GetExpiryTime() )
		{
			m_DeathNotices.Remove( i );
		}
	}

	// Do we have too many death messages in the queue?
	if ( m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() > (int)m_flMaxDeathNotices )
	{
		// First, remove any notices not involving the local player, since they are lower priority.		
		iCount = m_DeathNotices.Count();
		int iNeedToRemove = iCount - (int)m_flMaxDeathNotices;
		// loop condition is iCount-1 because we won't remove the most recent death notice, otherwise
		// new non-local-player-involved messages would not appear if the queue was full of messages involving the local player
		for ( int i = 0; i < iCount - 1 && iNeedToRemove > 0; i++ )
		{
			if ( !m_DeathNotices[i].bLocalPlayerInvolved )
			{
				m_DeathNotices.Remove( i );
				iCount--;
				iNeedToRemove--;
			}
		}

		// Now that we've culled any non-local-player-involved messages up to the amount we needed to remove, see
		// if we've removed enough
		iCount = m_DeathNotices.Count();
		iNeedToRemove = iCount - (int)m_flMaxDeathNotices;
		if ( iNeedToRemove > 0 )
		{
			// if we still have too many messages, then just remove however many we need, oldest first
			for ( int i = 0; i < iNeedToRemove; i++ )
			{
				m_DeathNotices.Remove( 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::FireGameEvent( IGameEvent *event )
{
	if ( !g_TF_PR )
		return;

	if ( hud_deathnotice_time.GetFloat() <= 0 )
		return;

	const char *pszEventName = event->GetName();

	// Add a new death message. NOTE: We always look it up by index rather than create a reference or pointer to it;
	// additional messages may get added during this function that cause the underlying array to get realloced, so don't
	// ever keep a pointer to memory here.
	int iMsg = AddDeathNoticeItem();
	int iLocalPlayerIndex = GetLocalPlayerIndex();


	bool bPlayerDeath = FStrEq( pszEventName, "player_death" );
	bool bObjectDeath = FStrEq( pszEventName, "object_destroyed" );
	bool bMirvDefused = FStrEq( pszEventName, "mirv_defused" );
	if ( bPlayerDeath || bObjectDeath || bMirvDefused )
	{
		int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		int iKiller = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const char *pszKillingWeapon = event->GetString( "weapon" );
		const char *pszKillingWeaponLog = event->GetString( "weapon_logclassname" );
		if ( bObjectDeath && iVictim == 0 )
		{
			// for now, no death notices of map placed objects
			m_DeathNotices.Remove( iMsg );
			return;
		}

		// Get the names of the players
		const char *pszKillerName = iKiller > 0 ? g_PR->GetPlayerName( iKiller ) : "";
		if ( !pszKillerName )
		{
			pszKillerName = "";
		}

		V_strcpy_safe( m_DeathNotices[iMsg].Killer.szName, pszKillerName );

		const char *pszVictimName = g_PR->GetPlayerName( iVictim );
		if ( !pszVictimName )
		{
			pszVictimName = "";
		}

		V_strcpy_safe( m_DeathNotices[iMsg].Victim.szName, pszVictimName );

		// Make a new death notice.
		bool bLocalPlayerInvolved = false;
		if ( iLocalPlayerIndex == iKiller || iLocalPlayerIndex == iVictim )
		{
			bLocalPlayerInvolved = true;
		}

		if ( !m_DeathNotices[iMsg].iCritType )
		{
			int iCritType = event->GetInt( "crittype" );
			switch ( iCritType )
			{
				case CTakeDamageInfo::CRIT_FULL:
					m_DeathNotices[iMsg].iconCritDeath = GetIcon( "d_crit", bLocalPlayerInvolved );
					break;
				case CTakeDamageInfo::CRIT_MINI:
					m_DeathNotices[iMsg].iconCritDeath = GetIcon( "d_minicrit", bLocalPlayerInvolved );
					break;
				case CTakeDamageInfo::CRIT_TRANQ:
					m_DeathNotices[iMsg].iconCritDeath = GetIcon( "d_tranqed", bLocalPlayerInvolved );
					break;
				default:
					m_DeathNotices[iMsg].iconCritDeath = NULL;
					break;
			}
			m_DeathNotices[iMsg].iCritType = iCritType;
		}

		m_DeathNotices[iMsg].bLocalPlayerInvolved = bLocalPlayerInvolved;
		m_DeathNotices[iMsg].Killer.iTeam = ( iKiller > 0 ) ? g_PR->GetTeam( iKiller ) : 0;
		m_DeathNotices[iMsg].Victim.iTeam = g_PR->GetTeam( iVictim );

		bool isSilentKill = event->GetBool("silent_kill");
		if ((bPlayerDeath || bObjectDeath) && isSilentKill && 
			m_DeathNotices[iMsg].Killer.iTeam != g_PR->GetTeam(iLocalPlayerIndex)
			&& !bLocalPlayerInvolved)
		{
			// Silent kill is a silent kill
			m_DeathNotices.Remove(iMsg);
			return;
		}

		if ( pszKillingWeapon && *pszKillingWeapon )
		{
			V_sprintf_safe( m_DeathNotices[iMsg].szIcon, "d_%s", pszKillingWeapon );
		}

		if ( !iKiller || iKiller == iVictim )
		{
			m_DeathNotices[iMsg].bSelfInflicted = true;
			m_DeathNotices[iMsg].Killer.szName[0] = 0;

			if ( event->GetInt( "damagebits" ) & DMG_FALL )
			{
				// Special case text for falling death.
				V_wcscpy_safe( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#DeathMsg_Fall" ) );
			}
			else if ( ( event->GetInt( "damagebits" ) & DMG_TRAIN ) || ( 0 == V_stricmp( m_DeathNotices[iMsg].szIcon, "d_tracktrain" ) ) )
			{
				// Special case icon for hit-by-vehicle death.
				V_strcpy_safe( m_DeathNotices[iMsg].szIcon, "d_vehicle" );
			}
			else if ( event->GetInt( "damagebits" ) & DMG_SAWBLADE )
			{
				V_strcpy_safe( m_DeathNotices[iMsg].szIcon, "d_saw_kill" );
			}
		}

		int assister = engine->GetPlayerForUserID( event->GetInt( "assister" ) );
		const char *assister_name = ( assister > 0 ? g_PR->GetPlayerName( assister ) : NULL );

		if ( assister_name )
		{
#if 1
			// Base TF2 assumes that the assister and the killer are on the same team, thus it 
			// writes both in the same string, which in turn gives them both the killer's team color
			// whether the assister is on the killer's team or not. -danielmm8888
			m_DeathNotices[iMsg].Assister.iTeam = ( assister > 0 ) ? g_TF_PR->GetTeam( assister ) : 0;
			V_strcpy_safe( m_DeathNotices[iMsg].Assister.szName, assister_name );
#else
			// This is the old code used for assister handling.
			char szKillerBuf[MAX_PLAYER_NAME_LENGTH * 2];
			V_sprintf_safe( szKillerBuf, "%s + %s", m_DeathNotices[iDeathNoticeMsg].Killer.szName, assister_name );
			V_strcpy_safe( m_DeathNotices[iDeathNoticeMsg].Killer.szName, szKillerBuf );
#endif

			if ( iLocalPlayerIndex == assister )
			{
				m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
			}
		}

		char szDeathMsg[512];

		// Record a log of the death notice in the console.
		if ( m_DeathNotices[iMsg].bSelfInflicted )
		{
			if ( !strcmp( m_DeathNotices[iMsg].szIcon, "d_worldspawn" ) )
			{
				V_sprintf_safe( szDeathMsg, "%s died", m_DeathNotices[iMsg].Victim.szName );
			}
			else
			{
				V_sprintf_safe( szDeathMsg, "%s suicided", m_DeathNotices[iMsg].Victim.szName );
			}
		}
		else
		{
			// Assisters get a mention too!
			if ( m_DeathNotices[iMsg].Assister.szName[0] )
			{
				V_sprintf_safe( szDeathMsg, "%s and %s killed %s", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Assister.szName, m_DeathNotices[iMsg].Victim.szName );
			}
			else
			{
				V_sprintf_safe( szDeathMsg, "%s killed %s", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName );
			}

			if ( pszKillingWeaponLog && pszKillingWeaponLog[0] && ( pszKillingWeaponLog[0] > 13 ) )
			{
				V_strcat_safe( szDeathMsg, VarArgs( " with %s", pszKillingWeaponLog ) );
			}
			else if ( m_DeathNotices[iMsg].szIcon[0] && ( m_DeathNotices[iMsg].szIcon[0] > 13 ) )
			{
				V_strcat_safe( szDeathMsg, VarArgs( " with %s", &m_DeathNotices[iMsg].szIcon[2] ) );
			}
		}

		if ( FStrEq( pszEventName, "player_death" ) )
		{
			switch ( m_DeathNotices[iMsg].iCritType )
			{
				case CTakeDamageInfo::CRIT_FULL:
					Msg( "%s (crit)\n", szDeathMsg );
					break;
				case CTakeDamageInfo::CRIT_MINI:
					Msg( "%s (minicrit)\n", szDeathMsg );
					break;
				case CTakeDamageInfo::CRIT_TRANQ:
					Msg( "%s (tranq-crit)\n", szDeathMsg );
					break;
				default:
					Msg( "%s\n", szDeathMsg );
					break;
			}
		}

		if ( bMirvDefused )
		{
			// Get the localized name for the object.
			char szLocalizedObjectName[MAX_PLAYER_NAME_LENGTH];
			szLocalizedObjectName[0] = 0;
			const wchar_t *wszLocalizedObjectName = g_pVGuiLocalize->Find( "#TF_Weapon_MIRV" );
			if ( wszLocalizedObjectName )
			{
				g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedObjectName, szLocalizedObjectName, ARRAYSIZE( szLocalizedObjectName ) );
			}
			else
			{
				Warning( "Couldn't find localized object name for '%s'\n", "#TF_Weapon_MIRV" );
				V_strcpy_safe( szLocalizedObjectName, "#TF_Weapon_MIRV" );
			}

			// Print a log message.
			Msg( "%s defused a %s thrown by %s\n", m_DeathNotices[iMsg].Killer.szName, szLocalizedObjectName, m_DeathNotices[iMsg].Victim.szName );

			// Compose the string.
			if ( m_DeathNotices[iMsg].Victim.szName[0] )
			{
				char szVictimBuf[MAX_PLAYER_NAME_LENGTH * 2];
				V_sprintf_safe( szVictimBuf, "%s (%s)", szLocalizedObjectName, m_DeathNotices[iMsg].Victim.szName );
				V_strcpy_safe( m_DeathNotices[iMsg].Victim.szName, szVictimBuf );
			}
			else
			{
				V_strcpy_safe( m_DeathNotices[iMsg].Victim.szName, szLocalizedObjectName );
			}
		}
		else if ( bObjectDeath )
		{
			// If this is an object destroyed message, set the victim name to "<object type> (<owner>)".
			int iObjectType = event->GetInt( "objecttype" );
			if ( iObjectType >= 0 && iObjectType < OBJ_LAST )
			{
				// Get the localized name for the object.
				char szLocalizedObjectName[MAX_PLAYER_NAME_LENGTH];
				szLocalizedObjectName[0] = 0;
				const wchar_t *wszLocalizedObjectName = g_pVGuiLocalize->Find( szLocalizedObjectNames[iObjectType] );
				if ( wszLocalizedObjectName )
				{
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedObjectName, szLocalizedObjectName, ARRAYSIZE( szLocalizedObjectName ) );
				}
				else
				{
					Warning( "Couldn't find localized object name for '%s'\n", szLocalizedObjectNames[iObjectType] );
					V_strcpy_safe( szLocalizedObjectName, szLocalizedObjectNames[iObjectType] );
				}

				// Print a log message.
				Msg( "%s destroyed a %s made by %s\n", m_DeathNotices[iMsg].Killer.szName, szLocalizedObjectName, m_DeathNotices[iMsg].Victim.szName );

				// Compose the string.
				if ( m_DeathNotices[iMsg].Victim.szName[0] )
				{
					char szVictimBuffer[MAX_PLAYER_NAME_LENGTH * 2];
					V_sprintf_safe( szVictimBuffer, "%s (%s)", szLocalizedObjectName, m_DeathNotices[iMsg].Victim.szName );
					V_strcpy_safe( m_DeathNotices[iMsg].Victim.szName, szVictimBuffer );
				}
				else
				{
					V_strcpy_safe( m_DeathNotices[iMsg].Victim.szName, szLocalizedObjectName );
				}
			}
			else
			{
				// Invalid object type.
				Assert( false );
			}
			// If this is a taser kill against a building, play a custom icon
			if (!Q_strcmp(pszKillingWeapon, "taser"))
			{
				V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_taser_uncharged");
			}
			// If this is a brick kill against a building, play a custom icon
			if (!Q_strcmp(pszKillingWeapon, "brick"))
			{
				V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_brick_building");
			}
		}
		else if ( tf2c_show_nemesis_relationships.GetBool() )
		{
			// If this death involved a player dominating another player or getting revenge on another player, add an additional message
			// mentioning that.
			int nDeathFlags = event->GetInt( "death_flags" );
			if ( nDeathFlags & TF_DEATH_DOMINATION )
			{
				AddAdditionalMsg( iKiller, iVictim, "#Msg_Dominating" );
				PlayRivalrySounds( iKiller, iVictim, TF_DEATH_DOMINATION );

				// Print a log message.
				Msg( "%s is dominating %s\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName );
			}

			if ( ( nDeathFlags & TF_DEATH_ASSISTER_DOMINATION ) && assister > 0 )
			{
				AddAdditionalMsg( assister, iVictim, "#Msg_Dominating" );
				PlayRivalrySounds( assister, iVictim, TF_DEATH_DOMINATION );

				// Print a log message.
				Msg( "%s is dominating %s (assist)\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName );
			}

			if ( nDeathFlags & TF_DEATH_REVENGE )
			{
				AddAdditionalMsg( iKiller, iVictim, "#Msg_Revenge" );
				PlayRivalrySounds( iKiller, iVictim, TF_DEATH_REVENGE );

				// Print a log message.
				Msg( "%s got revenge on %s\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName );
			}

			if ( ( nDeathFlags & TF_DEATH_ASSISTER_REVENGE ) && assister > 0 )
			{
				AddAdditionalMsg( assister, iVictim, "#Msg_Revenge" );
				PlayRivalrySounds( assister, iVictim, TF_DEATH_REVENGE );

				// Print a log message.
				Msg( "%s got revenge on %s (assist)\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName );
			}
		}

		switch ( event->GetInt( "customkill" ) )
		{
			case TF_DMG_CUSTOM_BACKSTAB:
				V_strcpy_safe( m_DeathNotices[iMsg].szIcon, "d_backstab" );
				break;
			case TF_DMG_CUSTOM_HEADSHOT:
			{
				const char *pszIcon;
				switch ( event->GetInt( "weaponid" ) )
				{
					case TF_WEAPON_COMPOUND_BOW:
						pszIcon = "d_huntsman_headshot";
						break;
					default:
						pszIcon = "d_headshot";
						break;
				}
				V_strcpy_safe( m_DeathNotices[iMsg].szIcon, pszIcon );
				break;
			}
			case TF_DMG_CUSTOM_SUICIDE:
			case TF_DMG_CUSTOM_SUICIDE_DISINTEGRATE:
			{
				// Display a different message if this was suicide, or assisted suicide (suicide w/recent damage, kill awarded to damager).
				bool bAssistedSuicide = event->GetInt( "userid" ) != event->GetInt( "attacker" );
				const wchar_t *pMsg = g_pVGuiLocalize->Find( bAssistedSuicide ? "#DeathMsg_AssistedSuicide" : "#DeathMsg_Suicide" );
				if ( pMsg )
				{
					V_wcscpy_safe( m_DeathNotices[iMsg].wzInfoText, pMsg );
				}
				break;
			}
			case TF_DMG_CUSTOM_SUICIDE_BOIOING:
			{
				// Display a different message if this was suicide, or assisted suicide (suicide w/recent damage, kill awarded to damager).
				bool bAssistedSuicide = event->GetInt("userid") != event->GetInt("attacker");
				const wchar_t* pMsg = g_pVGuiLocalize->Find(bAssistedSuicide ? "#DeathMsg_AssistedSuicide" : "#DeathMsg_Suicide_Boioing");
				if (pMsg)
				{
					V_wcscpy_safe(m_DeathNotices[iMsg].wzInfoText, pMsg);
				}
				break;
			}
			case TF_DMG_CUSTOM_SUICIDE_STOMP:
			{
				// Display a different message if this was suicide, or assisted suicide (suicide w/recent damage, kill awarded to damager).
				bool bAssistedSuicide = event->GetInt("userid") != event->GetInt("attacker");
				const wchar_t* pMsg = g_pVGuiLocalize->Find(bAssistedSuicide ? "#DeathMsg_AssistedSuicide" : "#DeathMsg_Suicide_Stomp");
				if (pMsg)
				{
					V_wcscpy_safe(m_DeathNotices[iMsg].wzInfoText, pMsg);
				}
				break;
			}
			case TF_DMG_CUSTOM_BURNING_ARROW:
			{
				// Special-case if the player is killed from a burning arrow after it has already landed.
				Q_strncpy( m_DeathNotices[iMsg].szIcon, "d_huntsman_burning", ARRAYSIZE( m_DeathNotices[iMsg].szIcon ) );
				m_DeathNotices[iMsg].wzInfoText[0] = 0;
				break;
			}
			case TF_DMG_CUSTOM_FLYINGBURN:
			{
				// Special-case if the player is killed from a burning arrow as the killing blow.
				Q_strncpy( m_DeathNotices[iMsg].szIcon, "d_huntsman_flyingburn", ARRAYSIZE( m_DeathNotices[iMsg].szIcon ) );
				m_DeathNotices[iMsg].wzInfoText[0] = 0;
				break;
			}
			case TF_DMG_CUSTOM_DECAPITATION:
			{
				if ( pszKillingWeapon && *pszKillingWeapon )
				{
					if (!Q_strcmp(pszKillingWeapon, "harvester"))
					{
						V_strcpy_safe( m_DeathNotices[iMsg].szIcon, "d_harvester_decapitation" );				
					}
				}
				break;
			}
#if 1
			case TF_DMG_CYCLOPS_COMBO_MIRV:
			{
				if (pszKillingWeapon && *pszKillingWeapon)
				{
					if (!Q_strcmp(pszKillingWeapon, "cyclops"))
					{
						V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_mirv_projectile");
					}
				}

				V_wcscpy_safe(m_DeathNotices[iMsg].wzInfoText, L"COMBO!!!");
				break;
			}
			case TF_DMG_CYCLOPS_COMBO_MIRV_BOMBLET:
			{
				if (pszKillingWeapon && *pszKillingWeapon)
				{
					if (!Q_strcmp(pszKillingWeapon, "cyclops"))
					{
						V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_mirv_bomb");
					}
				}

				V_wcscpy_safe(m_DeathNotices[iMsg].wzInfoText, L"COMBO!!!");
				break;
			}
			case TF_DMG_CYCLOPS_COMBO_STICKYBOMB:
			{
				if (pszKillingWeapon && *pszKillingWeapon)
				{
					if (!Q_strcmp(pszKillingWeapon, "cyclops"))
					{
						V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_tf_projectile_pipe_remote");
					}
				}

				V_wcscpy_safe(m_DeathNotices[iMsg].wzInfoText, L"COMBO!!!");
				break;
			}

			case TF_DMG_CYCLOPS_COMBO_PROXYMINE:
			{
				if (pszKillingWeapon && *pszKillingWeapon)
				{
					if (!Q_strcmp(pszKillingWeapon, "cyclops"))
					{
						V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_proxymine");
					}
				}

				V_wcscpy_safe(m_DeathNotices[iMsg].wzInfoText, L"COMBO!!!");
				break;
			}
#else
			case TF_DMG_CYCLOPS_COMBO_MIRV:
			{
				V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_cyclops_mirv_combo");
				break;
			}
			case TF_DMG_CYCLOPS_COMBO_MIRV_BOMBLET:
			{
				V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_cyclops_mirv_bomblet_combo");
				break;
			}
			case TF_DMG_CYCLOPS_COMBO_STICKYBOMB:
			{
				V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_cyclops_sticky_combo");
				break;
			}
			case TF_DMG_CYCLOPS_COMBO_PROXYMINE:
			{
				V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_cyclops_proxymine_combo");
				break;
			}
#endif
			case TF_DMG_MIRV_DIRECT_HIT:
			{
				if (pszKillingWeapon && *pszKillingWeapon)
				{
					if (!Q_strcmp(pszKillingWeapon, "mirv_projectile"))
					{
						V_strcpy_safe(m_DeathNotices[iMsg].szIcon, "d_mirv_direct");
					}
				}
				break;
			}
			default:
				break;
		}
	}
	else if ( FStrEq( "teamplay_point_captured", pszEventName ) )
	{
		GetLocalizedControlPointName( event, m_DeathNotices[iMsg].Victim.szName, ARRAYSIZE( m_DeathNotices[iMsg].Victim.szName ) );

		// Array of capper indices.
		const char *pszCappers = event->GetString( "cappers" );
		char szCappers[256] = { '\0' };
		int iLen = V_strlen( pszCappers );
		for ( int i = 0; i < iLen; i++ )
		{
			int iPlayerIndex = (int)pszCappers[i];
			Assert( iPlayerIndex > 0 && iPlayerIndex <= gpGlobals->maxClients );

			if ( i == 0 )
			{
				// use first player as the team
				m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
				m_DeathNotices[iMsg].Victim.iTeam = TEAM_UNASSIGNED;
			}
			else
			{
				V_strcat_safe( szCappers, ", ", 2 );
			}

			const char *pPlayerName = g_PR->GetPlayerName( iPlayerIndex );
			V_strcat_safe( szCappers, pPlayerName );
			if ( iLocalPlayerIndex == iPlayerIndex )
			{
				m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
			}
		}

		V_strcpy_safe( m_DeathNotices[iMsg].Killer.szName, szCappers );
		V_wcscpy_safe( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( iLen > 1 ? "#Msg_Captured_Multiple" : "#Msg_Captured" ) );

		// Set the icon.
		int iTeam = m_DeathNotices[iMsg].Killer.iTeam;
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		if ( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT )
		{
			V_sprintf_safe( m_DeathNotices[iMsg].szIcon, "d_%scapture", g_aTeamLowerNames[iTeam] );
		}

		// Print a log message.
		Msg( "%s captured %s for %s team\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName, g_aTeamUpperNamesShort[iTeam] );
	}
	else if ( FStrEq( "teamplay_capture_blocked", pszEventName ) )
	{
		GetLocalizedControlPointName( event, m_DeathNotices[iMsg].Victim.szName, ARRAYSIZE( m_DeathNotices[iMsg].Victim.szName ) );
		V_wcscpy_safe( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#Msg_Defended" ) );

		int iPlayerIndex = event->GetInt( "blocker" );
		const char *pszBlockerName = g_PR->GetPlayerName( iPlayerIndex );
		V_strcpy_safe( m_DeathNotices[iMsg].Killer.szName, pszBlockerName );

		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
		if ( iLocalPlayerIndex == iPlayerIndex )
		{
			m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
		}

		// Set the icon.
		int iTeam = m_DeathNotices[iMsg].Killer.iTeam;
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		if ( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT )
		{
			V_sprintf_safe( m_DeathNotices[iMsg].szIcon, "d_%sdefend", g_aTeamLowerNames[iTeam] );
		}

		// Print a log message.
		Msg( "%s defended %s for %s team\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName, g_aTeamUpperNamesShort[iTeam] );
	}
	else if ( FStrEq( "teamplay_flag_event", pszEventName ) )
	{
		int iPlayerIndex = event->GetInt( "player" );
		if ( iPlayerIndex < 1 || iPlayerIndex > MAX_PLAYERS )
		{
			m_DeathNotices.Remove( iMsg );
			return;
		}

		const char *pszMsgKey = NULL;
		int iEventType = event->GetInt( "eventtype" );
		int iTeam = event->GetInt( "team", 0 );
		switch ( iEventType )
		{
			case TF_FLAGEVENT_PICKUP:
				pszMsgKey = ( iTeam > TEAM_SPECTATOR ) ? "#Msg_PickedUpFlag" : "#Msg_PickedUpNeutralFlag";
				break;
			case TF_FLAGEVENT_CAPTURE:
				pszMsgKey = ( iTeam > TEAM_SPECTATOR ) ? "#Msg_CapturedFlag" : "#Msg_CapturedNeutralFlag";
				break;
			case TF_FLAGEVENT_DEFEND:
				pszMsgKey = ( iTeam > TEAM_SPECTATOR ) ? "#Msg_DefendedFlag" : "#Msg_DefendedNeutralFlag";
				break;
			case TF_FLAGEVENT_RETURN:
				pszMsgKey = ( iTeam > TEAM_SPECTATOR ) ? "#Msg_ReturnedFlag" : "#Msg_ReturnedNeutralFlag";
				break;
			case TF_FLAGEVENT_DROPPED: 
				pszMsgKey = ( iTeam > TEAM_SPECTATOR ) ? "#Msg_DroppedFlag" : "#Msg_DroppedNeutralFlag";
				break;
			default:
				// Unsupported, don't put anything up.
				m_DeathNotices.Remove( iMsg );
				return;
		}

		static wchar_t wszLocalized[128];
		g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( pszMsgKey ), 1, g_pVGuiLocalize->Find( g_aTeamNames_Localized[iTeam] ) );

		V_wcscpy_safe( m_DeathNotices[iMsg].wzInfoText, wszLocalized );

		const char *szPlayerName = g_PR->GetPlayerName( iPlayerIndex );
		V_strcpy_safe( m_DeathNotices[iMsg].Killer.szName, szPlayerName );
		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
		if ( iLocalPlayerIndex == iPlayerIndex )
		{
			m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
		}

		// Set the icon.
		int iKillerTeam = m_DeathNotices[iMsg].Killer.iTeam;
		Assert( iKillerTeam >= FIRST_GAME_TEAM && iKillerTeam < TF_TEAM_COUNT );
		if ( iKillerTeam >= FIRST_GAME_TEAM && iKillerTeam < TF_TEAM_COUNT )
		{
			V_sprintf_safe( m_DeathNotices[iMsg].szIcon, "d_%scapture", g_aTeamLowerNames[iKillerTeam] );
		}

		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		if ( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT )
		{
			// Print a log message.
			switch ( iEventType )
			{
				case TF_FLAGEVENT_PICKUP:
					Msg( "%s has picked up %s team's flag\n", m_DeathNotices[iMsg].Killer.szName, g_aTeamUpperNamesShort[iTeam] );
					break;
				case TF_FLAGEVENT_CAPTURE:
					Msg( "%s captured %s team's flag\n", m_DeathNotices[iMsg].Killer.szName, g_aTeamUpperNamesShort[iTeam] );
					break;
				case TF_FLAGEVENT_DEFEND:
					Msg( "%s defended %s team's flag\n", m_DeathNotices[iMsg].Killer.szName, g_aTeamUpperNamesShort[iTeam] );
					break;
				case TF_FLAGEVENT_RETURN:
					Msg( "%s team's flag has returned\n", g_aTeamUpperNamesShort[iTeam] );
					break;
				case TF_FLAGEVENT_DROPPED:
					Msg( "%s dropped %s team's flag\n", m_DeathNotices[iMsg].Killer.szName, g_aTeamUpperNamesShort[iTeam] );
					break;
				default:
					// How did we get here?		
					Assert( 0 );
					return;
			}
		}
	}
	else if ( FStrEq( "vip_death", pszEventName ) )
	{
		V_wcscpy_safe( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#Msg_KilledVIP" ) );

		int iPlayerIndex = event->GetInt( "attacker" );
		const char *killer_name = g_PR->GetPlayerName( iPlayerIndex );
		V_strcpy_safe( m_DeathNotices[iMsg].Killer.szName, killer_name );
		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
		if ( iLocalPlayerIndex == iPlayerIndex )
		{
			m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
		}

		// Set the icon.
		int iTeam = m_DeathNotices[iMsg].Killer.iTeam;
		Assert( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT );
		if ( iTeam >= FIRST_GAME_TEAM && iTeam < TF_TEAM_COUNT )
		{
			V_sprintf_safe( m_DeathNotices[iMsg].szIcon, "d_vip_killed" );
		}

		// Print a log message.
		Msg( "%s killed the VIP\n", m_DeathNotices[iMsg].Killer.szName );
	}

	//OnGameEvent( event, iMsg );

	if ( !m_DeathNotices[iMsg].iconDeath && m_DeathNotices[iMsg].szIcon )
	{
		// Try and find the death identifier in the icon list.
		// On consoles, we flip usage of the inverted icon to make it more visible.
		bool bInverted = m_DeathNotices[iMsg].bLocalPlayerInvolved;
		if ( IsConsole() )
		{
			bInverted = !bInverted;
		}

		m_DeathNotices[iMsg].iconDeath = GetIcon( m_DeathNotices[iMsg].szIcon, bInverted );
		if ( !m_DeathNotices[iMsg].iconDeath )
		{
#ifdef TF2C_BETA
			const char* pszKillingWeapon = event->GetString("weapon");
			if (pszKillingWeapon && pszKillingWeapon[0] && !m_DeathNotices[iMsg].wzInfoText[0])
			{
				if (bPlayerDeath || bObjectDeath || bMirvDefused)
				{
					wchar_t wszDescriptionString[64] = {};
					g_pVGuiLocalize->ConvertANSIToUnicode(pszKillingWeapon, wszDescriptionString, sizeof(wszDescriptionString));
					V_wcscpy_safe(m_DeathNotices[iMsg].wzInfoText, wszDescriptionString);
				}
			}
#endif
			// Can't find it, so use the default skull & crossbones icon.
			m_DeathNotices[iMsg].iconDeath = GetIcon( "d_skull_tf", m_DeathNotices[iMsg].bLocalPlayerInvolved );
		}
	}
}


void CTFHudDeathNotice::PlayRivalrySounds( int iKillerIndex, int iVictimIndex, int iType )
{
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	//We're not involved in this kill
	if ( iKillerIndex != iLocalPlayerIndex && iVictimIndex != iLocalPlayerIndex )
		return;

	// Stop any sounds that are already playing to avoid ear rape in case of
	// multiple dominations at once.
	C_BaseEntity::StopSound( SOUND_FROM_LOCAL_PLAYER, "Game.Domination" );
	C_BaseEntity::StopSound( SOUND_FROM_LOCAL_PLAYER, "Game.Nemesis" );
	C_BaseEntity::StopSound( SOUND_FROM_LOCAL_PLAYER, "Game.Revenge" );

	const char *pszSoundName = NULL;

	if ( iType == TF_DEATH_DOMINATION )
	{
		if ( iKillerIndex == iLocalPlayerIndex )
		{
			pszSoundName = "Game.Domination";
		}
		else if ( iVictimIndex == iLocalPlayerIndex )
		{
			pszSoundName = "Game.Nemesis";
		}
	}
	else if ( iType == TF_DEATH_REVENGE )
	{
		pszSoundName = "Game.Revenge";
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSoundName );
}

//-----------------------------------------------------------------------------
// Purpose: Gets the localized name of the control point sent in the event
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::GetLocalizedControlPointName( IGameEvent *event, char *namebuf, int namelen )
{
	// Cap point name ( MATTTODO: can't we find this from the point index ? )
	const char *pName = event->GetString( "cpname", "Unnamed Control Point" );
	const wchar_t *pLocalizedName = g_pVGuiLocalize->Find( pName );

	if ( pLocalizedName )
	{
		g_pVGuiLocalize->ConvertUnicodeToANSI( pLocalizedName, namebuf, namelen );
	}
	else
	{
		V_strncpy( namebuf, pName, namelen );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new death notice to the queue
//-----------------------------------------------------------------------------
int CTFHudDeathNotice::AddDeathNoticeItem()
{
	int iMsg = m_DeathNotices.AddToTail();
	DeathNoticeItem &msg = m_DeathNotices[iMsg];
	msg.flCreationTime = gpGlobals->curtime;
	return iMsg;
}

//-----------------------------------------------------------------------------
// Purpose: draw text helper
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::DrawText( int x, int y, HFont hFont, Color clr, const wchar_t *szText )
{
	surface()->DrawSetTextPos( x, y );
	surface()->DrawSetTextColor( clr );
	surface()->DrawSetTextFont( hFont );	//reset the font, draw icon can change it
	surface()->DrawUnicodeString( szText, vgui::FONT_DRAW_NONADDITIVE );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a rounded-corner polygon that fits in the specified bounds
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::GetBackgroundPolygonVerts( int x0, int y0, int x1, int y1, int iVerts, vgui::Vertex_t vert[] )
{
	Assert( iVerts == NUM_BACKGROUND_COORD );
	// use the offsets we generated for one corner and apply those to the passed-in dimensions to create verts for the poly
	for ( int i = 0; i < NUM_CORNER_COORD; i++ )
	{
		int j = ( NUM_CORNER_COORD - 1 ) - i;
		// upper left corner
		vert[i].Init( Vector2D( x0 + m_CornerCoord[i].x, y0 + m_CornerCoord[i].y ) );
		// upper right corner
		vert[i + NUM_CORNER_COORD].Init( Vector2D( x1 - m_CornerCoord[j].x, y0 + m_CornerCoord[j].y ) );
		// lower right corner
		vert[i + ( NUM_CORNER_COORD * 2 )].Init( Vector2D( x1 - m_CornerCoord[i].x, y1 - m_CornerCoord[i].y ) );
		// lower left corner
		vert[i + ( NUM_CORNER_COORD * 3 )].Init( Vector2D( x0 + m_CornerCoord[j].x, y1 - m_CornerCoord[j].y ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets specified icon
//-----------------------------------------------------------------------------
CHudTexture *CTFHudDeathNotice::GetIcon( const char *szIcon, bool bInvert )
{
	// get the inverted version if specified
	if ( bInvert && 0 == V_strncmp( "d_", szIcon, 2 ) )
	{
		// change prefix from d_ to dneg_
		char szIconTmp[255] = "dneg_";
		V_strcat( szIconTmp, szIcon + 2, ARRAYSIZE( szIconTmp ) );
		CHudTexture *pIcon = gHUD.GetIcon( szIconTmp );
		// return inverted version if found
		if ( pIcon )
			return pIcon;
		// if we didn't find the inverted version, keep going and try the normal version
	}
	return gHUD.GetIcon( szIcon );
}

//-----------------------------------------------------------------------------
// Purpose: Creates the offsets for rounded corners based on current screen res
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::CalcRoundedCorners()
{
	// generate the offset geometry for upper left corner
	int iMax = ARRAYSIZE( m_CornerCoord );
	for ( int i = 0; i < iMax; i++ )
	{
		m_CornerCoord[i].x = m_flCornerRadius * ( 1 - cos( ( (float)i / (float)( iMax - 1 ) ) * ( M_PI / 2 ) ) );
		m_CornerCoord[i].y = m_flCornerRadius * ( 1 - sin( ( (float)i / (float)( iMax - 1 ) ) * ( M_PI / 2 ) ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds an additional death message
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey, bool bDomination /*= true*/ )
{
	DeathNoticeItem &msg2 = m_DeathNotices[AddDeathNoticeItem()];
	V_strcpy_safe( msg2.Killer.szName, g_PR->GetPlayerName( iKillerID ) );
	V_strcpy_safe( msg2.Victim.szName, g_PR->GetPlayerName( iVictimID ) );

	msg2.Killer.iTeam = g_PR->GetTeam( iKillerID );
	msg2.Victim.iTeam = g_PR->GetTeam( iVictimID );

	const wchar_t *wzMsg = g_pVGuiLocalize->Find( pMsgKey );
	if ( wzMsg )
	{
		V_wcscpy_safe( msg2.wzInfoText, wzMsg );
	}

	if ( bDomination )
		msg2.iconDeath = m_iconDomination;

	int iLocalPlayerIndex = GetLocalPlayerIndex();
	if ( iLocalPlayerIndex == iVictimID || iLocalPlayerIndex == iKillerID )
	{
		msg2.bLocalPlayerInvolved = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the color to draw text in for this team.  
//-----------------------------------------------------------------------------
Color CTFHudDeathNotice::GetPlayerColor( const DeathNoticePlayer &playerItem, bool bLocalPlayerInvolved /*= false*/ )
{
	switch ( playerItem.iTeam )
	{
	case TF_TEAM_BLUE:
		return m_clrBlueText;
		break;
	case TF_TEAM_RED:
		return m_clrRedText;
		break;
	case TF_TEAM_GREEN:
		return m_clrGreenText;
		break;
	case TF_TEAM_YELLOW:
		return m_clrYellowText;
		break;
	case TEAM_UNASSIGNED:
		return bLocalPlayerInvolved ? COLOR_BLACK : COLOR_WHITE;
		break;
	default:
		AssertOnce( false );	// invalid team
		return bLocalPlayerInvolved ? COLOR_BLACK : COLOR_WHITE;
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the expiry time for this death notice item
//-----------------------------------------------------------------------------
float DeathNoticeItem::GetExpiryTime()
{
	float flDuration = hud_deathnotice_time.GetFloat();
	if ( bLocalPlayerInvolved )
	{
		// if the local player is involved, make the message last longer
		flDuration *= 2;
	}
	return flCreationTime + flDuration;
}
