//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include "engine/IEngineSound.h"
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "view.h"
#include "c_tf_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// these conditions allow provider to see damage done on enemies if applied on teammate
ETFCond g_eDamageProviderHUDConditions[] =
{
	TF_COND_DAMAGE_BOOST,
	TF_COND_CIV_SPEEDBUFF,
	TF_COND_LAST
};

// HUD Effect Type
enum HudEffectType
{
	TF_HUD_EFFECT_DAMAGE,
	TF_HUD_EFFECT_HEALING,
	TF_HUD_EFFECT_BONUS,
	TF_HUD_EFFECT_BLOCKED,
};

// Floating delta text items, float off the top of the head to 
// show damage done.
typedef struct
{
	int m_iAmount;
	float m_flDieTime;
	EHANDLE m_hEntity;
	Vector m_vDamagePos;
	bool bCrit;
	bool bAlwaysDraw; // If false, don't show.
	HudEffectType eEffectType;
} dmg_account_delta_t;

// When to show Crits, Mini-Crits, Etc.
enum EBonusEffectFilter_t
{
	kEffectFilter_AttackerOnly,
	kEffectFilter_AttackerTeam,
	kEffectFilter_VictimOnly,
	kEffectFilter_VictimTeam,
	kEffectFilter_AttackerAndVictimOnly,
	kEffectFilter_BothTeams,
};


class CDamageAccountPanel : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CDamageAccountPanel, EditablePanel );

public:
	CDamageAccountPanel( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual void	LevelInit( void );
	virtual bool	ShouldDraw( void );
	virtual void	Paint( void );

	virtual void	FireGameEvent( IGameEvent *event );
	void			OnDamaged( IGameEvent *event );
	void			OnHealed( IGameEvent *event );
	void			OnBonusPoints( IGameEvent* event );
	void			OnBlocked(IGameEvent* event);
	void			PlayHitSound( int iAmount, bool bKill );

	// Type of Hit
	enum
	{
		TF_HITTYPE_HIT = 0,
		TF_HITTYPE_KILL,
		TF_HITTYPE_COUNT,
	};

	// Hit Sound Preset
	enum
	{
		TF_HITSOUND_DEFAULT = 0,
		TF_HITSOUND_ELECTRO,
		TF_HITSOUND_NOTES,
		TF_HITSOUND_PERCUSSION,
		TF_HITSOUND_RETRO,
		TF_HITSOUND_SPACE,
		TF_HITSOUND_VORTEX,
		TF_HITSOUND_SQUASHER,
		TF_HITSOUND_CLASSIC,

		TF_HITSOUND_COUNT,
	};

	static const char *m_aDingalingSounds[TF_HITTYPE_COUNT][TF_HITSOUND_COUNT + 1];

private:
	CUtlVector<dmg_account_delta_t> m_AccountDeltaItems;
	float m_flLastHitSound;

	CPanelAnimationVarAliasType( float, m_flDeltaItemEndPos, "delta_item_end_y", "50", "float" );

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );

	CPanelAnimationVar( float, m_flDeltaLifetime, "delta_lifetime", "2.0" );

	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFont, "delta_item_font", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFontBig, "delta_item_font_big", "Default" );

};

const char *CDamageAccountPanel::m_aDingalingSounds[TF_HITTYPE_COUNT][TF_HITSOUND_COUNT + 1] =
{
	// Hit
	{
		"Player.HitSoundDefaultDing",
		"Player.HitSoundElectro",
		"Player.HitSoundNotes",
		"Player.HitSoundPercussion",
		"Player.HitSoundRetro",
		"Player.HitSoundSpace",
		"Player.HitSoundBeepo",
		"Player.HitSoundVortex",
		"Player.HitSoundSquasher",
		"Player.HitSoundClassic",
	},

	// Kill
	{
		"Player.KillSoundDefaultDing",
		"Player.KillSoundElectro",
		"Player.KillSoundNotes",
		"Player.KillSoundPercussion",
		"Player.KillSoundRetro",
		"Player.KillSoundSpace",
		"Player.KillSoundBeepo",
		"Player.KillSoundVortex",
		"Player.KillSoundSquasher",
		"Player.KillSoundClassic",
	},
};

DECLARE_HUDELEMENT( CDamageAccountPanel );

