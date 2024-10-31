/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_use_teleporter.h"
#include "tf_obj_teleporter.h"
#include "tf_nav_mesh.h"

class CSelectWaitAreaForTele : public ISearchSurroundingAreasFunctor
{
public:
	CSelectWaitAreaForTele( CTFNavArea *point_center_area, CUtlVector<CTFNavArea *> *p_areas, int team ) :
		m_PointCenterArea( point_center_area ), m_Areas( p_areas ), m_iTeam( team ) {}

	virtual bool operator()( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar ) override
	{
		if ( travelDistanceSoFar < 64.0f )
			return true;

		auto tf_area = static_cast<CTFNavArea *>( area );

		if ( area->IsPotentiallyVisible( this->m_PointCenterArea ) && ( this->m_PointCenterArea->GetCenter().z - area->GetCenter().z ) < 220.0f ) {
			this->m_Areas->AddToTail( tf_area );
		}

		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar ) override
	{
		if ( travelDistanceSoFar > 256.0f ) return false;

		if ( adjArea->IsBlocked( this->m_iTeam ) ) return false;

		float delta_z = currentArea->ComputeAdjacentConnectionHeightChange( adjArea );
		return ( abs( delta_z ) < 65.0f );
	}

private:
	CTFNavArea *m_PointCenterArea;
	CUtlVector<CTFNavArea *> *m_Areas;
	int m_iTeam;
};

CTFBotUseTeleporter::CTFBotUseTeleporter( CObjectTeleporter *tele, UseHowType how, const PathFollower *path ) :
m_hTele( tele ),
m_How( how )
{
	// instead of storing a ptr to the original path (BAD), just cache the info we want to retain
	if ( path != nullptr && path->IsValid() ) {
		this->m_OriginalPath_GoalArea = TheTFNavMesh->GetTFNavArea( path->GetEndPosition() );
		this->m_OriginalPath_GoalDist = path->GetLength() - path->GetCursorPosition();
	}
}

CTFBotUseTeleporter::~CTFBotUseTeleporter()
{
}

ActionResult<CTFBot> CTFBotUseTeleporter::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	this->m_ctFindNearbyTele.Start( 2.0f );
	this->m_hTele->AddPlayerToQueue( actor );
	
	Continue();
}

void CTFBotUseTeleporter::OnEnd( CTFBot *actor, Action<CTFBot> *action )
{
	if ( this->m_hTele )
	{
		this->m_hTele->RemovePlayerFromQueue( actor );
	}

	m_PathFollower.Invalidate();
}

