/* NextBotPlayer
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef NEXTBOT_NEXTBOTPLAYER_NEXTBOTPLAYER_H
#define NEXTBOT_NEXTBOTPLAYER_NEXTBOTPLAYER_H
#ifdef _WIN32
#pragma once
#endif


#include "NextBotPlayerInput.h"
#include "NextBotInterface.h"
#include "in_buttons.h"
#include "gameinterface.h"
#include "vprof.h"
#include "fmtstr.h"


#define PRESS_BUTTON(name, bit) \
	this->m_nBotButtons |= (bit); \
	this->m_ct##name.Start(duration)
#define RELEASE_BUTTON(name, bit) \
	this->m_nBotButtons &= ~(bit); \
	this->m_ct##name.Invalidate()


template<typename PLAYER>
class NextBotPlayer : public PLAYER, public INextBot, public INextBotPlayerInput
{
public:
	NextBotPlayer() {}
	virtual ~NextBotPlayer() {}
	
	/* PLAYER overrides */
	virtual void Spawn() override;
	virtual void Event_Killed(const CTakeDamageInfo& info) override;
	virtual INextBot *MyNextBotPointer() override { return this; }
	virtual bool IsNetClient() const override     { return false; }
	virtual void Touch(CBaseEntity *pOther) override;
	virtual void PhysicsSimulate() override;
	virtual void HandleAnimEvent(animevent_t *pEvent) override;
	virtual void Weapon_Equip(CBaseCombatWeapon *pWeapon) override;
	virtual void Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pVecTarget = nullptr, const Vector *pVelocity = nullptr) override;
	virtual int OnTakeDamage_Alive(const CTakeDamageInfo& info) override;
	virtual int OnTakeDamage_Dying(const CTakeDamageInfo& info) override;
	virtual void OnNavAreaChanged(CNavArea *enteredArea, CNavArea *leftArea) override;
	virtual bool IsFakeClient() const override { return true; }
	virtual CBaseEntity *EntSelectSpawnPoint() override;
	virtual bool IsBot() const override { return true; }
	
	/* INextBot overrides */
	virtual void Update() override;
	virtual bool IsRemovedOnReset() const override           { return false; }
	virtual CBaseCombatCharacter *GetEntity() const override { return const_cast<NextBotPlayer<PLAYER> *>(this); }
	virtual void RemoveEntity() override                     { this->Kick(); }
	
	/* INextBotPlayerInput overrides */
	virtual void PressFireButton(float duration = -1.0f) override        {   PRESS_BUTTON(Fire,        IN_ATTACK); }
	virtual void ReleaseFireButton() override                            { RELEASE_BUTTON(Fire,        IN_ATTACK); }
	virtual void PressAltFireButton(float duration = -1.0f) override     { this->PressMeleeButton(duration); }
	virtual void ReleaseAltFireButton() override                         { this->ReleaseMeleeButton(); }
	virtual void PressMeleeButton(float duration = -1.0f) override       {   PRESS_BUTTON(AltFire,     IN_ATTACK2); }
	virtual void ReleaseMeleeButton() override                           { RELEASE_BUTTON(AltFire,     IN_ATTACK2); }
	virtual void PressSpecialFireButton(float duration = -1.0f) override {   PRESS_BUTTON(SpecialFire, IN_ATTACK3); }
	virtual void ReleaseSpecialFireButton() override                     { RELEASE_BUTTON(SpecialFire, IN_ATTACK3); }
	virtual void PressUseButton(float duration = -1.0f) override         {   PRESS_BUTTON(Use,         IN_USE); }
	virtual void ReleaseUseButton() override                             { RELEASE_BUTTON(Use,         IN_USE); }
	virtual void PressReloadButton(float duration = -1.0f) override      {   PRESS_BUTTON(Reload,      IN_RELOAD); }
	virtual void ReleaseReloadButton() override                          { RELEASE_BUTTON(Reload,      IN_RELOAD); }
	virtual void PressForwardButton(float duration = -1.0f) override     {   PRESS_BUTTON(Forward,     IN_FORWARD); }
	virtual void ReleaseForwardButton() override                         { RELEASE_BUTTON(Forward,     IN_FORWARD); }
	virtual void PressBackwardButton(float duration = -1.0f) override    {   PRESS_BUTTON(Backward,    IN_BACK); }
	virtual void ReleaseBackwardButton() override                        { RELEASE_BUTTON(Backward,    IN_BACK); }
	virtual void PressLeftButton(float duration = -1.0f) override        {   PRESS_BUTTON(Left,        IN_MOVELEFT); }
	virtual void ReleaseLeftButton() override                            { RELEASE_BUTTON(Left,        IN_MOVELEFT); }
	virtual void PressRightButton(float duration = -1.0f) override       {   PRESS_BUTTON(Right,       IN_MOVERIGHT); }
	virtual void ReleaseRightButton() override                           { RELEASE_BUTTON(Right,       IN_MOVERIGHT); }
	virtual void PressJumpButton(float duration = -1.0f) override        {   PRESS_BUTTON(Jump,        IN_JUMP); }
	virtual void ReleaseJumpButton() override                            { RELEASE_BUTTON(Jump,        IN_JUMP); }
	virtual void PressCrouchButton(float duration = -1.0f) override      {   PRESS_BUTTON(Crouch,      IN_DUCK); }
	virtual void ReleaseCrouchButton() override                          { RELEASE_BUTTON(Crouch,      IN_DUCK); }
	virtual void PressWalkButton(float duration = -1.0f) override        {   PRESS_BUTTON(Walk,        IN_SPEED); }
	virtual void ReleaseWalkButton() override                            { RELEASE_BUTTON(Walk,        IN_SPEED); }
	virtual void SetButtonScale(float fwd, float side) override;
	
	virtual void SetSpawnPoint(CBaseEntity *ent)                     { this->m_hSpawnPoint = ent; }
	virtual bool IsDormantWhenDead() const                           { return true; }
	virtual void OnMainActivityComplete(Activity a1, Activity a2)    { this->OnAnimationActivityComplete(a2); }
	virtual void OnMainActivityInterrupted(Activity a1, Activity a2) { this->OnAnimationActivityInterrupted(a2); }
	virtual void AvoidPlayers(CUserCmd *usercmd)                     {}
	
	void Kick();
	
	float GetDistanceBetween(CBaseEntity *ent) const { return ent->GetAbsOrigin().DistTo(this->GetAbsOrigin()); }
	
