/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_vision.h"
#include "tf_bot.h"
#include "tf_gamerules.h"
#include "NextBotManager.h"


// these seem to be unreferenced by any actual code
// static ConVar tf_bot_choose_target_interval       ("tf_bot_choose_target_interval",        "0.3f", FCVAR_CHEAT, "How often, in seconds, a TFBot can reselect his target");
// static ConVar tf_bot_sniper_choose_target_interval("tf_bot_sniper_choose_target_interval", "3.0f", FCVAR_CHEAT, "How often, in seconds, a zoomed-in Sniper can reselect his target");


// TODO: put this somewhere
static CBaseObject *ToBaseObject(CBaseEntity *ent)
{
	if (ent != nullptr && ent->IsBaseObject()) {
		return assert_cast<CBaseObject *>(ent);
	}
	
	return nullptr;
}


CTFBotVision::CTFBotVision(INextBot *nextbot) :
	IVision(nextbot), m_pTFBot(assert_cast<CTFBot *>(nextbot)) {}


void CTFBotVision::Update()
{
	if (TFGameRules()->IsMannVsMachineMode()) {
		if (!this->m_ctUpdate.IsElapsed()) return;
		this->m_ctUpdate.Start(RandomFloat(0.9f, 1.1f));
	}
	
	IVision::Update();
	
	CUtlVector<CTFPlayer *> enemies;
	this->GetTFBot()->CollectEnemyPlayers(&enemies, true);
	
	for (auto enemy : enemies) {
		if (!enemy->IsPlayerClass(TF_CLASS_SPY, true)) continue;
		
		const CKnownEntity *known = this->GetKnown(enemy);
		if (known == nullptr || (!known->IsVisibleRecently() && enemy->m_Shared.InCond(TF_COND_DISGUISING))) {
			this->GetTFBot()->ForgetSpy(enemy);
		}
	}
}


void CTFBotVision::CollectPotentiallyVisibleEntities(CUtlVector<CBaseEntity *> *ents)
{
	VPROF_BUDGET("CTFBotVision::CollectPotentiallyVisibleEntities", "NextBot");
	
	ents->RemoveAll();
	
	ForEachPlayer([=](CBasePlayer *player){
		if (player->IsAlive()) {
			ents->AddToTail(player);
		}
		
		return true;
	});
	
	this->UpdatePotentiallyVisibleNPCVector();
	for (auto npc : this->m_PVNPCs) {
		ents->AddToTail(npc);
	}
}

float CTFBotVision::GetMaxVisionRange() const
{
	if (this->GetTFBot()->GetVisionRange() > 0.0f) {
		return this->GetTFBot()->GetVisionRange();
	} else {
		return 6000.0f;
	}
}

float CTFBotVision::GetMinRecognizeTime() const
{
	switch (this->GetTFBot()->GetSkill()) {
	default:
	case CTFBot::EASY:
		return 1.00f;
	case CTFBot::NORMAL:
		return 0.50f;
	case CTFBot::HARD:
		return 0.30f;
	case CTFBot::EXPERT:
		return 0.20f;
	}
}

bool CTFBotVision::IsIgnored(CBaseEntity *ent) const
{
	// BUG: live TF2 returns the wrong result for this comparison, it seems
	if (this->GetTFBot()->IsAttentionFocused() && !this->GetTFBot()->IsAttentionFocusedOn(ent)) {
		return true;
	}
	
	if (!this->GetTFBot()->IsEnemy(ent)) {
		return false;
	}
	
	if (ent->IsEffectActive(EF_NODRAW)) {
		return true;
	}

	if ( ent->GetFlags() & FL_NOTARGET ) {
		return true;
	}
	
	CTFPlayer *player = ToTFPlayer(ent);
	if (player != nullptr) {
		switch (player->GetPlayerClass()->GetClassIndex()) {
		case TF_CLASS_SCOUT:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_SCOUTS)) return true;
			break;
		case TF_CLASS_SNIPER:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_SNIPERS)) return true;
			break;
		case TF_CLASS_SOLDIER:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_SOLDIERS)) return true;
			break;
		case TF_CLASS_DEMOMAN:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_DEMOS)) return true;
			break;
		case TF_CLASS_MEDIC:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_MEDICS)) return true;
			break;
		case TF_CLASS_HEAVYWEAPONS:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_HEAVIES)) return true;
			break;
		case TF_CLASS_PYRO:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_PYROS)) return true;
			break;
		case TF_CLASS_SPY:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_SPIES)) return true;
			break;
		case TF_CLASS_ENGINEER:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_ENGIES)) return true;
			break;
		case TF_CLASS_CIVILIAN:
			if (this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_CIVILIANS)) return true;
			break;
		}
		
		/* essentially everything past this point is only really applicable to
		 * spies, but is tested against players of any class */
		
		if (this->GetTFBot()->IsKnownSpy(player)) {
			return false;
		}
		
		if (this->GetTFBot()->IsSpyInCloakRevealingCondition(player)) {
			return false;
		}
		
		/* speculated to have been used for 'radius stealth' MvM canteen */
	//	if (player->m_Shared.InCond(TF_COND_STEALTHED_USER_BUFF_FADING)) {
	//		return true;
	//	}
		
		// BUG: should we really short-circuit to false for players who are partially stealthed but not >=75%?
		if (player->m_Shared.IsStealthed()) {
			return (player->m_Shared.GetPercentInvisible() >= 0.75f);
		}
		
		if (!player->m_Shared.InCond(TF_COND_DISGUISED) || player->m_Shared.InCond(TF_COND_DISGUISING)) {
			return false;
		}
		
		if (!player->GetRecentSapTimer().IsElapsed()) {
			return false;
		}
		
		return (player->m_Shared.DisguiseFoolsTeam( this->GetTFBot()->GetTeamNumber() ));
	}
	
	CBaseObject *obj = ToBaseObject(ent);
	if (obj != nullptr) {
		if (obj->HasSapper() && !TFGameRules()->IsMannVsMachineMode()) {
			return true;
		}

		// Random chance for this bot to recognize that it's a building and not a toolbox.
		if (obj->IsPlacing() || obj->IsBeingCarried() || obj->InRemoteConstruction()) {
			return true;
		}
		
		if (obj->IsSentry() && this->GetTFBot()->CheckIgnoreMask(CTFBot::IGNORE_SENTRIES)) {
			return true;
		}
		
		return false;
	}
	
	/* any other entity type */
	return false;
}

