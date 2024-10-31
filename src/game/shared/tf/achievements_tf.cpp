//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================


#include "cbase.h"

#ifdef CLIENT_DLL

#include "achievements_tf.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "tf_hud_statpanel.h"
#include "c_tf_team.h"
#include "c_tf_player.h"
#include "tf_weapon_pipebomblauncher.h"

CAchievementMgr g_AchievementMgrTF;	// global achievement mgr for TF

bool CheckWinNoEnemyCaps( IGameEvent *event, int iRole );

// Grace period that we allow a player to start after level init and still consider them to be participating for the full round.
// This is fairly generous because it can in some cases take a client several minutes to connect with respect to when the server considers the game underway
#define TF_FULL_ROUND_GRACE_PERIOD	( 4 * 60.0f )

bool IsLocalTFPlayerClass( int iClass );

C_TFPlayer* GetVIPPlayerOnTeam(C_TFTeam* pTeam);
bool IsAchievementProtectVIPAsMedicIsCombatClass(int iClass);
float HammerUnitsToMeters(float flHU) { return flHU / 52.49344; };

// Helper class for achievements that check that the player was playing on a game team for the full round.
class CTFAchievementFullRound : public CBaseAchievement
{
	DECLARE_CLASS( CTFAchievementFullRound, CBaseAchievement );

public:
	void Init()
	{
		m_iFlags |= ACH_FILTER_FULL_ROUND_ONLY;		
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !Q_strcmp( event->GetName(), "teamplay_round_win" ) )
		{
			C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pLocalPlayer )
			{
				// is the player currently on a game team?
				int iTeam = pLocalPlayer->GetTeamNumber();
				if ( iTeam >= FIRST_GAME_TEAM ) 
				{
					float flRoundTime = event->GetFloat( "round_time", 0 );
					if ( flRoundTime > 0 )
					{
						Event_OnRoundComplete( flRoundTime, event );
					}
				}
			}
		}
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event ) = 0;
};

// WIP
// Don't forget to call SetCounterGoal in init
// use IncrementCounter() to counter when condition is met
// ListenForEvents must include CSingleLifeCounterAchievement::ListenForEvents() at the start
// FireGameEvent_Internal must include CSingleLifeCounterAchievement::FireGameEvent_Internal(event) at the start
// For some reason BaseClass:: just doesn't work
class CSingleLifeCounterAchievement : public CBaseAchievement
{
	DECLARE_CLASS(CSingleLifeCounterAchievement, CBaseAchievement);
public:
	CSingleLifeCounterAchievement() : CBaseAchievement()
	{
		m_iSingleLifeCounterGoal = 1;
	}

	virtual void ListenForEvents() OVERRIDE
	{
		ListenForGameEvent("teamplay_round_active");
		ListenForGameEvent("player_spawn");
		ListenForGameEvent("player_death");
		ResetCounter();	// Stuffed it here so it gets reset when achievement progress is reset.
	}

	virtual void FireGameEvent_Internal(IGameEvent* event) OVERRIDE
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (!Q_strcmp(event->GetName(), "teamplay_round_active") ||
			(!Q_strcmp(event->GetName(), "player_spawn") && pLocalPlayer->GetUserID() == event->GetInt("userid")) ||
			(!Q_strcmp(event->GetName(), "player_death") && pLocalPlayer->GetUserID() == event->GetInt("userid")))
		{
			ResetCounter();
		}
	}

	void IncrementSingeLifeCounter(int iCount = 1)
	{
		m_iSingleLifeCounter += iCount;
		if (m_iSingleLifeCounter >= m_iSingleLifeCounterGoal)
		{
			AwardAchievement();
		}
	}

	void SetCounterGoal(int iGoal) { m_iSingleLifeCounterGoal = iGoal; }
	int GetCounterGoal() { return m_iSingleLifeCounterGoal; }
protected:
	virtual void ResetCounter()
	{
		m_iSingleLifeCounter = 0;
	}

	int	m_iSingleLifeCounter;
	int m_iSingleLifeCounterGoal;
};

// Team Fortress 2 Classic Achievements
/*class CAchievementTF2CBetaParticipant : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetHideUntilAchieved( true );
		SetType( ACHIEVEMENT_TYPE_MAJOR );
	}
	
#if STAGING_ONLY
	void ListenForEvents()
	{
		ListenForGameEvent( "player_spawn" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pLocalPlayer )
			return;

		if ( !Q_strcmp( event->GetName(), "player_spawn" ) )
		{
			if ( pLocalPlayer->GetUserID() == event->GetInt( "userid" ) )
			{
				IncrementCount();
			}
		}
	}
#endif
};
DECLARE_ACHIEVEMENT( CAchievementTF2CBetaParticipant, TF2C_ACHIEVEMENT_BETA_PARTICIPANT, "TF2C_BETA_PARTICIPANT", 0 );*/

//----------------------------------------------------------------------------------------------------------------
// Team Fortress 2 Classic Pack
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CDistantRPGKill : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetType( ACHIEVEMENT_TYPE_MISTAKE );
	}

	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT( CAchievementTF2CDistantRPGKill, TF2C_ACHIEVEMENT_KILL_WITH_DISTANTRPG, "TF2C_KILL_WITH_DISTANTRPG", 25 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CKillBuildingsWithMIRV : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( ACHIEVEMENT_TF2C_NEST_DESTROYER_DESTRUCTION_REQUIREMENT );
		SetType( ACHIEVEMENT_TYPE_PLAYER );
	}

	void ListenForEvents()
	{
		ListenForGameEvent( "object_destroyed" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if ( !pLocalPlayer )
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if ( !Q_strcmp( event->GetName(), "object_destroyed" ) )
		{
			if ( pLocalPlayer->GetUserID() == event->GetInt( "attacker" ) )
			{
				if ( event->GetInt( "weaponid" ) == TF_WEAPON_GRENADE_MIRV ||
					event->GetInt( "weaponid" ) == TF_WEAPON_GRENADE_MIRV_DEMOMAN ||
					event->GetInt( "weaponid" ) == TF_WEAPON_GRENADE_MIRVBOMB )
				{
					IncrementCount();
					m_pAchievementMgr->SetDirty( true );
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF2CKillBuildingsWithMIRV, TF2C_ACHIEVEMENT_KILL_BUILDINGS_WITH_MIRV, "TF2C_KILL_BUILDINGS_WITH_MIRV", 15 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CTranqMeleeKill : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( ACHIEVEMENT_TF2C_TRANQ_MELEE_KILL_REQUIREMENT );
		SetType( ACHIEVEMENT_TYPE_PLAYER );
	}

	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT( CAchievementTF2CTranqMeleeKill, TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL, "TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL", 10 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CDefuseMIRV : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetType( ACHIEVEMENT_TYPE_TRIVIAL );
	}

	void ListenForEvents()
	{
		ListenForGameEvent( "mirv_defused" );
	}
	
	void FireGameEvent_Internal( IGameEvent *event )
	{

		C_TFPlayer *pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if ( !Q_strcmp( event->GetName(), "mirv_defused" ) )
		{
			if ( pLocalPlayer->GetUserID() == event->GetInt( "attacker" ) )
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF2CDefuseMIRV, TF2C_ACHIEVEMENT_DEFUSE_MIRV, "TF2C_DEFUSE_MIRV", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CPlayGameClassicMaps_1 : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTF2CPlayGameClassicMaps_1, CTFAchievementFullRound );

public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS | ACH_FILTER_FULL_ROUND_ONLY );

		static const char *szComponents[] =
		{
			"cp_dustbowl", "ctf_2fort", "cp_badlands", "ctf_casbah", "ctf_well"
		};

		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE( szComponents );
		SetGoal( m_iNumComponents );
		SetType( ACHIEVEMENT_TYPE_ROUND );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event )
	{
		float flTeamplayStartTime = m_pAchievementMgr->GetTeamplayStartTime();
		if ( flTeamplayStartTime > 0 ) 
		{	
			// Has the player been present and on a game team since the start of this round (minus a grace period)?
			if ( flTeamplayStartTime < ( gpGlobals->curtime - flRoundTime ) + TF_FULL_ROUND_GRACE_PERIOD )
			{
				// The achievement is satisfied for this map, set the corresponding bit.
				OnComponentEvent( m_pAchievementMgr->GetMapName() );
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF2CPlayGameClassicMaps_1, TF2C_ACHIEVEMENT_PLAY_GAME_CLASSICMAPS_1, "TF2C_PLAY_GAME_CLASSICMAPS_1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CDominateDeveloper : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetType( ACHIEVEMENT_TYPE_MISTAKE );
	}

	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT_(CAchievementTF2CDominateDeveloper, TF2C_ACHIEVEMENT_DOMINATE_DEVELOPER, "TF2C_DOMINATE_DEVELOPER", NULL, 10, true);

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CExtinguishBomblets : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal(ACHIEVEMENT_TF2C_EXTINGUISH_BOMBLETS_REQUIREMENT);
		SetType( ACHIEVEMENT_TYPE_PLAYER );
	}

	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT( CAchievementTF2CExtinguishBomblets, TF2C_ACHIEVEMENT_EXTINGUISH_BOMBLETS, "TF2C_ACHIEVEMENT_EXTINGUISH_BOMBLETS", 10 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CKillBlindRicochet : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetType( ACHIEVEMENT_TYPE_MINOR );
	}

	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT( CAchievementTF2CKillBlindRicochet, TF2C_ACHIEVEMENT_KILL_WITH_BLINDCOILRICOCHET, "TF2C_KILL_WITH_BLINDCOILRICOCHET", 10 );

//----------------------------------------------------------------------------------------------------------------
// V.I.P. Pack
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CWinCivilianNoDeaths : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetType( ACHIEVEMENT_TYPE_ROUND );
	}

	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT( CAchievementTF2CWinCivilianNoDeaths, TF2C_ACHIEVEMENT_WIN_CIVILIAN_NODEATHS, "TF2C_WIN_CIVILIAN_NODEATHS", 40 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CHealCivilian : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( ACHIEVEMENT_TF2C_LOYAL_SERVANT_HEALING_REQUIREMENT );
		SetType( ACHIEVEMENT_TYPE_PLAYER );
	}

	void ListenForEvents()
	{
		ListenForGameEvent( "vip_healed" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		C_TFPlayer *pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if ( !Q_strcmp( event->GetName(), "vip_healed" ) )
		{
			if ( pLocalPlayer->GetUserID() == event->GetInt( "healer" ) && !pLocalPlayer->IsVIP() )
			{
				IncrementCount( event->GetInt( "amount" ) );
				m_pAchievementMgr->SetDirty( true );
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF2CHealCivilian, TF2C_ACHIEVEMENT_HEAL_CIVILIAN, "TF2C_HEAL_CIVILIAN", 25 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CDominateCivilian : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetType( ACHIEVEMENT_TYPE_MINOR );
	}

	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT( CAchievementTF2CDominateCivilian, TF2C_ACHIEVEMENT_DOMINATE_CIVILIAN, "TF2C_DOMINATE_CIVILIAN", 15 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CPlayGameVIPMaps_1 : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTF2CPlayGameVIPMaps_1, CTFAchievementFullRound );

public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS | ACH_FILTER_FULL_ROUND_ONLY );

		static const char *szComponents[] =
		{
			"vip_badwater", "vip_mineside", "vip_harbor", "vip_trainyard"
		};

		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE( szComponents );
		SetGoal( m_iNumComponents );
		SetType( ACHIEVEMENT_TYPE_ROUND );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event )
	{
		float flTeamplayStartTime = m_pAchievementMgr->GetTeamplayStartTime();
		if ( flTeamplayStartTime > 0 ) 
		{	
			// Has the player been present and on a game team since the start of this round (minus a grace period)?
			if ( flTeamplayStartTime < ( gpGlobals->curtime - flRoundTime ) + TF_FULL_ROUND_GRACE_PERIOD )
			{
				// The achievement is satisfied for this map, set the corresponding bit.
				OnComponentEvent( m_pAchievementMgr->GetMapName() );
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF2CPlayGameVIPMaps_1, TF2C_ACHIEVEMENT_PLAY_GAME_VIPMAPS_1, "TF2C_PLAY_GAME_VIPMAPS_1", 5 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CKillCivilianDisguiseBoost : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetType( ACHIEVEMENT_TYPE_MAJOR );
	}
	
	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT( CAchievementTF2CKillCivilianDisguiseBoost, TF2C_ACHIEVEMENT_KILL_CIVILIAN_DISGUISEBOOST, "TF2C_KILL_CIVILIAN_DISGUISEBOOST", 50 );
//----------------------------------------------------------------------------------------------------------------
// Domination Pack
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CHoldAllPoints_Domination : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetType( ACHIEVEMENT_TYPE_ROUND );
	}
	
	// Server fires an event for this achievement.
};
DECLARE_ACHIEVEMENT( CAchievementTF2CHoldAllPoints_Domination, TF2C_ACHIEVEMENT_HOLD_ALLPOINTS_DOMINATION, "TF2C_HOLD_ALLPOINTS_DOMINATION", 10 );

//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2CPlayGameDominationMaps_1 : public CTFAchievementFullRound
{
	DECLARE_CLASS( CAchievementTF2CPlayGameDominationMaps_1, CTFAchievementFullRound );

public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS | ACH_FILTER_FULL_ROUND_ONLY );

		static const char *szComponents[] =
		{
			"dom_oilcanyon", "dom_hydro"
		};

		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE( szComponents );
		SetGoal( m_iNumComponents );
		SetType( ACHIEVEMENT_TYPE_ROUND );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "teamplay_round_win" );
	}

	virtual void Event_OnRoundComplete( float flRoundTime, IGameEvent *event )
	{
		float flTeamplayStartTime = m_pAchievementMgr->GetTeamplayStartTime();
		if ( flTeamplayStartTime > 0 ) 
		{	
			// Has the player been present and on a game team since the start of this round (minus a grace period)?
			if ( flTeamplayStartTime < ( gpGlobals->curtime - flRoundTime ) + TF_FULL_ROUND_GRACE_PERIOD )
			{
				// The achievement is satisfied for this map, set the corresponding bit.
				OnComponentEvent( m_pAchievementMgr->GetMapName() );
			}
		}
	}
};
DECLARE_ACHIEVEMENT( CAchievementTF2CPlayGameDominationMaps_1, TF2C_ACHIEVEMENT_PLAY_GAME_DOMMAPS_1, "TF2C_PLAY_GAME_DOMMAPS_1", 5 );




//----------------------------------------------------------------------------------------------------------------
// Patch 2.1.0 Pack
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2C_KillJumpPadStomp : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS );
		SetGoal(1);
		SetType( ACHIEVEMENT_TYPE_MINOR );
	}

	virtual void Event_EntityKilled( CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event )
	{
		C_TFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		int iEvent = event->GetInt( "customkill" );
		if ( iEvent == TF_DMG_CUSTOM_JUMPPAD_STOMP )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_KillJumpPadStomp, TF2C_ACHIEVEMENT_JUMPPAD_STOMP, "TF2C_ACHIEVEMENT_JUMPPAD_STOMP", 15);