ConVar hud_combattext( "hud_combattext", "2", FCVAR_ARCHIVE, "" );
ConVar hud_combattext_batching( "hud_combattext_batching", "1", FCVAR_ARCHIVE, "If set to 1, numbers that are too close together are merged." );
ConVar hud_combattext_batching_window( "hud_combattext_batching_window", "0.2", FCVAR_ARCHIVE, "Maximum delay between damage events in order to batch numbers." );
ConVar hud_combattext_doesnt_block_overhead_text( "hud_combattext_doesnt_block_overhead_text", "1", FCVAR_ARCHIVE, "If set to 1, allow text like \"CRIT\" to still show over a victim's head." );
ConVar hud_combattext_overhead( "hud_combattext_overhead", "1", FCVAR_ARCHIVE, "If set to 1, display the text over your victim's head." );
ConVar hud_combattext_occlusionignore( "hud_combattext_occlusionignore", "0", FCVAR_ARCHIVE, "If set to 1, display the text regardless of victim occlusion" );

ConVar hud_combattext_show_damage_blocked("hud_combattext_show_damage_blocked", "1", FCVAR_ARCHIVE, "If set to 1, display amount of damage blocked on teammates.");
ConVar hud_combattext_show_when_dead("hud_combattext_show_when_dead", "1", FCVAR_ARCHIVE, "If set to 1, display combat text even when dead.");

ConVar hud_combattext_red( "hud_combattext_red", "255", FCVAR_ARCHIVE );
ConVar hud_combattext_green( "hud_combattext_green", "0", FCVAR_ARCHIVE );
ConVar hud_combattext_blue( "hud_combattext_blue", "0", FCVAR_ARCHIVE );

ConVar tf_dingalingaling( "tf_dingalingaling", "2", FCVAR_ARCHIVE, "If set to 2, play a sound everytime you injure an enemy. 1 plays a sound on critical hits. The sound can be customized by replacing the 'tf/sound/ui/hitsound.wav' file." );
ConVar tf_dingalingaling_effect( "tf_dingalingaling_effect", "0", FCVAR_ARCHIVE, "Configure which preselected dingaling sound you wish to use." );

ConVar tf_dingaling_volume( "tf_dingaling_volume", "0.75", FCVAR_ARCHIVE, "Desired volume of the hit sound.", true, 0.0, true, 1.0 );
ConVar tf_dingaling_pitchmindmg( "tf_dingaling_pitchmindmg", "100", FCVAR_ARCHIVE, "Desired pitch of the hit sound when a minimal damage hit (<= 10 health) is done.", true, 1, true, 255 );
ConVar tf_dingaling_pitchmaxdmg( "tf_dingaling_pitchmaxdmg", "100", FCVAR_ARCHIVE, "Desired pitch of the hit sound when a maximum damage hit (>= 150 health) is done.", true, 1, true, 255 );
ConVar tf_dingalingaling_repeat_delay( "tf_dingalingaling_repeat_delay", "0", FCVAR_ARCHIVE, "Desired repeat delay of the hit sound. Set to 0 to play a sound for every instance of damage dealt." );

ConVar tf_dingalingaling_lasthit( "tf_dingalingaling_lasthit", "1", FCVAR_ARCHIVE, "If set to 1, play a sound whenever one of your attacks kills an enemy. The sound can be customized by replacing the 'tf/sound/ui/killsound.wav' file." );
ConVar tf_dingalingaling_last_effect( "tf_dingalingaling_last_effect", "0", FCVAR_ARCHIVE, "Configure which preselected dingaling sound you wish to use." );

ConVar tf_dingaling_lasthit_volume( "tf_dingaling_lasthit_volume", "0.75", FCVAR_ARCHIVE, "Desired volume of the last hit sound.", true, 0.0, true, 1.0 );
ConVar tf_dingaling_lasthit_pitchmindmg( "tf_dingaling_lasthit_pitchmindmg", "100", FCVAR_ARCHIVE, "Desired pitch of the last hit sound when a minimal damage hit (<= 10 health) is done.", true, 1, true, 255 );
ConVar tf_dingaling_lasthit_pitchmaxdmg( "tf_dingaling_lasthit_pitchmaxdmg", "100", FCVAR_ARCHIVE, "Desired pitch of the last hit sound when a maximum damage hit (>= 150 health) is done.", true, 1, true, 255 );

