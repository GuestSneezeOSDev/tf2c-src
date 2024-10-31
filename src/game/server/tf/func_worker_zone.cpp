#include "cbase.h"
#include "func_worker_zone.h"
#include "tf_announcer.h"
#include "tf_team.h"
#include "math.h"

#ifdef TF_INFILTRATION

#define WORKER_INWARDS_MOVE_FACTOR 60
#define MOVE_SPEED_FACTOR 0.5

ConVar tf2c_worker_move_time( "tf2c_worker_move_time", "2", FCVAR_NOTIFY );
ConVar tf2c_worker_think_interval( "tf2c_worker_think_interval", "0.1", FCVAR_NOTIFY );

BEGIN_DATADESC( CWorkerZone )
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_worker_zone, CWorkerZone )

CWorkerZone::CWorkerZone()
{
	iWorkers = 0;
}

void CWorkerZone::Spawn()
{
	InitTrigger();
}

void CWorkerZone::StartTouch( CBaseEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if( !pOther->IsPlayer() )
		return;
	
	CTFPlayer *pPlayer = ToTFPlayer( pOther );
	if( !pPlayer )
		return;

	if( pPlayer->GetTeamNumber() != GetTeamNumber() )
		return;

	CCaptureFlag *pFlag = pPlayer->GetTheFlag();
	if( !pFlag )
		return;

	CWorker *pWorker = dynamic_cast< CWorker *>( pFlag );
	if( !pWorker )
		return;

	pWorker->Capture( pPlayer, this );
}

