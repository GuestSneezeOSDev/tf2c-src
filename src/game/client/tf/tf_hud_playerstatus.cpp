//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
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
#include <vgui/ISurface.h>
#include <vgui/IImage.h>
#include <vgui_controls/Label.h>
#include <engine/IEngineSound.h>

#include "hud_numericdisplay.h"
#include "c_team.h"
#include "c_tf_player.h"
#include "tf_shareddefs.h"
#include "tf_hud_playerstatus.h"
#include "tf_hud_target_id.h"
#include "tf_gamerules.h"
#include "econ_wearable.h"

using namespace vgui;

ConVar cl_hud_playerclass_use_playermodel( "cl_hud_playerclass_use_playermodel", "0", FCVAR_ARCHIVE, "Use player model in player class HUD." );

ConVar tf2c_low_health_sound( "tf2c_low_health_sound", "0", FCVAR_ARCHIVE, "Play a warning sound when player's health drops below the percentage set by tf2c_low_health_jingle_threshold." );
ConVar tf2c_low_health_sound_threshold( "tf2c_low_health_sound_threshold", "0.49", FCVAR_ARCHIVE, "Low health warning threshold percentage.", true, 0.0f, true, 1.0f );

ConVar tf2c_show_status_effect_icons( "tf2c_show_status_effect_icons", "1", FCVAR_ARCHIVE, "Shows status effect icons near the player's health." );

extern ConVar cl_hud_minmode;

static const char *g_szClassImages[TF_CLASS_COUNT_ALL] =
{
	"",
	"../hud/class_scout",
	"../hud/class_sniper",
	"../hud/class_soldier",
	"../hud/class_demo",
	"../hud/class_medic",
	"../hud/class_heavy",
	"../hud/class_pyro",
	"../hud/class_spy",
	"../hud/class_engi",
	"../hud/class_civ",
};


CTFHudPlayerClass::CTFHudPlayerClass( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pClassImage = new CTFClassImage( this, "PlayerStatusClassImage" );
	m_pClassImageBG = new CTFImagePanel( this, "PlayerStatusClassImageBG" );
	m_pSpyImage = new CTFImagePanel( this, "PlayerStatusSpyImage" );
	m_pSpyOutlineImage = new CTFImagePanel( this, "PlayerStatusSpyOutlineImage" );

	m_pClassModelPanel = new CTFPlayerModelPanel( this, "classmodelpanel" );
	m_pClassModelPanelBG = new CTFImagePanel( this, "classmodelpanelBG" );

	m_pCarryingWeaponPanel = new EditablePanel( this, "CarryingWeapon" );

	m_nTeam = TEAM_UNASSIGNED;
	m_nClass = TF_CLASS_UNDEFINED;
	m_nDisguiseTeam = TEAM_UNASSIGNED;
	m_nDisguiseClass = TF_CLASS_UNDEFINED;
	m_flNextThink = 0.0f;

	ListenForGameEvent( "localplayer_changedisguise" );
	ListenForGameEvent( "post_inventory_application" );
}


void CTFHudPlayerClass::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "HudSpyDisguiseHide" );
}


void CTFHudPlayerClass::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudPlayerClass.res" );

	m_nTeam = TEAM_UNASSIGNED;
	m_nClass = TF_CLASS_UNDEFINED;
	m_nDisguiseTeam = TEAM_UNASSIGNED;
	m_nDisguiseClass = TF_CLASS_UNDEFINED;
	m_flNextThink = 0.0f;
	m_nCloakLevel = 0;
	m_hWeapon = NULL;

	// Hide the weapon name panel since we don't need it.
	m_pCarryingWeaponPanel->SetVisible( false );

	BaseClass::ApplySchemeSettings( pScheme );
}


