/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_medic_heal.h"
#include "tf_bot_medic_retreat.h"
#include "tf_bot_use_teleporter.h"
#include "scenario/capture_the_flag/tf_bot_fetch_flag.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "tf_obj_teleporter.h"


static ConVar tf_bot_medic_stop_follow_range      ("tf_bot_medic_stop_follow_range",         "75", FCVAR_CHEAT);
static ConVar tf_bot_medic_start_follow_range     ("tf_bot_medic_start_follow_range",       "250", FCVAR_CHEAT);
static ConVar tf_bot_medic_max_heal_range         ("tf_bot_medic_max_heal_range",           "600", FCVAR_CHEAT);
static ConVar tf_bot_medic_debug                  ("tf_bot_medic_debug",                      "0", FCVAR_CHEAT);
static ConVar tf_bot_medic_max_call_response_range("tf_bot_medic_max_call_response_range", "1000", FCVAR_CHEAT);
static ConVar tf_bot_medic_cover_test_resolution  ("tf_bot_medic_cover_test_resolution",      "8", FCVAR_CHEAT);


class CSelectPrimaryPatient : public IVision::IForEachKnownEntity
{
public:
	CSelectPrimaryPatient(CTFBot *medic, CTFPlayer *patient) :
		m_pMedic(medic), m_pMedigun(dynamic_cast<CWeaponMedigun *>(medic->GetActiveTFWeapon())), m_pPatient(patient) {}
	
	virtual bool Inspect(const CKnownEntity& known) override
	{
		CBaseEntity *ent = known.GetEntity();
		if (ent == nullptr)                 return true;
		if (!ent->IsPlayer())               return true;
		if (!ent->IsAlive())                return true;
		if (!this->m_pMedic->IsFriend(ent)) return true; // <--- TODO: fix implications of this for disguised spies!
		
		CTFPlayer *player = ToTFPlayer(ent);
		if (player == nullptr)              return true;
		if (this->m_pMedic->IsSelf(player)) return true;
		
		bool should_consider_player;
		if (player->HasTheFlag() || this->m_pMedic->IsInASquad()) {
			should_consider_player = true;
		} else {
			// TODO: handle new TF2C classes
			switch (player->GetPlayerClass()->GetClassIndex()) {
			case TF_CLASS_SCOUT:        should_consider_player = true;  break;
			case TF_CLASS_SNIPER:       should_consider_player = false; break;
			case TF_CLASS_SOLDIER:      should_consider_player = true;  break;
			case TF_CLASS_DEMOMAN:      should_consider_player = true;  break;
			case TF_CLASS_MEDIC:        should_consider_player = false; break;
			case TF_CLASS_HEAVYWEAPONS: should_consider_player = true;  break;
			case TF_CLASS_PYRO:         should_consider_player = true;  break;
			case TF_CLASS_SPY:          should_consider_player = false; break;
			case TF_CLASS_ENGINEER:     should_consider_player = false; break;
			case TF_CLASS_CIVILIAN:     should_consider_player = true;  break;
			default: Assert(false);     should_consider_player = true;  break;
			}
		}
		
		if (should_consider_player) {
			this->m_pPatient = this->SelectPreferred(this->m_pPatient, player);
		}
		
		return true;
	}
	
