/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_prepare_stickybomb_trap.h"
#include "tf_gamerules.h"
#include "tf_weapon_pipebomblauncher.h"


static ConVar tf_bot_stickybomb_density("tf_bot_stickybomb_density", "0.0001", FCVAR_CHEAT, "Number of stickies to place per square inch");


class PlaceStickyBombReply : public INextBotReply
{
public:
	virtual void OnSuccess(INextBot *nextbot) override
	{
		CTFBot *actor = ToTFBot(nextbot->GetEntity());
		
		CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
		if (weapon != nullptr && weapon->IsWeapon(TF_WEAPON_PIPEBOMBLAUNCHER)) {
			actor->PressFireButton(0.1f);
			
			Assert(this->m_pBombTargetArea != nullptr);
			if (this->m_pBombTargetArea != nullptr) {
				++this->m_pBombTargetArea->stickies;
			}
			
			Assert(this->m_pctAimTimeout != nullptr);
			if (this->m_pctAimTimeout != nullptr) {
				this->m_pctAimTimeout->Start(0.15f);
			}
		}
	}
	
	virtual void OnFail(INextBot *nextbot, FailureReason reason) override
	{
		Assert(this->m_pctAimTimeout != nullptr);
		if (this->m_pctAimTimeout != nullptr) {
			this->m_pctAimTimeout->Invalidate();
		}
	}
	
	CTFBotPrepareStickybombTrap::BombTargetArea *m_pBombTargetArea; // +0x04
	CountdownTimer *m_pctAimTimeout;                                // +0x08
};
// TODO: remove offsets


static PlaceStickyBombReply bombReply;


ActionResult<CTFBot> CTFBotPrepareStickybombTrap::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Secondary() != nullptr && typeid(actor->GetTFWeapon_Secondary()) == typeid(CTFPipebombLauncher));
	
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->GetTFWeapon_Secondary());
	if (launcher != nullptr && actor->GetAmmoCount(TF_AMMO_SECONDARY) >= launcher->GetMaxClip1() && launcher->Clip1() < launcher->GetMaxClip1()) {
		this->m_bReload = true;
	} else {
		this->m_bReload = false;
	}
	
	this->InitBombTargetAreas(actor);
	
	this->LookAroundForEnemies_Set(actor, false);
	
	Continue();
}

ActionResult<CTFBot> CTFBotPrepareStickybombTrap::Update(CTFBot *actor, float dt)
{
	if (!TFGameRules()->InSetup()) {
		const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
		
		if (threat != nullptr && (actor->GetAbsOrigin() - threat->GetLastKnownPosition()).IsLengthLessThan(500.0f)) {
			Done("Enemy nearby - giving up");
		}
	}
	
	CTFWeaponBase *active = actor->GetActiveTFWeapon();
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->GetTFWeapon_Secondary());
	if (active == nullptr || launcher == nullptr) {
		Done("Missing weapon");
	}
	
	if (!active->IsWeapon(TF_WEAPON_PIPEBOMBLAUNCHER)) {
		actor->Weapon_Switch(launcher);
	}
	
	if (this->m_bReload) {
		if (Min(launcher->GetMaxClip1(), actor->GetAmmoCount(TF_AMMO_SECONDARY)) <= launcher->Clip1()) {
			this->m_bReload = false;
		}
		
		actor->PressReloadButton();
		
		Continue();
	}
	
	if (launcher->GetPipeBombCount() < launcher->GetMaxPipeBombCount() && actor->GetAmmoCount(TF_AMMO_SECONDARY) > 0) {
		if (!this->m_ctAimTimeout.IsElapsed()) {
			Continue();
		}
		
		for (auto& target : this->m_BombTargetAreas) {
			int wanted_stickies = tf_bot_stickybomb_density.GetFloat() * (target.area->GetSizeX() * target.area->GetSizeY());
			wanted_stickies = Max(1, wanted_stickies);
			
			if (target.stickies < wanted_stickies) {
				bombReply.m_pBombTargetArea = &target;
				bombReply.m_pctAimTimeout   = &this->m_ctAimTimeout;
				
				this->m_ctAimTimeout.Start(2.0f);
				actor->GetBodyInterface()->AimHeadTowards(target.area->GetRandomPoint(), IBody::PRI_IMPORTANT, 5.0f, &bombReply, "Aiming a sticky bomb");
				
				Continue();
			}
		}
		
		Done("Exhausted bomb target areas");
	} else {
		Done("Max sticky bombs reached");
	}
}

void CTFBotPrepareStickybombTrap::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	actor->GetBodyInterface()->ClearPendingAimReply();
	
	this->LookAroundForEnemies_Reset(actor);
}


ActionResult<CTFBot> CTFBotPrepareStickybombTrap::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	Done();
}


EventDesiredResult<CTFBot> CTFBotPrepareStickybombTrap::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)
{
	Done("Ouch!", SEV_MEDIUM);
}


EventDesiredResult<CTFBot> CTFBotPrepareStickybombTrap::OnNavAreaChanged(CTFBot *actor, CNavArea *area1, CNavArea *area2)
{
	if (area2 != nullptr) {
		this->InitBombTargetAreas(actor);
	}
	
	Continue();
}


bool CTFBotPrepareStickybombTrap::IsPossible(CTFBot *actor)
{
	if (actor->GetTimeSinceLastInjury() < 1.0f)  return false;

	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat != nullptr && ( actor->GetAbsOrigin() - threat->GetLastKnownPosition() ).IsLengthLessThan( 500.0f ) ) {
		return false;
	}
	
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->GetTFWeapon_Secondary());
	if (launcher == nullptr) return false;
	
	if (actor->IsWeaponRestricted(launcher)) {
		return false;
	}
	
	if (launcher->GetPipeBombCount() >= launcher->GetMaxPipeBombCount()) {
		return false;
	}
	
	return (actor->GetAmmoCount(TF_AMMO_SECONDARY) > 0);
}


void CTFBotPrepareStickybombTrap::InitBombTargetAreas(CTFBot *actor)
{
	CTFNavArea* lastKnownArea = actor->GetLastKnownTFArea();
	if ( lastKnownArea == nullptr ) return;

	/* intentional vector copy */
	CUtlVector<CTFNavArea *> areas;
	areas = lastKnownArea->GetInvasionAreas(actor->GetTeamNumber());
	
	areas.Shuffle();
	
	this->m_BombTargetAreas.RemoveAll();
	for (auto area : areas) {
		this->m_BombTargetAreas.AddToTail({ area, 0 });
	}
	
	this->m_ctAimTimeout.Invalidate();
	actor->GetBodyInterface()->ClearPendingAimReply();
}
