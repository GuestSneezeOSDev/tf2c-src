//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//

#ifndef TF_GENERIC_BOMB_H
#define TF_GENERIC_BOMB_H
#ifdef _WIN32
#pragma once
#endif

//=============================================================================
//
// TF Generic Bomb Class
//

class CTFGenericBomb : public CBaseAnimating, public TAutoList<CTFGenericBomb>
{
public:
	DECLARE_CLASS( CTFGenericBomb, CBaseAnimating );
	DECLARE_DATADESC();

	CTFGenericBomb();

	virtual void		Precache( void );
	virtual void		Spawn( void );
	virtual int			OnTakeDamage( const CTakeDamageInfo &info );
	virtual void		Event_Killed( const CTakeDamageInfo &info );

	void				InputDetonate( inputdata_t &inputdata );

protected:
	float				m_flDamage;
	float				m_flRadius;
	string_t			m_iszParticleName;
	string_t			m_iszExplodeSound;
	bool				m_bFriendlyFire;

	COutputEvent		m_OnDetonate;
};

class CTFPumpkinBomb : public CBaseAnimating, public TAutoList<CTFPumpkinBomb>
{
public:
	DECLARE_CLASS(CTFPumpkinBomb, CBaseAnimating);

	virtual void	Precache(void);
	virtual void	Spawn(void);
	virtual void	Event_Killed(const CTakeDamageInfo &info);
};


#endif // TF_GENERIC_BOMB_H