void CTFHudPlayerClass::OnThink()
{
	if ( m_flNextThink < gpGlobals->curtime )
	{
		bool bTeamChange = false;
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			// set our background colors
			if ( m_nTeam != pPlayer->GetTeamNumber() )
			{
				bTeamChange = true;
				m_nTeam = pPlayer->GetTeamNumber();
			}

			int nCloakLevel = 0;
			bool bCloakChange = false;
			float flInvis = pPlayer->GetPercentInvisible();

			if ( flInvis > 0.9f )
			{
				nCloakLevel = 2;
			}
			else if ( flInvis > 0.1f )
			{
				nCloakLevel = 1;
			}

			if ( nCloakLevel != m_nCloakLevel )
			{
				m_nCloakLevel = nCloakLevel;
				bCloakChange = true;
			}

			bool bIsDisguised = pPlayer->m_Shared.IsDisguised();
			bool bIsDisguising = pPlayer->m_Shared.InCond( TF_COND_DISGUISING );

			// set our class image
			if ( m_nClass != pPlayer->GetPlayerClass()->GetClassIndex() || bTeamChange || bCloakChange ||
				( m_nClass == TF_CLASS_SPY && m_nDisguiseClass != pPlayer->m_Shared.GetDisguiseClass() ) ||
				( m_nClass == TF_CLASS_SPY && m_nDisguiseTeam != pPlayer->m_Shared.GetDisguiseTeam() ) ||
				( ( bIsDisguised && !bIsDisguising ) ? pPlayer->m_Shared.GetDisguiseWeapon() != m_hWeapon : pPlayer->GetActiveTFWeapon() != m_hWeapon ) )
			{
				m_nClass = pPlayer->GetPlayerClass()->GetClassIndex();
				m_hWeapon = ( bIsDisguised && !bIsDisguising ) ? pPlayer->m_Shared.GetDisguiseWeapon() : pPlayer->GetActiveTFWeapon();

				if ( m_nClass == TF_CLASS_SPY && bIsDisguised )
				{
					if ( !bIsDisguising )
					{
						m_nDisguiseTeam = pPlayer->m_Shared.GetTrueDisguiseTeam();
						m_nDisguiseClass = pPlayer->m_Shared.GetDisguiseClass();
					}
				}
				else
				{
					m_nDisguiseTeam = TEAM_UNASSIGNED;
					m_nDisguiseClass = TF_CLASS_UNDEFINED;
				}

				int iCloakState = 0;
				if ( pPlayer->IsPlayerClass( TF_CLASS_SPY, true ) )
				{
					iCloakState = m_nCloakLevel;
				}

				if ( cl_hud_playerclass_use_playermodel.GetBool()
#ifndef TF2C_BETA
					&& m_nDisguiseTeam != TF_TEAM_GLOBAL
#endif
					)
				{
					m_pClassImage->SetVisible( false );
					m_pSpyImage->SetVisible( false );
					m_pClassImageBG->SetVisible( false );
					m_pClassModelPanel->SetVisible( true );
					m_pClassModelPanelBG->SetVisible( true );

					UpdateModelPanel();
				}
				else
				{
					m_pClassImage->SetVisible( true );
					m_pClassImageBG->SetVisible( true );
					m_pClassModelPanel->SetVisible( false );
					m_pClassModelPanelBG->SetVisible( false );

					if ( m_nDisguiseTeam != TEAM_UNASSIGNED || m_nDisguiseClass != TF_CLASS_UNDEFINED )
					{
						m_pSpyImage->SetVisible( true );
						m_pClassImage->SetClass( m_nDisguiseTeam, m_nDisguiseClass, iCloakState );
					}
					else
					{
						m_pSpyImage->SetVisible( false );
						m_pClassImage->SetClass( m_nTeam, m_nClass, iCloakState );
					}
				}
			}
		}

		m_flNextThink = gpGlobals->curtime + 0.05f;
	}
}