private:
	int m_nBotButtons;
	int m_nBotButtonsOld;
	CountdownTimer m_ctFire;
	CountdownTimer m_ctAltFire;
	CountdownTimer m_ctSpecialFire;
	CountdownTimer m_ctUse;
	CountdownTimer m_ctReload;
	CountdownTimer m_ctForward;
	CountdownTimer m_ctBackward;
	CountdownTimer m_ctLeft;
	CountdownTimer m_ctRight;
	CountdownTimer m_ctJump;
	CountdownTimer m_ctCrouch;
	CountdownTimer m_ctWalk;
	CountdownTimer m_ctScale;
	IntervalTimer  m_itBurning;
	float m_flBtnScaleFwd;
	float m_flBtnScaleSide;
	CHandle<CBaseEntity> m_hSpawnPoint;
};


template<typename PLAYER> inline void NextBotPlayer<PLAYER>::Spawn()
{
	engine->SetFakeClientConVarValue(this->edict(), "cl_autohelp", "0");
	
	this->m_ctFire       .Invalidate();
	this->m_ctAltFire    .Invalidate();
	this->m_ctSpecialFire.Invalidate();
	this->m_ctUse        .Invalidate();
	this->m_ctReload     .Invalidate();
	this->m_ctForward    .Invalidate();
	this->m_ctBackward   .Invalidate();
	this->m_ctLeft       .Invalidate();
	this->m_ctRight      .Invalidate();
	this->m_ctJump       .Invalidate();
	this->m_ctCrouch     .Invalidate();
	this->m_ctWalk       .Invalidate();
	this->m_ctScale      .Invalidate();
	this->m_itBurning    .Invalidate();
	
	this->m_nBotButtons    = 0;
	this->m_nBotButtonsOld = 0;
	
	this->m_flBtnScaleFwd  = 0.04f;
	this->m_flBtnScaleSide = 0.04f;
	
	INextBot::Reset();
	PLAYER::Spawn();
}

template<typename PLAYER> inline void NextBotPlayer<PLAYER>::Event_Killed(const CTakeDamageInfo& info)
{
	INextBot::OnKilled(info);
	PLAYER::Event_Killed(info);
}

template<typename PLAYER> inline void NextBotPlayer<PLAYER>::Touch(CBaseEntity *pOther)
{
	if (INextBot::ShouldTouch(pOther)) {
		/* implicit operator assignment */
		trace_t tr;
		tr = CBaseEntity::GetTouchTrace();
		
		INextBot::OnContact(pOther, &tr);
	}
	
	PLAYER::Touch(pOther);
}

