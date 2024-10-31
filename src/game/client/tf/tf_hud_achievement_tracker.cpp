//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "hud_baseachievement_tracker.h"
#include "c_tf_player.h"
#include "iachievementmgr.h"
#include "achievementmgr.h"
#include "hud_vote.h"
#include "baseachievement.h"
#include "achievements_tf.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar hud_achievement_count_engineer( "hud_achievement_count_engineer", "3", FCVAR_ARCHIVE, "Max number of achievements that can be shown on the HUD when you're an engineer" );

class CHudAchievementTracker : public CHudBaseAchievementTracker
{
	DECLARE_CLASS_SIMPLE( CHudAchievementTracker, CHudBaseAchievementTracker );

public:
	CHudAchievementTracker( const char *pElementName );
	virtual void OnThink();
	virtual void PerformLayout();
	virtual int  GetMaxAchievementsShown();
	virtual bool ShouldShowAchievement( IAchievement *pAchievement );
	virtual bool ShouldDraw();

private:
	int m_iPlayerClass;
	CPanelAnimationVarAliasType( int, m_iNormalY, "NormalY", "5", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iEngineerY, "EngineerY", "170", "proportional_int" );

public:
#ifdef CLIENT_DLL
	static bool IsScoutAchievement(int id)
	{
		if (0)
			return true;

		return false;
	}

	static bool IsSoldierAchievement( int id )
	{
		if ( id == TF2C_ACHIEVEMENT_KILL_WITH_DISTANTRPG)
			return true;

		return false;
	}

	static bool IsPyroAchievement( int id )
	{
		if (id == TF2C_ACHIEVEMENT_EXTINGUISH_BOMBLETS ||
			id == TF2C_ACHIEVEMENT_FIRE_AXE_DEATH_AFTERBURN ||
			id == TF2C_ACHIEVEMENT_HARVESTER_COLLECT_DECAP ||
			id == TF2C_ACHIEVEMENT_HARVESTER_DECAP_SINGLE_LIFE ||
			id == TF2C_ACHIEVEMENT_SSG_SCOUT_HIT_PELLETS ||
			id == TF2C_ACHIEVEMENT_SSG_MEATSHOTS ||
			id == TF2C_ACHIEVEMENT_SSG_POUNCE ||
			id == TF2C_ACHIEVEMENT_PYRO_SHOTGUN_CORESHOT_STREAK ||
			id == TF2C_ACHIEVEMENT_FLARE_CRIT_TEAMWORK)
			return true;

		return false;
	}

	static bool IsDemomanAchievement( int id )
	{
		if ( id == TF2C_ACHIEVEMENT_KILL_BUILDINGS_WITH_MIRV ||
			id == TF2C_ACHIEVEMENT_TAUNT_KILL_MIRV ||
			id == TF2C_ACHIEVEMENT_MINES_UNSEEN_DAMAGE ||
			id == TF2C_ACHIEVEMENT_MINES_STREAK_SINGLE_LIFE ||
			id == TF2C_ACHIEVEMENT_STICKY_PRIMED_KILLS)
			return true;

		return false;
	}

	static bool IsHeavyAchievement(int id)
	{
		if (id == TF2C_ACHIEVEMENT_CHEKHOV_CRIT_KILL ||
			id == TF2C_ACHIEVEMENT_AA_VS_AIRBORNE ||
			id == TF2C_ACHIEVEMENT_AA_GUN_CHALLENGE ||
			id == TF2C_ACHIEVEMENT_HEAVY_SHOTGUN_CRITS ||
			id == TF2C_ACHIEVEMENT_SANDVICH_MINES ||
			id == TF2C_ACHIEVEMENT_MINIGUN_DUEL ||
			id == TF2C_ACHIEVEMENT_FISTS_MELEE_DUEL ||
			id == TF2C_ACHIEVEMENT_AA_GUN_SPLASH)
			return true;

		return false;
	}

	static bool IsEngineerAchievement( int id )
	{
		if ( id == TF2C_ACHIEVEMENT_DEFUSE_MIRV ||
			id == TF2C_ACHIEVEMENT_KILL_WITH_BLINDCOILRICOCHET ||
			id == TF2C_ACHIEVEMENT_TELEPORTER_PROGRESSION ||
			id == TF2C_ACHIEVEMENT_JUMPPAD_PROGRESSION ||
			id == TF2C_ACHIEVEMENT_JUMPPAD_PROGRESSION_ASSIST )
			return true;

		return false;
	}

	static bool IsMedicAchievement(int id)
	{
		if (id == TF2C_ACHIEVEMENT_CHEKHOV_SHOCK ||
			id == TF2C_ACHIEVEMENT_PROTECT_VIP_AS_MEDIC )
			return true;

		return false;
	}

	static bool IsSniperAchievement(int id)
	{
		if (id == TF2C_ACHIEVEMENT_HEADSHOT_KILL_MIDFLIGHT )
			return true;

		return false;
	}

