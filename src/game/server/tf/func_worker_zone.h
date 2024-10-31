#include "cbase.h"
#include "triggers.h"
#include "tf_player.h"
#include "entity_capture_flag.h"

#ifdef TF_INFILTRATION

class CWorkerZone : public CBaseTrigger , public TAutoList<CWorkerZone>
{
public:
	DECLARE_CLASS( CWorkerZone, CBaseTrigger );
	DECLARE_DATADESC();

	CWorkerZone();
	void Spawn();
	void StartTouch( CBaseEntity* pOther );
	void AddWorker() { iWorkers++; DevMsg( "Workers %i\n", iWorkers ); };
	void RemoveWorker() { iWorkers--; DevMsg( "Workers %i\n", iWorkers ); };
	int GetActiveWorkers() { return iWorkers; };

private:
	int iWorkers;
};

class CWorker : public CCaptureFlag
{
public:
	DECLARE_CLASS( CWorker, CCaptureFlag );
	DECLARE_DATADESC();

	CWorker();
	void Spawn();
	void ChangeHome( Vector m_vecResetPos );
	void Capture( CTFPlayer *pPlayer, CWorkerZone *zone );
	void PickUp( CTFPlayer *pPlayer, bool bInvisible );
	void Reset( void );
	void ThinkDropped();
	void SetWorkZone( CWorkerZone *zone ) { workZone = zone; }
	void MoveThink();
	void StartMoving();
private:
	CWorkerZone *workZone;	
	void MoveForward( float factor );
	float m_flNextMoveTime;
};
#endif