	CTFPlayer *SelectPreferred(CTFPlayer *p_old, CTFPlayer *p_new)
	{
		/* always prefer to heal humans during training mode */
		if (TFGameRules()->IsInTraining()) {
			if (p_old != nullptr && !p_old->IsBot()) {
				return p_old;
			} else {
				return p_new;
			}
		}
		
		if (p_old == nullptr) return p_new;
		if (p_new == nullptr) return p_old;
		
		/* always prefer to heal the squad leader when in a squad */
		if (this->m_pMedic->IsInASquad()) {
			CTFBot *squad_leader = this->m_pMedic->GetSquad()->GetLeader();
			if (squad_leader != nullptr) {
				if (p_old->entindex() == squad_leader->entindex()) return p_old;
				if (p_new->entindex() == squad_leader->entindex()) return p_new;
			}
		}
		
		/* if the old patient has another healer besides this medic, then spread
		 * the heals around to the new patient instead */
		for (int i = 0; i < p_old->m_Shared.GetNumHealers(); ++i) {
			CTFPlayer *healer = ToTFPlayer(p_old->m_Shared.GetHealerByIndex(i));
			if (healer == nullptr) continue;
			
			if (!this->m_pMedic->IsSelf(healer)) {
				return p_new;
			}
		}
		
		/* if the new patient has another healer besides this medic, then spread
		 * the heals around to the old patient instead */
		for (int i = 0; i < p_new->m_Shared.GetNumHealers(); ++i) {
			CTFPlayer *healer = ToTFPlayer(p_new->m_Shared.GetHealerByIndex(i));
			if (healer == nullptr) continue;
			
			if (!this->m_pMedic->IsSelf(healer)) {
				return p_old;
			}
		}
		
		// NOTE: live TF2 does some NavAreaTravelDistance calculations for each
		// player if they meet the medic-call criteria, but then it just
		// discards the results of those expensive calculations
		
		auto l_called_for_medic = [=](CTFPlayer *player){
			if (player->IsBot())                               return false;
			if (!player->GetMedicCallTimer().HasStarted())     return false;
			if (!player->GetMedicCallTimer().IsLessThan(5.0f)) return false;
			
			return this->m_pMedic->IsRangeLessThan(player, tf_bot_medic_max_call_response_range.GetFloat());
		};
		
		bool call_old = l_called_for_medic(p_old);
		bool call_new = l_called_for_medic(p_new);
		
		/* prefer players who have recently called for a medic */
		if (call_old && call_new) {
			float t_old = p_old->GetMedicCallTimer().GetElapsedTime();
			float t_new = p_new->GetMedicCallTimer().GetElapsedTime();
			
			/* if both players called, then prefer the more recent call */
			if (t_old < t_new) {
				return p_old;
			} else {
				return p_new;
			}
		} else if (call_old) {
			return p_old;
		} else if (call_new) {
			return p_new;
		}
		
		// TODO: possibly rework this?
		// (for now, I just shoved civilian into the top slot)
		// TODO: make this a constexpr array
		// TODO: use range-based for loop and get rid of the TF_CLASS_UNDEFINED terminator
		static int preferredClass[] = {
			TF_CLASS_CIVILIAN,
			TF_CLASS_HEAVYWEAPONS, // 0
			TF_CLASS_SOLDIER,      // 1
			TF_CLASS_PYRO,         // 2
			TF_CLASS_DEMOMAN,      // 3
			
			TF_CLASS_UNDEFINED,
		};
		
		// TODO: get rid of this 999 idiocy
		int pref_old = 999;
		int pref_new = 999;
		
		for (int i = 0; preferredClass[i] != TF_CLASS_UNDEFINED; ++i) {
			if (p_old->GetPlayerClass()->GetClassIndex() == preferredClass[i]) {
				// TODO: wtf is this?
			//	pref_old = (i > 2 ? i : 0);
				pref_old = (i > 3 ? i : 0);
			}
			
			if (p_new->GetPlayerClass()->GetClassIndex() == preferredClass[i]) {
				// TODO: wtf is this?
			//	pref_new = (i > 2 ? i : 0);
				pref_new = (i > 3 ? i : 0);
			}
		}
		
		/* equal class preferences: 300 HU distance-based hysteresis */
		if (pref_old == pref_new) {
			float dist_old = this->m_pMedic->GetDistanceBetween(p_old);
			float dist_new = this->m_pMedic->GetDistanceBetween(p_new);
			
			if (dist_old > dist_new + 300.0f) {
				return p_new;
			} else {
				return p_old;
			}
		}
		
		/* new patient class preferred: only switch if closer than 750 HU */
		if (pref_old > pref_new) {
			float dist_new = this->m_pMedic->GetDistanceBetween(p_new);
			
			// TODO: magic number 750.0f
			if (dist_new < 750.0f) {
				return p_new;
			} else {
				return p_old;
			}
		}
		
		/* old patient class preferred: always stay with old patient */
		return p_old;
	}
	