/*
class CAchievementTF2C_SovietUnion : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS );
		SetGoal(ACHIEVEMENT_TF2C_SOVIET_UNION_ASSIST_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer *pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		C_TFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		C_TFPlayer *pTFAssister = ToTFPlayer(UTIL_PlayerByUserId(event->GetInt("assister")));
		if (!pTFAssister)
			return;


		if (pLocalPlayer == pTFAttacker || pLocalPlayer == pTFAssister )
		{
			if (pTFAssister->IsPlayerClass(TF_CLASS_HEAVYWEAPONS) && pTFAttacker->IsPlayerClass(TF_CLASS_HEAVYWEAPONS) &&
				pTFAssister->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PRIMARY) && pTFAttacker->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PRIMARY) &&
				pTFAssister->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PRIMARY)->GetItemID() != pTFAttacker->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PRIMARY)->GetItemID())
			{
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SovietUnion, TF2C_ACHIEVEMENT_SOVIET_UNION, "TF2C_SOVIET_UNION", 15);
*/

/*class CAchievementTF2C_ExtremeShockTherapy : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	// Achievement is called by a server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_ExtremeShockTherapy, TF2C_ACHIEVEMENT_EXTREME_SHOCK_THERAPY, "TF2C_EXTREME_SHOCK_THERAPY", 15);*/

/*
class CAchievementTF2C_DisguiseMastery : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS);

		//These come from g_aRawPlayerClassNamesShort
		static const char *szComponents[] =
		{
			"scout", "soldier", "pyro",
			"demo", "heavy", "engineer",
			"medic", "sniper", "spy",
		};

		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE(szComponents);
		SetGoal(m_iNumComponents);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("disguise_mastery_achievement");
	}

	// Most of it is handled server-side
	void FireGameEvent_Internal(IGameEvent *event)
	{
		C_TFPlayer *pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->GetUserID() == event->GetInt("attacker"))
		{
			int iDisguiseClass = event->GetInt("disguiseclass");
			OnComponentEvent(g_aRawPlayerClassNamesShort[iDisguiseClass]);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_DisguiseMastery, TF2C_ACHIEVEMENT_DISGUISE_MASTERY, "TF2C_DISGUISE_MASTERY", 15);
*/

/*class CAchievementTF2C_WelcomeToTF2C : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		C_TFPlayer *pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer == pVictim && pLocalPlayer->IsPlayerClass(TF_CLASS_SCOUT) && !Q_strcmp(event->GetString("weapon"), "rpg"))
		{
			CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_WelcomeToTF2C, TF2C_ACHIEVEMENT_WELCOME_TO_TF2C, "TF2C_ACHIEVEMENT_WELCOME_TO_TF2C", 15);*/

/*class CAchievementTF2C_BodyshotMedic : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_BODYSHOT_MEDIC_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFVictim)
			return;

		if (event->GetInt("customkill") != TF_DMG_CUSTOM_HEADSHOT && !Q_strcmp(event->GetString("weapon"), "sniperrifle") 
			&& pTFVictim->IsPlayerClass(TF_CLASS_MEDIC))
		{
			CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_BodyshotMedic, TF2C_ACHIEVEMENT_BODYSHOT_MEDIC, "TF2C_ACHIEVEMENT_BODYSHOT_MEDIC", 15);*/

class CAchievementTF2C_ChekhovCritKill : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "chekhov") && event->GetInt("crittype") == CTakeDamageInfo::CRIT_FULL
			&& pTFAttacker->m_Shared.GetStoredCrits() == pTFAttacker->m_Shared.GetStoredCritsCapacity())
		{
			CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_ChekhovCritKill, TF2C_ACHIEVEMENT_CHEKHOV_CRIT_KILL, "TF2C_ACHIEVEMENT_CHEKHOV_CRIT_KILL", 15);

/*class CAchievementTF2C_CritKillMapHarvester : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "harvester") && event->GetInt("crittype") == CTakeDamageInfo::CRIT_FULL)
		{
			// This is better than setting map name filter as it allows multiple maps
			if (!Q_strcmp(m_pAchievementMgr->GetMapName(), "arena_haymaker") ||
				!Q_strcmp(m_pAchievementMgr->GetMapName(), "arena_haymaker2") ||
				!Q_strcmp(m_pAchievementMgr->GetMapName(), "arena_haymaker3") ||
				!Q_strcmp(m_pAchievementMgr->GetMapName(), "koth_harvest_final") ||
				!Q_strcmp(m_pAchievementMgr->GetMapName(), "koth_harvest_event"))
			{
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_CritKillMapHarvester, TF2C_ACHIEVEMENT_CRIT_KILL_MAP_HARVESTER, "TF2C_ACHIEVEMENT_CRIT_KILL_MAP_HARVESTER", 15);*/

/*
class CAchievementTF2C_PopTheMedic : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_POP_THE_MEDIC_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	//Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_PopTheMedic, TF2C_ACHIEVEMENT_POP_THE_MEDIC, "TF2C_ACHIEVEMENT_POP_THE_MEDIC", 15);
*/

