/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_engineer_build_teleport_entrance.h"
#include "tf_bot_engineer_move_to_build.h"
#include "tf_bot_get_ammo.h"
#include "tf_weapon_builder.h"
#include "tf_nav_mesh.h"
#include "func_respawnroom.h"


static ConVar tf_bot_max_teleport_entrance_travel       ("tf_bot_max_teleport_entrance_travel",        "2000", FCVAR_CHEAT, "Don't plant teleport entrances farther than this travel distance from our spawn room");
static ConVar tf_bot_teleport_build_surface_normal_limit("tf_bot_teleport_build_surface_normal_limit", "0.99", FCVAR_CHEAT, "If the ground normal Z component is less that this value, Engineer bots won't place their entrance teleporter");
static ConVar tf_bot_debug_teleporter                   ("tf_bot_debug_teleporter",                       "0", FCVAR_CHEAT );


ActionResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
//	/* loadout sanity check */
//	Assert(actor->GetTFWeapon_Building() != nullptr && typeid(actor->GetTFWeapon_Building()) == typeid(CTFWeaponBuilder));
	Continue();
}

ActionResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::Update(CTFBot *actor, float dt)
{
	if ( TFGameRules()->IsInWaitingForPlayers() || !actor->CanPlayerMove() ) {
		this->m_PathFollower.Invalidate();
		this->m_ctRecomputePath.Start( RandomFloat( 1.0f, 2.0f ) );

		Continue();
	}

	CTFNavArea *lkarea = actor->GetLastKnownTFArea();
	if ( lkarea == nullptr ) {
		ChangeTo( new CTFBotEngineerMoveToBuild(), "No nav mesh for teleporter entrance!" );
	}

	if ( actor->GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE ) != nullptr ) {
		ChangeTo( new CTFBotEngineerMoveToBuild(), "teleporter entrance already built" );
	}
	
	if (lkarea->GetIncursionDistance(actor->GetTeamNumber()) > tf_bot_max_teleport_entrance_travel.GetFloat()) {
		ChangeTo( new CTFBotEngineerMoveToBuild(), "Too far from our spawn room to build teleporter entrance" );
	}

	CTeamControlPoint* point = actor->GetMyControlPoint();
	if ( !point )
		Continue();

	if ( actor->CanBuild( OBJ_TELEPORTER ) == CB_NEED_RESOURCES ) {
		if ( CTFBotGetAmmo::IsPossible( actor ) ) {
			SuspendFor( new CTFBotGetAmmo(), "Need more metal to build teleporter entrance" );
		}
	}
	
	if (!this->m_PathFollower.IsValid()) {
		this->m_PathFollower.Compute( actor, point->WorldSpaceCenter(), CTFBotPathCost( actor, DEFAULT_ROUTE ) );
	}
	
	this->m_PathFollower.Update(actor);
	
	// TODO: maybe do this check via weapon ID rather than dynamic_cast?
	// (see also: CTFBotEngineerBuildTeleportExit::Update)
	auto builder = dynamic_cast<CTFWeaponBuilder *>(actor->GetActiveTFWeapon());
	if (builder != nullptr && builder->IsValidPlacement()) {
		Vector start = actor->WorldSpaceCenter() + (30.0f * EyeVectorsFwd(actor));
		Vector end   = VecPlusZ(start, -200.0f);
		
		trace_t tr;
		UTIL_TraceLine(start, end, MASK_PLAYERSOLID, actor, COLLISION_GROUP_NONE, &tr);
		
		if (tr.DidHit() && tr.plane.normal.z > tf_bot_teleport_build_surface_normal_limit.GetFloat()) {
			actor->PressFireButton();
		}
	} else {
		actor->StartBuildingObjectOfType(OBJ_TELEPORTER, TELEPORTER_TYPE_ENTRANCE);
	}
	
	Continue();
}


EventDesiredResult<CTFBot> CTFBotEngineerBuildTeleportEntrance::OnStuck(CTFBot *actor)
{
	this->m_PathFollower.Invalidate();
	
	Continue();
}