	CTFPlayer *GetResult() const { return this->m_pPatient; }
	
private:
	CTFBot *m_pMedic;           // +0x04
	CWeaponMedigun *m_pMedigun; // +0x08
	CTFPlayer *m_pPatient;      // +0x0c
};
// TODO: remove offsets


class CFindMostInjuredNeighbor : public IVision::IForEachKnownEntity
{
public:
	CFindMostInjuredNeighbor(CTFBot *medic, CWeaponMedigun *medigun, bool use_non_buffed_maxhealth) :
		m_pMedic(medic),
		m_flRangeLimit(0.9f * medigun->GetTargetRange()),
		m_bUseNonBuffedMaxHealth(use_non_buffed_maxhealth) {}
	
	virtual bool Inspect(const CKnownEntity& known) override
	{
		CTFPlayer *player = ToTFPlayer(known.GetEntity());
		if (player == nullptr) return true;
		
		if ( this->m_pMedic->IsRangeGreaterThan(player, this->m_flRangeLimit)) return true;
		if (!this->m_pMedic->IsLineOfFireClear(player->EyePosition()))         return true;
		if ( this->m_pMedic->IsSelf(player))                                   return true;
		if (!player->IsAlive())                                                return true;
		if (!this->m_pMedic->IsFriend(player))                                 return true;
		// ^^^ TODO: need to check whether player is an APPARENT friend (spy disguises)
		
		float health_ratio;
		if (this->m_bUseNonBuffedMaxHealth) {
			health_ratio = player->HealthFraction();
		} else {
			health_ratio = player->HealthFractionBuffed();
		}
		
		if (this->m_bIsOnFire) {
			if (!player->m_Shared.InCond(TF_COND_BURNING)) {
				return true;
			}
		} else {
			if (player->m_Shared.InCond(TF_COND_BURNING)) {
				this->m_pMostInjured  = player;
				this->m_flHealthRatio = health_ratio;
				this->m_bIsOnFire     = true;
				
				return true;
			}
		}
		
		if (health_ratio < this->m_flHealthRatio) {
			this->m_pMostInjured  = player;
			this->m_flHealthRatio = health_ratio;
		}
		
		return true;
	}
	
	CTFPlayer *GetMostInjured() const { return this->m_pMostInjured; }
	float GetHealthRatio() const      { return this->m_flHealthRatio; }
	bool IsOnFire() const             { return this->m_bIsOnFire; }
	
private:
	CTFBot *m_pMedic;                    // +0x04
	CTFPlayer *m_pMostInjured = nullptr; // +0x08
	float m_flHealthRatio = 1.0f;        // +0x0c
	bool m_bIsOnFire = false;            // +0x10
	float m_flRangeLimit;                // +0x14
	bool m_bUseNonBuffedMaxHealth;       // +0x18
};
// TODO: remove offsets


ActionResult<CTFBot> CTFBotMedicHeal::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Secondary() != nullptr && typeid(actor->GetTFWeapon_Secondary()) == typeid(CWeaponMedigun));
	
	this->m_ChasePath   .Initialize(actor);
	this->m_PathFollower.Initialize(actor);
	
	this->m_hPatient = nullptr;
	this->m_vecPatientPosition = vec3_origin;
	
	// TODO: DWORD @ 0x4868 = 0x00000000
	
	this->m_ct4834     .Invalidate();
	this->m_ctUberDelay.Invalidate();
	
	this->m_ct485c.Invalidate();
	this->m_ct486c.Invalidate();
	
	Continue();
}

