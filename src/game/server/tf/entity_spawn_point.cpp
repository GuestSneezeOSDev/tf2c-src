#include "cbase.h"
#include "entity_spawn_point.h"



LINK_ENTITY_TO_CLASS( entity_spawn_manager, CSpawnManager );

BEGIN_DATADESC(CSpawnManager)
DEFINE_KEYFIELD( m_szEntityName, FIELD_STRING, "entity_name" ),
DEFINE_KEYFIELD( m_iEntityCount, FIELD_INTEGER, "entity_count" ),
DEFINE_KEYFIELD( m_iRespawnTime, FIELD_INTEGER, "respawn_time" ),
DEFINE_KEYFIELD( m_bDropToGround, FIELD_BOOLEAN, "drop_to_ground" ),
DEFINE_KEYFIELD( m_bRandomRotation, FIELD_BOOLEAN, "random_rotation" ),
DEFINE_THINKFUNC( Think ),
END_DATADESC()

void CSpawnManager::Spawn( void )
{
	m_iActiveEntityCount = 0;
	m_flRemainingRespawnTime = 0;
	BaseClass::Spawn();
};

void CSpawnManager::addToSpawnPointList( CSpawnPoint *spawnPoint )
{
	m_hSpawnPoints.AddToTail( spawnPoint );
};

void CSpawnManager::Activate()
{
	while ( m_iActiveEntityCount < min( m_iEntityCount, m_hSpawnPoints.Count() ) )
	{
		int i = RandomInt( 0, m_hSpawnPoints.Count() - 1 );
		if ( m_hSpawnPoints[i]->pEntity == NULL )
		{
			m_hSpawnPoints[i]->spawnEntity( STRING( m_szEntityName ), m_bDropToGround, m_bRandomRotation );
			m_iActiveEntityCount++;
		}
	}

	SetThink( &CSpawnManager::Think );
	SetNextThink( gpGlobals->curtime );
	BaseClass::Activate();
};

void CSpawnManager::Think()
{
	if ( gpGlobals->curtime >= m_flRemainingRespawnTime ) 
	{
		while ( m_iActiveEntityCount < min( m_iEntityCount, m_hSpawnPoints.Count() ) )
		{
			int i = RandomInt( 0, m_hSpawnPoints.Count() - 1 );
			if ( m_hSpawnPoints[i]->pEntity == NULL && i != m_iLastKilledIndex )
			{
				m_hSpawnPoints[i]->spawnEntity( STRING( m_szEntityName ), m_bDropToGround, m_bRandomRotation );
				m_iActiveEntityCount++;
				m_flRemainingRespawnTime = gpGlobals->curtime + m_iRespawnTime;
				break;
			}
		}
	}
	SetNextThink( gpGlobals->curtime + 1 );
};

void CSpawnManager::entityKilled( CSpawnPoint *spawnPoint )
{
	m_iLastKilledIndex = m_hSpawnPoints.Find( spawnPoint );

	if ( m_flRemainingRespawnTime != 0 )
		m_flRemainingRespawnTime = min( m_flRemainingRespawnTime, gpGlobals->curtime + m_iRespawnTime );
	else
		m_flRemainingRespawnTime = gpGlobals->curtime + m_iRespawnTime;

	m_iActiveEntityCount--;
};


LINK_ENTITY_TO_CLASS( entity_spawn_point, CSpawnPoint );

BEGIN_DATADESC( CSpawnPoint )
DEFINE_KEYFIELD( m_szManagerName, FIELD_STRING, "spawn_manager_name" ),
END_DATADESC()

void CSpawnPoint::Spawn( void )
{
	BaseClass::Spawn();

	m_hSpawnManager = dynamic_cast<CSpawnManager *>( gEntList.FindEntityByName(NULL, STRING( m_szManagerName ) ) );
	if ( m_hSpawnManager )
	{
		m_hSpawnManager->addToSpawnPointList( this );
	}
	gEntList.AddListenerEntity( this );
};

void CSpawnPoint::spawnEntity( const char* entName, bool m_bDropToFloor, bool m_bRandomRotation )
{
	pEntity = CreateEntityByName( entName );
	pEntity->SetAbsAngles( this->GetAbsAngles() );
	pEntity->SetAbsOrigin( this->GetAbsOrigin() );
	
	if ( m_bRandomRotation )
	{
		pEntity->SetAbsAngles( QAngle( 0, RandomFloat( 0, 360 ), 0 ) );
	}
	
	if ( m_bDropToFloor ) 
	{
		UTIL_DropToFloor( pEntity, MASK_SOLID );
	}
	
	DispatchSpawn( pEntity );
};

void CSpawnPoint::OnEntityDeleted( CBaseEntity *pOther )
{
	if ( m_hSpawnManager && pOther == pEntity )
	{
		m_hSpawnManager->entityKilled( this );
		pEntity = NULL;
	}
};

void CSpawnPoint::UpdateOnRemove( void )
{
	gEntList.RemoveListenerEntity( this );
	BaseClass::UpdateOnRemove();
};