ActionResult<CTFBot> CTFBotUseTeleporter::Update(CTFBot *actor, float dt)
{
	CObjectTeleporter *entrance = this->m_hTele;
	if (entrance == nullptr) {
		Done("Teleporter is gone");
	}
	
	CObjectTeleporter *exit = entrance->GetMatchingTeleporter();
	if (exit == nullptr) {
		Done("Missing teleporter exit");
	}
	
	// TODO: double-check the flow control of this block
	if (entrance->IsSendingPlayer(actor)) {
		this->m_bSending = true;
		Continue();
	} else {
		if (this->m_bSending) {
			if (actor->IsRangeLessThan(exit, 25.0f)) {
				actor->UpdateLastKnownArea();
				Done("Successful teleport");
			}
		}
		else if ( !this->IsTeleporterAvailable() && this->m_ctFindNearbyTele.IsElapsed() )
		{
			// If the tele is recharging periodically check if another tele is available.
			this->m_ctFindNearbyTele.Start( RandomFloat( 1.0f, 2.0f ) );

			CObjectTeleporter *new_entrance = FindNearbyTeleporterInternal( actor, m_How, m_OriginalPath_GoalArea, m_OriginalPath_GoalDist, m_hTele );

			if ( new_entrance != nullptr )
			{
				if ( new_entrance != entrance )
				{
					entrance->RemovePlayerFromQueue( actor );
					new_entrance->AddPlayerToQueue( actor );
					this->m_hTele = new_entrance;
					entrance = new_entrance;
					this->m_ctRecomputePath.Invalidate();
				}
			}
			else
			{
				Done( "No teleporters available anymore" );
			}
		}
	}

	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat != nullptr && threat->IsVisibleRecently() ) {
		actor->EquipBestWeaponForThreat( threat );
	}

	// Wait for my turn.
	if ( !entrance->IsFirstInQueue( actor ) )
	{
		if ( this->m_ctCheckQueue.IsElapsed() || this->m_WaitingArea == nullptr )
		{
			if ( IsFasterToWalkToExit( actor, entrance, true ) )
			{
				Done( "Screw this, I'll just walk" );
			}

			this->m_ctCheckQueue.Start( RandomFloat( 1.0f, 2.0f ) );
		}

		// Wait for the other guy to teleport.
		if ( this->m_WaitingArea == nullptr )
		{
			this->m_WaitingArea = SelectAreaToWaitAt( actor );

			if ( this->m_WaitingArea )
			{
				this->m_PathFollower.Compute( actor, this->m_WaitingArea->GetCenter(), CTFBotPathCost( actor, DEFAULT_ROUTE ) );
			}
		}

		if ( this->m_WaitingArea && actor->GetLastKnownTFArea() != this->m_WaitingArea )
		{
			this->m_PathFollower.Update( actor );
		}

		if ( this->m_ctLookAtTele.IsElapsed() )
		{
			this->m_ctLookAtTele.Start( RandomFloat( 3.0f, 8.0f ) );
			actor->GetBodyInterface()->AimHeadTowards( entrance, IBody::PRI_INTERESTING, 1.0f, nullptr, "Look at the teleporter" );
		}

		Continue();
	}

	this->m_WaitingArea = nullptr;
	
	if (this->m_ctRecomputePath.IsElapsed()) {
		this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
		
		if (!this->m_PathFollower.Compute(actor, entrance->GetAbsOrigin(), CTFBotPathCost(actor, FASTEST_ROUTE))) {
			Done("Can't reach teleporter!");
		}
	}
	
	if (actor->GetLocomotionInterface()->GetGround() != entrance) {
		this->m_PathFollower.Update(actor);
	}
	
	Continue();
}

CTFNavArea *CTFBotUseTeleporter::SelectAreaToWaitAt( CTFBot *actor )
{
	CObjectTeleporter *entrance = this->m_hTele;

	CTFNavArea *tele_area = entrance->GetLastKnownTFArea();
	if ( tele_area == nullptr )
		return nullptr;

	CUtlVector<CTFNavArea *> areas;
	CSelectWaitAreaForTele functor( tele_area, &areas, actor->GetTeamNumber() );
	SearchSurroundingAreas( tele_area, functor );

	if ( !areas.IsEmpty() ) {
		return areas.Random();
	}
	else {
		return nullptr;
	}
}

bool CTFBotUseTeleporter::IsTeleporterAvailable() const
{
	if (this->m_hTele == nullptr)                            return false;
	if (!this->m_hTele->IsReady())                           return false;
	if (this->m_hTele->GetState() != TELEPORTER_STATE_READY) return false;
	
	return true;
}

class CTeleFilter : public INextBotFilter
{
public:
	CTeleFilter( CTFBot *actor, CTFNavArea *path_goalarea, float path_goaldist, CObjectTeleporter *current_tele ) :
		m_Actor( actor ),
		m_PathGoalArea( path_goalarea ),
		m_PathGoalDist( path_goaldist ),
		m_CurrentTele( current_tele )
	{}