ActionResult<CTFBot> CTFBotMedicHeal::Update(CTFBot *actor, float dt)
{
	if (actor->IsInASquad()) {
		// TODO: MvM stuff
		Assert(false);
	} else {
		actor->ClearMission();
	}
	
	CTFPlayer *patient = this->SelectPatient(actor, this->m_hPatient);
	this->m_hPatient = patient;
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		// TODO: MvM stuff
		Assert(false);
	}
	
	if (patient == nullptr) {
		if (TFGameRules()->IsMannVsMachineMode()) {
			ChangeTo(new CTFBotFetchFlag(), "Everyone is gone! Going for the flag");
		} else if (TFGameRules()->IsPVEModeActive()) {
			Continue();
		} else {
			SuspendFor(new CTFBotMedicRetreat(), "Retreating to find another patient to heal");
		}
	}
	
	if (this->m_vecPatientPosition.DistToSqr(patient->GetAbsOrigin()) > Square(200.0f)) {
		this->m_vecPatientPosition = patient->GetAbsOrigin();
		this->m_ct485c.Start(3.0f);
	}
	
	if (patient->m_Shared.InCond(TF_COND_SELECTED_TO_TELEPORT)) {
		auto tele = dynamic_cast<CObjectTeleporter *>( patient->GetGroundEntity() );

		if ( tele != nullptr )
		{
			SuspendFor( new CTFBotUseTeleporter( tele, CTFBotUseTeleporter::HOW_MEDIC ), "Following my patient through a teleporter" );
		}
	}
	
	CTFPlayer *patient_alt = patient;

	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	
	// TODO: names
	bool bool1 = true;
	bool bool2 = false;
	
	auto medigun = dynamic_cast<CWeaponMedigun *>(actor->m_Shared.GetActiveTFWeapon());
	if (medigun != nullptr) {
		// can't do this sanity check, since enum value TF_MEDIGUN_VACCINATOR is completely commented out right now
	//	if (medigun->GetMedigunType() == TF_MEDIGUN_VACCINATOR) {
	//		// TODO
	//		Assert(false);
	//	}
		
		if (!medigun->IsReleasingCharge() && this->IsStable(patient) && !TFGameRules()->IsInTraining() && !actor->IsInASquad()) {
			bool patient_fired_recently = (patient_alt->GetTimeSinceWeaponFired() < 1.0f);
			
			CFindMostInjuredNeighbor functor(actor, medigun, patient_fired_recently);
			actor->GetVisionInterface()->ForEachKnownEntity(functor);
			
			float ratio = (patient_fired_recently ? 0.5f : 1.0f);
			if (functor.GetMostInjured() != nullptr && functor.GetHealthRatio() < ratio) {
				patient_alt = functor.GetMostInjured();
			}
		}
		
		actor->GetBodyInterface()->AimHeadTowards(patient_alt, IBody::PRI_CRITICAL, 1.0f, nullptr, "Aiming at my patient");
		
		CBaseEntity *medigun_heal_target = medigun->GetHealTarget();
		if (medigun_heal_target == nullptr || medigun_heal_target == patient_alt) {
			actor->PressFireButton();
			
			bool1 = false;
			bool2 = (medigun_heal_target != nullptr);
		} else {
			if (this->m_ct4834.IsElapsed()) {
				this->m_ct4834.Start(RandomFloat(1.0f, 2.0f));
			} else {
				actor->PressFireButton();
			}
			
			bool1 = true;
			bool2 = false;
		}
		
		if (this->IsReadyToDeployUber(medigun)) {
			bool should_pop_uber = false;
			
		// can't do this sanity check, since enum value TF_MEDIGUN_VACCINATOR is completely commented out right now
		//	if (medigun->GetMedigunType() == TF_MEDIGUN_VACCINATOR) {
		//		// TODO
		//		Assert(false);
		//	} else {

			if ( medigun->GetMedigunType() == TF_MEDIGUN_KRITZKRIEG ) {
				CBaseCombatWeapon *pWeapon = patient->GetActiveWeapon();
				if ( pWeapon && (float)patient->GetAmmoCount( pWeapon->GetPrimaryAmmoType() ) > (float)patient->GetMaxAmmo( pWeapon->GetPrimaryAmmoType() ) * 0.50f ) {
					should_pop_uber = true;
				}

				if ( patient->m_Shared.InCond( TF_COND_INVULNERABLE ) || patient->m_Shared.InCond( TF_COND_MEGAHEAL ) ) {
					should_pop_uber = true;
				}
				else {
					should_pop_uber = ( patient->HealthFraction() > 0.50f );
				}

				if ( pWeapon && pWeapon->UsesClipsForAmmo1() && pWeapon->m_iClip1 <= 0 ) {
					should_pop_uber = false;
				}

				if ( threat == nullptr || threat->IsObsolete() || !threat->IsVisibleRecently() || actor->GetDistanceBetween(threat->GetEntity()) > 750.0f ) {
					should_pop_uber = false;
				}
				
			} else if ( medigun->GetMedigunType() == TF_MEDIGUN_STOCK ) {
				if (patient->m_Shared.InCond(TF_COND_INVULNERABLE) || patient->m_Shared.InCond(TF_COND_MEGAHEAL)) {
					should_pop_uber = false;
				} else {
					should_pop_uber = (patient->HealthFraction() < 0.50f);
				}
				
				if (actor->GetHealth() < actor->GetUberHealthThreshold() && (TFGameRules()->IsMannVsMachineMode() || actor->GetTimeSinceLastInjuryByAnyEnemyTeam() < 1.0f)) {
					should_pop_uber = true;
				}

				if ( threat && actor->HealthFraction() < 0.50f && actor->IsThreatFiringAtMe(threat->GetEntity()) ) {
					should_pop_uber = true;
				}
				
				if (actor->GetHealth() < 25) {
					should_pop_uber = true;
				}
				
				if (TFGameRules()->IsMannVsMachineMode() && patient->m_Shared.InCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED) && actor->m_Shared.InCond(TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED)) {
					should_pop_uber = false;
				}
			}
			
			// NOTE: m_ctUberDelay isn't invalidated if should_pop_uber becomes false;
			// so if a medic decides to pop uber, then during the delay stops deciding it should uber,
			// and then *later on* again decides it should pop uber, it will then be able to uber *immediately*
			if (should_pop_uber) {
				if (!this->m_ctUberDelay.HasStarted()) {
					this->m_ctUberDelay.Start(actor->GetUberDeployDelayDuration());
				}
				
				if (this->m_ctUberDelay.IsElapsed()) {
					this->m_ctUberDelay.Invalidate();
					
					actor->PressAltFireButton();
				}
			}
		}
	}

	bool bool3; // TODO: name
	if (threat != nullptr && threat->IsVisibleRecently() && threat->GetEntity() != nullptr) {
		bool3 = (patient_alt == nullptr || actor->GetRangeSquaredTo(patient_alt) > actor->GetRangeSquaredTo(threat->GetEntity()));
	} else {
		bool3 = false;
	}
	
	bool out_of_range = actor->IsRangeGreaterThan(patient_alt, 1.1f * tf_bot_medic_max_heal_range.GetFloat());
	
	// TODO: name
	bool bool4 = (patient_alt == nullptr || !actor->IsLineOfFireClear(patient_alt->EyePosition()));
	
	// TODO: use this->IsDeployingUber(medigun) instead of actor->m_Shared.InCond(TF_COND_INVULNERABLE))
	if (this->IsReadyToDeployUber(medigun) || actor->m_Shared.InCond(TF_COND_INVULNERABLE) || bool2 || (!bool3 && !out_of_range && !bool4)) {
		actor->SwitchToSecondary();
	} else {
		actor->EquipBestWeaponForThreat(threat);
		
		if (threat != nullptr && threat->GetEntity() != nullptr) {
			actor->GetBodyInterface()->AimHeadTowards(threat->GetEntity(), IBody::PRI_IMPORTANT, 1.0f, nullptr, "Aiming at an enemy");
		}
	}
	
	// TODO: use this->IsDeployingUber(medigun) instead of actor->m_Shared.InCond(TF_COND_INVULNERABLE))
	if (this->IsReadyToDeployUber(medigun) || actor->m_Shared.InCond(TF_COND_INVULNERABLE) || bool1) {
		if (actor->IsRangeGreaterThan(patient, tf_bot_medic_stop_follow_range.GetFloat()) || !actor->IsAbleToSee(patient, CBaseCombatCharacter::DISREGARD_FOV)) {
			this->m_ChasePath.Update(actor, patient, CTFBotPathCost(actor, FASTEST_ROUTE));
		}
	} else {
		if (this->m_ct486c.IsElapsed() || this->IsVisibleToEnemy(actor, actor->EyePosition())) {
			this->m_ct486c.Start(RandomFloat(0.5f, 1.0f));
			
			this->ComputeFollowPosition(actor);
			this->m_PathFollower.Compute(actor, this->m_vecFollowPosition, CTFBotPathCost(actor, FASTEST_ROUTE));
		}
		
		this->m_PathFollower.Update(actor);
	}
	
	Continue();
}


