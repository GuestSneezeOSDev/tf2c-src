/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_locomotion.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "particle_parse.h"
#include "tf_nav_area.h"


CTFBotLocomotion::CTFBotLocomotion(INextBot *nextbot) :
	PlayerLocomotion(nextbot), m_pTFBot(assert_cast<CTFBot *>(nextbot)) {}


void CTFBotLocomotion::Update()
{
	PlayerLocomotion::Update();
	
	if (!this->IsOnGround()) {
		this->GetTFBot()->PressCrouchButton(0.3f);
	} else {
		if (!this->GetTFBot()->IsPlayerClass(TF_CLASS_ENGINEER, true)) {
			this->GetTFBot()->ReleaseCrouchButton();
		}
	}
}


void CTFBotLocomotion::Approach(const Vector& dst, float f1)
{
	if (TFGameRules()->IsMannVsMachineMode() && !this->IsOnGround() && !this->IsClimbingOrJumping()) {
		return;
	}
	
	PlayerLocomotion::Approach(dst, f1);
}

void CTFBotLocomotion::Jump()
{
	PlayerLocomotion::Jump();
	
	if (TFGameRules()->IsMannVsMachineMode()) {
		int iCustomParticle = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(this->GetTFBot(), iCustomParticle, bot_custom_jump_particle);
		// TODO: add to items_game.txt
		
		if (iCustomParticle != 0) {
			DispatchParticleEffect("rocketjump_smoke", PATTACH_POINT_FOLLOW, this->GetTFBot(), "foot_L");
			DispatchParticleEffect("rocketjump_smoke", PATTACH_POINT_FOLLOW, this->GetTFBot(), "foot_R");
		}
	}
}

// TODO: remove this if possible;
// this implements the live-TF2 method of returning the fixed player-class-info-based max speed
// whereas we'd prefer to return the player's ACTUAL max speed (but that may be problematic for some reason)
float CTFBotLocomotion::GetRunSpeed() const
{
	// return this->GetTFBot()->GetPlayerClass()->GetMaxSpeed();
	return this->GetTFBot()->GetPlayerMaxSpeed();
}

bool CTFBotLocomotion::IsAreaTraversable(const CNavArea *area) const
{
	if (!PlayerLocomotion::IsAreaTraversable(area)) return false;
	
	if (static_cast<const CTFNavArea *>(area)->HasEnemySpawnRoom(this->GetTFBot()) && !(TFGameRules()->State_Get() == GR_STATE_TEAM_WIN && TFGameRules()->GetWinningTeam() == this->GetTFBot()->GetTeamNumber())) {
		return false;
	}
	
	return true;
}

bool CTFBotLocomotion::IsEntityTraversable(CBaseEntity *ent, TraverseWhenType when) const
{
	if (ent != nullptr && ent->IsPlayer()) return true;
	
	return PlayerLocomotion::IsEntityTraversable(ent, when);
}
