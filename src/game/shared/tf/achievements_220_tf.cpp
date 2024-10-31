#include "cbase.h"

#ifdef CLIENT_DLL

#include "achievements_220_tf.h"
#include "achievementmgr.h"
#include "baseachievement.h"
#include "tf_hud_statpanel.h"
#include "c_tf_team.h"
#include "c_tf_player.h"

//----------------------------------------------------------------------------------------------------------------
// Patch 2.1.0 Pack
//----------------------------------------------------------------------------------------------------------------
class CAchievementTF2C_KillEarthquake : public CBaseAchievement
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

		int iEvent = event->GetInt("customkill");
		if (iEvent == TF_DMG_EARTHQUAKE)
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_KillEarthquake, TF2C_ACHIEVEMENT_ANCHOR_EARTHQUAKE, "TF2C_ACHIEVEMENT_ANCHOR_EARTHQUAKE", 15);

class CAchievementTF2C_KillAnchorAirshotCrit : public CBaseAchievement
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

		int iEvent = event->GetInt("customkill");
		if ( iEvent == TF_DMG_ANCHOR_AIRSHOTCRIT )
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_KillAnchorAirshotCrit, TF2C_ACHIEVEMENT_ANCHOR_AIRSHOTCRIT, "TF2C_ACHIEVEMENT_ANCHOR_AIRSHOTCRIT", 15);

class CAchievementTF2C_KillHugeCyclopsCombo : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetType(ACHIEVEMENT_TYPE_MAJOR);
		m_iCount = 0;
		m_flTime = 0.0f;
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		int iEvent = event->GetInt("customkill");
		switch (iEvent)
		{
			case TF_DMG_CYCLOPS_COMBO_MIRV:
			case TF_DMG_CYCLOPS_COMBO_MIRV_BOMBLET:
			case TF_DMG_CYCLOPS_COMBO_STICKYBOMB:
			case TF_DMG_CYCLOPS_COMBO_PROXYMINE:
			{
				if (m_flTime < gpGlobals->curtime)
				{
					m_iCount = 0;
					m_flTime = gpGlobals->curtime + 0.1f;
				}

				m_iCount++;

				if (m_iCount >= 3)
				{
					AwardAchievement();
					m_iCount = 0;
				}

				return;
			}
		}

		m_iCount = 0;
	}

private:
	int		m_iCount;
	float	m_flTime;
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_KillHugeCyclopsCombo, TF2C_ACHIEVEMENT_CYCLOPS_HUGECOMBO, "TF2C_ACHIEVEMENT_CYCLOPS_HUGECOMBO", 15);

class CAchievementTF2C_AnchorNoRocket : public CBaseAchievement
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

		int iEvent = event->GetInt("customkill");
		if (iEvent == TF_DMG_EARTHQUAKE && !pTFAttacker->m_Shared.InCond(TF_COND_BLASTJUMPING))
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_AnchorNoRocket, TF2C_ACHIEVEMENT_ANCHOR_NO_ROCKET, "TF2C_ACHIEVEMENT_ANCHOR_NO_ROCKET", 15);

class CAchievementTF2C_BattleMedic : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS);
		SetGoal(5);
		SetType(ACHIEVEMENT_TYPE_MINOR);
	}

	virtual void Event_EntityKilled(CBaseEntity* pVictim, CBaseEntity* pAttacker, CBaseEntity* pInflictor, IGameEvent* event)
	{
		C_TFPlayer* pTFAttacker = ToTFPlayer(pAttacker);
		if (!pTFAttacker)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_INVULNERABLE) && pTFAttacker->m_Shared.GetConditionProvider(TF_COND_INVULNERABLE) && pTFAttacker == ToTFPlayer(pTFAttacker->m_Shared.GetConditionProvider(TF_COND_INVULNERABLE)))
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_BattleMedic, TF2C_ACHIEVEMENT_BATTLE_MEDIC, "TF2C_ACHIEVEMENT_BATTLE_MEDIC", 15);