class CAchievementTF2C_TranqKill : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer *pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "tranq") && pVictim)
		{
			Vector vecVictimCoord(pVictim->GetAbsOrigin());
			Vector vecSegment;

			VectorSubtract(pTFAttacker->GetAbsOrigin(), vecVictimCoord, vecSegment);
			float flDist2 = vecSegment.LengthSqr();
			if (flDist2 >= Square(ACHIEVEMENT_TF2C_TRANQ_KILL_DISTANCE))
			{
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TranqKill, TF2C_ACHIEVEMENT_TRANQ_KILL, "TF2C_ACHIEVEMENT_TRANQ_KILL", 15);

class CAchievementTF2C_AAVSAirborne : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_AA_VS_AIRBORNE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	//Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_AAVSAirborne, TF2C_ACHIEVEMENT_AA_VS_AIRBORNE, "TF2C_ACHIEVEMENT_AA_VS_AIRBORNE", 15);

class CAchievementTF2C_TranqMeleeKillAsTeammate : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	//Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TranqMeleeKillAsTeammate, TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL_AS_TEAMMATE, "TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL_AS_TEAMMATE", 15);

/*
class CAchievementTF2C_TranqMeleeKillTeammateAssist : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_TRANQ_MELEE_KILL_TEAMMATE_ASSIST_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	//Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TranqMeleeKillTeammateAssist, TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL_TEAMMATE_ASSIST, "TF2C_ACHIEVEMENT_TRANQ_MELEE_KILL_TEAMMATE_ASSIST", 15);
*/

/*
class CAchievementTF2C_TranqTeammateAssist : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_TRANQ_TEAMMATE_ASSIST_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	//Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TranqTeammateAssist, TF2C_ACHIEVEMENT_TRANQ_TEAMMATE_ASSIST, "TF2C_ACHIEVEMENT_TRANQ_TEAMMATE_ASSIST", 15);
*/

class CAchievementTF2C_TranqHitBlastJump : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	//Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TranqHitBlastJump, TF2C_ACHIEVEMENT_TRANQ_HIT_BLASTJUMP, "TF2C_ACHIEVEMENT_TRANQ_HIT_BLASTJUMP", 15);

class CAchievementTF2C_TranqCounterKill : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_TRANQ_COUNTER_KILL_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		CBaseEntity* pTranqProvider = pTFAttacker->m_Shared.GetConditionProvider(TF_COND_TRANQUILIZED);
		if (pTranqProvider && pVictim && pTranqProvider == pVictim)
		{
			CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TranqCounterKill, TF2C_ACHIEVEMENT_TRANQ_COUNTER_KILL, "TF2C_ACHIEVEMENT_TRANQ_COUNTER_KILL", 15);

/*
class CAchievementTF2C_DrunkenMaster : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "bottle") && pTFAttacker->m_Shared.InCond(TF_COND_TRANQUILIZED))
		{
			CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_DrunkenMaster, TF2C_ACHIEVEMENT_DRUNKEN_MASTER, "TF2C_ACHIEVEMENT_DRUNKEN_MASTER", 15);
*/

class CAchievementTF2C_TranqSupportProgression : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_TRANQ_SUPPORT_PROGRESSION_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("tranq_support");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "tranq_support"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				IncrementCount(event->GetInt("amount"));
				m_pAchievementMgr->SetDirty(true);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TranqSupportProgression, TF2C_ACHIEVEMENT_TRANQ_SUPPORT_PROGRESSION, "TF2C_ACHIEVEMENT_TRANQ_SUPPORT_PROGRESSION", 15);


class CAchievementTF2C_TauntKillMIRV : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (event->GetInt("customkill") == TF_DMG_CUSTOM_TAUNTATK_MIRV)
		{
			CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TauntKillMIRV, TF2C_ACHIEVEMENT_TAUNT_KILL_MIRV, "TF2C_ACHIEVEMENT_TAUNT_KILL_MIRV", 15);


class CAchievementTF2C_MinesUnseenDamage : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_MINES_UNSEEN_DAMAGE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}


	void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			CBaseEntity* pVictim = ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("userid")));
			if (pLocalPlayer->IsAlive() && pLocalPlayer->GetUserID() == event->GetInt("attacker") && pVictim && pLocalPlayer->IsEnemy(pVictim) && \
				event->GetInt("weaponid2") == TF_WEAPON_PIPEBOMBLAUNCHER)
			{
				int proxyMine = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_SECONDARY), proxyMine, mod_sticky_is_proxy);
				if (proxyMine > 0)
				{
					Vector vecMinePoint(event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z"));

					// It's true that by the time event arrives player coordinates may not be correct to when damage has taken place, but it's ok, it's a huge progression achievement
					trace_t tr;
					UTIL_TraceLine(pLocalPlayer->EyePosition(), vecMinePoint, MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr);
					if (tr.fraction != 1.0f)
					{
						IncrementCount(event->GetInt("damageamount"));
						m_pAchievementMgr->SetDirty(true);
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinesUnseenDamage, TF2C_ACHIEVEMENT_MINES_UNSEEN_DAMAGE, "TF2C_ACHIEVEMENT_MINES_UNSEEN_DAMAGE", 15);

class CAchievementTF2C_MinesStreakSingleLife : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetCounterGoal(ACHIEVEMENT_TF2C_MINES_STREAK_SINGLE_LIFE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
	}

	void ListenForEvents()
	{
		CSingleLifeCounterAchievement::ListenForEvents();

		ListenForGameEvent("mine_hurt_enemy_players");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		CSingleLifeCounterAchievement::FireGameEvent_Internal(event);

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "mine_hurt_enemy_players"))
		{
			if (pLocalPlayer->IsAlive() && pLocalPlayer->GetUserID() == event->GetInt("attacker"))
			{
				if (event->GetBool("isfullyprimed"))
				{
					IncrementSingeLifeCounter();
				}
			}
			
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinesStreakSingleLife, TF2C_ACHIEVEMENT_MINES_STREAK_SINGLE_LIFE, "TF2C_ACHIEVEMENT_MINES_STREAK_SINGLE_LIFE", 50);

class CAchievementTF2C_MinesTriggerWithoutHurt : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_MINES_TRIGGER_WITHOUT_HURT_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_TRIVIAL);
	}

	// Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinesTriggerWithoutHurt, TF2C_ACHIEVEMENT_MINES_TRIGGER_WITHOUT_HURT, "TF2C_ACHIEVEMENT_MINES_TRIGGER_WITHOUT_HURT", 15);

/*
class CAchievementTF2C_MinesJumpAndDestroy : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	// handled by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinesJumpAndDestroy, TF2C_ACHIEVEMENT_MINES_JUMP_AND_DESTROY, "TF2C_ACHIEVEMENT_MINES_JUMP_AND_DESTROY", 15);
*/

/*class CAchievementTF2C_MinesPreventSelfDetonation : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_TRIVIAL);
	}

	// Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinesPreventSelfDetonation, TF2C_ACHIEVEMENT_MINES_PREVENT_SELF_DETONATION, "TF2C_ACHIEVEMENT_MINES_PREVENT_SELF_DETONATION", 5);*/

class CAchievementTF2C_StickyPrimedKills : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_STICKY_PRIMED_KILLS_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	
	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "tf_projectile_pipe_remote"))
		{
			if (!(event->GetInt("damagebits") & DMG_USEDISTANCEMOD))
			{
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_StickyPrimedKills, TF2C_ACHIEVEMENT_STICKY_PRIMED_KILLS, "TF2C_ACHIEVEMENT_STICKY_PRIMED_KILLS", 15);

/*
class CAchievementTF2C_StickyMinesRemoval : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_STICKY_MINES_REMOVAL_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	// Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_StickyMinesRemoval, TF2C_ACHIEVEMENT_STICKY_MINES_REMOVAL, "TF2C_ACHIEVEMENT_STICKY_MINES_REMOVAL", 15);
*/

class CAchievementTF2C_SandvichMines : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_TRIVIAL);
	}

	// Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SandvichMines, TF2C_ACHIEVEMENT_SANDVICH_MINES, "TF2C_ACHIEVEMENT_SANDVICH_MINES", 15);




// single counter to have easy reset function to override
class CAchievementTF2C_SurfEnemyMine : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetCounterGoal(1);
		SetType(ACHIEVEMENT_TYPE_TRIVIAL);
	}

	void ListenForEvents()
	{
		CSingleLifeCounterAchievement::ListenForEvents();

		ListenForGameEvent("enemy_primed_mine_jump");
		ListenForGameEvent("blast_jump_landed");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		CSingleLifeCounterAchievement::FireGameEvent_Internal(event);

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "enemy_primed_mine_jump") \
			&& pLocalPlayer->IsAlive() && pLocalPlayer->GetUserID() == event->GetInt("userid"))
		{
			bEnemyMineJumped = true;
			vecEnemyMineJumpPos = Vector(pLocalPlayer->GetAbsOrigin().x, pLocalPlayer->GetAbsOrigin().y, 0);
		}

		if (!Q_strcmp(event->GetName(), "blast_jump_landed"))
		{
			if (bEnemyMineJumped && pLocalPlayer->IsAlive() && pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				Vector vecSegment;

				VectorSubtract(Vector(pLocalPlayer->GetAbsOrigin().x, pLocalPlayer->GetAbsOrigin().y, 0), vecEnemyMineJumpPos, vecSegment);
				float flDist2 = vecSegment.LengthSqr();
				if (flDist2 >= Square(ACHIEVEMENT_TF2C_SURF_ENEMY_MINE_DISTANCE))
				{
					IncrementSingeLifeCounter();
				}
			}

		}
	}
protected:
	void ResetCounter()
	{
		bEnemyMineJumped = false;
		vecEnemyMineJumpPos = vec3_origin;
	}
private:
	bool bEnemyMineJumped;
	Vector vecEnemyMineJumpPos;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SurfEnemyMine, TF2C_ACHIEVEMENT_SURF_ENEMY_MINE, "TF2C_ACHIEVEMENT_SURF_ENEMY_MINE", 15);

/*
class CAchievementTF2C_MinesLongRange : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_MINES_LONG_RANGE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}


	void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			CBaseEntity* pVictim = ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("userid")));
			if (pLocalPlayer->IsAlive() && pLocalPlayer->GetUserID() == event->GetInt("attacker") && pVictim && pLocalPlayer->IsEnemy(pVictim) && \
				event->GetInt("weaponid2") == TF_WEAPON_PIPEBOMBLAUNCHER && \
				event->GetBool("usedistancemod") == true) // Not primed!
			{
				int proxyMine = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_SECONDARY), proxyMine, mod_sticky_is_proxy);
				if (proxyMine > 0)
				{
					Vector vecMinePoint(event->GetFloat("x"), event->GetFloat("y"), event->GetFloat("z")); // why mine origin? shrug

					Vector vecSegment;

					VectorSubtract(pLocalPlayer->GetAbsOrigin(), vecMinePoint, vecSegment);
					float flDist2 = vecSegment.LengthSqr();
					if (flDist2 >= Square(ACHIEVEMENT_TF2C_MINES_LONG_RANGE_DISTANCE))
					{
						IncrementCount(event->GetInt("damageamount"));
						m_pAchievementMgr->SetDirty(true);
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinesLongRange, TF2C_ACHIEVEMENT_MINES_LONG_RANGE, "TF2C_ACHIEVEMENT_MINES_LONG_RANGE", 15);
*/

class CAchievementTF2C_RevolverKillsSingleLife : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetCounterGoal(ACHIEVEMENT_TF2C_REVOLVER_KILLS_SINGLE_LIFE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "revolver"))
		{
			IncrementSingeLifeCounter();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_RevolverKillsSingleLife, TF2C_ACHIEVEMENT_REVOLVER_KILLS_SINGLE_LIFE, "TF2C_ACHIEVEMENT_REVOLVER_KILLS_SINGLE_LIFE", 15);

/*
class CAchievementTF2CRevolverBuildingSabotage : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_REVOLVER_BUILDING_SABOTAGE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("object_destroyed");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "object_destroyed"))
		{

			CBaseEntity* pBuilder = ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("userid")));
			if (!(pBuilder && pBuilder->IsAlive()))
				return;

			if (pLocalPlayer->GetUserID() == event->GetInt("attacker"))
			{
				if (!Q_strcmp(event->GetString("weapon"), "revolver") && !event->GetBool("was_sapped"))
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2CRevolverBuildingSabotage, TF2C_ACHIEVEMENT_REVOLVER_BUILDING_SABOTAGE, "TF2C_ACHIEVEMENT_REVOLVER_BUILDING_SABOTAGE", 15);
*/

