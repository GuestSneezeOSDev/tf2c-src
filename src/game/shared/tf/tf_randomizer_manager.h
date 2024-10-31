//=============================================================================//
//
// Purpose: Manages certain aspects of the Randomizer Mode.
//
//=============================================================================//

#ifndef RANDOMIZERMANAGER_H
#define RANDOMIZERMANAGER_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFRandomizerManagerProxy C_TFRandomizerManagerProxy
#endif

#include "igamesystem.h"
#include "tf_weaponbase.h"

enum ETFRandomizerFlags
{
	TF_RANDOMIZER_OFF			= 0,

	TF_RANDOMIZER_CLASSES		= ( 1 << 0 ),
	TF_RANDOMIZER_ITEMS			= ( 1 << 1 ),
	TF_RANDOMIZER_ATTRIBUTES	= ( 1 << 2 ),
};

#ifdef GAME_DLL
class CTFRandomizerAttribute
{
public:
	CTFRandomizerAttribute();

	// Attribute IDs
	virtual void	SetAttributeID( int iAttributeID )	{ m_iAttributeID = iAttributeID; };
	virtual int		GetAttributeID( void )				{ return m_iAttributeID; };

	// Chance
	virtual void	SetValueChance( float flMin, float flMax )	{ m_flValueChance[0] = flMin; m_flValueChance[1] = flMax; };
	virtual float	GetRandomValue( void )						{ return RandomFloat( m_flValueChance[0], m_flValueChance[1] ); }

	//// Blacklists/Whitelists
	// Item IDs
	virtual bool	HasItemBlacklists( void )			{ return ( m_vecBlacklistedItems.Count() != 0 ); }
	virtual void	AddBlacklistedItem( int iItemID )	{ m_vecBlacklistedItems.AddToTail( iItemID ); };
	virtual bool	IsBlacklistedItem( int iItemID );

	virtual bool	HasItemWhitelists( void )			{ return ( m_vecWhitelistedItems.Count() != 0 ); }
	virtual void	AddWhitelistedItem( int iItemID )	{ m_vecWhitelistedItems.AddToTail( iItemID ); };
	virtual bool	IsWhitelistedItem( int iItemID );

	// Entity Names
	virtual bool	HasEntityBlacklists( void )						{ return ( m_vecBlacklistedEntities.Count() != 0 ); }
	virtual void	AddBlacklistedEntity( const char *pszEntity )	{ m_vecBlacklistedEntities.AddToTail( pszEntity ); };
	virtual bool	IsBlacklistedEntity( const char *pszEntity );

	virtual bool	HasEntityWhitelists( void )						{ return ( m_vecWhitelistedItems.Count() != 0 ); }
	virtual void	AddWhitelistedEntity( const char *pszEntity )	{ m_vecWhitelistedEntities.AddToTail( pszEntity ); };
	virtual bool	IsWhitelistedEntity( const char *pszEntity );

	// Projectiles
	virtual bool	HasProjectileBlacklists( void )				{ return ( m_vecBlacklistedProjectiles.Count() != 0 ); }
	virtual void	AddBlacklistedProjectile( int iProjectile )	{ m_vecBlacklistedProjectiles.AddToTail( iProjectile ); };
	virtual bool	IsBlacklistedProjectile( int iProjectile );

	virtual bool	HasProjectileWhitelists( void )				{ return ( m_vecWhitelistedProjectiles.Count() != 0 ); }
	virtual void	AddWhitelistedProjectile( int iProjectile )	{ m_vecWhitelistedProjectiles.AddToTail( iProjectile ); };
	virtual bool	IsWhitelistedProjectile( int iProjectile );

	//// Requirements
	virtual void	AddRequirement( const char *pszRequirement )	{ m_vecRequirements.AddToTail( pszRequirement ); };
	virtual bool	HasRequirement( const char *pszRequirement );

private:
	int		m_iAttributeID;
	float	m_flValueChance[2];

	CUtlVector<int>				m_vecBlacklistedItems;
	CUtlVector<int>				m_vecWhitelistedItems;

	CUtlVector<const char *>	m_vecBlacklistedEntities;
	CUtlVector<const char *>	m_vecWhitelistedEntities;

	CUtlVector<int>				m_vecBlacklistedProjectiles;
	CUtlVector<int>				m_vecWhitelistedProjectiles;

	CUtlVector<const char *>	m_vecRequirements;

};
#endif

class CTFRandomizerManagerProxy : public CBaseEntity
{
public:
	DECLARE_CLASS( CTFRandomizerManagerProxy, CBaseEntity );
	DECLARE_NETWORKCLASS();

	CTFRandomizerManagerProxy();
	~CTFRandomizerManagerProxy();

	// Don't carry these across a transition, they are recreated.
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	// ALWAYS transmit to all clients.
	virtual int UpdateTransmitState( void );

	// CTFRandomizerManagerProxy chains its NetworkStateChanged calls to here, since this
	// is the actual entity that will send the data.
	static void NotifyNetworkStateChanged();

private:
	static CTFRandomizerManagerProxy *s_pRandomizerManagerProxy;

};

// Client specific.
#ifdef CLIENT_DLL
EXTERN_RECV_TABLE( DT_TFRandomizerManager );
// Server specific.
#else
EXTERN_SEND_TABLE( DT_TFRandomizerManager );
#endif

class CTFRandomizerManager : public CAutoGameSystem
{
public:
	DECLARE_CLASS_GAMEROOT( CTFRandomizerManager, CAutoGameSystem );

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE();
#else
	DECLARE_SERVERCLASS_NOBASE();
#endif

	CTFRandomizerManager( const char *pszName );
	~CTFRandomizerManager();

	// IGameSystem
	virtual bool	Init( void );
	virtual void	Shutdown( void );

	virtual void	LevelInitPreEntity( void );
	virtual void	LevelShutdownPostEntity( void );

	// This function is here for our CNetworkVars.
	inline void NetworkStateChanged()
	{
		// Forward the call to the entity that will send the data.
		CTFRandomizerManagerProxy::NotifyNetworkStateChanged();
	}

	inline void NetworkStateChanged( void *pVar )
	{
		// Forward the call to the entity that will send the data.
		CTFRandomizerManagerProxy::NotifyNetworkStateChanged();
	}

	void			Reset( void );

#ifdef GAME_DLL
	void			ApplyRandomizerAttributes( CEconItemView *pItem, ETFLoadoutSlot iSlot, int iClass );
	float			OverrideValue( int iAttributeID, float flValue, CEconItemView *pItem, CTFWeaponInfo *pWeaponInfo );
#endif

	int				RandomizerFlags( void )			{ return m_iRandomizerFlags; }
	bool			RandomizerMode( int iMode )		{ return !!( m_iRandomizerFlags & iMode ); }
#ifdef GAME_DLL
	void			SetRandomizerMode( int iMode );
#endif

	bool			RandomizerWasEverOn( void ) { return m_bRandomizerWasOn; }

#ifdef GAME_DLL
	bool			ParseRandomizerAttributes( void );
	void			ClearRandomizerAttributes( void );
#endif

private:
#ifdef GAME_DLL
	int		m_iMaxRandomizerAttributes;
	int		m_iMaxRandomizerRetries;
#endif

	CNetworkVar( int, m_iRandomizerFlags );
	CNetworkVar( bool, m_bRandomizerWasOn );

#ifdef GAME_DLL
	CUtlVector<CTFRandomizerAttribute *>	m_vecAttributes;
#endif

};

CTFRandomizerManager *GetRandomizerManager();
#endif // RANDOMIZERMANAGER_H