ActionResult<CTFBot> CTFBotMedicHeal::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_ChasePath.Invalidate();
	Continue();
}


EventDesiredResult<CTFBot> CTFBotMedicHeal::OnActorEmoted(CTFBot *actor, CBaseCombatCharacter *who, int emoteconcept)
{
	if (emoteconcept == MP_CONCEPT_PLAYER_GO || emoteconcept == MP_CONCEPT_PLAYER_ACTIVATECHARGE) {
		CTFPlayer *emoter  = ToTFPlayer(who);
		CTFPlayer *patient = this->m_hPatient;
		
		if (emoter != nullptr && patient != nullptr && emoter->entindex() == patient->entindex()) {
			auto medigun = dynamic_cast<CWeaponMedigun *>(actor->m_Shared.GetActiveTFWeapon());
			if (this->IsReadyToDeployUber(medigun)) {
				actor->PressAltFireButton();
			}
		}
	}
	
	Continue();
}


ThreeState_t CTFBotMedicHeal::ShouldRetreat(const INextBot *nextbot) const
{
	auto actor = static_cast<CTFBot *>(nextbot->GetEntity());
	
	if (actor->m_Shared.IsControlStunned())    return TRS_TRUE;
	if (actor->m_Shared.IsLoserStateStunned()) return TRS_TRUE;
	
	return TRS_FALSE;
}

