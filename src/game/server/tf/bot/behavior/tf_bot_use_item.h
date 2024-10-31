/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_USE_ITEM_H
#define TF_BOT_USE_ITEM_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"


class CTFBotUseItem final : public Action<CTFBot>
{
public:
	CTFBotUseItem(CTFWeaponBase *item, CTFPlayer *target) : m_hItem(item), m_hTarget(target) {}
	virtual ~CTFBotUseItem() {}
	
	virtual const char *GetName() const override { return "UseItem"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
	virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnSuspend(CTFBot *actor, Action<CTFBot> *action) override;
	virtual ActionResult<CTFBot> OnResume(CTFBot *actor, Action<CTFBot> *action) override;
	
private:
	bool CanUseItem() const;
	
	bool IsItemReady() const;
	void UseItem() const;
	bool IsDoneUsingItem() const;
	
	void PushItem();
	void PopItem();
	
	CHandle<CTFWeaponBase> m_hItem;
	CHandle<CTFPlayer> m_hTarget;
	bool m_bReady;
	bool m_bPushed;

	float m_flStartTime;
};


#endif