class CAchievementTF2C_HarvesterCollectDecap : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);

		//These come from g_aRawPlayerClassNamesShort
		static const char* szComponents[] =
		{
			"scout", "soldier", "pyro",
			"demo", "heavy", "engineer",
			"medic", "sniper", "spy",
		};

		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE(szComponents);
		SetGoal(m_iNumComponents);
		SetType(ACHIEVEMENT_TYPE_ROUND);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (event->GetInt("customkill") == TF_DMG_CUSTOM_DECAPITATION && !Q_strcmp(event->GetString("weapon"), "harvester"))
		{
			C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
			if (!pTFVictim)
				return;

			C_TFPlayerClass* pPlayerClass = pTFVictim->GetPlayerClass();
			if (pPlayerClass)
			{
				OnComponentEvent(g_aRawPlayerClassNamesShort[pPlayerClass->GetClassIndex()]); // maybe this is going too far but whatever
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HarvesterCollectDecap, TF2C_ACHIEVEMENT_HARVESTER_COLLECT_DECAP, "TF2C_ACHIEVEMENT_HARVESTER_COLLECT_DECAP", 15);

class CAchievementTF2C_HarvesterDecapSingleLife : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetCounterGoal(ACHIEVEMENT_TF2C_HARVESTER_DECAP_SINGLE_LIFE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (event->GetInt("customkill") == TF_DMG_CUSTOM_DECAPITATION && !Q_strcmp(event->GetString("weapon"), "harvester"))
		{
			IncrementSingeLifeCounter();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HarvesterDecapSingleLife, TF2C_ACHIEVEMENT_HARVESTER_DECAP_SINGLE_LIFE, "TF2C_ACHIEVEMENT_HARVESTER_DECAP_SINGLE_LIFE", 15);

// I am getting lost in these send help immedaitltea

/*
class CAchievementTF2CHarvesterHealProgression : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_HARVESTER_HEAL_PROGRESSION_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("healed_on_afterburn");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "healed_on_afterburn"))
		{
			if (pLocalPlayer->entindex() == event->GetInt("entindex") && pLocalPlayer->IsPlayerClass(TF_CLASS_PYRO))
			{
				C_TFWeaponBase* pMeleeWeapon = pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_MELEE);
				if (pMeleeWeapon && pMeleeWeapon->GetWeaponID() == TF_WEAPON_BEACON)
				{
					if (event->GetInt("amount") > 0)
					{
						IncrementCount(event->GetInt("amount"));
						m_pAchievementMgr->SetDirty(true);
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2CHarvesterHealProgression, TF2C_ACHIEVEMENT_HARVESTER_HEAL_PROGRESSION, "TF2C_ACHIEVEMENT_HARVESTER_HEAL_PROGRESSION", 25);
*/

class CAchievementTF2C_HarvesterCounterLowHealth : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_HARVESTER_COUNTER_LOW_HEALTH_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	// Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HarvesterCounterLowHealth, TF2C_ACHIEVEMENT_HARVESTER_COUNTER_LOW_HEALTH, "TF2C_ACHIEVEMENT_HARVESTER_COUNTER_LOW_HEALTH", 15);

class CAchievementTF2C_HarvesterCounterSpy : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}

	// Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HarvesterCounterSpy, TF2C_ACHIEVEMENT_HARVESTER_COUNTER_SPY, "TF2C_ACHIEVEMENT_HARVESTER_COUNTER_SPY", 15);

class CAchievementTF2C_FireAxeDeathAfterburn : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_FIRE_AXE_DEATH_AFTERBURN_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}


	void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			if (!pLocalPlayer->IsAlive() && pLocalPlayer->GetUserID() == event->GetInt("attacker") && \
				event->GetBool("is_afterburn") == true)
			{
				C_TFWeaponBase* pWeapon = pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_MELEE);
				if (pWeapon && pWeapon->GetItemID() == 2) // that's fire axe
				{
					IncrementCount(event->GetInt("damageamount"));
					m_pAchievementMgr->SetDirty(true);
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_FireAxeDeathAfterburn, TF2C_ACHIEVEMENT_FIRE_AXE_DEATH_AFTERBURN, "TF2C_ACHIEVEMENT_FIRE_AXE_DEATH_AFTERBURN", 15);

class CAchievementTF2C_HeavyShotgunCrits : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_HEAVY_SHOTGUN_CRITS_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "shotgun_hwg") && event->GetInt("crittype") == CTakeDamageInfo::CRIT_FULL) // using shotgun-like secondaries is a fair game!
		{
			CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HeavyShotgunCrits, TF2C_ACHIEVEMENT_HEAVY_SHOTGUN_CRITS, "TF2C_ACHIEVEMENT_HEAVY_SHOTGUN_CRITS", 15);

/*
class CAchievementTF2C_ChekhovCloseMeleeKills : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_CHEKHOV_CLOSE_MELEE_KILLS_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	// called by server
	// we need to know what enemy player held in hand before his death that clears everything
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_ChekhovCloseMeleeKills, TF2C_ACHIEVEMENT_CHEKHOV_CLOSE_MELEE_KILLS, "TF2C_ACHIEVEMENT_CHEKHOV_CLOSE_MELEE_KILLS", 15);
*/

/*
class CAchievementTF2C_ScoutBattingHeavy : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_SCOUT_BATTING_HEAVY_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "bat"))
		{
			C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
			if (pTFVictim && pTFVictim->IsPlayerClass(TF_CLASS_HEAVYWEAPONS))
			{
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_ScoutBattingHeavy, TF2C_ACHIEVEMENT_SCOUT_BATTING_HEAVY, "TF2C_ACHIEVEMENT_SCOUT_BATTING_HEAVY", 15);
*/

class CAchievementTF2C_ChekhovShock : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "taser"))
		{
			C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
			if (pTFVictim && pTFVictim->IsPlayerClass(TF_CLASS_HEAVYWEAPONS))
			{
				C_TFWeaponBase* pVictimMelee = pTFVictim->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_MELEE);
				if (pVictimMelee && pVictimMelee->GetItemID() == 49)
				{
					CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_ChekhovShock, TF2C_ACHIEVEMENT_CHEKHOV_SHOCK, "TF2C_ACHIEVEMENT_CHEKHOV_SHOCK", 15);

class CAchievementTF2C_FistsMeleeDuel : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}

	// handled by server
	// gotta know if player held melee out
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_FistsMeleeDuel, TF2C_ACHIEVEMENT_FISTS_MELEE_DUEL, "TF2C_ACHIEVEMENT_FISTS_MELEE_DUEL", 15);

//---------------------------------------------------------------------------------------------------------------
// Jumppad Achievements pack
//---------------------------------------------------------------------------------------------------------------
class CAchievementTF2C_TeleporterProgression : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_TELEPORTER_PROGRESSION_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_teleported");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_teleported"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("builderid") && pLocalPlayer->GetUserID() != event->GetInt("userid"))
			{
				// we aren't going to check whether it was a Spy so you can't use it for spycheck
				IncrementCount(Ceil2Int(HammerUnitsToMeters(event->GetFloat("dist"))));
				m_pAchievementMgr->SetDirty(true);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TeleporterProgression, TF2C_ACHIEVEMENT_TELEPORTER_PROGRESSION, "TF2C_ACHIEVEMENT_TELEPORTER_PROGRESSION", 15);

class CAchievementTF2C_TeleporterEnemySpyKill : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer( pVictim );
		if ( !pTFVictim )
			return;

		if ( pTFVictim->IsPlayerClass( TF_CLASS_ENGINEER ) )
		{
			// Catch Telefrags. They happen before TF_COND_TELEPORTED is applied.
			if ( event->GetInt( "customkill" ) == TF_DMG_CUSTOM_TELEFRAG )
			{
				CBaseAchievement::Event_EntityKilled( pVictim, pAttacker, pInflictor, event );
			}

			if ( pTFAttacker->m_Shared.InCond( TF_COND_TELEPORTED ) || pTFAttacker->m_Shared.InCond(TF_COND_TELEPORTED_ALWAYS_SHOW) )
			{
				if ( pTFAttacker->m_Shared.GetTeleporterEffectColor() == pTFAttacker->GetTeamNumber() )
					return;

				CBaseAchievement::Event_EntityKilled( pVictim, pAttacker, pInflictor, event );
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TeleporterEnemySpyKill, TF2C_ACHIEVEMENT_TELEPORTER_ENEMY_SPY_KILL, "TF2C_ACHIEVEMENT_TELEPORTER_ENEMY_SPY_KILL", 15);

class CAchievementTF2C_JumppadEnemySpyBackstab : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (event->GetInt("customkill") == TF_DMG_CUSTOM_BACKSTAB && pTFAttacker->m_Shared.InCond(TF_COND_JUMPPAD_ASSIST))
		{
			C_TFPlayer* pTFCondProvider = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_JUMPPAD_ASSIST));
			if (pTFCondProvider)
			{
				if (pTFCondProvider->GetTeamNumber() != pTFAttacker->GetTeamNumber())
				{
					CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_JumppadEnemySpyBackstab, TF2C_ACHIEVEMENT_JUMPPAD_ENEMY_SPY_BACKSTAB, "TF2C_ACHIEVEMENT_JUMPPAD_ENEMY_SPY_BACKSTAB", 15);

class CAchievementTF2C_JumppadExtinguish : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_TRIVIAL);
	}

	// handled by server
	// need to know cond when was extinguished
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_JumppadExtinguish, TF2C_ACHIEVEMENT_JUMPPAD_EXTINGUISH, "TF2C_ACHIEVEMENT_JUMPPAD_EXTINGUISH", 15);

class CAchievementTF2C_JumppadProgression : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_JUMPPAD_PROGRESSION_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_used_jumppad");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_used_jumppad"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("builderid") && pLocalPlayer->GetUserID() != event->GetInt("userid"))
			{
				// we aren't going to check whether it was a Spy so you can't use it for spycheck
				IncrementCount(1);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_JumppadProgression, TF2C_ACHIEVEMENT_JUMPPAD_PROGRESSION, "TF2C_ACHIEVEMENT_JUMPPAD_PROGRESSION", 15);

class CAchievementTF2C_JumppadProgressionAssist : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_JUMPPAD_PROGRESSION_ASSIST_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}


	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (pTFAttacker != pLocalPlayer && pTFAttacker->m_Shared.InCond(TF_COND_JUMPPAD_ASSIST))
		{
			C_TFPlayer* pJumppadCondProvider = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_JUMPPAD_ASSIST));
			if (pJumppadCondProvider == pLocalPlayer)
			{
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_JumppadProgressionAssist, TF2C_ACHIEVEMENT_JUMPPAD_PROGRESSION_ASSIST, "TF2C_ACHIEVEMENT_JUMPPAD_PROGRESSION_ASSIST", 15);

