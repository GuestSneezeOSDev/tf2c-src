//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "tf_hud_itemeffectmeter.h"
#include "tf_weapon_invis.h"
#include "tf_weapon_umbrella.h"
#include "tf_weapon_lunchbox.h"
#include "tf_weapon_mirv.h"
#include "tf_weapon_coilgun.h"
#include "tf_weapon_taser.h"
#include "tf_weapon_russianroulette.h"
#include "tf_weapon_beacon.h"	// !!! foxysen beacon
#include "tf_weapon_doubleshotgun.h"	// !!! foxysen doubleshotgun
#include "tf_weapon_riot.h"		// !!! kaydemonlp riot shield
#include "tf_weapon_brimstonelauncher.h"	// !!! azzy brimstone
#include "tf_weapon_throwingknife.h"	// !!! foxysen throwingknife
#include "tf_weapon_heallauncher.h"	// !!! hogyn melyn medic gl
#include "tf_weapon_anchor.h"

#include "tf_weapon_throwable.h"
#include "tf_imagepanel.h"
#include <game/client/iviewport.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui/ILocalize.h"

#ifdef TF2C_BETA
#include "tf_weapon_pillstreak.h"	// !!! foxysen pillstreak
#include "tf_weapon_leapknife.h" // !!! sappho leapknife
#endif

#include "tf_randomizer_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf2c_vip_abilities;

#define DECLARE_ITEM_EFFECT_METER( weaponClass, weaponType, beeps, resfile ) \
hNewMeter = new CHudItemEffectMeter_Weapon<weaponClass>( pszElementName, pPlayer, weaponType, beeps, resfile ); \
if ( hNewMeter ) \
{ \
	gHUD.AddHudElement( hNewMeter ); \
	outMeters.AddToHead( hNewMeter ); \
	hNewMeter->SetVisible( false ); \
}

#define DECLARE_WEARABLE_EFFECT_METER( wearableClass, wearableType, beeps, resfile ) \
hNewMeter = new CHudItemEffectMeter_Wearable<wearableClass>( pszElementName, pPlayer, wearableType, beeps, resfile ); \
if ( hNewMeter ) \
{ \
	gHUD.AddHudElement( hNewMeter ); \
	outMeters.AddToHead( hNewMeter ); \
	hNewMeter->SetVisible( false ); \
}

using namespace vgui;

IMPLEMENT_AUTO_LIST( IHudItemEffectMeterAutoList );
											
CItemEffectMeterManager g_ItemEffectMeterManager;


CItemEffectMeterManager::~CItemEffectMeterManager()
{
	ClearExistingMeters();
}


void CItemEffectMeterManager::ClearExistingMeters()
{
	for ( int i = 0, c = m_Meters.Count(); i < c; i++ )
	{
		gHUD.RemoveHudElement( m_Meters[i].Get() );
		delete m_Meters[i].Get();
	}

	m_Meters.RemoveAll();
}


int CItemEffectMeterManager::GetNumEnabled( void )
{
	int nCount = 0;

	for ( int i = 0, c = m_Meters.Count(); i < c; i++ )
	{
		if ( m_Meters[i] && m_Meters[i]->IsEnabled() )
		{
			nCount++;
		}
	}

	return nCount;
}


void CItemEffectMeterManager::SetPlayer( C_TFPlayer *pPlayer )
{
	StopListeningForAllEvents();
	ListenForGameEvent( "post_inventory_application" );

	ClearExistingMeters();

	if ( pPlayer )
	{
		CHudItemEffectMeter::CreateHudElementsForClass( pPlayer, m_Meters );
	}
}


void CItemEffectMeterManager::Update( C_TFPlayer *pPlayer )
{
	for ( int i = 0; i < m_Meters.Count(); i++ )
	{
		if ( m_Meters[i] )
		{
			m_Meters[i]->Update( pPlayer );
		}
	}
}


