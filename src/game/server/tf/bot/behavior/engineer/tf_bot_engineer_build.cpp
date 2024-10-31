/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_engineer_build.h"
#include "tf_bot_engineer_build_teleport_entrance.h"
#include "tf_bot_engineer_move_to_build.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "tf_obj_dispenser.h"


// TODO: fix spelling; also probably remove this relic of "raid mode"
static ConVar tf_raid_engineer_infinte_metal("tf_raid_engineer_infinte_metal", "1", FCVAR_CHEAT);


ActionResult<CTFBot> CTFBotEngineerBuild::Update(CTFBot *actor, float dt)
{
	if (TFGameRules()->IsPVEModeActive() && tf_raid_engineer_infinte_metal.GetBool()) {
		actor->GiveAmmo(1000, TF_AMMO_METAL, true);
	}
	
	Continue();
}


Action<CTFBot> *CTFBotEngineerBuild::InitialContainedAction(CTFBot *actor)
{
	if (TFGameRules()->IsPVEModeActive()) {
		/* ancient relic of early MvM engineer (not used by modern MvM) */
		return new CTFBotEngineerMoveToBuild();
	} else if ( TFGameRules()->IsInArenaMode() ) {
		return new CTFBotEngineerMoveToBuild();
	} else {
		return new CTFBotEngineerBuildTeleportEntrance();
	}
}


ThreeState_t CTFBotEngineerBuild::ShouldHurry(const INextBot *nextbot) const
{
	auto actor = static_cast<CTFBot *>(nextbot->GetEntity());
	
	auto sentry    = assert_cast<CObjectSentrygun *>(actor->GetObjectOfType(OBJ_SENTRYGUN));
	auto dispenser = assert_cast<CObjectDispenser *>(actor->GetObjectOfType(OBJ_DISPENSER));
	
	if (sentry != nullptr && dispenser != nullptr && !sentry->IsBuilding() && !dispenser->IsBuilding()) {
		CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
		if (weapon != nullptr && weapon->IsWeapon(TF_WEAPON_WRENCH)) {
			if (!actor->IsAmmoLow() || dispenser->GetAvailableMetal() > 0) {
				return TRS_TRUE;
			} else {
				return TRS_FALSE;
			}
		}
	}
	
	return TRS_NONE;
}

ThreeState_t CTFBotEngineerBuild::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	// TODO: go through all the code where we do this sort of thing and decide if it ought to be
	// - static_cast
	// - assert_cast
	// - dynamic_cast
	// - ToTFBot
	auto actor = static_cast<CTFBot *>(nextbot->GetEntity());

	if ( TFGameRules()->IsInArenaMode() ) return TRS_TRUE;
	
	CTFPlayer *threat_player = ToTFPlayer(threat->GetEntity());
	if (threat_player != nullptr && threat_player->IsPlayerClass(TF_CLASS_SPY, true)) {
		return TRS_TRUE;
	}
	
	CBaseObject *sentry = actor->GetObjectOfType(OBJ_SENTRYGUN);
	if (sentry != nullptr && actor->IsRangeLessThan(sentry, 100.0f)) {
		return TRS_FALSE;
	}
	
	return TRS_NONE;
}