ThreeState_t CTFBotMedicHeal::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	auto actor = static_cast<CTFBot *>(nextbot->GetEntity());
	
	return (actor->IsCombatWeapon() ? TRS_TRUE : TRS_FALSE);
}


CTFPlayer *CTFBotMedicHeal::SelectPatient(CTFBot *actor, CTFPlayer *old_patient)
{
	CTFPlayer *patient = old_patient;
	
	auto medigun = dynamic_cast<CWeaponMedigun *>(actor->GetActiveTFWeapon());
	if (medigun != nullptr) {
		if (patient == nullptr || !patient->IsAlive()) {
			patient = ToTFPlayer(medigun->GetHealTarget());
		}
		
		if (medigun->IsReleasingCharge() || (patient != nullptr && this->IsReadyToDeployUber(medigun) && this->IsGoodUberTarget(patient))) {
			return patient;
		}
	}
	
	if (TFGameRules()->IsPVEModeActive()) {
		// TODO: MvM logic
		// has inlined call to IVision::ForEachKnownEntity<CSelectPrimaryPatient> :(
		Assert(false);
	}
	
	CSelectPrimaryPatient functor(actor, patient);
	actor->GetVisionInterface()->ForEachKnownEntity(functor);
	
	return functor.GetResult();
}

void CTFBotMedicHeal::ComputeFollowPosition(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotMedicHeal::ComputeFollowPosition", "NextBot");
	
	this->m_vecFollowPosition = actor->GetAbsOrigin();
	
	CTFPlayer *patient = this->m_hPatient;
	if (patient == nullptr) {
		return;
	}
	
	bool actor_is_visible_to_enemy;
	if (TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() == TF_TEAM_BLUE) {
		actor_is_visible_to_enemy = false;
	} else {
		actor_is_visible_to_enemy = this->IsVisibleToEnemy(actor, actor->EyePosition());
	}
	
	Vector patient_eyefwd_xy = VectorXY(EyeVectorsFwd(patient)).Normalized();
	
	bool actor_can_see_patient = false;
	if (actor->IsRangeLessThan(patient, tf_bot_medic_start_follow_range.GetFloat())) {
		actor_can_see_patient = actor->IsAbleToSee(patient, CBaseCombatCharacter::DISREGARD_FOV);
	}
	
	if (actor_is_visible_to_enemy) {
		NextBotTraceFilterIgnoreActors filter;
		float theta_inc = M_PI_F / tf_bot_medic_cover_test_resolution.GetFloat();
		
		float dist_begin = tf_bot_medic_stop_follow_range.GetFloat() + RandomFloat(0.0f, 100.0f);
		float dist_end = tf_bot_medic_max_heal_range.GetFloat();
		
		auto medigun = dynamic_cast<CWeaponMedigun *>(actor->GetActiveTFWeapon());
		if (!this->m_ct485c.IsElapsed() || this->IsReadyToDeployUber(medigun)) {
			dist_end = tf_bot_medic_start_follow_range.GetFloat();
		}
		
		Vector best_pos = actor->GetAbsOrigin();
		float best_distsqr = FLT_MAX;
		
		for (float dist = dist_begin; dist <= dist_end; dist += 100.0f) {
			for (float theta = 0.0f; theta <= 2 * M_PI_F; theta += theta_inc) {
				Vector dir;
				FastSinCos(theta, &dir.x, &dir.y);
				dir.z = 0.0f;
				
				trace_t tr;
				UTIL_TraceLine(patient->WorldSpaceCenter(), patient->WorldSpaceCenter() + (dist * dir), MASK_VISIBLE_AND_NPCS, &filter, &tr);
				
				Vector target = tr.endpos;
				if (tr.DidHit()) {
					target -= (0.5f * actor->GetBodyInterface()->GetHullWidth()) * dir;
				}
				TheTFNavMesh->GetSimpleGroundHeight(target, &target.z);
				
				if ((patient->GetAbsOrigin().z - target.z) > actor->GetLocomotionInterface()->GetStepHeight()) {
					if (tf_bot_medic_debug.GetBool()) {
						NDebugOverlay::Cross3D(target, 5.0f, NB_RGB_ORANGE_64, true, 1.0f);
						NDebugOverlay::Line(patient->WorldSpaceCenter(), target, NB_RGB_ORANGE_64, true, 1.0f);
					}
					
					continue;
				}
				
				// TODO: magic number origin?
				target.z += 62.0f;
				
				if (this->IsVisibleToEnemy(actor, target)) {
					if (tf_bot_medic_debug.GetBool()) {
						NDebugOverlay::Cross3D(target, 5.0f, NB_RGB_RED, true, 1.0f);
						NDebugOverlay::Line(patient->WorldSpaceCenter(), target, NB_RGB_RED, true, 0.2f);
					}
					
					continue;
				}
				
				float this_distsqr = (actor->EyePosition() - target).LengthSqr();
				if (this_distsqr < best_distsqr) {
					best_pos     = target;
					best_distsqr = this_distsqr;
				}
				
				if (tf_bot_medic_debug.GetBool()) {
					NDebugOverlay::Cross3D(target, 5.0f, NB_RGB_GREEN, true, 1.0f);
					NDebugOverlay::Line(patient->WorldSpaceCenter(), target, NB_RGB_GREEN, true, 0.5f);
				}
			}
		}

		// No cover -> go to regular follow position
		if ( best_pos == actor->GetAbsOrigin() ) {
			best_pos = patient->GetAbsOrigin() - ( tf_bot_medic_stop_follow_range.GetFloat() * patient_eyefwd_xy );
		}

		if ( tf_bot_medic_debug.GetBool() ) {
			NDebugOverlay::Cross3D( best_pos, 5.0f, NB_RGB_CYAN, true, 2.0f );
			NDebugOverlay::Line( patient->WorldSpaceCenter(), best_pos, NB_RGB_CYAN, true, 1.0f );
		}
		
		this->m_vecFollowPosition = best_pos;
	} else {
		if (actor_can_see_patient) {
			if (!TFGameRules()->InSetup() && patient->GetTimeSinceWeaponFired() > 5.0f && patient_eyefwd_xy.Dot(patient->GetAbsOrigin() - actor->GetAbsOrigin()) < 0.0f) {
				this->m_vecFollowPosition = patient->GetAbsOrigin() - (tf_bot_medic_stop_follow_range.GetFloat() * patient_eyefwd_xy);
			} else {
				this->m_vecFollowPosition = actor->GetAbsOrigin();
			}
		} else {
			this->m_vecFollowPosition = patient->GetAbsOrigin();
		}
	}
}