// #define USE_RES_FOR_DELTA_ITEM_END_YPOS


bool ShouldPlayEffect( EBonusEffectFilter_t filter, C_TFPlayer *pAttacker, C_TFPlayer *pVictim )
{
	Assert( pAttacker );
	Assert( pVictim );
	if ( !pAttacker || !pVictim )
		return false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Check if the right player relationship
	switch ( filter )
	{
		case kEffectFilter_AttackerOnly:
			return ( pAttacker == pPlayer );
		case kEffectFilter_AttackerTeam:
			return ( pAttacker->GetTeamNumber() == pPlayer->GetTeamNumber() );
		case kEffectFilter_VictimOnly:
			return ( pVictim == pPlayer );
		case kEffectFilter_VictimTeam:
			return ( pVictim->GetTeamNumber() == pPlayer->GetTeamNumber() );
		case kEffectFilter_AttackerAndVictimOnly:
			return ( pAttacker == pPlayer || pVictim == pPlayer );
		case kEffectFilter_BothTeams:
			return ( pAttacker->GetTeamNumber() == pPlayer->GetTeamNumber() || pVictim->GetTeamNumber() == pPlayer->GetTeamNumber() );
		default:
			Assert( "EBonusEffectFilter_t type not handled!" );
			return false;
	};
}


CDamageAccountPanel::CDamageAccountPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "CDamageAccountPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_flLastHitSound = 0.0f;

	ListenForGameEvent( "player_hurt" );
	ListenForGameEvent( "player_healed" );
	ListenForGameEvent( "npc_hurt" );
	ListenForGameEvent( "building_healed" );
	ListenForGameEvent( "player_bonuspoints" );
	ListenForGameEvent( "damage_blocked" );
}


void CDamageAccountPanel::FireGameEvent( IGameEvent *event )
{
	const char *pszEventName = event->GetName();
	if ( V_strcmp( pszEventName, "player_hurt" ) == 0 || V_strcmp( pszEventName, "npc_hurt" ) == 0 )
	{
		OnDamaged( event );
	}
	else if ( V_strcmp( pszEventName, "player_healed" ) == 0 || V_strcmp( pszEventName, "building_healed" ) == 0 )
	{
		OnHealed( event );
	}
	else if ( V_strcmp( pszEventName, "player_bonuspoints" ) == 0 )
	{
		OnBonusPoints( event );
	}
	else if (V_strcmp(pszEventName, "damage_blocked") == 0)
	{
		OnBlocked(event);
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}


void CDamageAccountPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudDamageAccount.res" );
}

//-----------------------------------------------------------------------------
// Purpose: called whenever a new level's starting
//-----------------------------------------------------------------------------
void CDamageAccountPanel::LevelInit( void )
{
	m_AccountDeltaItems.Purge();
	m_flLastHitSound = 0.0f;

	CHudElement::LevelInit();
}


bool CDamageAccountPanel::ShouldDraw( void )
{
	if ( m_AccountDeltaItems.Count() == 0 )
		return false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer || (!hud_combattext_show_when_dead.GetBool() && !pPlayer->IsAlive()))
	{
		m_AccountDeltaItems.RemoveAll();
		return false;
	}

	return CHudElement::ShouldDraw();
}