void CTFHudPlayerClass::UpdateModelPanel( void )
{
	if ( !cl_hud_playerclass_use_playermodel.GetBool() )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	m_pClassModelPanel->ClearCarriedItems();

	int i, c;
	ETFLoadoutSlot iActiveSlot = TF_LOADOUT_SLOT_INVALID;
	if ( m_nDisguiseTeam != TEAM_UNASSIGNED )
	{
		// If I'm disguised, show disguise class and weapon.
		m_pClassModelPanel->SetToPlayerClass( m_nDisguiseClass );
		m_pClassModelPanel->SetTeam( m_nDisguiseTeam );

		// Equip the disguise target's weapon.
		if ( pPlayer->m_Shared.GetDisguiseWeapon() )
		{
			CEconItemView *pItem = pPlayer->m_Shared.GetDisguiseWeapon()->GetItem();
			if ( pItem )
			{
				m_pClassModelPanel->AddCarriedItem( pItem );

				iActiveSlot = pItem->GetLoadoutSlot( m_nDisguiseClass );
			}
		}

		// Equip all disguise target's wearables.
		C_TFWearable *pWearable;
		for ( i = 0, c = pPlayer->GetNumWearables(); i < c; i++ )
		{
			pWearable = dynamic_cast<C_TFWearable *>( pPlayer->GetWearable( i ) );
			if ( !pWearable || !pWearable->IsDisguiseWearable() )
				continue;

			m_pClassModelPanel->AddCarriedItem( pWearable->GetItem() );
		}
	}
	else
	{
		m_pClassModelPanel->SetToPlayerClass( m_nClass );
		m_pClassModelPanel->SetTeam( m_nTeam );

		// Equip all weapons.
		C_EconEntity *pWeapon;
		CEconItemView *pItem;
		for ( i = 0, c = pPlayer->WeaponCount(); i < c; i++ )
		{
			pWeapon = dynamic_cast<C_EconEntity *>( pPlayer->GetWeapon( i ) );
			if ( pWeapon )
			{
				pItem = pWeapon->GetItem();
				if ( pItem )
				{
					m_pClassModelPanel->AddCarriedItem( pItem );

					if ( pWeapon == m_hWeapon )
					{
						iActiveSlot = pItem->GetLoadoutSlot( m_nClass );
					}
				}
			}
		}

		// Equip all wearables.
		C_TFWearable *pWearable;
		for ( i = 0, c = pPlayer->GetNumWearables(); i < c; i++ )
		{
			pWearable = dynamic_cast<C_TFWearable *>( pPlayer->GetWearable( i ) );
			if ( !pWearable || pWearable->IsDisguiseWearable() )
				continue;

			m_pClassModelPanel->AddCarriedItem( pWearable->GetItem() );
		}
	}

	// Hold active weapon.
	if ( iActiveSlot != TF_LOADOUT_SLOT_INVALID )
	{
		m_pClassModelPanel->HoldItemInSlot( iActiveSlot );
	}
}


void CTFHudPlayerClass::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( "localplayer_changedisguise", event->GetName() ) )
	{
		bool bFadeIn = event->GetBool( "disguised", false );
		if ( bFadeIn )
		{
			m_pSpyImage->SetAlpha( 0 );
		}
		else
		{
			m_pSpyImage->SetAlpha( 255 );
		}

		m_pSpyOutlineImage->SetAlpha( 0 );

		if ( !cl_hud_playerclass_use_playermodel.GetBool() )
		{
			m_pSpyImage->SetVisible( true );
		}
		
		m_pSpyOutlineImage->SetVisible( true );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( bFadeIn ? "HudSpyDisguiseFadeIn" : "HudSpyDisguiseFadeOut" );
	}
	else if ( FStrEq( "post_inventory_application", event->GetName() ) )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer && pPlayer->GetUserID() == event->GetInt( "userid" ) )
		{
			UpdateModelPanel();
		}
	}
}


CTFHealthPanel::CTFHealthPanel( Panel *parent, const char *name ) : vgui::Panel( parent, name )
{
	m_flHealth = 1.0f;

	m_iMaterialIndex = surface()->DrawGetTextureId( "hud/health_color" );
	if ( m_iMaterialIndex == -1 )
	{
		// We didn't find it, so create a new one.
		m_iMaterialIndex = surface()->CreateNewTextureID();	
	}

	surface()->DrawSetTextureFile( m_iMaterialIndex, "hud/health_color", true, false );

	m_iDeadMaterialIndex = surface()->DrawGetTextureId( "hud/health_dead" );
	if ( m_iDeadMaterialIndex == -1 )
	{
		// We didn't find it, so create a new one.
		m_iDeadMaterialIndex = surface()->CreateNewTextureID();	
	}
	surface()->DrawSetTextureFile( m_iDeadMaterialIndex, "hud/health_dead", true, false );

	m_iAlpha = 255;
}


