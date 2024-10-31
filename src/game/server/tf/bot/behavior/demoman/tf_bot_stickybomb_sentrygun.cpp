/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_stickybomb_sentrygun.h"
#include "tf_gamerules.h"
#include "tf_weapon_pipebomblauncher.h"


// TODO: can we derive these from some kind of pipebomb launcher weapon constants?
static ConVar tf_bot_sticky_base_range ("tf_bot_sticky_base_range",   "800", FCVAR_CHEAT);
static ConVar tf_bot_sticky_charge_rate("tf_bot_sticky_charge_rate", "0.01", FCVAR_CHEAT, "Seconds of charge per unit range beyond base");


ActionResult<CTFBot> CTFBotStickybombSentrygun::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Secondary() != nullptr && typeid(actor->GetTFWeapon_Secondary()) == typeid(CTFPipebombLauncher));
	
	actor->PressAltFireButton();
	
	this->LookAroundForEnemies_Set(actor, false);
	
	this->m_bReload = true;
	
	// WTF: whoa whoa whoa, you can't just magically do this!
	actor->SetAbsVelocity(vec3_origin);
	
	// DWORD @ +0x78 = 0x00000000
	this->m_b0058 = false;
	
	this->m_ctAimTimeout.Start(3.0f);
	
	this->m_b0048 = false;
	
	if (this->m_bOpportunistic) {
		this->m_vec0068 = actor->GetAbsOrigin();
		
		this->m_b0048 = true;
		
		this->m_flChargeLevel = this->m_flOptCharge;
		this->m_vecAimTarget = actor->EyePosition() + (1500.0f * AngleVecFwd(QAngle(this->m_flOptPitch, this->m_flOptYaw, 0.0f)));
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotStickybombSentrygun::Update(CTFBot *actor, float dt)
{
	CTFWeaponBase *active = actor->GetActiveTFWeapon();
	auto launcher = dynamic_cast<CTFPipebombLauncher *>(actor->GetTFWeapon_Secondary());
	if (active == nullptr || launcher == nullptr) {
		Done("Missing weapon");
	}
	
	if (!active->IsWeapon(TF_WEAPON_PIPEBOMBLAUNCHER)) {
		actor->Weapon_Switch(launcher);
	}
	
	CObjectSentrygun *sentry = this->m_hSentry;
	if (sentry == nullptr || sentry->GetHealth() == 0) {
		Done("Sentry destroyed");
	}
	
	if (!this->m_b0058 && this->m_ctAimTimeout.IsElapsed()) {
		Done("Can't find aim");
	}
	
	if (this->m_bReload) {
		if (Min(launcher->GetMaxClip1(), actor->GetAmmoCount(TF_AMMO_SECONDARY)) <= launcher->Clip1()) {
			this->m_bReload = false;
		}
		
		actor->PressReloadButton();
		
		Continue();
	}
	
	// TODO: magic number assumption about number of stickies needed to destroy a sentry gun
	/* lay enough stickies to destroy the sentry gun, if possible */
	if (launcher->GetPipeBombCount() < (TFGameRules()->IsMannVsMachineMode() ? 5 : 3) && actor->GetAmmoCount(TF_AMMO_SECONDARY) > 0) {
		if (this->m_b0048) {
			actor->GetBodyInterface()->AimHeadTowards(this->m_vecAimTarget, IBody::PRI_CRITICAL, 0.3f, nullptr, "Aiming a sticky bomb at a sentrygun");
			
			float charge_time = this->m_flChargeLevel * (1.1f * launcher->GetChargeMaxTime());
			if (gpGlobals->curtime - launcher->GetChargeBeginTime() < charge_time) {
				actor->PressFireButton();
			} else {
				actor->ReleaseFireButton();
				this->m_b0048 = false;
			}
			
			Continue();
		}
		
		if (gpGlobals->curtime > launcher->m_flNextPrimaryAttack) {
		//	#error Finish this stuff
			
			if (this->m_b0058) {
				if (actor->IsRangeGreaterThan(this->m_vec0068, 1.0f)) {
					this->m_b0058 = false;
					this->m_ctAimTimeout.Reset();
				}
				
				if (this->m_b0058) {
					goto LABEL_58;
				}
			}
			
			{ // REMOVE THIS SCOPE ONCE THE GOTO IS REMOVED!
			
			Vector aim_vec = (sentry->WorldSpaceCenter() - actor->EyePosition());
			QAngle aim_ang = VectorAngles(aim_vec);
			
			// TODO: names etc
			float the_pitch = 0.0f;
			float the_yaw   = 0.0f;
			float v45       = 1.0f;
			
			// TODO: factor some stuff out of this loop
			for (int tries = 100; tries != 0; --tries) {
				float try_pitch = RandomFloat(-85.0f, 85.0f);
				float try_yaw   = RandomFloat(-30.0f, 30.0f) + aim_ang.y;
				
				float try_charge = 0.00f;
				if (aim_vec.IsLengthGreaterThan(tf_bot_sticky_base_range.GetFloat())) {
					try_charge = RandomFloat(0.10f, 1.00f);
				}
				
				// TODO: REMOVE ME
				auto l_IsAimOnTarget = [&](CTFBot *a, float p, float y, float c){
					bool result = this->IsAimOnTarget(a, p, y, c);
					a->DrawProjectileImpactEstimation(false);
					return result;
				};
				
			//	if (this->IsAimOnTarget(actor, try_pitch, try_yaw, try_charge) && try_charge < v45) {
				if (l_IsAimOnTarget(actor, try_pitch, try_yaw, try_charge) && try_charge < v45) {
					this->m_flChargeLevel = try_charge;
					this->m_b0058 = true;
					
					the_pitch = try_pitch;
					the_yaw   = try_yaw;
					
					// WTF: wat
					if (try_charge < 0.01f) {
						break;
					}
					
					v45 = try_charge;
				}
			}
			
			aim_ang.x = the_pitch;
			aim_ang.y = the_yaw;
			aim_ang.z = 0.0f;
			
			this->m_vecAimTarget = actor->EyePosition() + (500.0f * AngleVecFwd(aim_ang));
			
			actor->GetBodyInterface()->AimHeadTowards(this->m_vecAimTarget, IBody::PRI_IMPORTANT, 0.3f, nullptr, "Searching for aim...");
			
			} // REMOVE THIS SCOPE ONCE THE GOTO IS REMOVED!
			
			if (this->m_b0058) {
			LABEL_58:
				this->m_vec0068 = actor->GetAbsOrigin();
				this->m_b0048 = true;
				
				actor->PressFireButton();
			}
		}
		
		Continue();
	}
	
	/* if some stickies haven't stuck, then don't detonate just yet */
	for (int i = launcher->GetPipeBombCount() - 1; i >= 0; --i) {
		CTFGrenadeStickybombProjectile *sticky = launcher->GetPipeBomb(i);
		if (sticky == nullptr) continue;
		
		if (!sticky->HasTouched()) {
			Continue();
		}
	}
	
	actor->PressAltFireButton();
	
	if (actor->GetAmmoCount(TF_AMMO_SECONDARY) > 0) {
		Continue();
	} else {
		Done("Out of ammo");
	}
}

void CTFBotStickybombSentrygun::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	actor->PressAltFireButton();
	
	this->LookAroundForEnemies_Reset(actor);
}


ActionResult<CTFBot> CTFBotStickybombSentrygun::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	actor->PressAltFireButton();
	
	Done();
}


EventDesiredResult<CTFBot> CTFBotStickybombSentrygun::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)
{
	Done("Ouch!", SEV_MEDIUM);
}


bool CTFBotStickybombSentrygun::IsAimOnTarget(CTFBot *actor, float pitch, float yaw, float charge)
{
	CTFWeaponBase *launcher = actor->GetTFWeapon_Secondary();
	if (launcher == nullptr) return false;
	
	Vector est = actor->EstimateStickybombProjectileImpactPosition(launcher, pitch, yaw, charge);
	
	CObjectSentrygun *sentry = this->m_hSentry;
	
	// TODO: magic number 75.0f
	if (sentry->WorldSpaceCenter().DistToSqr(est) >= Square(75.0f)) {
		return false;
	}
	
	trace_t tr;
	UTIL_TraceLine(sentry->WorldSpaceCenter(), est, MASK_SOLID_BRUSHONLY, NextBotTraceFilterIgnoreActors(), &tr);
	
	return !tr.DidHit();
}