/*
class CAchievementTF2C_JumppadKillOnUse : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_JUMPPAD_KILL_ON_USE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_JUMPPAD_ASSIST))
		{
			CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_JumppadKillOnUse, TF2C_ACHIEVEMENT_JUMPPAD_KILL_ON_USE, "TF2C_ACHIEVEMENT_JUMPPAD_KILL_ON_USE", 15);
*/

class CAchievementTF2C_JumppadProgressionDestroyed : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_JUMPPAD_PROGRESSION_DESTROYED_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("object_destroyed");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "object_destroyed"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("attacker"))
			{
				if (event->GetInt("objecttype") == OBJ_JUMPPAD)
				{
					IncrementCount();
					m_pAchievementMgr->SetDirty(true);
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_JumppadProgressionDestroyed, TF2C_ACHIEVEMENT_JUMPPAD_PROGRESSION_DESTROYED, "TF2C_ACHIEVEMENT_JUMPPAD_PROGRESSION_DESTROYED", 15);

class CAchievementTF2C_AirCombat : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	// Called by server
	// Need to know about victim cond states
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_AirCombat, TF2C_ACHIEVEMENT_AIR_COMBAT, "TF2C_ACHIEVEMENT_AIR_COMBAT", 15);

class CAchievementTF2C_HeadshotKillMidflight : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
	}

	// Called by server
	// Need to know about victim cond states	
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HeadshotKillMidflight, TF2C_ACHIEVEMENT_HEADSHOT_KILL_MIDFLIGHT, "TF2C_ACHIEVEMENT_HEADSHOT_KILL_MIDFLIGHT", 15);

/*
class CAchievementTF2C_JumppadJumpAndSap : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_JUMPPAD_JUMP_AND_SAP_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_sapped_object");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_sapped_object"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				if (pLocalPlayer->m_Shared.InCond(TF_COND_JUST_USED_JUMPPAD))
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_JumppadJumpAndSap, TF2C_ACHIEVEMENT_JUMPPAD_JUMP_AND_SAP, "TF2C_ACHIEVEMENT_JUMPPAD_JUMP_AND_SAP", 15);
*/

//----------------------------------------------------------------------------------------------------------------
// TF2C Map Achievements 2.1.0
//----------------------------------------------------------------------------------------------------------------

/*
class CAchievementTF2C_MapFloodgate : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_ROUND);
		SetMapNameFilter("arena_floodgate_b2");
	}

	void ListenForEvents()
	{
		ListenForGameEvent("teamplay_round_win");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (!Q_strcmp(event->GetName(), "teamplay_round_win"))
		{
			if (event->GetInt("team") == GetLocalPlayerTeam() && event->GetInt("winreason") == WINREASON_ALL_POINTS_CAPTURED)
			{
				if (pLocalPlayer->IsAlive())
				{
					IncrementCount();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MapFloodgate, TF2C_ACHIEVEMENT_MAP_FLOODGATE, "TF2C_ACHIEVEMENT_MAP_FLOODGATE", 15);
*/

//----------------------------------------------------------------------------------------------------------------
// TF2C Misc Achievements 2.1.0
//----------------------------------------------------------------------------------------------------------------

class CAchievementTF2C_4TeamBackstab : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("teamplay_round_active"); 
		ListenForGameEvent("localplayer_changeteam");
		ResetVars();
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		if (!Q_strcmp(event->GetName(), "teamplay_round_active") \
			|| !Q_strcmp(event->GetName(), "localplayer_changeteam"))
		{
			ResetVars();
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFVictim)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		CTFGameRules* pTFGameRules = TFGameRules();
		if (pTFGameRules && pTFGameRules->IsFourTeamGame())
		{
			if (event->GetInt("customkill") == TF_DMG_CUSTOM_BACKSTAB)
			{
				vecDidTeams[pTFVictim->GetTeamNumber() - (LAST_SHARED_TEAM + 1)] = true;
				for (int i = 0; i < 4; ++i)
				{
					if (i == (GetLocalPlayerTeam() - (LAST_SHARED_TEAM + 1)))
						continue;

					if (vecDidTeams[i] != true)
						return;
				}
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
			}
		}
	}

	void ResetVars()
	{
		for (int i = 0; i < 4; ++i)
		{
			vecDidTeams[i] = false;
		}
	}

	bool vecDidTeams[4];
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_4TeamBackstab, TF2C_ACHIEVEMENT_4TEAM_BACKSTAB, "TF2C_ACHIEVEMENT_4TEAM_BACKSTAB", 15);

/*
class CAchievementTF2C_TNTAirborneKill : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	// fired by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TNTAirborneKill, TF2C_ACHIEVEMENT_TNT_AIRBORNE_KILL, "TF2C_ACHIEVEMENT_TNT_AIRBORNE_KILL", 15);
*/

//----------------------------------------------------------------------------------------------------------------
// TF2C VIP Achievements 2.1.0
//----------------------------------------------------------------------------------------------------------------

class CAchievementTF2C_VIPProtectUnseen : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_VIP_PROTECT_UNSEEN_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("damage_blocked");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "damage_blocked"))
		{
			CBaseEntity* pAttacker = ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("attacker")));
			if (pAttacker && pLocalPlayer->GetUserID() == event->GetInt("provider") && pLocalPlayer->IsVIP())
			{
				// It's true that by the time event arrives player coordinates may not be correct to when damage blocked has taken place, but it's ok, it's a huge progression achievement
				// It doesn't ignore enemy teammates but I don't want to make it too complicated. Just as long as it doesn't pass through glass.
				trace_t tr;

				// Let's try to shoot into Civ's center
				UTIL_TraceLine(pAttacker->EyePosition(), pLocalPlayer->WorldSpaceCenter(), MASK_TFSHOT, pAttacker, COLLISION_GROUP_NONE, &tr);
				if ( tr.fraction < 1.0f )
				{
					// Now let's try to shoot into head so we aren't in sniper danger either
					UTIL_TraceLine(pAttacker->EyePosition(), pLocalPlayer->EyePosition(), MASK_TFSHOT, pAttacker, COLLISION_GROUP_NONE, &tr);
					if ( tr.fraction < 1.0f )
					{
						IncrementCount(event->GetInt("amount"));
						m_pAchievementMgr->SetDirty(true);
					}
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_VIPProtectUnseen, TF2C_ACHIEVEMENT_VIP_PROTECT_UNSEEN, "TF2C_ACHIEVEMENT_VIP_PROTECT_UNSEEN", 15);

/*
class CAchievementTF2C_VIPHealOthers : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_VIP_HEAL_OTHERS_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_healed");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_healed"))
		{

			if (pLocalPlayer->GetUserID() == event->GetInt("healer") && pLocalPlayer->GetUserID() != event->GetInt("patient") && \
				pLocalPlayer->IsVIP())
			{
				IncrementCount(event->GetInt("amount"));
				m_pAchievementMgr->SetDirty(true);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_VIPHealOthers, TF2C_ACHIEVEMENT_VIP_HEAL_OTHERS, "TF2C_ACHIEVEMENT_VIP_HEAL_OTHERS", 15);
*/

/*
class CAchievementTF2C_HealedByVIP : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_HEALED_BY_VIP_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_healed");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_healed"))
		{
			C_TFPlayer* pHealer = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("healer"))));
			if (!pHealer || !pHealer->IsVIP())
				return;

			if (pLocalPlayer != pHealer && pLocalPlayer->GetUserID() == event->GetInt("patient"))
			{
				IncrementCount(event->GetInt("amount"));
				m_pAchievementMgr->SetDirty(true);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HealedByVIP, TF2C_ACHIEVEMENT_HEALED_BY_VIP, "TF2C_ACHIEVEMENT_HEALED_BY_VIP", 15);
*/

class CAchievementTF2C_KillstreakWhileVIPBoosted : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
		SetCounterGoal(ACHIEVEMENT_TF2C_KILLSTREAK_WHILE_VIP_BOOSTED_REQUIREMENT);
	}

	void ListenForEvents()
	{
		CSingleLifeCounterAchievement::ListenForEvents();

		ListenForGameEvent("vip_boost");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		CSingleLifeCounterAchievement::FireGameEvent_Internal(event);

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (!Q_strcmp(event->GetName(), "vip_boost"))
		{
			C_TFPlayer* pTFProvider = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("provider"))));
			if (pTFProvider /* && !pTFProvider->IsEnemyPlayer() //teehee */ && pLocalPlayer->GetUserID() == event->GetInt("target"))
			{
				if (event->GetInt("condition") == TF_COND_DAMAGE_BOOST || event->GetInt("condition") == TF_COND_CIV_SPEEDBUFF)
					ResetCounter();
			}
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFVictim)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_DAMAGE_BOOST))
		{
			C_TFPlayer* pTFProvider = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_DAMAGE_BOOST));
			if (pTFProvider && pTFAttacker != pTFProvider /*&& !pTFProvider->IsEnemyPlayer() //teehee */
				&& pTFProvider->IsVIP())
			{
				IncrementSingeLifeCounter();
				return;
			}

		}
		if (pTFAttacker->m_Shared.InCond(TF_COND_CIV_SPEEDBUFF))
		{
			C_TFPlayer* pTFProvider = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_CIV_SPEEDBUFF));
			if (pTFProvider && pTFAttacker != pTFProvider /*&& !pTFProvider->IsEnemyPlayer() //teehee */
				&& pTFProvider->IsVIP())
			{
				IncrementSingeLifeCounter();
				return;
			}

		}
	}

};
DECLARE_ACHIEVEMENT(CAchievementTF2C_KillstreakWhileVIPBoosted, TF2C_ACHIEVEMENT_KILLSTREAK_WHILE_VIP_BOOSTED, "TF2C_ACHIEVEMENT_KILLSTREAK_WHILE_VIP_BOOSTED", 15);

// We assume that VIP-us can't boost more than one person at the same time, otherwise I would have to overengineer it further
// ok why did I set it to single life type before
class CAchievementTF2C_VIPTeammateKillstreakDamageBoost : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("vip_boost");
		iKilledSoFar = 0;
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (!Q_strcmp(event->GetName(), "vip_boost"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("provider") && pLocalPlayer->IsVIP() && event->GetInt("condition") == TF_COND_DAMAGE_BOOST)
			{
				iKilledSoFar = 0;
			}
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFVictim)
			return;

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;
		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (pLocalPlayer->IsVIP() && pTFAttacker->m_Shared.InCond(TF_COND_DAMAGE_BOOST)
			&& pLocalPlayer == pTFAttacker->m_Shared.GetConditionProvider(TF_COND_DAMAGE_BOOST)
			&& !pTFAttacker->IsEnemyPlayer())
		{
			++iKilledSoFar;
			if (iKilledSoFar >= ACHIEVEMENT_TF2C_VIP_TEAMMATE_KILLSTREAK_DAMAGE_BOOST_REQUIREMENT)
			{
				IncrementCount();
			}
		}
	}

	int iKilledSoFar;

};
DECLARE_ACHIEVEMENT(CAchievementTF2C_VIPTeammateKillstreakDamageBoost, TF2C_ACHIEVEMENT_VIP_TEAMMATE_KILLSTREAK_DAMAGE_BOOST, "TF2C_ACHIEVEMENT_VIP_TEAMMATE_KILLSTREAK_DAMAGE_BOOST", 15);