void CTFHealthPanel::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];	
	float uv1 = 0.0f;
	float uv2 = 1.0f;
	int xpos = 0, ypos = 0;

	if ( m_flHealth <= 0 )
	{
		// Draw the dead material.
		surface()->DrawSetTexture( m_iDeadMaterialIndex );
		
		vert[0].Init( Vector2D( xpos, ypos ), Vector2D( uv1, uv1 ) );
		vert[1].Init( Vector2D( xpos + w, ypos ), Vector2D( uv2, uv1 ) );
		vert[2].Init( Vector2D( xpos + w, ypos + h ), Vector2D( uv2, uv2 ) );				
		vert[3].Init( Vector2D( xpos, ypos + h ), Vector2D( uv1, uv2 ) );

		surface()->DrawSetColor( Color( 255, 255, 255, m_iAlpha ) );
	}
	else
	{
		float flDamageY = h * ( 1.0f - m_flHealth );

		// Blend in the red "damage" part.
		surface()->DrawSetTexture( m_iMaterialIndex );

		Vector2D uv11( uv1, uv2 - m_flHealth );
		Vector2D uv21( uv2, uv2 - m_flHealth );
		Vector2D uv22( uv2, uv2 );
		Vector2D uv12( uv1, uv2 );

		vert[0].Init( Vector2D( xpos, flDamageY ), uv11 );
		vert[1].Init( Vector2D( xpos + w, flDamageY ), uv21 );
		vert[2].Init( Vector2D( xpos + w, ypos + h ), uv22 );				
		vert[3].Init( Vector2D( xpos, ypos + h ), uv12 );

		Color color = GetFgColor();
		int r, g, b, a;
		color.GetColor( r, g, b, a );
		surface()->DrawSetColor( Color( r, g, b, m_iAlpha ) );
	}

	surface()->DrawTexturedPolygon( 4, vert );
}

const char *zsUnusedStatusIcons[]
{
	"PlayerStatusHookBleedImage",
	"PlayerStatusMilkImage",
	"PlayerStatusGasImage",

	"PlayerStatus_MedicUberBulletResistImage",
	"PlayerStatus_MedicUberBlastResistImage",
	"PlayerStatus_MedicUberFireResistImage",
	"PlayerStatus_MedicSmallBulletResistImage",
	"PlayerStatus_MedicSmallBlastResistImage",
	"PlayerStatus_MedicSmallFireResistImage",

	"PlayerStatus_WheelOfDoom",

	"PlayerStatus_SoldierOffenseBuff",
	"PlayerStatus_SoldierDefenseBuff",
	"PlayerStatus_SoldierHealOnHitBuff",

	"PlayerStatus_SpyMarked",
	"PlayerStatus_Parachute",

	"PlayerStatus_RuneStrength",
	"PlayerStatus_RuneHaste",
	"PlayerStatus_RuneRegen",
	"PlayerStatus_RuneResist",
	"PlayerStatus_RuneVampire",
	"PlayerStatus_RuneReflect",
	"PlayerStatus_RunePrecision",
	"PlayerStatus_RuneAgility",
	"PlayerStatus_RuneKnockout",
	"PlayerStatus_RuneKing",
	"PlayerStatus_RunePlague",
	"PlayerStatus_RuneSupernova",
};


CTFHudPlayerHealth::CTFHudPlayerHealth( Panel *parent, const char *name ) : EditablePanel( parent, name )
{
	m_pHealthImage = new CTFHealthPanel( this, "PlayerStatusHealthImage" );	
	m_pHealthImageBG = new ImagePanel( this, "PlayerStatusHealthImageBG" );
	m_pHealthBonusImage = new ImagePanel( this, "PlayerStatusHealthBonusImage" );

	m_pBuildingHealthImageBG = new ImagePanel( this, "BuildingStatusHealthImageBG" );
	m_pBuildingHealthImageBG->SetVisible( false );

	m_pHealthValueLabel = new CExLabel( this, "PlayerStatusHealthValue", "%Health%" );

	m_pBleedImage = new ImagePanel( this, "PlayerStatusBleedImage" );
	m_pMarkedForDeathImage = new ImagePanel( this, "PlayerStatusMarkedForDeathImage" );
	m_pMarkedForDeathImageSilent = new ImagePanel( this, "PlayerStatusMarkedForDeathSilentImage" );
	m_pSlowImage = new ImagePanel( this, "PlayerStatusSlowed" );
	m_pHasteImage = new ImagePanel( this, "PlayerStatus_RuneHaste" );

	m_flNextThink = 0.0f;

	m_iAlpha = 255;

	m_flHealthbarScale = 1.0f;

	// The order of each object in the vector array determine their X-Pos priority.
	m_vecBuffInfo.AddToTail(
		new CTFBuffInfo( TF_COND_RESISTANCE_BUFF,
			BUFF_CLASS_CIVILIAN_OFFENSE,
			new ImagePanel( this, "PlayerStatusCivilianBuff" ),
			"../HUD/civilian_buff_blue",
			"../HUD/civilian_buff_red",
			"../HUD/civilian_buff_yellow",
			"../HUD/civilian_buff_green" ) );
}