void CItemEffectMeterManager::FireGameEvent( IGameEvent *event )
{
	bool bNeedsUpdate = false;

	if ( FStrEq( "post_inventory_application", event->GetName() ) )
	{
		// Force a refresh. Our items may have changed causing us to now draw, etc.
		int iUserID = event->GetInt( "userid" );
		C_TFPlayer *pPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
		if ( pPlayer && pPlayer->GetUserID() == iUserID )
		{
			bNeedsUpdate = true;
		}
	}

	if ( bNeedsUpdate )
	{
		SetPlayer( C_TFPlayer::GetLocalTFPlayer() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudItemEffectMeter::CHudItemEffectMeter( const char *pszElementName, C_TFPlayer *pPlayer ) : CHudElement( pszElementName ), BaseClass( NULL, "HudItemEffectMeter" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	if ( !m_pProgressBar )
	{
		m_pProgressBar = new ContinuousProgressBar( this, "ItemEffectMeter" );
	}

	if ( !m_pLabel )
	{
		m_pLabel = new Label( this, "ItemEffectMeterLabel", "" );
	}

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_pPlayer = pPlayer;
	m_bEnabled = true;
	m_flOldProgress = 1.f;

	RegisterForRenderGroup( "inspect_panel" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHudItemEffectMeter::~CHudItemEffectMeter()
{
}


void CHudItemEffectMeter::CreateHudElementsForClass( C_TFPlayer *pPlayer, CUtlVector<vgui::DHANDLE<CHudItemEffectMeter>>& outMeters )
{
	vgui::DHANDLE<CHudItemEffectMeter> hNewMeter;
	const char *pszElementName = "HudItemEffectMeter";

	// This entire system was DEFINITELY not designed for this but I can't do much about it now
	DECLARE_WEARABLE_EFFECT_METER( CTFWearable, 0, true, "resource/UI/HUDItemEffectMeter_Charge.res" );
	DECLARE_WEARABLE_EFFECT_METER( CTFWearable, 1, true, "resource/UI/HUDItemEffectMeter_Charge.res" );

	// Use a for loop here because of randomizer mode.
	for ( int iClass = TF_CLASS_SCOUT; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		// If we're not in randomizer mode, just set the class index here and
		bool bHasRandomizedItems = GetRandomizerManager()->RandomizerMode( TF_RANDOMIZER_ITEMS );
		if ( !bHasRandomizedItems )
		{
			iClass = pPlayer->GetPlayerClass()->GetClassIndex();
		}

		// generic
		DECLARE_ITEM_EFFECT_METER(CTFWeaponMirv2, TF_WEAPON_GRENADE_MIRV2, true, "resource/UI/HUDItemEffectMeter_Scout.res");

		switch ( iClass )
		{
			case TF_CLASS_SCOUT:
				DECLARE_ITEM_EFFECT_METER( CTFWeaponThrowable, TF_WEAPON_THROWABLE_BRICK, true, "resource/UI/HUDItemEffectMeter_Scout.res" );
				break;
		
			case TF_CLASS_PYRO:
				//DECLARE_ITEM_EFFECT_METER(CTFDoubleShotgun, TF_WEAPON_DOUBLESHOTGUN, false, "resource/UI/HUDItemEffectMeter_Sniper.res");	// !!! foxysen doubleshotgun
				DECLARE_ITEM_EFFECT_METER(CTFBeacon, TF_WEAPON_BEACON, false, NULL); // !!! foxysen beacon
				break;

			case TF_CLASS_DEMOMAN:
				DECLARE_ITEM_EFFECT_METER(CTFWeaponMirv, TF_WEAPON_GRENADE_MIRV, true, "resource/UI/HUDItemEffectMeter_Scout.res");
#ifdef TF2C_BETA
				DECLARE_ITEM_EFFECT_METER( CTFPillstreak, TF_WEAPON_PILLSTREAK, false, "resource/UI/HUDItemEffectMeter_Pillstreak.res" ); // !!! foxysen pillstreak
#endif
				DECLARE_ITEM_EFFECT_METER(CTFBrimstoneLauncher, TF_WEAPON_BRIMSTONELAUNCHER, true, "resource/UI/HUDItemEffectMeter_Chekhov.res" );	// !!! azzy brimstone
				break;

			case TF_CLASS_MEDIC:
				DECLARE_ITEM_EFFECT_METER( CTFTaser, TF_WEAPON_TASER, true, "resource/UI/HUDItemEffectMeter_Scout.res" );
				//DECLARE_ITEM_EFFECT_METER( CTFHealLauncher, TF_WEAPON_HEALLAUNCHER, true, "resource/UI/HUDItemEffectMeter_Nader.res" ); // !!! hogyn medic gl
				break;

			case TF_CLASS_HEAVYWEAPONS:
				DECLARE_ITEM_EFFECT_METER( CTFLunchBox, TF_WEAPON_LUNCHBOX, true, NULL );
				DECLARE_ITEM_EFFECT_METER( CTFRussianRoulette, TF_WEAPON_RUSSIANROULETTE, false, "resource/UI/HUDItemEffectMeter_Chekhov.res" );
				DECLARE_ITEM_EFFECT_METER( CTFRiot, TF_WEAPON_RIOT_SHIELD, true, "resource/UI/HUDItemEffectMeter_Scout.res" );			
				break;

			/* case TF_CLASS_ENGINEER:
				DECLARE_ITEM_EFFECT_METER( CTFCoilGun, TF_WEAPON_COILGUN, true, "resource/UI/HUDItemEffectMeter_Engineer.res" );
				break; */

			case TF_CLASS_SNIPER:
				DECLARE_ITEM_EFFECT_METER(CTFWeaponMirv, TF_WEAPON_GRENADE_MIRV, true, "resource/UI/HUDItemEffectMeter_Scout.res");	// !!! foxysen sniperbait
				break;

			case TF_CLASS_SOLDIER:
				DECLARE_ITEM_EFFECT_METER( CTFWeaponMirv, TF_WEAPON_GRENADE_MIRV, true, "resource/UI/HUDItemEffectMeter_Scout.res" );	// !!! trotim rockettnt
				DECLARE_ITEM_EFFECT_METER(CTFAnchor, TF_WEAPON_ANCHOR, false, "resource/UI/HudItemEffectMeter_Anchor.res");
				break;

			case TF_CLASS_SPY:
				DECLARE_ITEM_EFFECT_METER(CTFWeaponInvis, TF_WEAPON_INVIS, false, NULL);
				DECLARE_ITEM_EFFECT_METER(CTFThrowingKnife, TF_WEAPON_THROWINGKNIFE, true, "resource/UI/HUDItemEffectMeter_Sniper.res");	// !!! foxysen throwingknife
#ifdef TF2C_BETA
				DECLARE_ITEM_EFFECT_METER(CTFLeapKnife, TF_WEAPON_LEAPKNIFE, true, "resource/UI/HUDItemEffectMeter_Sniper.res");	// !!! foxysen throwingknife
#endif
				break;

			case TF_CLASS_CIVILIAN:
				DECLARE_ITEM_EFFECT_METER( CTFUmbrella, TF_WEAPON_UMBRELLA, true, NULL );
				break;
		}

		// ...break early.
		if ( !bHasRandomizedItems )
		{
			break;
		}
	}
}


void CHudItemEffectMeter::ApplySchemeSettings( IScheme *pScheme )
{
	// Load control settings.
	LoadControlSettings( GetResFile() );

	// Update the label.
	const wchar_t *pLocalized = g_pVGuiLocalize->Find( GetLabelText() );
	if ( pLocalized )
	{
		wchar_t wszLabel[128];
		V_wcsncpy( wszLabel, pLocalized, sizeof( wszLabel ) );

		wchar_t wszFinalLabel[128];
		UTIL_ReplaceKeyBindings( wszLabel, 0, wszFinalLabel, sizeof( wszFinalLabel ) );

		m_pLabel->SetText( wszFinalLabel, true );
	}
	else
	{
		m_pLabel->SetText( GetLabelText() );	
	}

	BaseClass::ApplySchemeSettings( pScheme );

	CTFImagePanel *pIcon = dynamic_cast<CTFImagePanel *>( FindChildByName( "ItemEffectIcon" ) );
	if ( pIcon )
	{
		pIcon->SetImage( GetIconName() );
	}
}


void CHudItemEffectMeter::PerformLayout()
{
	BaseClass::PerformLayout();

	// Slide over by 1 for Medic.
	int iOffset = 0;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_MEDIC )
	{
		iOffset = 1;
	}

	if ( g_ItemEffectMeterManager.GetNumEnabled() + iOffset > 1 )
	{
		int xPos = 0, yPos = 0;
		GetPos( xPos, yPos );
		SetPos( xPos - m_iXOffset, yPos );
	}
}


bool CHudItemEffectMeter::ShouldDraw( void )
{
	bool bShouldDraw = true;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || !pPlayer->IsAlive() )
	{
		bShouldDraw = false;
	}
	else if ( !m_pProgressBar )
	{
		bShouldDraw = false;
	}
	else if ( IsEnabled() )
	{
		bShouldDraw = CHudElement::ShouldDraw();
	}
	else
	{
		bShouldDraw = false;
	}

	if ( IsVisible() != bShouldDraw )
	{
		SetVisible( bShouldDraw );

		if ( bShouldDraw )
		{
			// If we're going to be visible, redo our layout.
			InvalidateLayout( false, true );
		}
	}

	return bShouldDraw;
}


void CHudItemEffectMeter::Update( C_TFPlayer *pPlayer, const char *pSoundScript )
{
	if ( !IsEnabled() )
		return;

	if ( !m_pProgressBar )
		return;

	if ( !pPlayer )
		return;

	// Progress counts override progress bars.
	int iCount = GetCount();
	if ( iCount >= 0 )
	{
		if ( ShowPercentSymbol() )
		{
			SetDialogVariable( "progresscount", VarArgs( "%d%%", iCount ) );
		}
		else
		{
			SetDialogVariable( "progresscount", iCount );
		}
	}

	float flProgress = GetProgress();

	// Play a sound if we are refreshed.
	if ( C_TFPlayer::GetLocalTFPlayer() && flProgress >= 1.f && m_flOldProgress < 1.f &&
		pPlayer->IsAlive() && ShouldBeep() )
	{
		m_flOldProgress = flProgress;
		C_TFPlayer::GetLocalTFPlayer()->EmitSound( pSoundScript );	
	}
	else
	{
		m_flOldProgress = flProgress;
	}

	// Update the meter GUI element.
	m_pProgressBar->SetProgress( flProgress );

	// Flash the bar if this class implementation requires it.
	if ( ShouldFlash() )
	{
		int iOffset = ( (int)( gpGlobals->realtime * 10 ) ) % 10;
		int iRed = 160 + ( iOffset * 10 );
		m_pProgressBar->SetFgColor( Color( iRed, 0, 0, 255 ) );
	}
	else
	{
		m_pProgressBar->SetFgColor( GetFgColor() );
	}
}


const char *CHudItemEffectMeter::GetLabelText( void )
{ 
	return "#TF_Cloak"; 
}


float CHudItemEffectMeter::GetProgress( void )
{
	if ( m_pPlayer )
		return m_pPlayer->m_Shared.GetSpyCloakMeter() / 100.0f;
	
	return 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Tracks the weapon's regen.
//-----------------------------------------------------------------------------
template <class T>
CHudItemEffectMeter_Weapon<T>::CHudItemEffectMeter_Weapon( const char *pszElementName, C_TFPlayer *pPlayer, ETFWeaponID iWeaponID, bool bBeeps /*= true*/, const char *pszResFile /*= NULL*/ )
	: CHudItemEffectMeter( pszElementName, pPlayer )
{
	m_iWeaponID = iWeaponID;
	m_bBeeps = bBeeps;
	m_pszResFile = pszResFile;
}


template <class T>
const char *CHudItemEffectMeter_Weapon<T>::GetResFile( void )
{
	if ( m_pszResFile )
		return m_pszResFile;

	return "resource/UI/HudItemEffectMeter.res";
}

//-----------------------------------------------------------------------------
// Purpose: Caches the weapon reference.
//-----------------------------------------------------------------------------
template <class T>
T* CHudItemEffectMeter_Weapon<T>::GetWeapon( void )
{
	if ( m_bEnabled && m_pPlayer && !m_pWeapon )
	{
		m_pWeapon = dynamic_cast<T *>( m_pPlayer->Weapon_OwnsThisID( m_iWeaponID ) );
		if ( !m_pWeapon )
		{
			m_bEnabled = false;
		}
	}

	return m_pWeapon;
}


template <class T>
bool CHudItemEffectMeter_Weapon<T>::IsEnabled( void )
{
	T *pWeapon = GetWeapon();
	if ( pWeapon )
		return true;
	
	return CHudItemEffectMeter::IsEnabled();
}

template <class T>
const char *CHudItemEffectMeter_Weapon<T>::GetLabelText( void )
{
	T *pWeapon = GetWeapon();
	if ( pWeapon )
	{
		string_t strCustomText = NULL_STRING;
		CALL_ATTRIB_HOOK_STRING_ON_OTHER( pWeapon, strCustomText, meter_label );
		if ( strCustomText != NULL_STRING )
			return STRING(strCustomText);
		
		return pWeapon->GetEffectLabelText();
	}
	
	return "";
}

template <class T>
float CHudItemEffectMeter_Weapon<T>::GetProgress( void )
{
	T *pWeapon = GetWeapon();
	if ( pWeapon )
		return pWeapon->GetProgress();
	
	return 0.0f;
}


template <class T>
void CHudItemEffectMeter_Weapon<T>::Update( C_TFPlayer *pPlayer, const char *pSoundScript )
{
	// Uncomment this and stick things here for weapon-specific overrides.
	/*T *pWeapon = GetWeapon();
	if ( pWeapon )
	{
	
	}*/

	CHudItemEffectMeter::Update( pPlayer, pSoundScript );
}


template <class T>
bool CHudItemEffectMeter_Weapon<T>::ShouldDraw()
{
	return CHudItemEffectMeter::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: Tracks the wearable's regen.
//-----------------------------------------------------------------------------
template <class T>
CHudItemEffectMeter_Wearable<T>::CHudItemEffectMeter_Wearable( const char *pszElementName, C_TFPlayer *pPlayer, int iWearableNum, bool bBeeps /*= true*/, const char *pszResFile /*= NULL*/ )
	: CHudItemEffectMeter( pszElementName, pPlayer )
{
	m_iWearableNum = iWearableNum;
	m_bBeeps = bBeeps;
	m_pszResFile = pszResFile;
}


template <class T>
const char *CHudItemEffectMeter_Wearable<T>::GetResFile( void )
{
	if ( m_pszResFile )
		return m_pszResFile;

	return "resource/UI/HudItemEffectMeter.res";
}

//-----------------------------------------------------------------------------
// Purpose: Caches the weapon reference.
//-----------------------------------------------------------------------------
template <class T>
T* CHudItemEffectMeter_Wearable<T>::GetWearable( void )
{
	if ( m_bEnabled && m_pPlayer && !m_pWearable )
	{
		int iCount = 0;
		for ( int i = 0, c = m_pPlayer->GetNumWearables(); i < c; i++ )
		{
			CTFWearable *pWearable = (CTFWearable*)m_pPlayer->GetWearable(i);

			// Don't track disguise wearables or cosmetic wearables
			if( pWearable && !pWearable->IsDisguiseWearable() && !pWearable->GetWeaponAssociatedWith() && pWearable->UsesProgressBar() )
			{
				if( iCount == m_iWearableNum )
				{
					m_pWearable = pWearable;
					InvalidateLayout( true, true );
					return pWearable;
				}
				else
					iCount++;
			}
		}

		if ( !m_pWearable )
		{
			m_bEnabled = false;
		}
	}

	return m_pWearable;
}


template <class T>
bool CHudItemEffectMeter_Wearable<T>::IsEnabled( void )
{
	T *pWearable = GetWearable();
	if ( pWearable )
		return true;
	
	return CHudItemEffectMeter::IsEnabled();
}


template <class T>
float CHudItemEffectMeter_Wearable<T>::GetProgress( void )
{
	T *pWearable = GetWearable();
	if ( pWearable )
		return pWearable->GetProgress();
	
	return 0.0f;
}


template <class T>
void CHudItemEffectMeter_Wearable<T>::Update( C_TFPlayer *pPlayer, const char *pSoundScript )
{
	// Uncomment this and stick things here for weapon-specific overrides.
	/*T *pWearable = GetWearable();
	if ( pWearable )
	{
	
	}*/

	CHudItemEffectMeter::Update( pPlayer, pSoundScript );
}


template <class T>
bool CHudItemEffectMeter_Wearable<T>::ShouldDraw()
{
	return CHudItemEffectMeter::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Specializations for meters that do unique things follow after this...
//-----------------------------------------------------------------------------

// CTFUmbrella

template <>
bool CHudItemEffectMeter_Weapon<CTFUmbrella>::IsEnabled( void )
{
	if ( tf2c_vip_abilities.GetInt() < 2 )
		return false;

	return true;
}

// CTFCoilGun

template <>
bool CHudItemEffectMeter_Weapon<CTFCoilGun>::ShouldFlash( void )
{
	return GetProgress() >= 1.0f;
}

template <>
bool CHudItemEffectMeter_Weapon<CTFRiot>::ShouldFlash(void)
{
	// Uncomment this and stick things here for weapon-specific overrides.
	CTFRiot *pWeapon = GetWeapon();
	if ( pWeapon )
		return !pWeapon->CanBeSelected();

	return false;
}

template <>
void CHudItemEffectMeter_Weapon<CTFCoilGun>::ApplySchemeSettings( IScheme *scheme )
{
	CHudItemEffectMeter::ApplySchemeSettings( scheme );

	// Hide the number counter.
	Label *pCounter = dynamic_cast<Label *>( FindChildByName( "ItemEffectMeterCount" ) );
	if ( pCounter )
	{
		pCounter->SetVisible( false );
		pCounter->SetEnabled( false );
	}

	// Show the meter.
	ContinuousProgressBar *pMeter = dynamic_cast<ContinuousProgressBar *>( FindChildByName( "ItemEffectMeter" ) );
	if ( pMeter )
	{
		pMeter->SetVisible( true );
		pMeter->SetEnabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:		// !!! foxysen throwingknife
//-----------------------------------------------------------------------------
template <>
void CHudItemEffectMeter_Weapon<CTFThrowingKnife>::ApplySchemeSettings(IScheme *scheme)
{
	CHudItemEffectMeter::ApplySchemeSettings(scheme);

	// Hide the number counter.
	Label *pCounter = dynamic_cast<Label *>(FindChildByName("ItemEffectMeterCount"));
	if (pCounter)
	{
		pCounter->SetVisible(false);
		pCounter->SetEnabled(false);
	}

	// Show the meter.
	ContinuousProgressBar *pMeter = dynamic_cast<ContinuousProgressBar *>(FindChildByName("ItemEffectMeter"));
	if (pMeter)
	{
		pMeter->SetVisible(true);
		pMeter->SetEnabled(true);
	}
}

//-----------------------------------------------------------------------------
// Purpose:		// !!! foxysen doubleshotgun
//-----------------------------------------------------------------------------
template <>
void CHudItemEffectMeter_Weapon<CTFDoubleShotgun>::ApplySchemeSettings(IScheme *scheme)
{
	CHudItemEffectMeter::ApplySchemeSettings(scheme);

	// Hide the number counter.
	Label *pCounter = dynamic_cast<Label *>(FindChildByName("ItemEffectMeterCount"));
	if (pCounter)
	{
		pCounter->SetVisible(false);
		pCounter->SetEnabled(false);
	}

	// Show the meter.
	ContinuousProgressBar *pMeter = dynamic_cast<ContinuousProgressBar *>(FindChildByName("ItemEffectMeter"));
	if (pMeter)
	{
		pMeter->SetVisible(true);
		pMeter->SetEnabled(true);
	}
}

#ifdef TF2C_BETA
//-----------------------------------------------------------------------------
// Purpose:		// !!! foxysen/hogyn pillstreak
//-----------------------------------------------------------------------------
template <>
void CHudItemEffectMeter_Weapon<CTFPillstreak>::ApplySchemeSettings( IScheme *scheme )
{
	CHudItemEffectMeter::ApplySchemeSettings( scheme );

	// Show the number counter.
	Label *pCounter = dynamic_cast<Label *>(FindChildByName( "ItemEffectMeterCount" ));
	if ( pCounter )
	{
		pCounter->SetVisible( true );
		pCounter->SetEnabled( true );
	}

	// Show the meter.
	ContinuousProgressBar *pMeter = dynamic_cast<ContinuousProgressBar *>(FindChildByName( "ItemEffectMeter" ));
	if ( pMeter )
	{
		pMeter->SetVisible( true );
		pMeter->SetEnabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose:		// !!! sappho leapknife
//-----------------------------------------------------------------------------
template <>
void CHudItemEffectMeter_Weapon<CTFLeapKnife>::ApplySchemeSettings(IScheme* scheme)
{
	CHudItemEffectMeter::ApplySchemeSettings(scheme);

	// Hide the number counter.
	Label* pCounter = dynamic_cast<Label*>(FindChildByName("ItemEffectMeterCount"));
	if (pCounter)
	{
		pCounter->SetVisible(false);
		pCounter->SetEnabled(false);
	}

	// Show the meter.
	ContinuousProgressBar* pMeter = dynamic_cast<ContinuousProgressBar*>(FindChildByName("ItemEffectMeter"));
	if (pMeter)
	{
		pMeter->SetVisible(true);
		pMeter->SetEnabled(true);
	}
}
#endif
//-----------------------------------------------------------------------------
// Purpose:		// !!! azzy brimstone
//-----------------------------------------------------------------------------
template <>
void CHudItemEffectMeter_Weapon<CTFBrimstoneLauncher>::ApplySchemeSettings( IScheme *scheme )
{
	CHudItemEffectMeter::ApplySchemeSettings( scheme );

	// Show the number counter.
	Label *pCounter = dynamic_cast<Label *>(FindChildByName( "ItemEffectMeterCount" ));
	if ( pCounter )
	{
		pCounter->SetVisible( false );
		pCounter->SetEnabled( false );
	}

	// Show the meter.
	ContinuousProgressBar *pMeter = dynamic_cast<ContinuousProgressBar *>(FindChildByName( "ItemEffectMeter" ));
	if ( pMeter )
	{
		pMeter->SetVisible( true );
		pMeter->SetEnabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 	// !!! foxysen beacon
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFBeacon>::ShouldFlash(void)
{
	return GetProgress() >= 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 	// !!! azzy brimstone
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFBrimstoneLauncher>::ShouldFlash(void)
{
	CTFBrimstoneLauncher *pWeapon = GetWeapon();
	bool IsActive = false;
	if ( pWeapon )
		IsActive = pWeapon->IsChargeActive();

	return GetProgress() >= 1.0f || IsActive;
}

//-----------------------------------------------------------------------------
// Purpose: 	// !!! azzy brimstone
//-----------------------------------------------------------------------------
template <>
bool CHudItemEffectMeter_Weapon<CTFAnchor>::ShouldFlash(void)
{
	return GetProgress() >= 1.0f;
}

template <>
void CHudItemEffectMeter_Weapon<CTFAnchor>::ApplySchemeSettings(IScheme* scheme)
{
	CHudItemEffectMeter::ApplySchemeSettings(scheme);

	// Hide the number counter.
	Label* pCounter = dynamic_cast<Label*>(FindChildByName("ItemEffectMeterCount"));
	if (pCounter)
	{
		pCounter->SetVisible(false);
		pCounter->SetEnabled(false);
	}

	// Show the meter.
	ContinuousProgressBar* pMeter = dynamic_cast<ContinuousProgressBar*>(FindChildByName("ItemEffectMeter"));
	if (pMeter)
	{
		pMeter->SetVisible(true);
		pMeter->SetEnabled(true);
	}
}