bool CTFBotMedicHeal::IsStable(CTFPlayer *player) const
{
	if (player->GetTimeSinceLastInjuryByAnyEnemyTeam() < 3.0f) return false;
	if (player->HealthFraction() < 1.0f)                       return false;
	if (player->m_Shared.InCond(TF_COND_BURNING))              return false;
	if (player->m_Shared.InCond(TF_COND_BLEEDING))             return false;
	
	return true;
}

// TODO: rework this whole function
bool CTFBotMedicHeal::IsGoodUberTarget(CTFPlayer *player) const
{
#if 0
	// TODO: handle new TF2C classes
	switch (player->GetPlayerClass()->GetClassIndex()) {
	case TF_CLASS_SCOUT:        return false;
	case TF_CLASS_SNIPER:       return false;
	case TF_CLASS_SOLDIER:      return true;
	case TF_CLASS_DEMOMAN:      return true;
	case TF_CLASS_MEDIC:        return false;
	case TF_CLASS_HEAVYWEAPONS: return true;
	case TF_CLASS_PYRO:         return true;
	case TF_CLASS_SPY:          return false;
	case TF_CLASS_ENGINEER:     return false;
	default: Assert(false);     return false;
	}
#endif
	
	// NOTE: live TF2 does some checks and then just always returns false (for some reason)
	return false;
}