void CTFHudPlayerHealth::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
	m_nHealth = -1;
}


void CTFHudPlayerHealth::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( GetResFilename() );

	// We grab the base pos and size of each of these images since we want to scale them dynamically.
	if ( m_pHealthBonusImage )
		m_pHealthBonusImage->GetBounds( m_BonusHealthPosSize.m_OrigX, m_BonusHealthPosSize.m_OrigY, m_BonusHealthPosSize.m_OrigW, m_BonusHealthPosSize.m_OrigH );

	if ( m_pHealthImage )
		m_pHealthImage->GetBounds( m_HealthPosSize.m_OrigX, m_HealthPosSize.m_OrigY, m_HealthPosSize.m_OrigW, m_HealthPosSize.m_OrigH );

	if ( m_pHealthImageBG )
		m_pHealthImageBG->GetBounds( m_HealthBGPosSize.m_OrigX, m_HealthBGPosSize.m_OrigY, m_HealthBGPosSize.m_OrigW, m_HealthBGPosSize.m_OrigH );

	m_flNextThink = 0.0f;

	BaseClass::ApplySchemeSettings( pScheme );

	Panel *pUnusedStatusIcon;
	for ( int i = 0, c = ARRAYSIZE( zsUnusedStatusIcons ); i < c; i++ )
	{
		pUnusedStatusIcon = FindChildByName( zsUnusedStatusIcons[i] );
		if ( pUnusedStatusIcon )
		{
			pUnusedStatusIcon->SetVisible( false );
		}
	}
}

void CTFHudPlayerHealth::SetHealthbarScale( float flScale )
{
	m_flHealthbarScale = flScale;

	// Here's a tutorial on how to scale an element around its centre in VGUI!
	// You must make its position:
	// = original_pos_x + (original_width * scale * (1/2*scale - 1/2))
	// = original_pos_y + (original_height * scale * (1/2*scale - 1/2))
	// "Why", you ask? Because fuck you that's why. 

	float flFactor = flScale * ((1.0f/(2.0f*flScale)) - 0.5f);

	m_pHealthImage->SetBounds( m_HealthPosSize.m_OrigX + (m_HealthPosSize.m_OrigW * flFactor),
		m_HealthPosSize.m_OrigY + (m_HealthPosSize.m_OrigH * flFactor),
		m_HealthPosSize.m_OrigW * flScale,
		m_HealthPosSize.m_OrigH * flScale );

	m_pHealthImageBG->SetBounds( m_HealthBGPosSize.m_OrigX + (m_HealthBGPosSize.m_OrigW * flFactor),
		m_HealthBGPosSize.m_OrigY + (m_HealthBGPosSize.m_OrigH * flFactor),
		m_HealthBGPosSize.m_OrigW * flScale,
		m_HealthBGPosSize.m_OrigH * flScale );
}

void CTFHudPlayerHealth::SetHealthTextVisibility( bool bVisible )
{
	m_pHealthValueLabel->SetVisible( bVisible );
}

// Sets the alpha overrides of the child components.
void CTFHudPlayerHealth::SetChildrenAlpha( int iAlpha )
{
	m_pHealthImage->m_iAlpha = iAlpha;
	
	m_pHealthBonusImage->SetAlpha(iAlpha);
	m_pHealthImageBG->SetAlpha( iAlpha );
	m_pBuildingHealthImageBG->SetAlpha( iAlpha );
}


