//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Point entity used to create templates out of other entities or groups of entities
//
//=============================================================================//

#ifndef POINT_TEMPLATE_H
#define POINT_TEMPLATE_H
#ifdef _WIN32
#pragma once
#endif

#define MAX_NUM_TEMPLATES		16

struct template_t
{
	int			iTemplateIndex;
	VMatrix		matEntityToTemplate;

	DECLARE_SIMPLE_DATADESC();
};

struct ScriptTemplate_t
{
	const char *pszTempName;
	HSCRIPT hSpawnTable;
};

void ScriptInstallPreSpawnHook();
bool ScriptPreInstanceSpawn( CScriptScope *pScriptScope, CBaseEntity *pChild, string_t iszKeyValueData );
void ScriptPostSpawn( CScriptScope *pScriptScope, CBaseEntity **ppEntities, int nEntities );

class CPointTemplate : public CLogicalEntity
{
	DECLARE_CLASS( CPointTemplate, CLogicalEntity );
public:
	DECLARE_DATADESC();

	virtual void	Spawn( void );
	virtual void	Precache();

	// Template initialization
	void			StartBuildingTemplates( void );
	void			FinishBuildingTemplates( void );

	// Template Entity accessors
	int				GetNumTemplateEntities( void );
	CBaseEntity		*GetTemplateEntity( int iTemplateNumber );
	void			AddTemplate( CBaseEntity *pEntity, const char *pszMapData, int nLen );
	bool			ShouldRemoveTemplateEntities( void );
	bool			AllowNameFixup();

	// Templates accessors
	int				GetNumTemplates( void );
	int				GetTemplateIndexForTemplate( int iTemplate );

	// Template instancing
	virtual bool	CreateInstance( const Vector &vecOrigin, const QAngle &vecAngles, CUtlVector<CBaseEntity*> *pEntities );
	void			CreationComplete( const CUtlVector<CBaseEntity *> &entities );

	// Inputs
	void			InputForceSpawn( inputdata_t &inputdata );

	virtual void	PerformPrecache();

protected:
	string_t						m_iszTemplateEntityNames[MAX_NUM_TEMPLATES];

	// List of map entities this template targets. Built inside our Spawn().
	// It's only valid between Spawn() & Activate(), because the map entity parsing
	// code removes all the entities in it once it finishes turning them into templates.
	CUtlVector< CBaseEntity * >		m_hTemplateEntities;

	// List of templates, generated from our template entities.
	CUtlVector< template_t >		m_hTemplates;

	COutputEvent					m_pOutputOnSpawned;
};

class CPointScriptTemplate : public CPointTemplate
{
	DECLARE_CLASS( CPointScriptTemplate, CLogicalEntity );
public:
	DECLARE_DATADESC();
	DECLARE_ENT_SCRIPTDESC( CPointScriptTemplate );

	virtual ~CPointScriptTemplate();
	
	virtual void	Spawn( void );

	void			AddTemplate( char const *pszTempName, HSCRIPT hTable );
	void			SetGroupSpawnTables( HSCRIPT hInTable, HSCRIPT hOutTable );

	// Template instancing
	virtual bool	CreateInstance( const Vector &vecOrigin, const QAngle &vecAngles, CUtlVector<CBaseEntity*> *pEntities );

	// Inputs
	void			InputForceSpawn( inputdata_t &inputdata );

private:
	CUtlVector<ScriptTemplate_t> m_hScriptTemplates;

	HSCRIPT unk1;
	HSCRIPT unk2;
};

#endif // POINT_TEMPLATE_H
