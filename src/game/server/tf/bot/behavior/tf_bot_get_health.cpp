/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_get_health.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "entity_healthkit.h"
#include "tf_ammo_pack.h"
#include "tf_obj_dispenser.h"
#include "func_regenerate.h"


       ConVar tf_bot_health_critical_ratio   ("tf_bot_health_critical_ratio",     "0.3", FCVAR_CHEAT);
       ConVar tf_bot_health_ok_ratio         ("tf_bot_health_ok_ratio",           "0.8", FCVAR_CHEAT);
static ConVar tf_bot_health_search_near_range("tf_bot_health_search_near_range", "1000", FCVAR_CHEAT);
static ConVar tf_bot_health_search_far_range ("tf_bot_health_search_far_range",  "2000", FCVAR_CHEAT);


class CHealthFilter : public INextBotFilter
{
public:
	CHealthFilter(CTFBot *actor) : m_Actor(actor) {}
	
	virtual bool IsSelected(const CBaseEntity *ent) const override
	{
		CTFNavArea *area = TheTFNavMesh->GetNearestTFNavArea(ent->WorldSpaceCenter());
		if (area == nullptr) return false;
		
		CClosestTFPlayer functor(ent->WorldSpaceCenter());
		ForEachPlayer(functor);
		
		bool enemy_nearby = (functor.GetPlayer() != nullptr && this->m_Actor->IsEnemy(functor.GetPlayer()));
		if (enemy_nearby) return false;
		
		int type = CTFBotGetHealth::DetermineHealthSourceType( const_cast<CBaseEntity *>( ent ) );
		if ( type == CTFBotGetHealth::TFBOT_HEALTHSOURCE_INVALID ) return false;

		return CTFBotGetHealth::IsValidHealthSource( this->m_Actor, const_cast<CBaseEntity *>( ent ), type );
	}
	
private:
	CTFBot *m_Actor;
};


CHandle<CBaseEntity> CTFBotGetHealth::s_possibleHealth;
CTFBot              *CTFBotGetHealth::s_possibleBot   = nullptr;
int                  CTFBotGetHealth::s_possibleFrame = -1;