bool CTFBotVision::IsVisibleEntityNoticed(CBaseEntity *ent) const
{
	CTFPlayer *player = ToTFPlayer(ent);
	if (player != nullptr) {
		if (!this->GetTFBot()->IsEnemy(player)) {
			return true;
		}
		
		if (this->GetTFBot()->IsSpyInCloakRevealingCondition(player)) {
			if (player->m_Shared.InCond(TF_COND_STEALTHED)) {
				this->GetTFBot()->RealizeSpy(player);
			}
			
			return true;
		}
		
		/* speculated to have been used for 'radius stealth' MvM canteen */
	//	if (player->m_Shared.InCond(TF_COND_STEALTHED_USER_BUFF_FADING)) {
	//		this->GetTFBot()->ForgetSpy(player);
	//		return false;
	//	}
		
		if (player->m_Shared.IsStealthed()) {
			if (player->m_Shared.GetPercentInvisible() >= 0.75f) {
				this->GetTFBot()->ForgetSpy(player);
				return false;
			} else {
				this->GetTFBot()->RealizeSpy(player);
				return true;
			}
		}
		
		if (TFGameRules()->IsMannVsMachineMode()) {
			// TODO: MvM stuff
			Assert(false);
		}
		
		if (this->GetTFBot()->IsKnownSpy(player)) {
			return true;
		}
		
		// TODO: go take a look at CTFWeaponBuilder::PrimaryAttack;
		// the logic for m_ctRecentSap is pretty baffling overall because it
		// gets set for ALL players, and then is reset in CTFPlayer::Spawn,
		// and overall it doesn't make a whole lot of sense
		if (!TFGameRules()->IsMannVsMachineMode() && !player->GetRecentSapTimer().IsElapsed()) {
			this->GetTFBot()->RealizeSpy(player);
			return true;
		}
		
		if (player->m_Shared.InCond(TF_COND_DISGUISING)) {
			this->GetTFBot()->RealizeSpy(player);
			return true;
		}
		
		if (player->m_Shared.InCond(TF_COND_DISGUISED) && player->m_Shared.DisguiseFoolsTeam(this->GetTFBot()->GetTeamNumber())) {
			return false;
		}
		
		return true;
	}
	
	/* non-players */
	return true;
}


void CTFBotVision::UpdatePotentiallyVisibleNPCVector()
{
	if (!this->m_ctUpdatePVNPCs.IsElapsed()) return;
	this->m_ctUpdatePVNPCs.Start(RandomFloat(3.0f, 4.0f));
	
	this->m_PVNPCs.RemoveAll();
	
	bool ignore_teles = (TFGameRules()->IsMannVsMachineMode() && this->GetTFBot()->GetTeamNumber() == TF_TEAM_BLUE);
	for (auto obj : CBaseObject::AutoList()) {
		if (obj->IsSentry() || obj->IsNormalDispenser() || (obj->IsTeleporter() && !ignore_teles)) {
			this->m_PVNPCs.AddToTail(obj);
		}
	}
	
	TheNextBots().ForEachCombatCharacter([=](CBaseCombatCharacter *npc){
		if (npc != nullptr && !npc->IsPlayer()) {
			this->m_PVNPCs.AddToTail(npc);
		}
		
		return true;
	});
}