void CTFHudPlayerHealth::SetHealth( int iNewHealth, int iMaxHealth, int	iMaxBuffedHealth )
{
	int nPrevHealth = m_nHealth;

	// Set our health.
	m_nHealth = iNewHealth;
	m_nMaxHealth = iMaxHealth;
	m_pHealthImage->SetHealth( (float)( m_nHealth ) / (float)( m_nMaxHealth ) );
	m_pHealthImage->SetFgColor( Color( 255, 255, 255, m_iAlpha ) );

	if ( m_nHealth <= 0 )
	{
		m_pHealthImageBG->SetVisible( false );
		HideHealthBonusImage();
	}
	else
	{
		m_pHealthImageBG->SetVisible( true );

		float flFactor = m_flHealthbarScale * ((1.0f / (2.0f*m_flHealthbarScale)) - 0.5f);

		// Are we getting a health bonus?
		if ( m_nHealth > m_nMaxHealth )
		{
			if ( !m_pHealthBonusImage->IsVisible() )
			{
				m_pHealthBonusImage->SetVisible( true );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulse" );
			}

			m_pHealthBonusImage->SetDrawColor( Color( 255, 255, 255, m_iAlpha ) );

			// Scale the flashing image based on how much health bonus we currently have.
			float flBoostMaxAmount = (iMaxBuffedHealth)-m_nMaxHealth;
			float flPercent = Min( 1.0f, ( m_nHealth - m_nMaxHealth ) / flBoostMaxAmount ); // Clamped to 1 to not cut off for values above 150%.

			int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj);
			int nSizeAdj = 2 * nPosAdj;

			m_pHealthBonusImage->SetBounds( m_BonusHealthPosSize.m_OrigX + ((m_BonusHealthPosSize.m_OrigW + nSizeAdj) * flFactor) - nPosAdj,
				m_BonusHealthPosSize.m_OrigY + ((m_BonusHealthPosSize.m_OrigH + nSizeAdj) * flFactor) - nPosAdj,
				(m_BonusHealthPosSize.m_OrigW + nSizeAdj) * m_flHealthbarScale,
				(m_BonusHealthPosSize.m_OrigH + nSizeAdj) * m_flHealthbarScale );
		}
		// Are we close to dying?
		else if ( m_nHealth < m_nMaxHealth * m_flHealthDeathWarning )
		{
			if ( !m_pHealthBonusImage->IsVisible() )
			{
				m_pHealthBonusImage->SetVisible( true );
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulse" );
			}

			m_pHealthBonusImage->SetDrawColor( m_clrHealthDeathWarningColor );
			m_pHealthBonusImage->SetAlpha( m_iAlpha );

			// Scale the flashing image based on how much health bonus we currently have.
			float flBoostMaxAmount = m_nMaxHealth * m_flHealthDeathWarning;
			float flPercent = ( flBoostMaxAmount - m_nHealth ) / flBoostMaxAmount;

			int nPosAdj = RoundFloatToInt( flPercent * m_nHealthBonusPosAdj );
			int nSizeAdj = 2 * nPosAdj;

			m_pHealthBonusImage->SetBounds( m_BonusHealthPosSize.m_OrigX + ((m_BonusHealthPosSize.m_OrigW + nSizeAdj) * flFactor) - nPosAdj,
				m_BonusHealthPosSize.m_OrigY + ((m_BonusHealthPosSize.m_OrigH + nSizeAdj) * flFactor) - nPosAdj,
				(m_BonusHealthPosSize.m_OrigW + nSizeAdj) * m_flHealthbarScale,
				(m_BonusHealthPosSize.m_OrigH + nSizeAdj) * m_flHealthbarScale );

			m_pHealthImage->SetFgColor( m_clrHealthDeathWarningColor );
			m_pHealthImage->SetAlpha( m_iAlpha );
		}
		// Turn it off.
		else
		{
			HideHealthBonusImage();
		}
	}

	// Set our health display value.
	if ( nPrevHealth != m_nHealth )
	{
		if ( m_nHealth > 0 )
		{
			SetDialogVariable( "Health", m_nHealth );
		}
		else
		{
			SetDialogVariable( "Health", "" );
		}	
	}

	// Set our max health display value, too.
	if ( m_nHealth <= ( m_nMaxHealth - 5 ) )
	{
		SetDialogVariable( "MaxHealth", m_nMaxHealth );
	}
	else
	{
		SetDialogVariable( "MaxHealth", "" );
	}
}


void CTFHudPlayerHealth::HideHealthBonusImage( void )
{
	if ( m_pHealthBonusImage->IsVisible() )
	{
		m_pHealthBonusImage->SetBounds( m_BonusHealthPosSize.m_OrigX, m_BonusHealthPosSize.m_OrigY,	m_BonusHealthPosSize.m_OrigW, m_BonusHealthPosSize.m_OrigH );
		m_pHealthBonusImage->SetVisible( false );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthBonusPulseStop" );
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "HudHealthDyingPulseStop" );
	}
}


