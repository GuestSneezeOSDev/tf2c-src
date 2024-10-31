/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_use_item.h"
#include "tf_weapon_umbrella.h"
#include "tf_weapon_taser.h"

extern ConVar tf_bot_health_ok_ratio;

extern ConVar tf2c_vip_abilities;


#pragma message("TODO: see email sent to TF team regarding CTFBotUseItem and take the advice I gave into account here")


ActionResult<CTFBot> CTFBotUseItem::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_bReady  = false;
	this->m_bPushed = false;
	
	if (this->m_hItem == nullptr) {
		Done("Item handle is invalid");
	}
	
	if (!this->CanUseItem()) {
		Done(CFmtStr("Don't know how to use this type of item: %s", WeaponIdToAlias(this->m_hItem->GetWeaponID())));
	}

	m_flStartTime = gpGlobals->curtime;
	
	this->PushItem();
	
	Continue();
}

ActionResult<CTFBot> CTFBotUseItem::Update(CTFBot *actor, float dt)
{
	if (this->m_hItem == nullptr) {
		Done("Item handle became invalid");
	}

	if (this->m_hTarget == nullptr)
	{
		Done("Target Handle became invalid");
	}

	if (!this->m_bReady) {
		if (this->IsItemReady()) {
			this->m_bReady = true;
		} else {
			Continue();
		}
	}
	
	if (this->IsDoneUsingItem()) {
		Done("Item used");
	}

	if ( gpGlobals->curtime > m_flStartTime + 5.0f )
	{
		Done( "Took too long to use item" );
	}
	
	this->UseItem();
	
	Continue();
}

void CTFBotUseItem::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	this->PopItem();
}

ActionResult<CTFBot> CTFBotUseItem::OnSuspend(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO: special handling maybe?
	Continue();
}

ActionResult<CTFBot> CTFBotUseItem::OnResume(CTFBot *actor, Action<CTFBot> *action)
{
	// TODO: special handling maybe?
	Continue();
}


bool CTFBotUseItem::CanUseItem() const
{
	// TODO: handle buff items (if they ever exist)
	if ( this->m_hItem->IsWeapon( TF_WEAPON_LUNCHBOX ) ) return true;
	if ( this->m_hItem->IsWeapon( TF_WEAPON_UMBRELLA ) ) return true;
	if ( this->m_hItem->IsWeapon( TF_WEAPON_TASER ) ) return true;
	
	return false;
}


bool CTFBotUseItem::IsItemReady() const
{
	if (this->m_hItem->IsWeapon(TF_WEAPON_LUNCHBOX)) {
		if (!this->GetActor()->IsActiveTFWeapon(TF_WEAPON_LUNCHBOX))   return false;
		if (gpGlobals->curtime < this->m_hItem->m_flNextPrimaryAttack) return false;
		if (this->GetActor()->IsTaunting())                            return false;
		
		return true;
	}

	if ( this->m_hItem->IsWeapon( TF_WEAPON_UMBRELLA ) && tf2c_vip_abilities.GetInt() > 1 ) {
		if ( !this->GetActor()->IsActiveTFWeapon( TF_WEAPON_UMBRELLA ) ) return false;

		auto umbrella = dynamic_cast<CTFUmbrella *>( this->GetActor()->GetTFWeapon_Melee() );
		if ( umbrella == nullptr ) return false;
		if ( gpGlobals->curtime < umbrella->GetNextBoostAttack() ) return false;

		return true;
	}

	if ( this->m_hItem->IsWeapon( TF_WEAPON_TASER ) ) {
		if ( this->GetActor()->IsTaunting() )                            return false;

		auto taser = dynamic_cast<CTFTaser *>( this->GetActor()->GetTFWeapon_Melee() );
		if ( taser == nullptr ) return false;
		if ( taser->GetProgress() < 1.0f ) return false;

		return true;
	}
	
	return true;
}

void CTFBotUseItem::UseItem() const
{
	if (this->m_hItem->IsWeapon(TF_WEAPON_LUNCHBOX)) {
		this->GetActor()->PressFireButton();
	}

	if ( this->m_hItem->IsWeapon( TF_WEAPON_UMBRELLA ) ) {
		this->GetActor()->GetBodyInterface()->AimHeadTowards( m_hTarget, IBody::PRI_CRITICAL, 0.5f, nullptr, "Aiming at Umbrella target" );
		this->GetActor()->PressAltFireButton();
	}

	if ( this->m_hItem->IsWeapon( TF_WEAPON_TASER ) ) {
		this->GetActor()->GetBodyInterface()->AimHeadTowards( m_hTarget, IBody::PRI_CRITICAL, 0.5f, nullptr, "Aiming at Taser target" );
		this->GetActor()->PressFireButton();
	}
}

bool CTFBotUseItem::IsDoneUsingItem() const
{
	if (this->m_hItem->IsWeapon(TF_WEAPON_LUNCHBOX)) {
		if ( this->GetActor()->HealthFraction() >= tf_bot_health_ok_ratio.GetFloat() ) return true;
		return this->GetActor()->IsTaunting();
	}

	if ( this->m_hItem->IsWeapon( TF_WEAPON_UMBRELLA ) ) 
	{
		if ( !this->GetActor()->IsLineOfFireClear( m_hTarget ) ) return true;

		auto umbrella = dynamic_cast<CTFUmbrella *>( this->GetActor()->GetTFWeapon_Melee() );
		if ( umbrella == nullptr ) return true;
		return ( gpGlobals->curtime < umbrella->GetNextBoostAttack() );
	}

	if ( this->m_hItem->IsWeapon( TF_WEAPON_TASER ) )
	{
		auto taser = dynamic_cast<CTFTaser *>( this->GetActor()->GetTFWeapon_Melee() );
		if ( taser == nullptr ) return true;

		if ( this->GetActor()->GetDistanceBetween( m_hTarget ) > taser->GetSwingRange() ) return true;

		return ( taser->GetProgress() < 1.0f );
	}
	
	return true;
}


void CTFBotUseItem::PushItem()
{
	Assert(!this->m_bPushed);
	
	this->GetActor()->PushRequiredWeapon(this->m_hItem);
	this->m_bPushed = true;
}

void CTFBotUseItem::PopItem()
{
	if (this->m_bPushed) {
		this->m_bPushed = false;
		this->GetActor()->PopRequiredWeapon();
	}
}