void CDamageAccountPanel::OnDamaged( IGameEvent *event )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;

	if (!hud_combattext_show_when_dead.GetBool() && !pPlayer->IsAlive())
		return;

	bool bIsPlayer = V_strcmp( event->GetName(), "npc_hurt" ) != 0;
	int iAttacker = 0, iVictim = 0;

	if ( bIsPlayer )
	{
		iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
	}
	else
	{
		iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker_player" ) );
		iVictim = event->GetInt( "entindex" );
	}

	// No pointers to these players, woops.
	C_TFPlayer *pAttacker = ToTFPlayer( UTIL_PlayerByIndex( iAttacker ) );
	C_BaseEntity *pVictim = ClientEntityList().GetEnt( iVictim );		
	if ( !pAttacker || !pVictim )
		return;

	// No self-damage notifications.
	if ( bIsPlayer && pAttacker == pVictim )
		return;

	// Don't show anything if no damage was done.
	int iDmgAmount = event->GetInt( "damageamount" );
	if ( iDmgAmount == 0 )
		return;

	int iHealth = event->GetInt( "health" );

	if ( bIsPlayer )
	{
		bool bShowDisguisedCrit = event->GetBool( "showdisguisedcrit", 0 );

		// Don't show damage notifications for spies disguised as our team (unless it's a kill).
		C_TFPlayer *pTFVictim = ToTFPlayer( pVictim );
		if ( pTFVictim && iHealth > 0 && ( !bShowDisguisedCrit && ( pTFVictim->IsDisguisedEnemy( true ) || pTFVictim->m_Shared.IsStealthed() ) ) )
			return;
	}

	// Show damage to the attacker and his healer.
	C_BaseEntity *pHealTarget = pPlayer->MedicGetHealTarget();
	bool bAttackerIsLocalPlayer = iAttacker == pPlayer->entindex() || ( pHealTarget && pHealTarget->entindex() == iAttacker );
	if (!bAttackerIsLocalPlayer && !pAttacker->IsEnemyPlayer())
	{
		for (int i = 0; g_eDamageProviderHUDConditions[i] != TF_COND_LAST; i++)
		{
			if (pAttacker->m_Shared.InCond(g_eDamageProviderHUDConditions[i]) &&
				pPlayer == pAttacker->m_Shared.GetConditionProvider(g_eDamageProviderHUDConditions[i]))
			{
				bAttackerIsLocalPlayer = true;
				break;
			}
		}
	}
	if ( bAttackerIsLocalPlayer )
	{
		// Leftover from old code?
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "DamagedPlayer" );
	}

	// Stop here if we chose not to show ANYTHING.
	if ( !hud_combattext.GetBool() && !tf_dingalingaling.GetBool() && !tf_dingalingaling_lasthit.GetBool() )
		return;

	bool bBlockedDamage = event->GetBool( "damagewasblocked", false );

	// By default we get kBonusEffect_None. We want to use whatever value we get here if it's not kBonusEffect_None.
	// If it's not, then check for crit or minicrit.
	EAttackBonusEffects_t eBonusEffect = (EAttackBonusEffects_t)event->GetInt( "bonuseffect", (int)kBonusEffect_None );
	if ( eBonusEffect == kBonusEffect_None )
	{
		eBonusEffect = event->GetBool( "minicrit", false ) ? kBonusEffect_MiniCrit : eBonusEffect;
		eBonusEffect = event->GetBool( "crit", false ) ? kBonusEffect_Crit : eBonusEffect;
	}

	EBonusEffectFilter_t eParticleFilter = kEffectFilter_AttackerOnly;
	EBonusEffectFilter_t eSoundFilter = kEffectFilter_AttackerOnly;
	if ( event->GetBool( "allseecrit", false ) )
	{
		eParticleFilter = kEffectFilter_AttackerTeam;
		eSoundFilter = kEffectFilter_AttackerTeam;
	}

	// Play crit hit effects.
	if ( bIsPlayer && ( eBonusEffect == kBonusEffect_Crit || eBonusEffect == kBonusEffect_MiniCrit ) )
	{
		C_TFPlayer *pPlayerVictim = ToTFPlayer( pVictim );
		if ( pPlayerVictim )
		{
			if ( ( tf_dingalingaling.GetBool() || ( tf_dingalingaling_lasthit.GetBool() && iHealth <= 0 ) ) && ShouldPlayEffect( eSoundFilter, pAttacker, pPlayerVictim ) )
			{
				// Sound effects
				EmitSound_t params;
				params.m_flSoundTime = 0;
				params.m_pflSoundDuration = 0;
				params.m_pSoundName = ( eBonusEffect == kBonusEffect_Crit ? "TFPlayer.CritHit" : "TFPlayer.CritHitMini" );
				if( bBlockedDamage )
					params.m_pSoundName = VarArgs( "%sBlocked", params.m_pSoundName );

				CPASFilter filter( pVictim->GetAbsOrigin() );
				if ( pAttacker == pPlayer )
				{
					// Don't let the attacker hear this version if its to be played in their ears.
					filter.RemoveRecipient( pAttacker );

					// Play a sound in the ears of the attacker.
					CSingleUserRecipientFilter attackerFilter( pAttacker );
					C_BaseEntity::EmitSound( attackerFilter, pAttacker->entindex(), params );
				}

				// Play it globally.
				C_BaseEntity::EmitSound( filter, pVictim->entindex(), params );
			}

			// Display overhead text if applicable.
			if ( hud_combattext.GetBool() && hud_combattext_doesnt_block_overhead_text.GetBool() && ShouldPlayEffect( eParticleFilter, pAttacker, pPlayerVictim ) )
			{
				DispatchParticleEffect( eBonusEffect == kBonusEffect_Crit ? "crit_text" : "minicrit_text", pPlayerVictim->WorldSpaceCenter() + Vector( 0, 0, 32 ), vec3_angle );
			}
		}
	}
	else if( bBlockedDamage )
	{
		// Sound effects
		EmitSound_t params;
		params.m_flSoundTime = 0;
		params.m_pflSoundDuration = 0;
		params.m_pSoundName = "TFPlayer.BlockedDamage";

		CSingleUserRecipientFilter attackerFilter( pAttacker );
		C_BaseEntity::EmitSound( attackerFilter, pAttacker->entindex(), params );
	}

	if ( !bAttackerIsLocalPlayer )
		return;

	// Play hit sound, if applicable.
	if ( tf_dingalingaling_lasthit.GetInt() > 1 && iHealth <= 0 )
	{
		// This guy is dead, play kill sound.
		PlayHitSound( iDmgAmount, true );
	}
	else if ( tf_dingalingaling.GetInt() > 1 )
	{
		PlayHitSound( iDmgAmount, false );
	}

	// Stop here if we chose not to show hit numbers.
	if ( hud_combattext.GetInt() < 2 )
		return;

	// Don't show the numbers if we can't see the victim.
	if ( pVictim->IsDormant() )
		return;

	Vector vecTextPos;
	if ( hud_combattext_overhead.GetBool() )
	{
		if ( pVictim->IsBaseObject() )
		{
			vecTextPos = pVictim->GetAbsOrigin() + Vector( 0, 0, pVictim->WorldAlignMaxs().z );
		}
		else
		{
			vecTextPos = pVictim->EyePosition();
		}
	}
	else
	{
		vecTextPos = Vector( event->GetFloat( "x" ), event->GetFloat( "y" ), event->GetFloat( "z" ) );
	}

	bool bBatch = false;
	dmg_account_delta_t *pDelta = NULL;

	if ( hud_combattext_batching.GetBool() )
	{
		// Cycle through deltas and search for the one that belongs to this player.
		for ( int i = 0; i < m_AccountDeltaItems.Count(); i++ )
		{
			if (m_AccountDeltaItems[i].eEffectType != TF_HUD_EFFECT_DAMAGE)
				continue;
			if ( m_AccountDeltaItems[i].m_hEntity.Get() == pVictim )
			{
				// See if its lifetime is inside the batching window.
				if ( gpGlobals->curtime - (m_AccountDeltaItems[i].m_flDieTime - m_flDeltaLifetime) <= hud_combattext_batching_window.GetFloat() )
				{
					pDelta = &m_AccountDeltaItems[i];
					bBatch = true;
					break;
				}
			}
		}
	}

	if ( !pDelta )
	{
		pDelta = &m_AccountDeltaItems[m_AccountDeltaItems.AddToTail()];
	}

	bool bHide = true;
	trace_t tr;
	UTIL_TraceLine( MainViewOrigin(), pVictim->WorldSpaceCenter(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr );
	bHide = (tr.fraction != 1.0f && !hud_combattext_occlusionignore.GetBool());

	pDelta->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
	pDelta->m_iAmount = bBatch ? pDelta->m_iAmount + iDmgAmount : iDmgAmount;
	pDelta->m_hEntity = pVictim;
	pDelta->m_vDamagePos = vecTextPos + Vector( 0, 0, 18 );
	pDelta->bCrit = eBonusEffect == kBonusEffect_Crit || eBonusEffect == kBonusEffect_MiniCrit;
	pDelta->bAlwaysDraw = !bHide;
	pDelta->eEffectType = TF_HUD_EFFECT_DAMAGE;

	// Custom HUDs need this.
	SetDialogVariable( "metal", -iDmgAmount );
}


