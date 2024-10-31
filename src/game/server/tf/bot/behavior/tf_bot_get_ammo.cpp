/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_get_ammo.h"
#include "tf_nav_mesh.h"
#include "tf_gamerules.h"
#include "entity_ammopack.h"
#include "tf_ammo_pack.h"
#include "tf_obj_dispenser.h"
#include "func_regenerate.h"


static ConVar tf_bot_ammo_search_range    ("tf_bot_ammo_search_range",     "5000", FCVAR_CHEAT, "How far bots will search to find ammo around them");
static ConVar tf_bot_debug_ammo_scavanging("tf_bot_debug_ammo_scavanging",    "0", FCVAR_CHEAT);


class CAmmoFilter : public INextBotFilter
{
public:
	CAmmoFilter(CTFBot *actor) : m_Actor(actor) {}
	
	virtual bool IsSelected(const CBaseEntity *ent) const override
	{
		CTFNavArea *area = TheTFNavMesh->GetNearestTFNavArea(ent->WorldSpaceCenter());
		if (area == nullptr) return false;
		
		CClosestTFPlayer functor(ent->WorldSpaceCenter());
		ForEachPlayer(functor);
		
		bool enemy_nearby = (functor.GetPlayer() != nullptr && this->m_Actor->IsEnemy(functor.GetPlayer()));
		if (enemy_nearby) return false;

		int type = CTFBotGetAmmo::DetermineAmmoSourceType( const_cast<CBaseEntity *>( ent ) );
		if ( type == CTFBotGetAmmo::TFBOT_AMMOSOURCE_INVALID ) return false;

		return CTFBotGetAmmo::IsValidAmmoSource( this->m_Actor, const_cast<CBaseEntity *>( ent ), type );
	}
	
private:
	CTFBot *m_Actor;
};


CHandle<CBaseEntity> CTFBotGetAmmo::s_possibleAmmo;
CTFBot              *CTFBotGetAmmo::s_possibleBot   = nullptr;
int                  CTFBotGetAmmo::s_possibleFrame = -1;


ActionResult<CTFBot> CTFBotGetAmmo::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	VPROF_BUDGET("CTFBotGetAmmo::OnStart", "NextBot");
	
	this->m_PathFollower.Initialize(actor);
	this->m_ChasePath.Initialize( actor );
	
	if (s_possibleFrame != gpGlobals->framecount || s_possibleBot != actor) {
		if (!IsPossible(actor) || s_possibleAmmo == nullptr) {
			Done("Can't get ammo");
		}
	}
	
	this->m_hAmmo = s_possibleAmmo;
	this->m_AmmoSourceType = DetermineAmmoSourceType( m_hAmmo );
	this->m_bNearDispenser = false;
	
	if (!this->m_PathFollower.Compute(actor, this->m_hAmmo->WorldSpaceCenter(), CTFBotPathCost(actor, FASTEST_ROUTE))) {
		Done("No path to ammo!");
	}
	
	if (actor->IsPlayerClass(TF_CLASS_SPY, true) && !actor->m_Shared.IsStealthed()) {
		actor->PressAltFireButton();
	}
	
	Continue();
}

ActionResult<CTFBot> CTFBotGetAmmo::Update(CTFBot *actor, float dt)
{
	if (actor->IsAmmoFull()) {
		Done("My ammo is full");
	}
	
	if (this->m_hAmmo == nullptr) {
		Done("Ammo I was going for is gone");
	}

	if ( !IsValidAmmoSource( actor, this->m_hAmmo, this->m_AmmoSourceType ) )
	{
		Done("Ammo I was going for has become invalid");
	}
	
	if ( this->m_AmmoSourceType == TFBOT_AMMOSOURCE_DISPENSER ) {

		if ( static_cast<CObjectDispenser *>( this->m_hAmmo.Get() )->IsHealingTarget( actor ) )
		{
			this->m_bNearDispenser = true;
		}
		else if ( this->m_bNearDispenser )
		{
			this->m_bNearDispenser = false;

			if ( !this->m_PathFollower.Compute( actor, this->m_hAmmo->WorldSpaceCenter(), CTFBotPathCost( actor, FASTEST_ROUTE ) ) )
			{
				Done( "No path to dispenser!" );
			}
		}

		const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( !actor->IsAmmoLow() && threat != nullptr && threat->IsVisibleInFOVNow() ) {
			Done( "No time to wait for more ammo, I must fight" );
		}
	}
	
	if ( this->m_AmmoSourceType == TFBOT_AMMOSOURCE_DROPPEDPACK )
	{
		// Dropped packs move around so chase them instead.
		this->m_ChasePath.Update( actor, this->m_hAmmo, CTFBotPathCost( actor, FASTEST_ROUTE ) );

		if ( !this->m_ChasePath.IsValid() )
		{
			Done( "My path became invalid" );
		}
	}
	else if ( !this->m_bNearDispenser ) // If the dispenser is healing us, stop.
	{
		if ( !this->m_PathFollower.IsValid() ) {
			Done( "My path became invalid" );
		}

		this->m_PathFollower.Update( actor );
	}

	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	actor->EquipBestWeaponForThreat( threat );
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotGetAmmo::OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail)
{
	Done("Failed to reach ammo", SEV_CRITICAL);
}

