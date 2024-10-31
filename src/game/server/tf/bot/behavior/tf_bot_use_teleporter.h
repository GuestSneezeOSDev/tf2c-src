/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_USE_TELEPORTER_H
#define TF_BOT_USE_TELEPORTER_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_bot.h"
#include "NextBotPathFollow.h"


class CObjectTeleporter;


class CTFBotUseTeleporter final : public Action<CTFBot>
{
public:	
	enum UseHowType
	{
		HOW_NORMAL, // from CTFBotTacticalMonitor::Update
		HOW_MEDIC,  // from CTFBotMedicHeal::Update
	};

	CTFBotUseTeleporter(CObjectTeleporter *tele, UseHowType how, const PathFollower *path = nullptr);
	virtual ~CTFBotUseTeleporter();
	
	virtual const char *GetName() const override { return "UseTeleporter"; }
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
	virtual void OnEnd( CTFBot *actor, Action<CTFBot> *action ) override;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;

	static CObjectTeleporter *FindNearbyTeleporter( CTFBot *actor, UseHowType how, const PathFollower *path );
	static bool IsFasterToWalkToExit( CTFBot *actor, CObjectTeleporter *tele, bool alreadyInQueue );
	
private:
	static CObjectTeleporter *FindNearbyTeleporterInternal(CTFBot *actor, UseHowType how, CTFNavArea *path_goalarea, float path_goaldist, CObjectTeleporter *current_tele);

	bool IsTeleporterAvailable() const;
	CTFNavArea *SelectAreaToWaitAt( CTFBot *actor );
	
	CHandle<CObjectTeleporter> m_hTele; // +0x0034
	UseHowType m_How;                   // +0x0038
	CTFNavArea *m_OriginalPath_GoalArea = nullptr;
	float       m_OriginalPath_GoalDist =    0.0f;
	PathFollower m_PathFollower;        // +0x003c
	CountdownTimer m_ctRecomputePath;   // +0x4810
	bool m_bSending = false;            // +0x481c
	CTFNavArea *m_WaitingArea = nullptr;
	CountdownTimer m_ctFindNearbyTele;
	CountdownTimer m_ctGoCommand;
	CountdownTimer m_ctLookAtTele;
	CountdownTimer m_ctCheckQueue;
};


#endif