void CDamageAccountPanel::OnHealed( IGameEvent *event )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;

	if (!hud_combattext_show_when_dead.GetBool() && !pPlayer->IsAlive())
		return;

	if ( !hud_combattext.GetBool() )
		return;
	
	bool bIsPlayer = V_strcmp( event->GetName(), "building_healed" ) != 0;
	int iPatient = 0;
	if ( bIsPlayer )
	{
		iPatient = event->GetInt( "patient" );
	}
	else
	{
		iPatient = event->GetInt( "building" );
	}

	int iHealer = event->GetInt( "healer" );
	int iAmount = event->GetInt( "amount" );

	// Did we heal this guy?
	if ( pPlayer->GetUserID() != iHealer )
		return;

	// Just in case.
	if ( iAmount == 0 )
		return;

	C_BaseEntity *pPatient;
	if ( bIsPlayer )
	{
		// Not sure what the actual difference here is, it's done with GetBaseEntity for damages.
		pPatient = UTIL_PlayerByUserId( iPatient );
	}
	else
	{
		pPatient = ClientEntityList().GetBaseEntity( iPatient );
	}

	if ( !pPatient )
		return;

	// Don't show the healing numbers for invisible enemy players
	// Intended to stop engineer from seeing healing numbers over cloaked enemy spies using their dispenser
	// Note: You may see healing number pop up if undisguised enemy spy uncloaks near dispenser, but he won't be invisible at that point
	if ( bIsPlayer )
	{
		C_TFPlayer* pPatientPlayer = static_cast<C_TFPlayer*>(pPatient);
		if (pPatientPlayer->IsEnemy(pPlayer) && pPatientPlayer->m_Shared.GetPercentInvisible() > 0.0f)
			return;
	}

	// Don't show the numbers if we can't see the patient.
	// ^ But why? Should we not know our ally's position?
	trace_t tr;
	UTIL_TraceLine( MainViewOrigin(), pPatient->WorldSpaceCenter(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0f && !hud_combattext_occlusionignore.GetBool() )
		return;

	Vector vecTextPos = pPatient->EyePosition();

	dmg_account_delta_t *pDelta = &m_AccountDeltaItems[m_AccountDeltaItems.AddToTail()];
	pDelta->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
	pDelta->m_iAmount = iAmount;
	pDelta->m_hEntity = pPatient;
	pDelta->m_vDamagePos = vecTextPos + Vector( 0, 0, 18 );
	pDelta->bCrit = false;
	pDelta->eEffectType = TF_HUD_EFFECT_HEALING;

	// Custom HUDs need this.
	SetDialogVariable( "metal", iAmount );
}