/*
class CAchievementTF2C_VIPTeammateProgressionHaste : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_VIP_TEAMMATE_PROGRESSION_HASTE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFVictim)
			return;

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;
		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (pLocalPlayer->IsVIP() && pTFAttacker->m_Shared.InCond(TF_COND_CIV_SPEEDBUFF)
			&& pLocalPlayer == pTFAttacker->m_Shared.GetConditionProvider(TF_COND_CIV_SPEEDBUFF)
			&& !pTFAttacker->IsEnemyPlayer())
		{
			if (!(pTFAttacker->m_Shared.InCond(TF_COND_RESISTANCE_BUFF) && pTFAttacker->m_Shared.GetConditionProvider(TF_COND_RESISTANCE_BUFF) == pLocalPlayer))
			{
				IncrementCount();
			}
		}
	}

};
DECLARE_ACHIEVEMENT(CAchievementTF2C_VIPTeammateProgressionHaste, TF2C_ACHIEVEMENT_VIP_TEAMMATE_PROGRESSION_HASTE, "TF2C_ACHIEVEMENT_VIP_TEAMMATE_PROGRESSION_HASTE", 15);
*/

/*
class CAchievementTF2C_TeammateKillstreakHasteBoost : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
		SetCounterGoal(ACHIEVEMENT_TF2C_TEAMMATE_KILLSTREAK_HASTE_BOOST_REQUIREMENT);
	}

	void ListenForEvents()
	{
		CSingleLifeCounterAchievement::ListenForEvents();

		ListenForGameEvent("vip_boost");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		CSingleLifeCounterAchievement::FireGameEvent_Internal(event);

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (!Q_strcmp(event->GetName(), "vip_boost"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("provider") && pLocalPlayer->IsVIP() && event->GetInt("condition") == TF_COND_CIV_SPEEDBUFF)
			{
				ResetCounter();
			}
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFVictim)
			return;

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;
		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (pLocalPlayer->IsVIP() && pTFAttacker->m_Shared.InCond(TF_COND_CIV_SPEEDBUFF)
			&& pLocalPlayer == pTFAttacker->m_Shared.GetConditionProvider(TF_COND_CIV_SPEEDBUFF)
			&& !pTFAttacker->IsEnemyPlayer())
		{
			IncrementSingeLifeCounter();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_TeammateKillstreakHasteBoost, TF2C_ACHIEVEMENT_TEAMMATE_KILLSTREAK_HASTE_BOOST, "TF2C_ACHIEVEMENT_TEAMMATE_KILLSTREAK_HASTE_BOOST", 15);
*/

class CAchievementTF2C_VIPKillCritKillstreak : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_ROUND);
		SetCounterGoal(ACHIEVEMENT_TF2C_VIP_KILL_CRIT_KILLSTREAK_REQUIREMENT);
	}

	void ListenForEvents()
	{
		CSingleLifeCounterAchievement::ListenForEvents();

		ListenForGameEvent("vip_death");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		CSingleLifeCounterAchievement::FireGameEvent_Internal(event);

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "vip_death"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("attacker"))
			{
				ResetCounter();
			}
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFVictim)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_ON_KILL))
		{
			C_TFPlayer* pTFProvider = ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_CRITBOOSTED_ON_KILL));
			if (pTFProvider && pTFProvider->IsVIP())
			{
				// don't count VIP kills due to sync issues, often counting VIP kill itself as the first kill online
				if (!pTFVictim->IsVIP())
				{
					IncrementSingeLifeCounter();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_VIPKillCritKillstreak, TF2C_ACHIEVEMENT_VIP_KILL_CRIT_KILLSTREAK, "TF2C_ACHIEVEMENT_VIP_KILL_CRIT_KILLSTREAK", 15);

class CAchievementTF2C_KillVIPDamagers : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_KILL_VIP_DAMAGERS_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
		ListenForGameEvent("vip_death");
		ListenForGameEvent("teamplay_round_active");
		ListenForGameEvent("localplayer_changeteam");

		vecVIPDamagers.Purge();
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "vip_death") || !Q_strcmp(event->GetName(), "teamplay_round_active")
			|| !Q_strcmp(event->GetName(), "localplayer_changeteam"))
		{
			vecVIPDamagers.Purge();
		}
		else if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			C_TFPlayer* pTFAttacker = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("attacker"))));
			C_TFPlayer* pTFVictim = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("userid"))));
			if (pTFAttacker && pTFVictim && !pTFVictim->IsEnemyPlayer() && pTFVictim->IsVIP())
			{
				VIPDamagerInfo tempVIPDamagerInfo;
				tempVIPDamagerInfo.pDamager = pTFAttacker;
				tempVIPDamagerInfo.flDamageTime = gpGlobals->curtime;
				vecVIPDamagers.AddToTail(tempVIPDamagerInfo);
				if (vecVIPDamagers.Size() > 50)
				{
					PurgeIllegalDamagers();
				}
			}
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->IsVIP())
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		FOR_EACH_VEC(vecVIPDamagers, i)
		{
			if (vecVIPDamagers[i].pDamager && vecVIPDamagers[i].pDamager == pVictim
				&& (gpGlobals->curtime - vecVIPDamagers[i].flDamageTime) <= ACHIEVEMENT_TF2C_KILL_VIP_DAMAGERS_LAST_DAMAGE_TIME)
			{
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
				return;
			}
		}
	}

	void PurgeIllegalDamagers()
	{
		FOR_EACH_VEC_BACK(vecVIPDamagers, i)
		{
			if (!vecVIPDamagers[i].pDamager)
			{
				vecVIPDamagers.Remove(i);
				continue;
			}
			if ((gpGlobals->curtime - vecVIPDamagers[i].flDamageTime) > ACHIEVEMENT_TF2C_KILL_VIP_DAMAGERS_LAST_DAMAGE_TIME)
			{
				vecVIPDamagers.Remove(i);
				continue;
			}
		}
	}

	struct VIPDamagerInfo
	{
		CHandle<C_TFPlayer> pDamager;
		float				flDamageTime;
	};

	CUtlVector<VIPDamagerInfo> vecVIPDamagers;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_KillVIPDamagers, TF2C_ACHIEVEMENT_KILL_VIP_DAMAGERS, "TF2C_ACHIEVEMENT_KILL_VIP_DAMAGERS", 15);



// unconfirmed if it works
class CAchievementTF2C_ProtectVIPAsMedic : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
		ListenForGameEvent("vip_death");
		ListenForGameEvent("teamplay_round_active");
		ListenForGameEvent("localplayer_changeteam");

		vecVIPDamagers.Purge();
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "vip_death") || !Q_strcmp(event->GetName(), "teamplay_round_active")
			|| !Q_strcmp(event->GetName(), "localplayer_changeteam"))
		{
			vecVIPDamagers.Purge();
		}
		else if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			C_TFPlayer* pTFAttacker = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("attacker"))));
			C_TFPlayer* pTFVictim = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("userid"))));
			if (pTFAttacker && pTFVictim && !pTFVictim->IsEnemyPlayer() && pTFVictim->IsVIP())
			{
				VIPDamagerInfo tempVIPDamagerInfo;
				tempVIPDamagerInfo.pDamager = pTFAttacker;
				tempVIPDamagerInfo.flDamageTime = gpGlobals->curtime;
				vecVIPDamagers.AddToTail(tempVIPDamagerInfo);
				if (vecVIPDamagers.Size() > 50)
				{
					PurgeIllegalDamagers();
				}
			}
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!pTFAttacker->IsPlayerClass(TF_CLASS_MEDIC))
			return;

		C_TFPlayer* pVIP = GetVIPPlayerOnTeam(pTFAttacker->GetTFTeam());
		if (!pVIP)
			return;

		FOR_EACH_VEC(vecVIPDamagers, i)
		{
			if (vecVIPDamagers[i].pDamager && vecVIPDamagers[i].pDamager == pVictim
				&& (gpGlobals->curtime - vecVIPDamagers[i].flDamageTime) <= ACHIEVEMENT_TF2C_PROTECT_VIP_AS_MEDIC_LAST_DAMAGE_TIME)
			{
				if (pVIP->GetAbsOrigin().DistToSqr(pVictim->GetAbsOrigin()) <= Square(ACHIEVEMENT_TF2C_PROTECT_VIP_AS_MEDIC_DISTANCE_SQ))
				{
					/*C_TFTeam* pTeam = pTFAttacker->GetTFTeam();
					for (int i = 0, c = pTeam->GetNumPlayers(); i < c; ++i)
					{
						C_TFPlayer* pPlayer = ToTFPlayer(pTeam->GetPlayer(i));
						if (!pPlayer)
							continue;

						if (pPlayer == pTFAttacker)
							continue;

						if (pPlayer->GetPlayerClass() &&
							IsAchievementProtectVIPAsMedicIsCombatClass(pPlayer->GetPlayerClass()->GetClassIndex()))
						{
							if (pVIP->GetAbsOrigin().DistToSqr(pPlayer->GetAbsOrigin()) <= Square(ACHIEVEMENT_TF2C_PROTECT_VIP_AS_MEDIC_DISTANCE_SQ))
								return;
						}
					}*/
					CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
					return;
				}
			}
		}
	}

	void PurgeIllegalDamagers()
	{
		FOR_EACH_VEC_BACK(vecVIPDamagers, i)
		{
			if (!vecVIPDamagers[i].pDamager)
			{
				vecVIPDamagers.Remove(i);
				continue;
			}
			if ((gpGlobals->curtime - vecVIPDamagers[i].flDamageTime) > ACHIEVEMENT_TF2C_PROTECT_VIP_AS_MEDIC_LAST_DAMAGE_TIME)
			{
				vecVIPDamagers.Remove(i);
				continue;
			}
		}
	}

	struct VIPDamagerInfo
	{
		CHandle<C_TFPlayer> pDamager;
		float				flDamageTime;
	};

	CUtlVector<VIPDamagerInfo> vecVIPDamagers;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_ProtectVIPAsMedic, TF2C_ACHIEVEMENT_PROTECT_VIP_AS_MEDIC, "TF2C_ACHIEVEMENT_PROTECT_VIP_AS_MEDIC", 15);