EventDesiredResult<CTFBot> CTFBotGetAmmo::OnStuck(CTFBot *actor)
{
	Done("Stuck trying to reach ammo", SEV_CRITICAL);
}


bool CTFBotGetAmmo::IsPossible(CTFBot *actor)
{
	VPROF_BUDGET("CTFBotGetAmmo::IsPossible", "NextBot");
	
	CAmmoFilter filter(actor);

	CUtlVector<CHandle<CBaseEntity>> potential_ammo_ents;
	for (auto ent : CAmmoPack::AutoList())
		potential_ammo_ents.AddToTail(ent);
	for (auto ent : CTFAmmoPack::AutoList())
		potential_ammo_ents.AddToTail(ent);
	for (auto ent : CObjectDispenser::AutoList())
		potential_ammo_ents.AddToTail(ent);
	for (auto ent : CRegenerateZone::AutoList())
		potential_ammo_ents.AddToTail(ent);
	
	CUtlVector<CHandle<CBaseEntity>> ammo_ents;
	actor->SelectReachableObjects(potential_ammo_ents, &ammo_ents, filter, actor->GetLastKnownTFArea(), tf_bot_ammo_search_range.GetFloat());

	if ( ammo_ents.IsEmpty() )
	{
		if ( actor->IsDebugging( INextBot::DEBUG_BEHAVIOR ) ) {
			Warning( "%3.2f: No ammo nearby\n", gpGlobals->curtime );
		}

		return false;
	}

	if ( tf_bot_debug_ammo_scavanging.GetBool() )
	{
		for ( auto ent : ammo_ents )
		{
			if ( !ent )
				continue;

			if ( ent->ClassMatches( "tf_ammo_pack" ) )
			{
				NDebugOverlay::Cross3D( ent->WorldSpaceCenter(), 5.0f, NB_RGB_ORANGE_64, true, 10.0f );
			}
			else
			{
				NDebugOverlay::Cross3D( ent->WorldSpaceCenter(), 5.0f, NB_RGB_YELLOW, true, 10.0f );
			}
		}
	}
	
	s_possibleBot = actor;
	s_possibleAmmo = ammo_ents[0];
	s_possibleFrame = gpGlobals->framecount;

	return true;
}


bool CTFBotGetAmmo::IsValidAmmoSource( CTFBot *actor, CBaseEntity *ent, int type )
{
	switch ( type )
	{
	case TFBOT_AMMOSOURCE_AMMOPACK:
	{
		CAmmoPack *ammo = assert_cast<CAmmoPack *>( ent );
		return ammo->ValidTouch( actor );
	}
	case TFBOT_AMMOSOURCE_DROPPEDPACK:
	{
		CTFAmmoPack *pack = assert_cast<CTFAmmoPack *>( ent );
		return !pack->IsActuallyHealth();
	}
	case TFBOT_AMMOSOURCE_DISPENSER:
	{
		CObjectDispenser *dispenser = assert_cast<CObjectDispenser *>( ent );

		if ( !actor->IsFriend( dispenser ) ) return false;

		if ( actor->IsPlayerClass( TF_CLASS_ENGINEER, true ) ) {
			if ( !dispenser->IsCartDispenser() && actor->GetObjectOfType( OBJ_SENTRYGUN ) != nullptr ) {
				return false;
			}

			if ( dispenser->GetAvailableMetal() <= 0 ) return false;
		}

		if ( dispenser->IsPlacing() )      return false;
		if ( dispenser->IsDisabled() )     return false;
		if ( dispenser->IsBuilding() )     return false;

		return true;
	}
	case TFBOT_AMMOSOURCE_RESUPPLY:
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

int CTFBotGetAmmo::DetermineAmmoSourceType( CBaseEntity *ent )
{
	if ( ent->ClassMatches( "item_ammopack*" ) )
	{
		return TFBOT_AMMOSOURCE_AMMOPACK;
	}
	else if ( ent->ClassMatches( "tf_ammo_pack" ) )
	{
		return TFBOT_AMMOSOURCE_DROPPEDPACK;
	}
	else if ( ent->IsBaseObject() && static_cast<const CBaseObject *>( ent )->IsDispenser() )
	{
		return TFBOT_AMMOSOURCE_DISPENSER;
	}
	else if ( ent->ClassMatches( "func_regenerate" ) )
	{
		return TFBOT_AMMOSOURCE_RESUPPLY;
	}

	return TFBOT_AMMOSOURCE_INVALID;
}