	static bool IsSpyAchievement( int id )
	{
		if (id == TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL ||
			id == TF2C_ACHIEVEMENT_KILL_CIVILIAN_DISGUISEBOOST ||
			id == TF2C_ACHIEVEMENT_TRANQ_KILL ||
			id == TF2C_ACHIEVEMENT_TRANQ_SUPPORT_PROGRESSION ||
			id == TF2C_ACHIEVEMENT_TRANQ_HIT_BLASTJUMP ||
			id == TF2C_ACHIEVEMENT_REVOLVER_KILLS_SINGLE_LIFE ||
			id == TF2C_ACHIEVEMENT_HARVESTER_COUNTER_SPY ||
			id == TF2C_ACHIEVEMENT_TELEPORTER_ENEMY_SPY_KILL ||
			id == TF2C_ACHIEVEMENT_JUMPPAD_ENEMY_SPY_BACKSTAB ||
			id == TF2C_ACHIEVEMENT_4TEAM_BACKSTAB ||
			id == TF2C_ACHIEVEMENT_SPEEDWATCH_SPEED_KILL ||
			id == TF2C_ACHIEVEMENT_INVIS_RECHARGE_CLOAKED ||
			id == TF2C_ACHIEVEMENT_SPEEDWATCH_RECOVERY)
			return true;

		return false;
	}

	// This one was used for Gunboats related achievement at some point
	static bool IsSoldierDemomanAchievement(int id)
	{
		return false;
	}

	static bool IsCivilianAchievement( int id )
	{
		if ( id == TF2C_ACHIEVEMENT_WIN_CIVILIAN_NODEATHS ||
			id == TF2C_ACHIEVEMENT_VIP_PROTECT_UNSEEN ||
			id == TF2C_ACHIEVEMENT_VIP_TEAMMATE_KILLSTREAK_DAMAGE_BOOST )
			return true;

		return false;
	}

	static bool IsHealerAchievement( int id )
	{
		if ( id == TF2C_ACHIEVEMENT_HEAL_CIVILIAN)
			return true;

		return false;
	}

	static bool IsTeamAchievement( int id, int iTeamNumber )
	{
		// TODO: Do some more advanced checks later down the road.
		if ( iTeamNumber != TF_TEAM_RED &&
			( id == TF2C_ACHIEVEMENT_DOMINATE_CIVILIAN ||
			id == TF2C_ACHIEVEMENT_KILL_CIVILIAN_DISGUISEBOOST ||
			id == TF2C_ACHIEVEMENT_VIP_KILL_CRIT_KILLSTREAK) )
			return false;

		if ( iTeamNumber != TF_TEAM_BLUE &&
			( id == TF2C_ACHIEVEMENT_HEAL_CIVILIAN ||
			id == TF2C_ACHIEVEMENT_VIP_PROTECT_UNSEEN ||
			id == TF2C_ACHIEVEMENT_KILLSTREAK_WHILE_VIP_BOOSTED ||
			id == TF2C_ACHIEVEMENT_VIP_TEAMMATE_KILLSTREAK_DAMAGE_BOOST ||
			id == TF2C_ACHIEVEMENT_KILL_VIP_DAMAGERS ||
			id == TF2C_ACHIEVEMENT_PROTECT_VIP_AS_MEDIC) )
			return false;

		return true;
	}

	/*
	static bool IsWizardAchievement(int id, int iTeamNumber)
	{
		if (id == TF2C_ACHIEVEMENT_WIZARD &&
			iTeamNumber == TF_TEAM_YELLOW)
			return false;

		return true;
	}
	*/

	static bool IsGamemodeAchievement( int id )
	{
		CTFGameRules *pTFGameRules = TFGameRules();
		if ( pTFGameRules )
		{
			if ( !pTFGameRules->IsVIPMode() &&
				( id == TF2C_ACHIEVEMENT_WIN_CIVILIAN_NODEATHS ||
				id == TF2C_ACHIEVEMENT_HEAL_CIVILIAN ||
				id == TF2C_ACHIEVEMENT_DOMINATE_CIVILIAN ||
				id == TF2C_ACHIEVEMENT_PLAY_GAME_VIPMAPS_1 ||
				id == TF2C_ACHIEVEMENT_KILL_CIVILIAN_DISGUISEBOOST ||
				id == TF2C_ACHIEVEMENT_VIP_PROTECT_UNSEEN ||
				id == TF2C_ACHIEVEMENT_KILLSTREAK_WHILE_VIP_BOOSTED ||
				id == TF2C_ACHIEVEMENT_VIP_TEAMMATE_KILLSTREAK_DAMAGE_BOOST ||
				id == TF2C_ACHIEVEMENT_VIP_KILL_CRIT_KILLSTREAK ||
				id == TF2C_ACHIEVEMENT_KILL_VIP_DAMAGERS ||
				id == TF2C_ACHIEVEMENT_PROTECT_VIP_AS_MEDIC) )
				return false;

			if ( !pTFGameRules->IsInDominationMode() &&
				( id == TF2C_ACHIEVEMENT_HOLD_ALLPOINTS_DOMINATION ||
				id == TF2C_ACHIEVEMENT_PLAY_GAME_DOMMAPS_1 ) )
				return false;

			if (!pTFGameRules->IsFourTeamGame() &&
				(id == TF2C_ACHIEVEMENT_4TEAM_BACKSTAB))
				return false;
		}

		return true;
	}

