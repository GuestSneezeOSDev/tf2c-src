#include "cbase.h"
class CSpawnPoint;

class CSpawnManager : public CBaseEntity
{
public:
	DECLARE_CLASS(CSpawnManager, CBaseEntity);
	DECLARE_DATADESC();

	void	Spawn( void );
	void	addToSpawnPointList( CSpawnPoint *spawnPoint );
	void	Activate( void );
	void	Think( void );
	void	entityKilled( CSpawnPoint *spawnPoint );
	
private:
	CUtlVector<CSpawnPoint *> m_hSpawnPoints;
	
	string_t m_szEntityName;
	int m_iEntityCount;
	int m_iActiveEntityCount;
	int m_iLastKilledIndex;

	int m_iRespawnTime;
	float m_flRemainingRespawnTime;

	

	bool m_bDropToGround;
	bool m_bRandomRotation;
};

class CSpawnPoint : public CBaseEntity, public IEntityListener
{
public:
	DECLARE_CLASS( CSpawnPoint, CBaseEntity );
	DECLARE_DATADESC();

	void	Spawn( void );
	void	spawnEntity( const char* entName, bool m_bDropToFloor, bool m_bRandomRotation );
	void	OnEntityDeleted( CBaseEntity *pOther );
	void	UpdateOnRemove( void );

private:
	CHandle<CSpawnManager>	m_hSpawnManager;
	string_t	m_szManagerName;
	
public:
	CBaseEntity *pEntity;
};
