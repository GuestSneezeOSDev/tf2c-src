//=============================================================================//
//
// Purpose: HUD for drawing screen overlays.
//
//=============================================================================//
#include "cbase.h"
#include "tf_hud_screenoverlays.h"
#include "iclientmode.h"
#include "c_tf_player.h"
#include "view_scene.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

struct ScreenOverlayData_t
{
	const char *pszName;
	bool bTeamColored;
};

ScreenOverlayData_t g_aScreenOverlays[TF_OVERLAY_COUNT] =
{
	{ "effects/imcookin",			false	},
	{ "effects/invuln_overlay_%s",	true	},
	{ "effects/bleed_overlay",		false	},
};

DECLARE_HUDELEMENT( CHudScreenOverlays );

CHudScreenOverlays::CHudScreenOverlays( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudScreenOverlays" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_fColorValue = 0.0f;

	ListenForGameEvent( "localplayer_changeteam" );
}


void CHudScreenOverlays::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	int iWide, iTall;
	ipanel()->GetSize( GetVParent(), iWide, iTall );
	SetBounds( 0, 0, iWide, iTall );
	SetZPos( -10 );
}


void CHudScreenOverlays::FireGameEvent( IGameEvent *event )
{
	if ( !V_strcmp( event->GetName(), "localplayer_changeteam" ) )
	{
		C_TFPlayer *pLocalPlayer = GetLocalObservedPlayer( true );
		InitOverlayMaterials( pLocalPlayer ? pLocalPlayer->GetTeamNumber() : TF_TEAM_BLUE );
	}
}


void CHudScreenOverlays::Cache( bool bCache /*= true*/ )
{
	// Precache all overlays.
	for ( int i = 0; i < TF_OVERLAY_COUNT; i++ )
	{
		if ( g_aScreenOverlays[i].bTeamColored )
		{
			// Precache all team colored versions.
			ForEachTeamName( [=]( const char *pszTeam )
			{
				ReferenceMaterial( VarArgs( g_aScreenOverlays[i].pszName, pszTeam ), bCache );
			} );
		}
		else
		{
			ReferenceMaterial( g_aScreenOverlays[i].pszName, bCache );
		}
	}
}

#define MAKEFLAG(x)	( 1 << x )


void CHudScreenOverlays::Paint( void )
{
	C_TFPlayer *pPlayer = GetLocalObservedPlayer( true );
	if ( !pPlayer )
		return;
	
	if ( pPlayer->ShouldDrawThisPlayer() )
		return;
	
	// Check which overlays we should draw.
	int nOverlaysToDraw = 0;

	if ( pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE ) )
	{
		nOverlaysToDraw |= MAKEFLAG( TF_OVERLAY_INVULN );
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_BURNING ) )
	{
		nOverlaysToDraw |= MAKEFLAG( TF_OVERLAY_BURNING );

		if ( pPlayer->m_Shared.GetFlameRemoveTime() - gpGlobals->curtime < 1.0f )
		{
			m_fColorValue = Approach( 0.0f, m_fColorValue, 255.0f * gpGlobals->frametime );
		}
		else
		{
			m_fColorValue = Approach( 255.0f, m_fColorValue, 555.0f * gpGlobals->frametime );
		}
	}
	else
	{
		m_fColorValue = 0.0f;
	}

	if ( pPlayer->m_Shared.InCond( TF_COND_BLEEDING ) )
	{
		nOverlaysToDraw |= MAKEFLAG( TF_OVERLAY_BLEED );
	}

	// Draw overlays, the order is important.
	for ( int i = 0; i < TF_OVERLAY_COUNT; i++ )
	{
		if ( ( nOverlaysToDraw & MAKEFLAG( i ) ) && m_ScreenOverlays[i].IsValid() )
		{
			int x, y, w, h;
			GetBounds( x, y, w, h );

			if ( m_ScreenOverlays[i]->NeedsFullFrameBufferTexture() )
			{
				// FIXME: Check with multi/sub-rect renders. Should this be 0,0,w,h instead?
				DrawScreenEffectMaterial( m_ScreenOverlays[i], x, y, w, h );
			}
			else if ( m_ScreenOverlays[i]->NeedsPowerOfTwoFrameBufferTexture() )
			{
				// First copy the FB off to the offscreen texture
				UpdateRefractTexture( x, y, w, h, true );

				// Now draw the entire screen using the material...
				CMatRenderContextPtr pRenderContext( materials );
				ITexture *pTexture = GetPowerOfTwoFrameBufferTexture();
				int sw = pTexture->GetActualWidth();
				int sh = pTexture->GetActualHeight();
				// Note - don't offset by x,y - already done by the viewport.
				pRenderContext->DrawScreenSpaceRectangle( m_ScreenOverlays[i], 0, 0, w, h,
					0, 0, sw - 1, sh - 1, sw, sh );
			}
			else
			{
				byte color[4] = { (int)m_fColorValue, (int)m_fColorValue, (int)m_fColorValue, 255 };
				render->ViewDrawFade( color, m_ScreenOverlays[i] );
			}
		}
	}
}


void CHudScreenOverlays::InitOverlayMaterials( int iTeam )
{
	for ( int i = 0; i < TF_OVERLAY_COUNT; i++ )
	{
		const char *pszName = g_aScreenOverlays[i].pszName;
		if ( g_aScreenOverlays[i].bTeamColored )
		{
			pszName = ConstructTeamParticle( g_aScreenOverlays[i].pszName, iTeam );
		}

		m_ScreenOverlays[i].Init( pszName, TEXTURE_GROUP_CLIENT_EFFECTS );
	}
}