	virtual bool IsSelected( const CBaseEntity *ent ) const override
	{
		auto obj = assert_cast<CBaseObject *>( const_cast<CBaseEntity *>( ent ) );
		if ( !obj->IsTeleEntrance() ) return false;

		auto tele = assert_cast<CObjectTeleporter *>( obj );
		if ( !tele->IsReady() ) return false;

		auto tele_exit = tele->GetMatchingTeleporter();

		if ( tele != m_CurrentTele )
		{
			tele->UpdateLastKnownArea();
			CTFNavArea *tele_area = tele->GetLastKnownTFArea();
			if ( tele_area == nullptr ) return false;

			CTFNavArea *actor_area = m_Actor->GetLastKnownTFArea();
			if ( actor_area == nullptr ) return false;

			float actor_incdist = actor_area->GetIncursionDistance( m_Actor->GetTeamNumber() );
			float tele_incdist = tele_area->GetIncursionDistance( m_Actor->GetTeamNumber() );

			if ( tele_incdist + 350.0f <= actor_incdist )  return false;

			// Figure out if this teleporter actually puts us closer to the goal.

			if ( m_PathGoalArea == nullptr ) return false;
			if ( m_PathGoalDist == 0.0f )    return false;

			tele_exit->UpdateLastKnownArea();
			CTFNavArea *tele_exit_area = tele_exit->GetLastKnownTFArea();
			if ( tele_exit_area == nullptr ) return false;

			// FIXME: This check is broken by SeekAndDestroy behavior which makes the bot go to the enemy spawn room which they can't reach.
			float goalDistFromExit = NavAreaTravelDistance( tele_exit_area, m_PathGoalArea, CTFBotPathCost( m_Actor, FASTEST_ROUTE ), 0.0f, m_Actor->GetTeamNumber() );
			if ( goalDistFromExit == -1.0f )         return false;
			if ( goalDistFromExit > m_PathGoalDist ) return false;
		}

		return ( CTFBotUseTeleporter::IsFasterToWalkToExit( m_Actor, tele, tele == m_CurrentTele ) == false );
	}

private:
	CTFBot *m_Actor;
	CTFNavArea *m_PathGoalArea;
	float       m_PathGoalDist;
	CObjectTeleporter *m_CurrentTele;
};

static int TeleportersSort( const EHANDLE *p1, const EHANDLE *p2 )
{
	float time1 = static_cast<CObjectTeleporter *>( p1->Get() )->GetLastCalculatedWaitTime();
	float time2 = static_cast<CObjectTeleporter *>( p2->Get() )->GetLastCalculatedWaitTime();

	if ( time1 > time2 )
		return 1;

	if ( time1 < time2 )
		return -1;

	return 0;
}

// ONLY call this from OUTSIDE the class!
CObjectTeleporter *CTFBotUseTeleporter::FindNearbyTeleporter( CTFBot *actor, UseHowType how, const PathFollower *path )
{
	CTFNavArea *goal_area = nullptr;
	float       goal_dist = 0.0f;

	if ( path != nullptr && path->IsValid() ) {
		goal_area = TheTFNavMesh->GetTFNavArea( path->GetEndPosition() );
		goal_dist = path->GetLength() - path->GetCursorPosition();
	}

	return FindNearbyTeleporterInternal( actor, how, goal_area, goal_dist, nullptr );
}

CObjectTeleporter *CTFBotUseTeleporter::FindNearbyTeleporterInternal( CTFBot *actor, UseHowType how, CTFNavArea *path_goalarea, float path_goaldist, CObjectTeleporter *current_tele )
{
	// Don't take teles if we're not actually going anywhere.
	if ( how != HOW_MEDIC && path_goalarea == nullptr )
		return nullptr;

	CUtlVector<EHANDLE> objs;
	CUtlVector<EHANDLE> teles;
	for ( auto obj : CBaseObject::AutoList() ) {
		if ( actor->GetTeamNumber() == obj->GetTeamNumber() )
			objs.AddToTail( obj );
	}
	
	CTeleFilter filter( actor, path_goalarea, path_goaldist, current_tele );
	actor->SelectReachableObjects( objs, &teles, filter, actor->GetLastKnownTFArea(), 1000.0f );

	if ( teles.IsEmpty() )
		return nullptr;

	// Use the teleporter with the smallest wait time.
	teles.Sort( TeleportersSort );
	return static_cast<CObjectTeleporter *>( teles[0].Get() );
}

bool CTFBotUseTeleporter::IsFasterToWalkToExit( CTFBot *actor, CObjectTeleporter *tele, bool alreadyInQueue )
{
	CObjectTeleporter *tele_exit = tele->GetMatchingTeleporter();
	tele_exit->UpdateLastKnownArea();

	// Figure out if running to the exit would take less time than waiting in queue.
	float travelDist = NavAreaTravelDistance( actor->GetLastKnownArea(), tele_exit->GetLastKnownArea(), CTFBotPathCost( actor, FASTEST_ROUTE ), 0.0f, actor->GetTeamNumber() );
	if ( travelDist == -1.0f )
	{
		// Exit is unreachable, it's probably a trap so ignore.
		return true;
	}

	float travelTime = travelDist / actor->GetLocomotionInterface()->GetRunSpeed();
	tele->CalcWaitTimeForPlayer( actor, alreadyInQueue );

	return ( tele->GetLastCalculatedWaitTime() >= travelTime );
}