ActionResult<CTFBot> CTFBotGetHealth::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotGetHealth::OnStart", "NextBot");
	
	this->m_PathFollower.Initialize(actor);
	
	if (s_possibleFrame != gpGlobals->framecount || s_possibleBot != actor) {
		if (!IsPossible(actor) || s_possibleHealth == nullptr) {
			Done("Can't get health");
		}
	}
	
	this->m_hHealth = s_possibleHealth;
	this->m_HealthSourceType = DetermineHealthSourceType(m_hHealth);
	this->m_bNearDispenser = false;
	
	if (!this->m_PathFollower.Compute(actor, this->m_hHealth->WorldSpaceCenter(), CTFBotPathCost(actor, SAFEST_ROUTE))) {
		Done("No path to health!");
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY, true) && !actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotGetHealth::Update(CTFBot *actor, float dt)
{
	if (actor->GetHealth() >= actor->GetMaxHealth()) {
		Done("I've been healed");
	}
	
	if (this->m_hHealth == nullptr) {
		CallMedic( actor );
		Done("Health source I was going for is gone");
	}

	if ( IsValidHealthSource( actor, this->m_hHealth, this->m_HealthSourceType ) == false )
	{
		CallMedic( actor );
		Done( "Health source I was going for has become invalid" );
	}
	
	for (int i = 0; i < actor->m_Shared.GetNumHealers(); ++i) {
		if (!actor->m_Shared.HealerIsDispenser(i)) {
			Done("A Medic is healing me");
		}
	}
	
	/* healing from dispenser */
	if (actor->m_Shared.GetNumHealers() != 0) {
		const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
		if (threat != nullptr && threat->IsVisibleInFOVNow()) {
			Done("No time to wait for health, I must fight");
		}
	}
	
	CClosestTFPlayer functor(this->m_hHealth->WorldSpaceCenter());
	ForEachPlayer(functor);
	
	if (functor.GetPlayer() != nullptr && actor->IsEnemy(functor.GetPlayer())) {
		CallMedic( actor );
		Done("An enemy is closer to it");
	}
	
	// TODO: handle other scoping-capable weapons
	if (actor->IsSniperRifle() && actor->m_Shared.InCond(TF_COND_ZOOMED)) {
		actor->PressAltFireButton();
	}

	if ( this->m_HealthSourceType == TFBOT_HEALTHSOURCE_DISPENSER )
	{
		if ( static_cast<CObjectDispenser *>( this->m_hHealth.Get() )->IsHealingTarget( actor ) )
		{
			m_bNearDispenser = true;
		}
		else if ( m_bNearDispenser )
		{
			m_bNearDispenser = false;

			if ( !this->m_PathFollower.Compute( actor, this->m_hHealth->WorldSpaceCenter(), CTFBotPathCost( actor, FASTEST_ROUTE ) ) )
			{
				Done( "No path to dispenser!" );
			}
		}
	}

	// If the dispenser is healing us, stop.
	if ( !m_bNearDispenser )
	{
		if ( !this->m_PathFollower.IsValid() ) {
			Done( "My path became invalid" );
		}

		this->m_PathFollower.Update( actor );
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	actor->EquipBestWeaponForThreat(threat);
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotGetHealth::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	Done("Failed to reach health kit", SEV_CRITICAL);
}

EventDesiredResult<CTFBot> CTFBotGetHealth::OnStuck(CTFBot *actor)
{
	Done("Stuck trying to reach health kit", SEV_CRITICAL);
}


bool CTFBotGetHealth::IsPossible(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotGetHealth::IsPossible", "NextBot");
	
	if (actor->m_Shared.GetNumHealers() > 0 || TFGameRules()->IsMannVsMachineMode()) {
		return false;
	}
	
	float range_near = tf_bot_health_search_near_range.GetFloat();
	float range_far  = tf_bot_health_search_far_range .GetFloat();
	
	float level_ok   = tf_bot_health_ok_ratio      .GetFloat();
	float level_crit = tf_bot_health_critical_ratio.GetFloat();
	
	float ratio = RemapValClamped(actor->HealthFraction(), level_crit, level_ok, 0.0f, 1.0f);
	
	// TODO: add bleeding, etc (anything that is "cured" or shortened by health packs and/or dispensers)
	float max_range;
	if (actor->m_Shared.InCond(TF_COND_BURNING)) {
		max_range = range_far;
	} else {
		max_range = range_far - (ratio * (range_far - range_near));
	}
	
	CHealthFilter filter(actor);
	
	CUtlVector<CHandle<CBaseEntity>> potential_health_ents;
	for (auto ent : CHealthKit::AutoList())
		potential_health_ents.AddToTail(ent);
	for (auto ent : CTFAmmoPack::AutoList())
		potential_health_ents.AddToTail(ent);
	for (auto ent : CObjectDispenser::AutoList())
		potential_health_ents.AddToTail(ent);
	for (auto ent : CRegenerateZone::AutoList())
		potential_health_ents.AddToTail(ent);
	
	CUtlVector<CHandle<CBaseEntity>> health_ents;
	actor->SelectReachableObjects(potential_health_ents, &health_ents, filter, actor->GetLastKnownTFArea(), max_range);
	
	if (health_ents.IsEmpty()) {
		if (actor->IsDebugging(INextBot::DEBUG_BEHAVIOR)) {
			Warning("%3.2f: No health nearby\n", gpGlobals->curtime);
		}
		
		return false;
	}

	s_possibleBot = actor;
	s_possibleHealth = health_ents[0];
	s_possibleFrame = gpGlobals->framecount;

	return true;
}


bool CTFBotGetHealth::IsValidHealthSource( CTFBot *actor, CBaseEntity *ent, int type )
{
	switch ( type )
	{
	case TFBOT_HEALTHSOURCE_HEALTHKIT:
	{
		CHealthKit *health = assert_cast<CHealthKit *>( ent );
		return health->ValidTouch( actor );
	}
	case TFBOT_HEALTHSOURCE_DROPPEDPACK:
	{
		CTFAmmoPack *pack = assert_cast<CTFAmmoPack *>( ent );
		return pack->IsActuallyHealth();
	}
	case TFBOT_HEALTHSOURCE_DISPENSER:
	{
		CObjectDispenser *dispenser = assert_cast<CObjectDispenser *>( ent );

		if ( !actor->IsFriend( dispenser ) )	return false;
		if ( dispenser->IsPlacing() )			return false;
		if ( dispenser->IsDisabled() )			return false;
		if ( dispenser->IsBuilding() )			return false;

		return true;
	}
	case TFBOT_HEALTHSOURCE_RESUPPLY:
	{
		CTFNavArea *area = TheTFNavMesh->GetNearestTFNavArea( ent->WorldSpaceCenter() );
		if ( area == nullptr ) return false;

		return !area->HasEnemySpawnRoom( actor );
	}
	default:
		Assert( false );
		return false;
	}
}

int CTFBotGetHealth::DetermineHealthSourceType( CBaseEntity *ent )
{
	if ( ent->ClassMatches( "item_healthkit*" ) )
	{
		return TFBOT_HEALTHSOURCE_HEALTHKIT;
	}
	else if ( ent->ClassMatches( "tf_ammo_pack" ) )
	{
		return TFBOT_HEALTHSOURCE_DROPPEDPACK;
	}
	else if ( ent->IsBaseObject() && static_cast<const CBaseObject *>( ent )->IsDispenser() )
	{
		return TFBOT_HEALTHSOURCE_DISPENSER;
	}
	else if ( ent->ClassMatches( "func_regenerate" ) )
	{
		return TFBOT_HEALTHSOURCE_RESUPPLY;
	}

	return TFBOT_HEALTHSOURCE_INVALID;
}

void CTFBotGetHealth::CallMedic( CTFBot *actor )
{
	TFGameRules()->VoiceCommand( actor, 0, 0 );
}