template<typename PLAYER> inline void NextBotPlayer<PLAYER>::PhysicsSimulate()
{
	VPROF("NextBotPlayer::PhysicsSimulate");
	
	extern ConVar NextBotStop;
	if (!engine->IsPaused() && !(this->IsDormantWhenDead() && this->IsDead()) && !NextBotStop.GetBool()) {
		int buttons;
		
		if (INextBot::BeginUpdate()) {
			INextBot::Update();
			
			if (!this->m_ctFire       .IsElapsed()) this->m_nBotButtons |= IN_ATTACK;
			if (!this->m_ctAltFire    .IsElapsed()) this->m_nBotButtons |= IN_ATTACK2;
			if (!this->m_ctSpecialFire.IsElapsed()) this->m_nBotButtons |= IN_ATTACK3;
			if (!this->m_ctUse        .IsElapsed()) this->m_nBotButtons |= IN_USE;
			if (!this->m_ctReload     .IsElapsed()) this->m_nBotButtons |= IN_RELOAD;
			if (!this->m_ctForward    .IsElapsed()) this->m_nBotButtons |= IN_FORWARD;
			if (!this->m_ctBackward   .IsElapsed()) this->m_nBotButtons |= IN_BACK;
			if (!this->m_ctLeft       .IsElapsed()) this->m_nBotButtons |= IN_MOVELEFT;
			if (!this->m_ctRight      .IsElapsed()) this->m_nBotButtons |= IN_MOVERIGHT;
			if (!this->m_ctJump       .IsElapsed()) this->m_nBotButtons |= IN_JUMP;
			if (!this->m_ctCrouch     .IsElapsed()) this->m_nBotButtons |= IN_DUCK;
			if (!this->m_ctWalk       .IsElapsed()) this->m_nBotButtons |= IN_SPEED;
			
			buttons = this->m_nBotButtons;
			this->m_nBotButtonsOld = buttons;
			
			INextBot::EndUpdate();
		} else {
			this->GetBodyInterface()->Update();
			buttons = this->m_nBotButtonsOld | this->m_nBotButtons;
		}
		
		ILocomotion *loco = this->GetLocomotionInterface();
		
		if (this->GetBodyInterface()->IsActualPosture(IBody::POSTURE_CROUCH)) {
			buttons |= IN_DUCK;
		}
		
		float upmove = 0.0f;
		if ((this->m_nBotButtons & IN_JUMP) != 0) {
			upmove = loco->GetRunSpeed();
		}
		
		float forwardmove = 0.0f;
		if ((buttons & IN_FORWARD) != 0) {
			forwardmove = loco->GetRunSpeed();
		} else if ((buttons & IN_BACK) != 0) {
			forwardmove = -loco->GetRunSpeed();
		}
		
		float sidemove = 0.0f;
		if ((buttons & IN_MOVELEFT) != 0) {
			sidemove = -loco->GetRunSpeed();
		} else if ((buttons & IN_MOVERIGHT) != 0) {
			sidemove = loco->GetRunSpeed();
		}
		
		extern ConVar NextBotPlayerWalk;
		if (NextBotPlayerWalk.GetBool()) {
			buttons |= IN_SPEED;
		}
		
		extern ConVar NextBotPlayerCrouch;
		if (NextBotPlayerCrouch.GetBool()) {
			buttons |= IN_DUCK;
		}
		
		// BUG: this doesn't set negative values for back/left movement
		// BUG: also it doesn't set zero if not moving
		if (!this->m_ctScale.IsElapsed()) {
			forwardmove = loco->GetRunSpeed() * this->m_flBtnScaleFwd;
			sidemove    = loco->GetRunSpeed() * this->m_flBtnScaleSide;
		}
		
		extern ConVar NextBotPlayerMove;
		if (!NextBotPlayerMove.GetBool()) {
			buttons &= ~(IN_JUMP | IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT);
			
			upmove      = 0.0f;
			sidemove    = 0.0f;
			forwardmove = 0.0f;
		}
		
		CUserCmd usercmd;
		usercmd.command_number = gpGlobals->tickcount;
		usercmd.viewangles     = this->EyeAngles();
		usercmd.forwardmove    = forwardmove;
		usercmd.sidemove       = sidemove;
		usercmd.upmove         = upmove;
		usercmd.buttons        = buttons;
		usercmd.random_seed    = random->RandomInt(0, INT_MAX);
		
		this->AvoidPlayers(&usercmd);
		PLAYER::ProcessUsercmds(&usercmd, 1, 1, 0, false);
		
		this->m_nBotButtons = 0;
	}
	
	PLAYER::PhysicsSimulate();
}