void CTFHudPlayerHealth::ShowBuildingBG( bool bShow )
{
	m_pBuildingHealthImageBG->SetVisible( bShow );
}


static void SetPlayerHealthImagePanelVisibility( CTFPlayer *pPlayer, ETFCond eCond, vgui::ImagePanel *pImagePanel, int& nXOffset, const Color& colorIfVisible, bool bOnlyWhen = true )
{
	Assert( pImagePanel );

	if ( bOnlyWhen && pPlayer->m_Shared.InCond( eCond ) && !pImagePanel->IsVisible() )
	{
		pImagePanel->SetVisible( true );
		pImagePanel->SetDrawColor( colorIfVisible );

		// Reposition ourselves and increase the offset if we are active.
		int x,y;
		pImagePanel->GetPos( x, y );
		pImagePanel->SetPos( nXOffset, y );
		nXOffset += 100.0f;
	}
}


void CTFBuffInfo::Update( C_TFPlayer *pPlayer )
{
	Assert( m_pImagePanel && pPlayer );

	if ( pPlayer->m_Shared.InCond( m_eCond ) )
	{
		if ( ( m_pzsBlueImage && m_pzsBlueImage[0] && m_pzsRedImage && m_pzsRedImage[0] ) || ( m_pzsYellowImage && m_pzsYellowImage[0] && m_pzsGreenImage && m_pzsGreenImage[0] ) )
		{
			switch ( pPlayer->GetTeamNumber() )
			{
				case TF_TEAM_RED:
					m_pImagePanel->SetImage( m_pzsRedImage );
					break;
				default:
				case TF_TEAM_BLUE:
					m_pImagePanel->SetImage( m_pzsBlueImage );
					break;
				case TF_TEAM_GREEN:
					m_pImagePanel->SetImage( m_pzsGreenImage );
					break;
				case TF_TEAM_YELLOW:
					m_pImagePanel->SetImage( m_pzsYellowImage );
					break;
			}
		}
	}
}

// This may look weird up here, but it's alright.
#define STUNNED_STATUS_COLOR	Color( iColorFade, iColorFade, 75 - ( iColorFade / 3 ), 255 )


