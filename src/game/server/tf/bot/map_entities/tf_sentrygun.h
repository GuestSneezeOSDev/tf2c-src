/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_SENTRYGUN_H
#define TF_SENTRYGUN_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_hint_entity.h"


class CTFPlayer;


class CTFBotHintSentrygun : public CBaseTFBotHintEntity, public TAutoList<CTFBotHintSentrygun>
{
public:
	CTFBotHintSentrygun() {}
	virtual ~CTFBotHintSentrygun() {}
	
	DECLARE_CLASS(CTFBotHintSentrygun, CBaseTFBotHintEntity);
	DECLARE_DATADESC();
	
	virtual HintType GetHintType() const override { return SENTRY_GUN; }
	
	bool IsAvailableForSelection(CTFPlayer *player) const;
	
	void OnSentryGunDestroyed(CBaseEntity *ent);
	
	CTFPlayer *GetOwnerPlayer() const      { return this->m_hOwnerPlayer; }
	void SetOwnerPlayer(CTFPlayer *player) { this->m_hOwnerPlayer = player; }
	
	bool IsSticky() const { return this->m_isSticky; }
	
private:
	bool m_isSticky = false;                   // +0x36c
	// 370 int [init 0]
	COutputEvent m_outputOnSentryGunDestroyed; // +0x374
	CHandle<CTFPlayer> m_hOwnerPlayer;         // +0x38c
};
// TODO: remove offsets


#endif
