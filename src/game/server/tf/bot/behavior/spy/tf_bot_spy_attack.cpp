/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_spy_attack.h"
#include "tf_bot_spy_sap.h"
#include "tf_bot_retreat_to_cover.h"
#include "tf_gamerules.h"
#include "tf_weapon_knife.h"


static ConVar tf_bot_spy_knife_range                  ("tf_bot_spy_knife_range",                   "300", FCVAR_CHEAT, "If threat is closer than this, prefer our knife");
static ConVar tf_bot_spy_change_target_range_threshold("tf_bot_spy_change_target_range_threshold", "300", FCVAR_CHEAT);


ActionResult<CTFBot> CTFBotSpyAttack::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ChasePath.Initialize(actor);
	
	this->m_bUseRevolver = false;
	
	// TODO: figure out whether this is an overreach of magical vision powers
	if (this->m_hVictim != nullptr) {
		actor->GetVisionInterface()->AddKnownEntity(this->m_hVictim);
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotSpyAttack::Update(CTFBot *actor, float dt)
{
	const CKnownEntity *k_victim = actor->GetVisionInterface()->GetKnown(this->m_hVictim);

	if ( k_victim == nullptr || k_victim->IsObsolete() ) {
		Done( "No threat" );
	}

	CBaseEntity *pVictimEnt = k_victim->GetEntity();

	const CKnownEntity *k_threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	CBaseEntity *pThreatEnt = k_threat ? k_threat->GetEntity() : nullptr;
	
	if (k_victim != nullptr) {
		// BUG: ptr comparison of k_victim to k_threat isn't reasonable (compare ->GetEntity()'s instead)
		if ( k_threat != nullptr && pThreatEnt != nullptr && pVictimEnt != nullptr && pVictimEnt != pThreatEnt ) {
			float range_victim = actor->GetRangeTo(k_victim->GetLastKnownPosition());
			float range_threat = actor->GetRangeTo(k_threat->GetLastKnownPosition());
			
			if ( (range_victim - range_threat) > tf_bot_spy_change_target_range_threshold.GetFloat() && pThreatEnt->IsPlayer() ) {
				this->m_hVictim = ToTFBot( pThreatEnt );
				k_victim = k_threat;
				
				this->m_bUseRevolver = false;
			}
		}
	} else {
		this->m_bUseRevolver = false;
		
		if (k_threat != nullptr) {
			this->m_hVictim = ToTFPlayer( pThreatEnt );
		} else {
			this->m_hVictim = nullptr;
		}
		
		k_victim = k_threat;
	}

	
	CBaseObject *obj = actor->GetNearestKnownSappableTarget();
	if (obj != nullptr && actor->IsEntityBetweenTargetAndSelf( obj, pVictimEnt ) ) {
		ChangeTo(new CTFBotSpySap(obj), "Opportunistically sapping an enemy object between my victim and me");
	}
	
	if (actor->IsAnyEnemySentryAbleToAttackMe()) {
		this->m_bUseRevolver = true;
		actor->SwitchToPrimary();
		ChangeTo(new CTFBotRetreatToCover(), "Escaping sentry fire!");
	}
	
	CTFPlayer *victim = ToTFPlayer( pVictimEnt );
	if (victim == nullptr) {
		// Check if victim is a building (i.e. Sentry)
		CBaseObject *pObjThreat = static_cast<CBaseObject *>( pVictimEnt );
		if ( pObjThreat )
		{
			ChangeTo( new CTFBotSpySap( pObjThreat ), "Sapping building threatening me" );
		}

		Done("Current 'threat' is not a player or building?");
	}
	
	if (actor->m_Shared.IsStealthed() && this->m_ctRecloak.IsElapsed()) {
		actor->PressAltFireButton();
		this->m_ctRecloak.Start(1.0f);
	}
	
	bool use_knife = false;
	if (actor->m_Shared.InCond(TF_COND_DISGUISED) || actor->m_Shared.InCond(TF_COND_DISGUISING) || actor->m_Shared.IsStealthed()) {
		use_knife = true;
	}
	
	Vector delta = (victim->GetAbsOrigin() - actor->GetAbsOrigin());
	float range = VectorNormalize(delta);
	
	float dot_threshold = 0.0000f;
	switch (actor->GetSkill()) {
	case CTFBot::EASY:   dot_threshold = 0.9000f; break; // +/- ~25.8 degrees
	case CTFBot::NORMAL: dot_threshold = 0.7071f; break; // +/-  45.0 degrees
	case CTFBot::HARD:   dot_threshold = 0.2000f; break; // +/- ~78.5 degrees
	case CTFBot::EXPERT: dot_threshold = 0.0000f; break; // +/-  90.0 degrees
	default:
		Assert(false);
	}
	
	/* MvM: effectively override backstab ability to normal skill level */
	if (TFGameRules()->IsMannVsMachineMode()) {
		dot_threshold = 0.7071f;
	}
	
	bool go_for_a_backstab = true;
	if (actor->GetSkill() != CTFBot::EASY) {
		go_for_a_backstab = (delta.Dot(EyeVectorsFwd(victim)) > dot_threshold);
	}
	
	if (range < tf_bot_spy_knife_range.GetFloat() || (k_victim->IsVisibleInFOVNow() && go_for_a_backstab)) {
		use_knife = true;
	}
	
	if (actor->IsThreatAimingTowardMe(victim, 0.99f) && actor->GetTimeSinceLastInjuryByAnyEnemyTeam() < 1.0f && victim->GetTimeSinceWeaponFired() < 0.25f) {
		this->m_bUseRevolver = true;
		ChangeTo( new CTFBotRetreatToCover(), "Abort backstab attempt - victim shooting at me" );
	}

	auto *pGun = static_cast<CTFWeaponBaseGun *>( actor->GetTFWeapon_Primary() );
	if ( pGun && victim->GetHealth() <= pGun->GetProjectileDamage() )
	{
		this->m_bUseRevolver = true;
	}
	
	if (this->m_bUseRevolver || actor->AmInCloakRevealingCondition() || !use_knife) {
		actor->SwitchToPrimary();
	} else {
		actor->SwitchToMelee();
	}
	
	CTFWeaponBase *weapon = actor->m_Shared.GetActiveTFWeapon();
	if (weapon != nullptr && weapon->IsMeleeWeapon()) {

		auto *pKnife = static_cast<CTFKnife *>( actor->GetTFWeapon_Melee() );
		if ( pKnife && pKnife->IsReadyToBackstab() )
		{
			actor->PressFireButton();
		}

		if (k_victim->IsVisibleInFOVNow()) {
			bool dont_continue = true;
			if (range < 250.0f) {
				actor->GetBodyInterface()->AimHeadTowards(victim, IBody::PRI_OVERRIDE, 0.1f, nullptr, "Aiming my stab!");
				
				if (go_for_a_backstab) {
					if (TFGameRules()->IsMannVsMachineMode() && this->m_ctMvMChuckle.IsElapsed()) {
						this->m_ctMvMChuckle.Start(1.0f);
						
						actor->EmitSound("Spy.MVM_Chuckle");
					}
				} else {
					Vector  actor_eyefwd = EyeVectorsFwd(actor);
					Vector victim_eyefwd = EyeVectorsFwd(victim);
					
					if ((victim_eyefwd.x * actor_eyefwd.y) - (actor_eyefwd.x * victim_eyefwd.y) < 0.0f) {
						actor->PressRightButton();
					} else {
						actor->PressLeftButton();
					}
					
					dont_continue = (range >= 100.0f);
				}
			}
			
			if (actor->GetDesiredAttackRange() > range && (!actor->m_Shared.InCond(TF_COND_DISGUISED) || go_for_a_backstab || this->m_bUseRevolver)) {
				actor->PressFireButton();
			}
			
			if (!dont_continue) {
				Continue();
			}
		}
	} else {
		actor->GetBodyInterface()->AimHeadTowards(victim, IBody::PRI_OVERRIDE, 0.1f, nullptr, "Aiming my pistol");
	}
	
	if (!k_victim->IsVisibleRecently() || actor->IsRangeGreaterThan(victim->GetAbsOrigin(), actor->GetDesiredAttackRange()) || !actor->IsLineOfFireClear(victim->EyePosition())) {
		if (!k_victim->IsVisibleRecently() && actor->IsRangeLessThan(k_victim->GetLastKnownPosition(), 20.0f)) {
			actor->GetVisionInterface()->ForgetEntity(victim);
			
			Done("I lost my target!");
		}
		
		this->m_ChasePath.Update(actor, victim, CTFBotPathCost(actor, FASTEST_ROUTE));
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotSpyAttack::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ChasePath.Invalidate();
	
	this->m_hVictim = nullptr;
	
	this->m_bUseRevolver = false;
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyAttack::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	if (actor->IsEnemy(ent)) {
		CBaseCombatCharacter *bcc = ToBaseCombatCharacter(ent);
		if (bcc != nullptr && bcc->IsLookingTowards(actor, 0.9f)) {
			this->m_bUseRevolver = true;
		}
	}
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotSpyAttack::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)
{
	if (actor->IsEnemy(info.GetAttacker()) && !actor->m_Shared.InCond(TF_COND_DISGUISED)) {
		this->m_bUseRevolver = true;
		actor->SwitchToPrimary();
		ChangeTo(new CTFBotRetreatToCover(), "Time to get out of here!", SEV_MEDIUM);
	}
	
	Continue();
}


ThreeState_t CTFBotSpyAttack::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	if (!this->m_bUseRevolver && !actor->AmInCloakRevealingCondition()) {
		return TRS_FALSE;
	}
	
	return TRS_TRUE;
}

ThreeState_t CTFBotSpyAttack::IsHindrance(const INextBot *nextbot, CBaseEntity *it) const
{
	if (it != nullptr && this->m_hVictim != nullptr && it->entindex() == this->m_hVictim->entindex()) {
		return TRS_FALSE;
	}
	
	return TRS_NONE;
}

const CKnownEntity *CTFBotSpyAttack::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	if (!actor->IsSelf(them)) {
		return nullptr;
	}
	
	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if (weapon == nullptr || !weapon->IsMeleeWeapon()) {
		return nullptr;
	}
	
	float dist1 = actor->GetRangeSquaredTo(threat1->GetEntity());
	float dist2 = actor->GetRangeSquaredTo(threat2->GetEntity());
	
	if (dist1 < dist2) {
		return threat1;
	} else {
		return threat2;
	}
}
