/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_TELEPORTER_EXIT_H
#define TF_TELEPORTER_EXIT_H
#ifdef _WIN32
#pragma once
#endif


#include "tf_hint_entity.h"


class CTFBotHintTeleporterExit : public CBaseTFBotHintEntity, public TAutoList<CTFBotHintTeleporterExit>
{
public:
	CTFBotHintTeleporterExit() {}
	virtual ~CTFBotHintTeleporterExit() {}
	
	DECLARE_CLASS(CTFBotHintTeleporterExit, CBaseTFBotHintEntity);
	DECLARE_DATADESC();
	
	virtual HintType GetHintType() const override { return TELEPORTER_EXIT; }
};


#endif
