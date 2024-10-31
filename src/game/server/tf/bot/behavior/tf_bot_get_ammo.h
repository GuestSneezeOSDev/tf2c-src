/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_GET_AMMO_H
#define TF_BOT_GET_AMMO_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"
#include "NextBotChasePath.h"


// TODO: reconcile the weird differences between CTFBotGetAmmo and CTFBotGetHealth


class CTFBotGetAmmo final : public Action<CTFBot>
{
public:
	CTFBotGetAmmo() {}
	virtual ~CTFBotGetAmmo() {}

	enum
	{
		TFBOT_AMMOSOURCE_INVALID = -1,
		TFBOT_AMMOSOURCE_AMMOPACK = 0,
		TFBOT_AMMOSOURCE_DROPPEDPACK,
		TFBOT_AMMOSOURCE_DISPENSER,
		TFBOT_AMMOSOURCE_RESUPPLY,
	};
	
	virtual const char *GetName() const override { return "GetAmmo"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) override;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) override;
	
	virtual ThreeState_t ShouldHurry(const INextBot *nextbot) const override { return TRS_TRUE; }
	
	static bool IsPossible(CTFBot *actor);
	
	static bool IsValidAmmoSource(CTFBot *actor, CBaseEntity *ent, int type);
	static int DetermineAmmoSourceType( CBaseEntity *ent );
	
private:
	PathFollower m_PathFollower;  // +0x0034
	ChasePath m_ChasePath;
	CHandle<CBaseEntity> m_hAmmo; // +0x4808
	int m_AmmoSourceType;
	bool m_bNearDispenser;
	
	static CHandle<CBaseEntity> s_possibleAmmo;
	static CTFBot *s_possibleBot;
	static int s_possibleFrame;
};
// TODO: remove offsets


#endif
