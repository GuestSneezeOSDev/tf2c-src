//=============================================================================
//
// Purpose: Stub class for compatibility with item schema
//
//=============================================================================
#ifndef TF_WEARABLE_H
#define TF_WEARABLE_H

#ifdef _WIN32
#pragma once
#endif

#include "econ_wearable.h"

#ifdef CLIENT_DLL
#define CTFWearable C_TFWearable
#define CTFWearableVM C_TFWearableVM
#endif

class CTFWearable : public CEconWearable
{
	DECLARE_CLASS( CTFWearable, CEconWearable );
	DECLARE_NETWORKCLASS();

public:
	CTFWearable();

#ifdef GAME_DLL
	virtual void	Equip( CBasePlayer *pPlayer );
	void			UpdateModelToClass( void );
#else
	virtual int		GetSkin( void );
	virtual int		InternalDrawModel( int flags );
	virtual bool	ShouldDraw();
#endif

	virtual void	ReapplyProvision( void );
	void			Reset();

	void			SetDisguiseWearable( bool bState ) { m_bDisguiseWearable = bState; }
	bool			IsDisguiseWearable( void ) const { return m_bDisguiseWearable; }
	void			SetWeaponAssociatedWith( CBaseEntity *pWeapon ) { m_hWeaponAssociatedWith = pWeapon; }
	CBaseEntity		*GetWeaponAssociatedWith( void ) const { return m_hWeaponAssociatedWith.Get(); }

	void			WearableFrame( void );

	virtual bool	UpdateBodygroups( CBasePlayer *pOwner, bool bForce );

	virtual bool	SecondaryAttack( void );

	bool			UsesProgressBar( void );
	void			WearableEffectThink( void );
	void			StopEffect( void );
	virtual int		GetCount() { return -1; }
	virtual float	GetProgress( void ){ return m_flChargeMeter; }
	const char		*GetEffectLabelText( void )	
	{
#ifdef GAME_DLL
		return UTIL_VarArgs( "#%s", GetItem()->GetStaticData()->item_name);
#else
		return VarArgs( "#%s", GetItem()->GetStaticData()->item_name );
#endif
	}

private:
	CNetworkVar( bool, m_bDisguiseWearable );
	CNetworkVar( float,	m_flChargeMeter );
	CNetworkHandle( CBaseEntity, m_hWeaponAssociatedWith );

};

class CTFWearableVM : public CTFWearable
{
	DECLARE_CLASS( CTFWearableVM, CTFWearable );
public:
	DECLARE_NETWORKCLASS();

	virtual bool	IsViewModelWearable( void ) const { return true; }
};
#endif // TF_WEARABLE_H