class CAchievementTF2C_CaneCollectBoost : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS);

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

	void ListenForEvents()
	{
		ListenForGameEvent("vip_boost");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "vip_boost"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("provider") && pLocalPlayer->IsVIP() && event->GetInt("condition") == TF_COND_CIV_SPEEDBUFF)
			{
				C_TFPlayer* pTFTarget = ToTFPlayer(ClientEntityList().GetEnt(engine->GetPlayerForUserID(event->GetInt("target"))));
				if (!pTFTarget)
					return;

				C_TFPlayerClass* pPlayerClass = pTFTarget->GetPlayerClass();
				if (pPlayerClass)
				{
					OnComponentEvent(g_aRawPlayerClassNamesShort[pPlayerClass->GetClassIndex()]); // maybe this is going too far but whatever
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_CaneCollectBoost, TF2C_ACHIEVEMENT_CANE_COLLECT_BOOST, "TF2C_ACHIEVEMENT_CANE_COLLECT_BOOST", 15);

class CAchievementTF2C_HealMilestoneNader : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(ACHIEVEMENT_TF2C_HEAL_MILESTONE_REQUIREMENT_NADER);
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

		if (!pLocalPlayer->GetActiveTFWeapon() || !pLocalPlayer->GetActiveTFWeapon()->GetWeaponID())
			return;

		if (!Q_strcmp(event->GetName(), "player_healed"))
		{

			if (pLocalPlayer->GetUserID() == event->GetInt("healer") && pLocalPlayer->GetUserID() != event->GetInt("patient") && pLocalPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_HEALLAUNCHER)
			{
				IncrementCount(event->GetInt("amount"));
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_HealMilestoneNader, TF2C_ACHIEVEMENT_HEAL_MILESTONE_NADER, "TF2C_ACHIEVEMENT_HEAL_MILESTONE_NADER", 15);

class CAchievementTF2C_ExtendUber : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_chargeextended");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_chargeextended"))
		{

			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_ExtendUber, TF2C_ACHIEVEMENT_EXTEND_UBER, "TF2C_ACHIEVEMENT_EXTEND_UBER", 15);

class CAchievementTF2C_UberMany : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("player_ubermany");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "player_ubermany"))
		{

			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_UberMany, TF2C_ACHIEVEMENT_UBER_MANY, "TF2C_ACHIEVEMENT_UBER_MANY", 15);

class CAchievementTF2CKillSentryWithNailgun : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(5);
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
				if (event->GetInt("weaponid") == TF_WEAPON_NAILGUN && event->GetInt("objecttype") == OBJ_SENTRYGUN)
					
				{
					IncrementCount();
					m_pAchievementMgr->SetDirty(true);
				}
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2CKillSentryWithNailgun, TF2C_ACHIEVEMENT_KILL_SENTRY_WITH_NAILGUN, "TF2C_ACHIEVEMENT_KILL_SENTRY_WITH_NAILGUN", 15);

class CAchievementTF2CDestroyProjectileCyclops : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(60);
		SetType(ACHIEVEMENT_TYPE_PLAYER);
	}

	void ListenForEvents()
	{
		ListenForGameEvent("cyclops_destroy_proj");
	}

	void FireGameEvent_Internal(IGameEvent* event)
	{
		C_TFPlayer* pLocalPlayer = ToTFPlayer(C_BasePlayer::GetLocalPlayer());
		if (!pLocalPlayer)
			return;

		if (pLocalPlayer->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		if (!Q_strcmp(event->GetName(), "cyclops_destroy_proj"))
		{
			if (pLocalPlayer->GetUserID() == event->GetInt("userid"))
			{
				IncrementCount();
				m_pAchievementMgr->SetDirty(true);
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2CDestroyProjectileCyclops, TF2C_ACHIEVEMENT_DESTROY_PROJ_CYCLOPS, "TF2C_ACHIEVEMENT_DESTROY_PROJ_CYCLOPS", 15);

class CAchievementTF2C_CyclopsAirborne : public CBaseAchievement
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
		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFAttacker || !pTFVictim)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		int iEvent = event->GetInt("customkill");
		if (iEvent == TF_DMG_CYCLOPS_DELAYED && pTFVictim->m_Shared.InCond(TF_COND_BLASTJUMPING))
		{
			IncrementCount();
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_CyclopsAirborne, TF2C_ACHIEVEMENT_CYCLOPS_AIRBORNE, "TF2C_ACHIEVEMENT_CYCLOPS_AIRBORNE", 15);

class CAchievementTF2C_CyclopsBlind : public CBaseAchievement
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
		C_TFPlayer* pTFVictim = ToTFPlayer(pVictim);
		if (!pTFAttacker || !pTFVictim)
			return;

		if (pTFAttacker->m_Shared.InCond(TF_COND_CRITBOOSTED_BONUS_TIME))
			return;

		int iEvent = event->GetInt("customkill");

		if (iEvent == TF_DMG_CYCLOPS_DELAYED) {
			trace_t tr;
			UTIL_TraceLine(pAttacker->WorldSpaceCenter(), pVictim->WorldSpaceCenter(), MASK_VISIBLE, NULL, COLLISION_GROUP_NONE, &tr);
			if (tr.fraction != 1.0f)
			{
				IncrementCount();
			}
		}
	}
};
DECLARE_ACHIEVEMENT(CAchievementTF2C_CyclopsBlind, TF2C_ACHIEVEMENT_CYCLOPS_BLIND, "TF2C_ACHIEVEMENT_CYCLOPS_BLIND", 15);

#endif