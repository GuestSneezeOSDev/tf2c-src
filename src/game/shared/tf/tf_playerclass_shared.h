//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
// Purpose:
//
//=============================================================================
#ifndef TF_PLAYERCLASS_SHARED_H
#define TF_PLAYERCLASS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

#define TF_NAME_LENGTH		128

// Client specific.
#ifdef CLIENT_DLL

EXTERN_RECV_TABLE( DT_TFPlayerClassShared );

// Server specific.
#else

EXTERN_SEND_TABLE( DT_TFPlayerClassShared );

#endif


//-----------------------------------------------------------------------------
// Cache structure for the TF player class data (includes citizen). 
//-----------------------------------------------------------------------------

#define MAX_PLAYERCLASS_SOUND_LENGTH	128

struct TFPlayerClassData_t
{
	char		m_szClassName[TF_NAME_LENGTH];
	char		m_szModelName[TF_NAME_LENGTH];
	char		m_szHWMModelName[TF_NAME_LENGTH];
	char		m_szModelHandsName[TF_NAME_LENGTH];
	char		m_szLocalizableName[TF_NAME_LENGTH];
	float		m_flMaxSpeed;
	float		m_flFlagMaxSpeed;
	int			m_nMaxHealth;
	int			m_nMaxArmor;
	ETFWeaponID	m_aWeapons[TF_PLAYER_WEAPON_COUNT];
	ETFWeaponID	m_aGrenades[TF_PLAYER_GRENADE_COUNT];
	int			m_aAmmoMax[TF_AMMO_COUNT];
	int			m_aBuildable[TF_PLAYER_BUILDABLE_COUNT];

	bool		m_bParsed;

//#ifdef GAME_DLL
	// sounds
	char		m_szDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szCritDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szMeleeDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szExplosionDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
//#endif

	TFPlayerClassData_t();
	const char *GetModelName() const;
	void Parse( const char *pszClassName );

private:

	// Parser for the class data.
	void ParseData( KeyValues *pKeyValuesData );
};

class CTFPlayerClassDataMgr : public CAutoGameSystem
{
public:
	CTFPlayerClassDataMgr();
	virtual bool Init( void );	
	TFPlayerClassData_t *Get( unsigned int iClass );

private:
	TFPlayerClassData_t m_aTFPlayerClassData[TF_CLASS_COUNT_ALL];

};

extern CTFPlayerClassDataMgr *g_pTFPlayerClassDataMgr;

// Legacy.
TFPlayerClassData_t *GetPlayerClassData( unsigned int iClass );

//-----------------------------------------------------------------------------
// TF Player Class Shared
//-----------------------------------------------------------------------------
class CTFPlayerClassShared
{
public:

	CTFPlayerClassShared();

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerClassShared );

	bool		Init( int iClass );
	bool		IsClass( int iClass ) const						{ return ( m_iClass == iClass ); }
	int			GetClassIndex( void ) const						{ return m_iClass; }
	void		Reset( void );

	const char	*GetName( void ) const							{ return GetPlayerClassData( m_iClass )->m_szClassName; }
	const char	*GetModelName( void ) const;
	const char	*GetHandModelName( void ) const					{ return GetPlayerClassData( m_iClass )->m_szModelHandsName; }		
	float		GetMaxSpeed( void ) const						{ return GetPlayerClassData( m_iClass )->m_flMaxSpeed; }
	int			GetMaxHealth( void ) const						{ return GetPlayerClassData( m_iClass )->m_nMaxHealth; }
	int			GetMaxArmor( void ) const						{ return GetPlayerClassData( m_iClass )->m_nMaxArmor; }

#ifdef CLIENT_DLL
	bool		HasCustomModel( void ) const					{ return m_iszCustomModel[0] != '\0'; }
#else
	bool		HasCustomModel( void ) const					{ return ( m_iszCustomModel.Get() != NULL_STRING ); }
#endif

	// SetCustomModel
#ifndef CLIENT_DLL
	#define USE_CLASS_ANIMATIONS true
	void		SetCustomModel( const char *pszModelName, bool isUsingClassAnimations = false );
	void		SetCustomModelOffset( Vector const &vecOffset )	{ m_vecCustomModelOffset = vecOffset; }
	void		SetCustomModelRotates( bool bRotates )			{ m_bCustomModelRotates = bRotates; }
	void		SetCustomModelRotation( QAngle const &vecOffset ){ m_angCustomModelRotation = vecOffset; m_bCustomModelRotationSet = true; }
	void		ClearCustomModelRotation( void )				{ m_bCustomModelRotationSet = false; }
	void		SetCustomModelVisibleToSelf( bool bVisible )	{ m_bCustomModelVisibleToSelf = bVisible; }
#endif

	Vector		GetCustomModelOffset( void ) const				{ return m_vecCustomModelOffset.Get(); }
	QAngle		GetCustomModelRotation( void ) const			{ return m_angCustomModelRotation.Get(); }
	bool		CustomModelRotationSet( void )					{ return m_bCustomModelRotationSet.Get(); }
	bool		CustomModelRotates( void ) const				{ return m_bCustomModelRotates.Get(); }
	bool		CustomModelIsVisibleToSelf( void ) const		{ return m_bCustomModelVisibleToSelf.Get(); }
	bool		CustomModelUsesClassAnimations( void ) const	{ return m_bUseClassAnimations.Get(); }
	bool		CustomModelHasChanged( void );

	TFPlayerClassData_t  *GetData( void ) const					{ return GetPlayerClassData( m_iClass ); }

	// If needed, put this into playerclass scripts
	bool CanBuildObject( int iObjectType ) const;

protected:

	CNetworkVar( int,	m_iClass );

#ifdef CLIENT_DLL
	char		m_iszCustomModel[MAX_PATH];
#else
	CNetworkVar( string_t, m_iszCustomModel );
#endif
	CNetworkVar( Vector, m_vecCustomModelOffset );
	CNetworkVar( QAngle, m_angCustomModelRotation );
	CNetworkVar( bool, m_bCustomModelRotates );
	CNetworkVar( bool, m_bCustomModelRotationSet );
	CNetworkVar( bool, m_bCustomModelVisibleToSelf );
	CNetworkVar( bool, m_bUseClassAnimations );
	CNetworkVar( int,  m_iClassModelParity );
	int			m_iOldClassModelParity;
};

#endif // TF_PLAYERCLASS_SHARED_H