/*
class CAchievementTF2C_HealMilestone : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_HEAL_MILESTONE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_healed");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_healed"))
		{

			if (pLocalPlayer->GetUserID() == event->GetInt("healer") && pLocalPlayer->GetUserID() != event->GetInt("patient"))
			{
				IncrementCount(event->GetInt("amount"));
				m_pAchievementMgr->SetDirty(true);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HealMilestone, TF2C_ACHIEVEMENT_HEAL_MILESTONE, "TF2C_ACHIEVEMENT_HEAL_MILESTONE", 15);
*/

//-----------------------------------------------------------------------------
// Speedwatch and invis!
//-----------------------------------------------------------------------------

/*
class CAchievementTF2C_SpeedwatchCloakChallenge : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetCounterGoal(ACHIEVEMENT_TF2C_SPEEDWATCH_CLOAK_CHALLENGE_COUNTER);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		CSingleLifeCounterAchievement::ListenForEvents();

		ListenForGameEvent("player_cloaked");
		ListenForGameEvent("player_picked_ammo");
		ListenForGameEvent("player_picked_dropped_weapon");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		CSingleLifeCounterAchievement::FireGameEvent_Internal(event);

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_cloaked"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				ResetCounter();
			}
		}

		if (!Q_strcmp(event->GetName(), "player_picked_ammo") || !Q_strcmp(event->GetName(), "player_picked_dropped_weapon"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid") &&
				pLocalPlayer->m_Shared.InCond(TF_COND_STEALTHED))
			{
				C_TFWeaponBase* pWatch = pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PDA2);
				if (pWatch && pWatch->GetItemID() == 48)
				{
					IncrementSingeLifeCounter();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SpeedwatchCloakChallenge, TF2C_ACHIEVEMENT_SPEEDWATCH_CLOAK_CHALLENGE, "TF2C_ACHIEVEMENT_SPEEDWATCH_CLOAK_CHALLENGE", 15);
*/

class CAchievementTF2C_SpeedwatchRecovery : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_TRIVIAL);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_cloaked");
		ListenForGameEvent("player_hurt");
		ListenForGameEvent("player_picked_ammo");
		ListenForGameEvent("player_picked_dropped_weapon");

		bGotHitByEnemy = true;
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_cloaked"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				bGotHitByEnemy = false;
			}
		}

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				C_TFPlayer* pTFAttacker = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("attacker"))));
				if (pTFAttacker && pTFAttacker->IsEnemyPlayer() && pLocalPlayer->m_Shared.InCond(TF_COND_STEALTHED))
				{
					bGotHitByEnemy = true;
				}
			}
		}

		if (!Q_strcmp(event->GetName(), "player_picked_ammo") || !Q_strcmp(event->GetName(), "player_picked_dropped_weapon"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid") &&
				pLocalPlayer->m_Shared.InCond(TF_COND_STEALTHED) && bGotHitByEnemy == true)
			{
				C_TFWeaponBase* pWatch = pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PDA2);
				if (pWatch && pWatch->GetItemID() == 48)
				{
					IncrementCount();
				}
			}
		}
	}

	bool bGotHitByEnemy;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SpeedwatchRecovery, TF2C_ACHIEVEMENT_SPEEDWATCH_RECOVERY, "TF2C_ACHIEVEMENT_SPEEDWATCH_RECOVERY", 15);


class CAchievementTF2C_SpeedwatchSpeedKill : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}


	void ListenForEvents()
	{
		ListenForGameEvent("player_spawn");
		flSpawnTime = 0;
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		//if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
		//	return;

		if (!Q_strcmp(event->GetName(), "player_spawn") && pLocalPlayer->GetUserID() == event->GetInt("userid"))
		{
			flSpawnTime = gpGlobals->curtime;
		}
	}


	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (event->GetInt("customkill") == TF_DMG_CUSTOM_BACKSTAB && (gpGlobals->curtime <= flSpawnTime + ACHIEVEMENT_TF2C_SPEEDWATCH_SPEED_KILL_TIMER))
		{
			C_TFWeaponBase* pWatch = pTFAttacker->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PDA2);
			if (pWatch && pWatch->GetItemID() == 48)
			{
				IncrementCount();
			}
		}
	}

private:
	float flSpawnTime = 0.0f;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SpeedwatchSpeedKill, TF2C_ACHIEVEMENT_SPEEDWATCH_SPEED_KILL, "TF2C_ACHIEVEMENT_SPEEDWATCH_SPEED_KILL", 15);

class CAchievementTF2C_SpeedwatchCounter : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_TRIVIAL);
	}

	// Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SpeedwatchCounter, TF2C_ACHIEVEMENT_SPEEDWATCH_COUNTER, "TF2C_ACHIEVEMENT_SPEEDWATCH_COUNTER", 15);

class CAchievementTF2C_InvisRechargeCloaked : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_TRIVIAL);
	}

	// Called by server
	// Let's keep it simple for now
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_InvisRechargeCloaked, TF2C_ACHIEVEMENT_INVIS_RECHARGE_CLOAKED, "TF2C_ACHIEVEMENT_INVIS_RECHARGE_CLOAKED", 15);


//-----------------------------------------------------------------------------
// AAA Gun Gaming
//-----------------------------------------------------------------------------

/*
class CAchievementTF2C_AAGunDefence : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetCounterGoal(ACHIEVEMENT_TF2C_AA_GUN_DEFENCE_COUNTER);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		CSingleLifeCounterAchievement::ListenForEvents();

		ListenForGameEvent("heavy_windup");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		CSingleLifeCounterAchievement::FireGameEvent_Internal(event);

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "heavy_windup") && pLocalPlayer->GetUserID() == event->GetInt("userid"))
		{
			ResetCounter();
		}

	}


	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (!pVictim)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "aa_cannon"))
		{
			if (pTFAttacker->GetAbsOrigin().DistToSqr(pVictim->GetAbsOrigin()) >= Square(ACHIEVEMENT_TF2C_AA_GUN_DEFENCE_RANGE))
			{
				if (pTFAttacker->m_Shared.InCond(TF_COND_AIMING))
				{
					IncrementSingeLifeCounter();
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_AAGunDefence, TF2C_ACHIEVEMENT_AA_GUN_DEFENCE, "TF2C_ACHIEVEMENT_AA_GUN_DEFENCE", 15);
*/

class CAchievementTF2C_AAGunSplash : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_AA_GUN_SPLASH_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			if (event->GetInt("weaponid2") == TF_WEAPON_AAGUN && pLocalPlayer->GetUserID() == event->GetInt("attacker"))
			{
				C_TFPlayer* pTFInflictorEnemy = ToTFPlayer(ClientEntityList().GetEnt(event->GetInt("inflictor_enemy")));
				C_TFPlayer* pTFVictim = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("userid"))));
				if (pTFInflictorEnemy && pTFVictim && pTFVictim != pTFInflictorEnemy && pTFVictim != pLocalPlayer)
				{
					IncrementCount(event->GetInt("damageamount"));
				}
			}
		}

	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_AAGunSplash, TF2C_ACHIEVEMENT_AA_GUN_SPLASH, "TF2C_ACHIEVEMENT_AA_GUN_SPLASH", 15);


class CAchievementTF2C_AAGunChallenge : public CSingleLifeCounterAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetCounterGoal(ACHIEVEMENT_TF2C_AA_GUN_CHALLENGE_COUNTER);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
	}

	void ListenForEvents()
	{
		CSingleLifeCounterAchievement::ListenForEvents();

		ListenForGameEvent("player_hurt");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		CSingleLifeCounterAchievement::FireGameEvent_Internal(event);

		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			if (event->GetInt("weaponid2") == TF_WEAPON_AAGUN && pLocalPlayer->GetUserID() == event->GetInt("attacker"))
			{
				if (event->GetInt("userid") == pLocalPlayer->GetUserID())
				{
					ResetCounter();
				}
				else
				{
					IncrementSingeLifeCounter(event->GetInt("damageamount"));
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_AAGunChallenge, TF2C_ACHIEVEMENT_AA_GUN_CHALLENGE, "TF2C_ACHIEVEMENT_AA_GUN_CHALLENGE", 15);


class CAchievementTF2C_AAGunBlastSuicide : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}

	// Called by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_AAGunBlastSuicide, TF2C_ACHIEVEMENT_AA_GUN_BLAST_SUICIDE, "TF2C_ACHIEVEMENT_AA_GUN_BLAST_SUICIDE", 15);

/*
class CAchievementTF2C_MinigunVsHeavy : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_MINIGUN_VS_HEAVY_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFVictim)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "minigun") && pTFVictim->IsPlayerClass(TF_CLASS_HEAVYWEAPONS))
		{
			C_TFWeaponBase* pPrimary = pTFVictim->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PRIMARY);
			if (pPrimary && pPrimary->GetItemID() != 15)
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinigunVsHeavy, TF2C_ACHIEVEMENT_MINIGUN_VS_HEAVY, "TF2C_ACHIEVEMENT_MINIGUN_VS_HEAVY", 15);
*/

/*
class CAchievementTF2C_MinigunKillstreak : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("teamplay_round_active");
		ListenForGameEvent("player_spawn");
		ListenForGameEvent("player_death");

		vecMinigunKills.Purge();
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "teamplay_round_active") ||
			(!Q_strcmp(event->GetName(), "player_spawn") && pLocalPlayer->GetUserID() == event->GetInt("userid")) ||
			(!Q_strcmp(event->GetName(), "player_death") && pLocalPlayer->GetUserID() == event->GetInt("userid")))
		{
			vecMinigunKills.Purge();
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "minigun"))
		{
			vecMinigunKills.AddToTail(gpGlobals->curtime);
			PurgeIllegalKills();
			if (vecMinigunKills.Count() >= ACHIEVEMENT_TF2C_MINIGUN_KILLSTREAK_COUNTER)
			{
				CBaseAchievement::Event_EntityKilled(pVictim, pAttacker, pInflictor, event);
			}
		}
	}

	void PurgeIllegalKills()
	{
		FOR_EACH_VEC_BACK(vecMinigunKills, i)
		{
			if ((gpGlobals->curtime - vecMinigunKills[i]) > ACHIEVEMENT_TF2C_MINIGUN_KILLSTREAK_TIMER)
			{
				vecMinigunKills.Remove(i);
				continue;
			}
		}
	}

private:
	CUtlVector<float> vecMinigunKills;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinigunKillstreak, TF2C_ACHIEVEMENT_MINIGUN_KILLSTREAK, "TF2C_ACHIEVEMENT_MINIGUN_KILLSTREAK", 15);
*/

/*
class CAchievementTF2C_MinigunDamageStreak : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("teamplay_round_active");
		ListenForGameEvent("player_spawn");
		ListenForGameEvent("player_death");
		ListenForGameEvent("player_hurt");

		vecMinigunDamages.Purge();
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "teamplay_round_active") ||
			(!Q_strcmp(event->GetName(), "player_spawn") && pLocalPlayer->GetUserID() == event->GetInt("userid")) ||
			(!Q_strcmp(event->GetName(), "player_death") && pLocalPlayer->GetUserID() == event->GetInt("userid")))
		{
			vecMinigunDamages.Purge();
		}

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			if (event->GetInt("weaponid2") == TF_WEAPON_MINIGUN && pLocalPlayer->GetUserID() == event->GetInt("attacker"))
			{
				C_TFWeaponBase* pPrimaryWeapon = pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PRIMARY);
				if (pPrimaryWeapon && pPrimaryWeapon->GetItemID() == 15)
				{
					MinigunDamage newMinigunDamage;
					newMinigunDamage.flTime = gpGlobals->curtime;
					newMinigunDamage.iDamage = event->GetInt("damageamount");

					vecMinigunDamages.AddToTail(newMinigunDamage);
					PurgeIllegalDamages();

					if (GetTotalDamage() >= ACHIEVEMENT_TF2C_MINIGUN_DAMAGE_STREAK_AMOUNT)
					{
						IncrementCount();
					}
				}
			}
		}
	}

	void PurgeIllegalDamages()
	{
		FOR_EACH_VEC_BACK(vecMinigunDamages, i)
		{
			if ((gpGlobals->curtime - vecMinigunDamages[i].flTime) > ACHIEVEMENT_TF2C_MINIGUN_DAMAGE_STREAK_TIMER)
			{
				vecMinigunDamages.Remove(i);
				continue;
			}
		}
	}

	int GetTotalDamage()
	{
		int iTotal = 0;
		FOR_EACH_VEC(vecMinigunDamages, i)
		{
			iTotal += vecMinigunDamages[i].iDamage;
		}
		return iTotal;
	}

	struct MinigunDamage
	{
		int iDamage;
		float flTime;
	};

	CUtlVector<MinigunDamage> vecMinigunDamages;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinigunDamageStreak, TF2C_ACHIEVEMENT_MINIGUN_DAMAGE_STREAK, "TF2C_ACHIEVEMENT_MINIGUN_DAMAGE_STREAK", 15);
*/

class CAchievementTF2C_MinigunDuel : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_MINIGUN_DUEL_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	// awarded by server
	// need to know if the killed one was in aiming cond
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_MinigunDuel, TF2C_ACHIEVEMENT_MINIGUN_DUEL, "TF2C_ACHIEVEMENT_MINIGUN_DUEL", 15);