	#endif // CLIENT_DLL
};

DECLARE_HUDELEMENT( CHudAchievementTracker );


CHudAchievementTracker::CHudAchievementTracker( const char *pElementName ) : BaseClass( pElementName )
{
	m_iPlayerClass = -1;
}

// layout panel again if player class changes
void CHudAchievementTracker::OnThink()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		C_TFPlayerClass *pClass = pPlayer->GetPlayerClass();
		if ( pClass && m_iPlayerClass != pClass->GetClassIndex() )
		{
			InvalidateLayout();
			m_iPlayerClass = pClass->GetClassIndex();
			m_flNextThink = gpGlobals->curtime - 0.1f;
		}
	}
	
	BaseClass::OnThink();
}

// Show less achievements on the HUD for the engineer
int CHudAchievementTracker::GetMaxAchievementsShown()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsPlayerClass( TF_CLASS_ENGINEER, true ) )
	{
		return hud_achievement_count_engineer.GetInt();
	}

	return BaseClass::GetMaxAchievementsShown();
}

// shift panel down for the engineer
void CHudAchievementTracker::PerformLayout()
{
	BaseClass::PerformLayout();

	int x, y;
	GetPos( x, y );

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsPlayerClass( TF_CLASS_ENGINEER, true ) )
	{		
		SetPos( x, m_iEngineerY );
	}
	else
	{
		SetPos( x, m_iNormalY );
	}
}

bool CHudAchievementTracker::ShouldShowAchievement( IAchievement *pAchievement )
{
	if ( !BaseClass::ShouldShowAchievement( pAchievement ) )
		return false;
	
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	// Filter out class specific achievements
	int id = pAchievement->GetAchievementID();

	if ( IsScoutAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_SCOUT ) )
		return false;
	
	if ( IsSoldierAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) )
		return false;

	if ( IsPyroAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_PYRO ) )
		return false;

	if ( IsDemomanAchievement(id) && !pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) )
		return false;

	if ( IsHeavyAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) )
		return false;

	if ( IsEngineerAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
		return false;

	if ( IsMedicAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) )
		return false;

	if ( IsSniperAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_SNIPER ) )
		return false;

	if ( IsSpyAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_SPY ) )
		return false;

	if (IsSoldierDemomanAchievement(id) && !(pPlayer->IsPlayerClass(TF_CLASS_SOLDIER) || pPlayer->IsPlayerClass(TF_CLASS_DEMOMAN)))
		return false;

	if ( IsCivilianAchievement( id ) && !pPlayer->IsPlayerClass( TF_CLASS_CIVILIAN ) )
		return false;

	// Multiple classes can heal others, so shlonk them in here.
	if ( IsHealerAchievement( id ) &&
		!( pPlayer->IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) ||
		pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) ||
		pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) ) )
		return false;

	// A rare, team-specific achievement has been spotted in the wild.
	if ( !IsTeamAchievement( id, pPlayer->GetTeamNumber() ) )
		return false;

	// Achievements that are limited to specific gamemodes.
	if ( !IsGamemodeAchievement( id ) )
		return false;

	// Wizard!
	//if (!IsWizardAchievement(id, pPlayer->GetTeamNumber()))
	//	return false;

	CBaseAchievement *pBaseAchievement = dynamic_cast< CBaseAchievement * >( pAchievement );
	if ( pBaseAchievement && pBaseAchievement->GetAchievementMgr() )
	{
		if ( pBaseAchievement->GetMapNameFilter() )
		{
			if ( Q_strcmp( pBaseAchievement->GetAchievementMgr()->GetMapName(), pBaseAchievement->GetMapNameFilter() ) != 0 )
				return false;
		}

		// Filter out achievements with multi-map components if the player isn't on any of these maps and/or already has that component.
		if ( pBaseAchievement->HasComponents() && ( pBaseAchievement->GetFlags() & ACH_FILTER_FULL_ROUND_ONLY ) )
		{
			for ( int i = 0; i < pBaseAchievement->GetGoal(); i++ )
			{
				if ( !Q_strcmp( pBaseAchievement->GetAchievementMgr()->GetMapName(), pBaseAchievement->GetComponentNames()[i] ) )
				{
					if ( !( pBaseAchievement->GetComponentBits() & ( ( uint64 ) 1 ) << i ) )
					{
						return true;
					}
				}
			}

			return false;
		}
	}

	return true;
}


bool CHudAchievementTracker::ShouldDraw()
{
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( pPlayer && pPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
		if ( pHudVote && pHudVote->ShouldDraw() )
			return false;
	}

	return BaseClass::ShouldDraw();
}