template<typename PLAYER> inline void NextBotPlayer<PLAYER>::HandleAnimEvent(animevent_t *pEvent)
{
	INextBot::OnAnimationEvent(pEvent);
	PLAYER::HandleAnimEvent(pEvent);
}

template<typename PLAYER> inline void NextBotPlayer<PLAYER>::Weapon_Equip(CBaseCombatWeapon *pWeapon)
{
	INextBot::OnPickUp(pWeapon, nullptr);
	PLAYER::Weapon_Equip(pWeapon);
}

template<typename PLAYER> inline void NextBotPlayer<PLAYER>::Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pVecTarget, const Vector *pVelocity)
{
	INextBot::OnDrop(pWeapon);
	PLAYER::Weapon_Drop(pWeapon, pVecTarget, pVelocity);
}

template<typename PLAYER> inline int NextBotPlayer<PLAYER>::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	if ((info.GetDamageType() & DMG_BURN) != 0) {
		if (!this->m_itBurning.HasStarted() || this->m_itBurning.IsGreaterThan(1.0f)) {
			INextBot::OnIgnite();
			this->m_itBurning.Start();
		}
	}
	
	INextBot::OnInjured(info);
	return PLAYER::OnTakeDamage_Alive(info);
}

template<typename PLAYER> inline int NextBotPlayer<PLAYER>::OnTakeDamage_Dying(const CTakeDamageInfo& info)
{
	if ((info.GetDamageType() & DMG_BURN) != 0) {
		if (!this->m_itBurning.HasStarted() || this->m_itBurning.IsGreaterThan(1.0f)) {
			INextBot::OnIgnite();
			this->m_itBurning.Start();
		}
	}
	
	INextBot::OnInjured(info);
	return PLAYER::OnTakeDamage_Dying(info);
}

template<typename PLAYER> inline void NextBotPlayer<PLAYER>::OnNavAreaChanged(CNavArea *enteredArea, CNavArea *leftArea)
{
	FOR_EACH_RESPONDER(OnNavAreaChanged, enteredArea, leftArea);
	PLAYER::OnNavAreaChanged(enteredArea, leftArea);
}

template<typename PLAYER> inline CBaseEntity *NextBotPlayer<PLAYER>::EntSelectSpawnPoint()
{
	if (this->m_hSpawnPoint != nullptr) {
		return this->m_hSpawnPoint;
	}
	
	return PLAYER::EntSelectSpawnPoint();
}


template<typename PLAYER> inline void NextBotPlayer<PLAYER>::Update()
{
	if (!this->IsAlive() && this->IsDormantWhenDead()) return;
	
	extern ConVar NextBotPlayerStop;
	if (NextBotPlayerStop.GetBool()) return;
	
	INextBot::Update();
}


template<typename PLAYER> inline void NextBotPlayer<PLAYER>::SetButtonScale(float fwd, float side)
{
	this->m_flBtnScaleFwd  = fwd;
	this->m_flBtnScaleSide = side;
	
	this->m_ctScale.Start(0.001f);
}


template<typename PLAYER> inline void NextBotPlayer<PLAYER>::Kick()
{
	/* ideally we'd do this in a manner not involving strings, but that's essentially unviable */
	engine->ServerCommand(CFmtStr("kickid %d\n", this->GetUserID()));
}


template<typename BOT> BOT *NextBotCreatePlayerBot(const char *name, bool fake_client = true)
{
	ClientPutInServerOverride(&BOT::AllocatePlayerEntity);
	edict_t *client = engine->CreateFakeClientEx(name, fake_client);
	ClientPutInServerOverride(nullptr);
	
	if (client == nullptr) {
		Msg("CreatePlayerBot: Unable to create bot %s - CreateFakeClient() returned NULL.\n", name);
		return nullptr;
	}
	
	auto bot = dynamic_cast<BOT *>(CBaseEntity::Instance(client));
	if (bot == nullptr) {
		Error("CreatePlayerBot: Could not Instance() from the bot edict.\n");
		return nullptr;
	}
	
	bot->SetPlayerName(name);
	bot->ClearFlags();
	bot->AddFlag(FL_CLIENT | FL_FAKECLIENT);
	
	return bot;
}


#endif