// TODO: when the rest of this class is RE'd, investigate the implications of
// whether this function handles invisible spies properly etc
bool CTFBotMedicHeal::IsVisibleToEnemy(CTFBot *actor, const Vector& vec) const
{
	// Live TF2 uses a custom CKnownCollector IVision::IForEachKnownEntity functor which
	// (a) is unnecessary given the existence of IVision::CollectKnownEntities
	// (b) would be completely obsoleted by the existence of lambdas anyway
	
	CUtlVector<CKnownEntity> knowns;
	actor->GetVisionInterface()->CollectKnownEntities(&knowns);
	
	for (const auto& known : knowns) {
		CBaseCombatCharacter *bcc = ToBaseCombatCharacter(known.GetEntity());
		if (bcc == nullptr)       continue;
		if (!actor->IsEnemy(bcc)) continue;
		
		if (bcc->IsLineOfSightClear(vec, CBaseCombatCharacter::IGNORE_ACTORS)) {
			return true;
		}
	}
	
	return false;
}

bool CTFBotMedicHeal::IsReadyToDeployUber(const ITFHealingWeapon *medigun) const
{
	if (medigun == nullptr)       return false;
	if (TFGameRules()->InSetup()) return false;
	
	return (medigun->GetChargeLevel() >= medigun->GetMinChargeAmount());
}

bool CTFBotMedicHeal::IsDeployingUber( const ITFHealingWeapon *medigun ) const
{
	if (medigun == nullptr) return false;
	
	return medigun->IsReleasingCharge();
}
