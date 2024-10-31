/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_HINT_H
#define TF_HINT_H
#ifdef _WIN32
#pragma once
#endif


class CTFBot;


class CTFBotHint : public CBaseEntity, public TAutoList<CTFBotHint>
{
public:
	CTFBotHint() {}
	virtual ~CTFBotHint() {}
	
	DECLARE_CLASS(CTFBotHint, CBaseEntity);
	DECLARE_DATADESC();
	
	void InputEnable(inputdata_t& inputdata);
	void InputDisable(inputdata_t& inputdata);
	
	virtual void Spawn() override;
	virtual void UpdateOnRemove() override;
	
	bool IsFor(CTFBot *bot) const;
	void UpdateNavDecoration();
	
	bool IsSniperSpot() const { return (this->m_hint == SNIPER_SPOT); }
	bool IsSentrySpot() const { return (this->m_hint == SENTRY_SPOT); }
	
private:
	enum // m_hint
	{
		SNIPER_SPOT = 0,
		SENTRY_SPOT = 1,
	};
	
	int m_team;
	int m_hint;
	bool m_isDisabled = false;
};


#endif