BEGIN_DATADESC( CWorker )
DEFINE_THINKFUNC( MoveThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( item_worker, CWorker )

CWorker::CWorker()
{
	workZone = NULL;
	m_flNextMoveTime = 0;
}

void CWorker::Spawn()
{
	BaseClass::Spawn();
	
	/*PrecacheModel("models/bots/bot_worker/bot_worker_a.mdl"); //RD robot model for fun
	SetModel("models/bots/bot_worker/bot_worker_a.mdl"); 
	PrecacheScriptSound("Robot.Collide");*/
	SetModel( "models/player/medic.mdl" ); // Temporary for movement animations(scientist doesn't have any).
	SetSolid( SOLID_VPHYSICS );

	RegisterThinkContext( "MoveContext" );
	SetContextThink( &CWorker::MoveThink, gpGlobals->curtime, "MoveContext" );
}

void CWorker::ChangeHome( Vector m_vecResetPos )
{
	this->m_vecResetPos = m_vecResetPos;
}

void CWorker::Capture( CTFPlayer *pPlayer, CWorkerZone *zone )
{
	SetWorkZone( zone );
	UTIL_DropToFloor( this, MASK_SOLID );

	// Calculate the direction of the vector from the worker to the origin of the brush entity.
	Vector dir = (zone->WorldSpaceCenter() - GetAbsOrigin()).Normalized();
	dir.z = 0; // Remove up and down movement.

	this->m_vecResetPos = GetAbsOrigin() + dir * WORKER_INWARDS_MOVE_FACTOR;
	ChangeTeam( pPlayer->GetTeamNumber() );
	
	// Call the CTFItem::Drop function.
	BaseClass::BaseClass::Drop( pPlayer, true );
	// CCaptureFlag::Pickup will setTouch to null when being caried so when we drop it we have to set the touch function again.
	SetTouch( &CCaptureFlag::FlagTouch ); 

	Reset();
	UpdateReturnIcon();

	// Start mvoing around.
	StartMoving();

	// Play captured sounds.
	CTeamRecipientFilter filterFriendly( GetTeamNumber(), true );
	g_TFAnnouncer.Speak( filterFriendly, TF_ANNOUNCER_CTF_ENEMYCAPTURED );

	CTeamRecipientFilter filterEnemy( pPlayer->GetTeamNumber(), true );
	g_TFAnnouncer.Speak( filterEnemy, TF_ANNOUNCER_CTF_TEAMCAPTURED );
}

void CWorker::StartMoving() 
{
	// Set the animation and start moving around.
	SetSequence( LookupSequence( "Run_MELEE" ) ); //for medic model
	//SetSequence( LookupSequence( "idle" ) ); //for RD robot model

	float dir_x = MOVE_SPEED_FACTOR, dir_y = 0.0;
	SetPoseParameter( "move_x", dir_x );
	SetPoseParameter( "move_y", dir_y );

	SetNextThink( gpGlobals->curtime + 0.1, "MoveContext" );
}

void CWorker::MoveForward( float factor )
{
	float move_x = cos( RadianEuler( GetAbsAngles() ).z );
	float move_y = sin( RadianEuler( GetAbsAngles() ).z );

	float speed = GetSequenceGroundSpeed( GetSequence() );
	//float speed = 100; // For RD robot model
	SetMoveType( MOVETYPE_FLYGRAVITY );
	SetAbsVelocity( Vector( speed * move_x * factor, speed * move_y * factor, GetAbsVelocity().z ) );
}

void CWorker::MoveThink()
{
	if( IsHome() && workZone )
	{	
		bool bInWorkZone = workZone->PointIsWithin( this->GetAbsOrigin() );
					
		if( bInWorkZone  )
		{
			if( gpGlobals->curtime >= m_flNextMoveTime )
			{
				// Set random angle and move forward.
				SetAbsAngles( QAngle( 0, RandomAngle( 0, 360 ).y, 0 ) );
				m_flNextMoveTime = gpGlobals->curtime + tf2c_worker_move_time.GetFloat();
			}
		}
		else
		{
			// If we are not in our zone then we have to get back to the zone.
			// We can do this by calculating the angle to the center of the zone.
			Vector dir = ( workZone->WorldSpaceCenter() - GetAbsOrigin() );
			dir.z = 0;
			dir = dir.Normalized(); // This ensures the vector length is one after we removed the z component.

			float y_angle = ( ( acos( dir.x ) * 180.f ) / 3.14159265358979323846f ) * ( dir.y / abs( dir.y ) );
			SetAbsAngles( QAngle( 0, y_angle, 0 ) );
			//EmitSound("Robot.Collide"); //For RD robot model
		}
		
		MoveForward( MOVE_SPEED_FACTOR );
		SetNextThink( gpGlobals->curtime + tf2c_worker_think_interval.GetFloat(), "MoveContext" );
	}
}

void CWorker::Reset( void )
{
	// Resetting after capture or just ressetting in general.
	// Ressetting makes the worker return to it's work zone so we have to add it to the counter again.
	BaseClass::Reset();
	if (workZone)
	{
		workZone->AddWorker();
		StartMoving();
	}
}

void CWorker::ThinkDropped() 
{
	if ( m_flResetTime && gpGlobals->curtime > m_flResetTime || GetTeamNumber() == TEAM_UNASSIGNED)
	{
		// If the scientist is already neutral then there is no need to notify anyone.
		if ( GetTeamNumber() != TEAM_UNASSIGNED )
		{
			// Notify all teams that the scientist is now neutral.
			for ( int iTeam = TF_TEAM_RED; iTeam < GetNumberOfTeams(); ++iTeam )
			{
				CTeamRecipientFilter filter( iTeam, true );
				TFGameRules()->SendHudNotification( filter, "A scientist has become neutral!", "ico_notify_flag_dropped", iTeam );
			}
		}

		// Reset the team to the original team setting (when it spawned).
		ChangeTeam( TEAM_UNASSIGNED );

		// Reset flag timer since it is no longer needed.
		ResetFlagReturnTime();
		UpdateReturnIcon();

		// Set flag status to none so it is no longer in the dropped state and this function won't be called again.
		SetFlagStatus( TF_FLAGINFO_NONE );
		workZone = NULL;

		SetMoveType( MOVETYPE_NONE );
	}
}

void CWorker::PickUp( CTFPlayer *pPlayer, bool bInvisible )
{
	// Following checks are just copied from the ctf flag
	// Is the flag enabled?
	if (IsDisabled())
		return;

	// Check whether we have a weapon that's prohibiting us from picking the flag up
	if (!pPlayer->IsAllowedToPickUpFlag())
		return;

	// Stop it from moving by itself.
	SetMoveType( MOVETYPE_NONE );

	if( workZone && IsHome() )
	{
		workZone->RemoveWorker();
	}
	BaseClass::PickUp( pPlayer, bInvisible );
}

#endif