void CDamageAccountPanel::OnBonusPoints( IGameEvent* event )
{
	C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;

	if (!hud_combattext_show_when_dead.GetBool() && !pPlayer->IsAlive())
		return;

	if ( !hud_combattext.GetBool() )
		return;

	const int iBonusPlayer = event->GetInt( "player_entindex" );
	CBasePlayer* pBonusPlayer = UTIL_PlayerByIndex( iBonusPlayer );

	const int iSource = event->GetInt( "source_entindex" );
	C_BaseEntity* pSource = ClientEntityList().GetBaseEntity( iSource );

	int iAmount = event->GetInt( "points" );

	// Did we heal this guy?
	if ( pPlayer && pPlayer != pBonusPlayer )
		return;

	if ( !pSource )
		return;

	// Don't show the numbers if we can't see the patient.										// No reason not to! You're not gonna track enemies by bloody PURPLE TEXT!
	//trace_t tr;
	//UTIL_TraceLine( MainViewOrigin(), pSource->WorldSpaceCenter(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr );
	//if ( tr.fraction != 1.0f )
	//	return;

	Vector vecTextPos = pSource->GetAbsOrigin();
	if ( pSource->IsPlayer() )
	{
		vecTextPos.z += VEC_HULL_MAX_SCALED( pSource->GetBaseAnimating() ).z + 18;
	}

	dmg_account_delta_t* pDelta = &m_AccountDeltaItems[m_AccountDeltaItems.AddToTail()];
	pDelta->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
	pDelta->m_iAmount = iAmount;
	pDelta->m_hEntity = pSource;
	pDelta->m_vDamagePos = vecTextPos;
	pDelta->bCrit = false;
	pDelta->eEffectType = TF_HUD_EFFECT_BONUS;
}

