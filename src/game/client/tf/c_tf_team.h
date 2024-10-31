//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: Client side CTFTeam class
//
// $NoKeywords: $
//=============================================================================

#ifndef C_TF_TEAM_H
#define C_TF_TEAM_H
#ifdef _WIN32
#pragma once
#endif

#include "c_team.h"
#include "shareddefs.h"
#include "c_tf_player.h"

class C_BaseEntity;
class C_BaseObject;
class CBaseTechnology;

//-----------------------------------------------------------------------------
// Purpose: TF's Team manager
//-----------------------------------------------------------------------------
class C_TFTeam : public C_Team
{
	DECLARE_CLASS( C_TFTeam, C_Team );
	DECLARE_CLIENTCLASS();

public:

					C_TFTeam();
	virtual			~C_TFTeam();

	C_TFPlayer		*GetVIP( void );
	int				GetVIPIndex( void ) { return m_iVIP; }

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual void	ClientThink( void );

	int				GetFlagCaptures( void ) { return m_nFlagCaptures; }
	int				GetRole( void ) { return m_iRole; }
	bool			IsEscorting( void ) { return m_bEscorting; }
	int				GetRoundScore( void ) { return m_iRoundScore; }
	void			UpdateTeamName( void );
	const wchar_t	*GetTeamName( void ) { return m_wszLocalizedTeamName; }

	int				GetNumObjects( int iObjectType = -1 );
	C_BaseObject	*GetObject( int num );

	CUtlVector< CHandle<C_BaseObject> > m_aObjects;

private:

	int		m_nFlagCaptures;
	int		m_iRole;
	bool	m_bEscorting;
	int		m_iRoundScore;
	int		m_iVIP;
	wchar_t	m_wszLocalizedTeamName[128];
};

C_TFTeam *GetGlobalTFTeam( int iTeamNumber );
const wchar_t *GetLocalizedTeamName( int iTeamNumber );

#endif // C_TF_TEAM_H