//-----------------------------------------------------------------------------
// Pyro SSG Achievements time
//-----------------------------------------------------------------------------

class CAchievementTF2C_SSGScoutHitPellets : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			if (event->GetInt("weaponid2") == TF_WEAPON_DOUBLESHOTGUN && pLocalPlayer->GetUserID() == event->GetInt("attacker")
				&& event->GetInt("multicount") >= ACHIEVEMENT_TF2C_SSG_SCOUT_HIT_PELLETS_MULTI_COUNT)
			{

				C_TFPlayer* pTFVictim = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("userid"))));
				if (pTFVictim && pTFVictim->IsPlayerClass(TF_CLASS_SCOUT))
				{
					IncrementCount();
				}
			}
		}

	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SSGScoutHitPellets, TF2C_ACHIEVEMENT_SSG_SCOUT_HIT_PELLETS, "TF2C_ACHIEVEMENT_SSG_SCOUT_HIT_PELLETS", 15);

class CAchievementTF2C_SSGMeatshots : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_SSG_MEATSHOTS_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetString("weapon"), "doubleshotgun") && event->GetInt("multicount") >= ACHIEVEMENT_TF2C_SSG_MEATSHOTS_MIN_MULTI_COUNT)
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SSGMeatshots, TF2C_ACHIEVEMENT_SSG_MEATSHOTS, "TF2C_ACHIEVEMENT_SSG_MEATSHOTS", 15);


class CAchievementTF2C_SSGPounce : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(ACHIEVEMENT_TF2C_SSG_POUNCE_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("teamplay_round_active");
		ListenForGameEvent("player_spawn");
		ListenForGameEvent("player_death");
		ListenForGameEvent("weapon_knockback_jump");

		flLastPounceTime = -ACHIEVEMENT_TF2C_SSG_POUNCE_TIMER;
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "teamplay_round_active") ||
			(!Q_strcmp(event->GetName(), "player_spawn") && pLocalPlayer->GetUserID() == event->GetInt("userid")) ||
			(!Q_strcmp(event->GetName(), "player_death") && pLocalPlayer->GetUserID() == event->GetInt("userid")))
		{
			flLastPounceTime = -ACHIEVEMENT_TF2C_SSG_POUNCE_TIMER;
		}

		if (!Q_strcmp(event->GetName(), "weapon_knockback_jump"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				C_TFWeaponBase* pSecondary = pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_SECONDARY);

				if (pSecondary && pSecondary->GetItemID() == 50)
				{
					flLastPounceTime = gpGlobals->curtime;
				}
			}
		}
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (gpGlobals->curtime - flLastPounceTime <= ACHIEVEMENT_TF2C_SSG_POUNCE_TIMER)
		{
			IncrementCount();
		}
	}
private:
	float flLastPounceTime;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_SSGPounce, TF2C_ACHIEVEMENT_SSG_POUNCE, "TF2C_ACHIEVEMENT_SSG_POUNCE", 15);

class CAchievementTF2C_PyroShotgunCoreshotStreak : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_PYRO_SHOTGUN_CORESHOT_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("teamplay_round_active");
		ListenForGameEvent("player_spawn");
		ListenForGameEvent("player_death");
		ListenForGameEvent("player_hurt");

		vecCoreshotHits.Purge();
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "teamplay_round_active") ||
			(!Q_strcmp(event->GetName(), "player_spawn") && pLocalPlayer->GetUserID() == event->GetInt("userid")) ||
			(!Q_strcmp(event->GetName(), "player_death") && pLocalPlayer->GetUserID() == event->GetInt("userid")))
		{
			vecCoreshotHits.Purge();
		}

		if (!Q_strcmp(event->GetName(), "player_hurt"))
		{
			if (event->GetInt("weaponid2") == TF_WEAPON_SHOTGUN_PYRO && pLocalPlayer->GetUserID() == event->GetInt("attacker")
				&& event->GetInt("multicount") >= ACHIEVEMENT_TF2C_PYRO_SHOTGUN_CORESHOT_STREAK_MIN_MULTICOUNT)
			{

				C_TFWeaponBase* pSecondary = pLocalPlayer->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_SECONDARY);

				if (pSecondary && pSecondary->GetItemID() == 12)
				{
					vecCoreshotHits.AddToTail(gpGlobals->curtime);
					PurgeIllegalShots();
					if (vecCoreshotHits.Count() >= ACHIEVEMENT_TF2C_PYRO_SHOTGUN_CORESHOT_STREAK_SHOTS)
					{
						IncrementCount();
						vecCoreshotHits.Purge();
					}
				}
			}
		}
	}

	void PurgeIllegalShots()
	{
		FOR_EACH_VEC_BACK(vecCoreshotHits, i)
		{
			if ((gpGlobals->curtime - vecCoreshotHits[i]) > ACHIEVEMENT_TF2C_PYRO_SHOTGUN_CORESHOT_STREAK_TIMER)
			{
				vecCoreshotHits.Remove(i);
				continue;
			}
		}
	}

private:
	CUtlVector<float> vecCoreshotHits;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_PyroShotgunCoreshotStreak, TF2C_ACHIEVEMENT_PYRO_SHOTGUN_CORESHOT_STREAK, "TF2C_ACHIEVEMENT_PYRO_SHOTGUN_CORESHOT_STREAK", 15);

/*
class CAchievementTF2C_PyroRaw : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_PYRO_RAW_REQUIREMENT);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	// handled by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_PyroRaw, TF2C_ACHIEVEMENT_PYRO_RAW, "TF2C_ACHIEVEMENT_PYRO_RAW", 15);
*/

class CAchievementTF2C_FlareCritTeamwork : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MISTAKE);
	}

	// handled by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_FlareCritTeamwork, TF2C_ACHIEVEMENT_FLARE_CRIT_TEAMWORK, "TF2C_ACHIEVEMENT_FLARE_CRIT_TEAMWORK", 15);

class CAchievementTF2C_Archaeology : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
	}

	virtual void ListenForEvents() OVERRIDE
	{
		ListenForGameEvent("archaeology");
	}

	virtual void FireGameEvent_Internal(IGameEvent* event) OVERRIDE
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if ( !Q_strcmp(event->GetName(), "archaeology") )
			if ( pLocalPlayer->GetUserID() == event->GetInt("userid") )
				AwardAchievement();
	}

	// handled by server
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_Archaeology, TF2C_ACHIEVEMENT_ARCHAEOLOGY, "TF2C_ACHIEVEMENT_ARCHAEOLOGY", 15);


//-----------------------------------------------------------------------------
// Purpose: see if a round win was a win for the local player with no enemy caps
//-----------------------------------------------------------------------------
bool CheckWinNoEnemyCaps( IGameEvent *event, int iRole )
{
	if ( !Q_strcmp( event->GetName(), "teamplay_round_win" ) )
	{
		if ( event->GetInt( "team" ) == GetLocalPlayerTeam() )
		{
			int iLosingTeamCaps = event->GetInt( "losing_team_num_caps" );
			if ( 0 == iLosingTeamCaps )
			{
				C_TFTeam *pLocalTeam = GetGlobalTFTeam( GetLocalPlayerTeam() );
				if ( pLocalTeam )
				{
					int iRolePlayer = pLocalTeam->GetRole();
					if ( iRole > TEAM_ROLE_NONE && ( iRolePlayer != iRole ) )
						return false;

					return true;
				}
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to determine if local player is specified class
//-----------------------------------------------------------------------------
bool IsLocalTFPlayerClass( int iClass )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	return( pLocalPlayer && pLocalPlayer->IsPlayerClass( iClass, true ) );
}


//-----------------------------------------------------------------------------
// Purpose: Helper function to find VIP player on player's team
//-----------------------------------------------------------------------------

C_TFPlayer* GetVIPPlayerOnTeam(C_TFTeam* pTeam)
{
	if (!pTeam)
		return nullptr;

	for (int i = 0, c = pTeam->GetNumPlayers(); i < c; ++i)
	{
		C_TFPlayer* pPlayer = ToTFPlayer(pTeam->GetPlayer(i));
		if (!pPlayer)
			continue;

		if (pPlayer->IsVIP())
			return pPlayer;
	}
	return nullptr;
}


bool IsAchievementProtectVIPAsMedicIsCombatClass(int iClass)
{
	return (iClass == TF_CLASS_SCOUT ||
		iClass == TF_CLASS_SOLDIER ||
		iClass == TF_CLASS_PYRO ||
		iClass == TF_CLASS_DEMOMAN ||
		iClass == TF_CLASS_HEAVYWEAPONS);
}

#endif // CLIENT_DLL