void CDamageAccountPanel::OnBlocked(IGameEvent* event)
{
	C_TFPlayer* pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;

	if (!hud_combattext_show_when_dead.GetBool() && !pPlayer->IsAlive())
		return;

	if (!hud_combattext_show_damage_blocked.GetBool())
		return;

	int iProvider = 0, iVictim = 0;

	iProvider = engine->GetPlayerForUserID(event->GetInt("provider"));
	iVictim = engine->GetPlayerForUserID(event->GetInt("victim"));

	// No pointers to these players, woops.
	C_TFPlayer* pProvider = ToTFPlayer(UTIL_PlayerByIndex(iProvider));
	C_TFPlayer* pVictim = ToTFPlayer(UTIL_PlayerByIndex(iVictim));
	if (!pProvider || !pVictim)
		return;

	// Show nothing on ourselves
	if (pPlayer == pVictim)
		return;

	// Don't show anything if no damage was blocked.
	int iAmount = event->GetInt("amount");
	if (iAmount == 0)
		return;

	// Don't show if we are't the provider
	if (pPlayer != pProvider)
		return;

	// Show only if victim is our teammate
	if (pVictim->IsEnemyPlayer())
		return;

	// Don't show the numbers if we can't see the victim.
	if (pVictim->IsDormant())
		return;

	Vector vecTextPos;
	if (pVictim->IsBaseObject()) // won't happened
	{
		vecTextPos = pVictim->GetAbsOrigin() + Vector(0, 0, pVictim->WorldAlignMaxs().z);
	}
	else
	{
		vecTextPos = pVictim->EyePosition();
	}

	bool bBatch = false;
	dmg_account_delta_t* pDelta = NULL;

	if (hud_combattext_batching.GetBool())
	{
		// Cycle through deltas and search for the one that belongs to this player.
		for (int i = 0; i < m_AccountDeltaItems.Count(); i++)
		{
			if (m_AccountDeltaItems[i].eEffectType != TF_HUD_EFFECT_BLOCKED)
				continue;
			if (m_AccountDeltaItems[i].m_hEntity.Get() == pVictim)
			{
				// See if its lifetime is inside the batching window.
				if (gpGlobals->curtime - (m_AccountDeltaItems[i].m_flDieTime - m_flDeltaLifetime) <= hud_combattext_batching_window.GetFloat())
				{
					pDelta = &m_AccountDeltaItems[i];
					bBatch = true;
					break;
				}
			}
		}
	}

	if (!pDelta)
	{
		pDelta = &m_AccountDeltaItems[m_AccountDeltaItems.AddToTail()];
	}

	bool bHide = true;
	trace_t tr;
	UTIL_TraceLine(MainViewOrigin(), pVictim->WorldSpaceCenter(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr);
	bHide = (tr.fraction != 1.0f && !hud_combattext_occlusionignore.GetBool());

	pDelta->m_flDieTime = gpGlobals->curtime + m_flDeltaLifetime;
	pDelta->m_iAmount = bBatch ? pDelta->m_iAmount + iAmount : iAmount;
	pDelta->m_hEntity = pVictim;
	pDelta->m_vDamagePos = vecTextPos + Vector(0, 0, 18);
	pDelta->bCrit = false;
	pDelta->bAlwaysDraw = !bHide;
	pDelta->eEffectType = TF_HUD_EFFECT_BLOCKED;
}

void CDamageAccountPanel::PlayHitSound( int iAmount, bool bKill )
{
	if ( !bKill )
	{
		float flRepeatDelay = tf_dingalingaling_repeat_delay.GetFloat();
		if ( flRepeatDelay > 0 && gpGlobals->curtime - m_flLastHitSound <= flRepeatDelay )
			return;
	}

	EmitSound_t params;
	params.m_nChannel = CHAN_STATIC;

	float flPitchMin, flPitchMax;
	if ( bKill )
	{
		params.m_pSoundName = m_aDingalingSounds[TF_HITTYPE_KILL][Min<int>( Max<int>( tf_dingalingaling_last_effect.GetInt(), 0 ), TF_HITSOUND_COUNT )];

		params.m_flVolume = tf_dingaling_lasthit_volume.GetFloat();

		flPitchMin = tf_dingaling_lasthit_pitchmindmg.GetFloat();
		flPitchMax = tf_dingaling_lasthit_pitchmaxdmg.GetFloat();
	}
	else
	{
		params.m_pSoundName = m_aDingalingSounds[TF_HITTYPE_HIT][Min<int>( Max<int>( tf_dingalingaling_effect.GetInt(), 0 ), TF_HITSOUND_COUNT )];

		params.m_flVolume = tf_dingaling_volume.GetFloat();

		flPitchMin = tf_dingaling_pitchmindmg.GetFloat();
		flPitchMax = tf_dingaling_pitchmaxdmg.GetFloat();
	}

	params.m_SoundLevel = SNDLVL_NORM;

	params.m_nPitch = RemapValClamped( (float)iAmount, 10, 150, flPitchMin, flPitchMax );
	params.m_nFlags = ( SND_CHANGE_VOL | SND_CHANGE_PITCH | SND_STOP );

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, params ); // People like hearing.

	params.m_nFlags &= ~SND_STOP;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, params ); // Ding!

	if ( !bKill )
	{
		m_flLastHitSound = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Paint the deltas
//-----------------------------------------------------------------------------
void CDamageAccountPanel::Paint( void )
{
	BaseClass::Paint();

	for ( int i = m_AccountDeltaItems.Count() - 1; i >= 0; i-- )
	{
		// update all the valid delta items
		if ( m_AccountDeltaItems[i].m_flDieTime > gpGlobals->curtime )
		{
			// position and alpha are determined from the lifetime
			// color is determined by the delta - green for positive, red for negative

			Color c = Color();
			switch (m_AccountDeltaItems[i].eEffectType)
			{
			case TF_HUD_EFFECT_DAMAGE:
				c = Color(hud_combattext_red.GetInt(), hud_combattext_green.GetInt(), hud_combattext_blue.GetInt(), 255); // Previously: m_DeltaNegativeColor
				break;
			case TF_HUD_EFFECT_HEALING:
				c = m_DeltaPositiveColor;
				break;
			case TF_HUD_EFFECT_BONUS:
				c = Color(255, 0, 255, 255);
				break;
			case TF_HUD_EFFECT_BLOCKED:
				c = Color(100, 100, 255, 255);
				break;
			}

			float flLifetimePercent = ( m_AccountDeltaItems[i].m_flDieTime - gpGlobals->curtime ) / m_flDeltaLifetime;

			// fade out after half our lifetime
			if ( flLifetimePercent < 0.5f )
			{
				c[3] = (int)( 255.0f * ( flLifetimePercent / 0.5f ) );
			}

			int x, y;
			bool bOnscreen = GetVectorInScreenSpace(m_AccountDeltaItems[i].m_vDamagePos, x, y);

			if ( !bOnscreen || !m_AccountDeltaItems[i].bAlwaysDraw)
				continue;

#ifdef USE_RES_FOR_DELTA_ITEM_END_YPOS
			float flHeight = m_flDeltaItemEndPos;
#else
			float flHeight = 50.0f;
#endif
			y -= (int)( ( 1.0f - flLifetimePercent ) * flHeight );

			// Use BIGGER font for crits.
			HFont hFont = m_AccountDeltaItems[i].bCrit ? m_hDeltaItemFontBig : m_hDeltaItemFont;

			wchar_t wBuf[16];
			switch (m_AccountDeltaItems[i].eEffectType)
			{
			case TF_HUD_EFFECT_DAMAGE:
				V_swprintf_safe(wBuf, L"-%d", m_AccountDeltaItems[i].m_iAmount);
				break;
			case TF_HUD_EFFECT_HEALING:
			case TF_HUD_EFFECT_BONUS:
				V_swprintf_safe(wBuf, L"+%d", m_AccountDeltaItems[i].m_iAmount);
				break;
			case TF_HUD_EFFECT_BLOCKED:
				V_swprintf_safe(wBuf, L"%d", m_AccountDeltaItems[i].m_iAmount);
				break;
			}

			// Offset x pos so the text is centered.
			x -= UTIL_ComputeStringWidth( hFont, wBuf ) / 2;

			vgui::surface()->DrawSetTextFont( hFont );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawSetTextPos( x, y );

			vgui::surface()->DrawPrintText( wBuf, wcslen( wBuf ), FONT_DRAW_NONADDITIVE );
		}
		else
		{
			// Remove if it's past the batching interval.
			float flCreateTime = m_AccountDeltaItems[i].m_flDieTime - m_flDeltaLifetime;
			if ( gpGlobals->curtime - flCreateTime > hud_combattext_batching_window.GetFloat() )
			{
				m_AccountDeltaItems.Remove( i );
			}
		}
	}
}