void CTFHudPlayerHealth::OnThink()
{
	if ( m_flNextThink < gpGlobals->curtime )
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pPlayer )
		{
			int iOldHealth = m_nHealth;
			SetHealth( pPlayer->GetHealth(), pPlayer->GetMaxHealth(), pPlayer->m_Shared.GetMaxBuffedHealth() );

			if ( tf2c_low_health_sound.GetBool() && pPlayer->IsAlive() )
			{
				float flWarningHealth = m_nMaxHealth * tf2c_low_health_sound_threshold.GetFloat();
				if ( (float)iOldHealth >= flWarningHealth && (float)m_nHealth < flWarningHealth )
				{
					CLocalPlayerFilter filter;
					C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, "TFPlayer.LowHealth" );
				}
			}

			// Find our starting point, just above the health '+'.
			int nXOffset, y;
			m_pHealthImage->GetPos( nXOffset, y );

			// Nudge over a bit to get centered.
			nXOffset += 25;

			// Turn all the panels off, and below conditionally turn them on.
			FOR_EACH_VEC( m_vecBuffInfo, i )
			{
				m_vecBuffInfo[i]->m_pImagePanel->SetVisible( false );
			}

			m_pHasteImage->SetVisible( false );
			m_pSlowImage->SetVisible( false );
			m_pBleedImage->SetVisible( false );
			m_pMarkedForDeathImage->SetVisible( false );
			m_pMarkedForDeathImageSilent->SetVisible( false );

			if ( tf2c_show_status_effect_icons.GetBool() )
			{
				int iColorOffset = (int)( gpGlobals->realtime * 10 ) % 5;
				int iColorFade = 160 + ( iColorOffset * 10 );

				// Cycle through all the buffs and update them.
				CUtlVector<BuffClass_t> m_vecActiveClasses;
				FOR_EACH_VEC( m_vecBuffInfo, i )
				{
					// Skip if this class of buff is already being drawn.
					if ( m_vecActiveClasses.Find( m_vecBuffInfo[i]->m_eClass ) != m_vecActiveClasses.InvalidIndex() )
						continue;

					m_vecBuffInfo[i]->Update( pPlayer );
					SetPlayerHealthImagePanelVisibility( pPlayer, m_vecBuffInfo[i]->m_eCond, m_vecBuffInfo[i]->m_pImagePanel, nXOffset, Color( 255, 255, 255, min(iColorFade, m_iAlpha) ) );

					// This class of buff is now active.
					if ( m_vecBuffInfo[i]->m_pImagePanel->IsVisible() )
					{
						m_vecActiveClasses.AddToTail( m_vecBuffInfo[i]->m_eClass );
					}
				}

				// The order of when each of these functions are executed determine their X-Pos priority.
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_MARKEDFORDEATH,			m_pMarkedForDeathImage,			nXOffset,	Color(255 - iColorFade, 245 - iColorFade, 245 - iColorFade, m_iAlpha) );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_SUPERMARKEDFORDEATH,		m_pMarkedForDeathImage,			nXOffset,	Color(255 - iColorFade, 245 - iColorFade, 245 - iColorFade, m_iAlpha) );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_MARKEDFORDEATH_SILENT,	m_pMarkedForDeathImageSilent,	nXOffset,	Color(125 - iColorFade, 255 - iColorFade, 255 - iColorFade, m_iAlpha) );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_SUPERMARKEDFORDEATH_SILENT, m_pMarkedForDeathImageSilent, nXOffset,	Color(125 - iColorFade, 255 - iColorFade, 255 - iColorFade, m_iAlpha) );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_BLEEDING,					m_pBleedImage,					nXOffset,	Color( iColorFade, 0, 0, m_iAlpha ) );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_STUNNED,					m_pSlowImage,					nXOffset,	STUNNED_STATUS_COLOR, pPlayer->m_Shared.IsSnared() );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_MIRV_SLOW,				m_pSlowImage,					nXOffset,   STUNNED_STATUS_COLOR );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_TRANQUILIZED,				m_pSlowImage,					nXOffset,	STUNNED_STATUS_COLOR );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_CIV_SPEEDBUFF,			m_pHasteImage,					nXOffset,	STUNNED_STATUS_COLOR );
				SetPlayerHealthImagePanelVisibility( pPlayer, TF_COND_SPEED_BOOST,				m_pHasteImage,					nXOffset,	STUNNED_STATUS_COLOR );

			}
		}

		m_flNextThink = gpGlobals->curtime + 0.05f;
	}
}

DECLARE_HUDELEMENT( CTFHudPlayerStatus );


CTFHudPlayerStatus::CTFHudPlayerStatus( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPlayerStatus" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pHudPlayerClass = new CTFHudPlayerClass( this, "HudPlayerClass" );
	m_pHudPlayerHealth = new CTFHudPlayerHealth( this, "HudPlayerHealth" );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );
}


void CTFHudPlayerStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// HACK: Work around the scheme application order failing
	// to reload the player class hud element's scheme in minmode.
	ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
	if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
	{
		m_pHudPlayerClass->InvalidateLayout( false, true );
	}
}


void CTFHudPlayerStatus::Reset()
{
	if ( m_pHudPlayerClass )
	{
		m_pHudPlayerClass->Reset();
	}

	if ( m_pHudPlayerHealth )
	{
		m_pHudPlayerHealth->Reset();
	}
}


void CTFClassImage::SetClass( int iTeam, int iClass, int iCloakstate )
{
	if ( iTeam < FIRST_GAME_TEAM )
		return;

	if ( iClass == TF_CLASS_UNDEFINED || iClass == TF_CLASS_RANDOM )
		return;

	char szImage[128];
	szImage[0] = '\0';

	V_strcpy_safe( szImage, g_szClassImages[iClass] );
	V_strcat_safe( szImage, iTeam == TF_TEAM_GLOBAL ? "global" : g_aTeamLowerNames[iTeam] );

	switch ( iCloakstate )
	{
		case 2:
			V_strcat_safe( szImage, "_cloak" );
			break;
		case 1:
			V_strcat_safe( szImage, "_halfcloak" );
			break;
		default:
			break;
	}

	SetImage( szImage );
}
