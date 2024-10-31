//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_OBJ_JUMPPAD_H
#define C_OBJ_JUMPPAD_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseobject.h"
#include "ObjectControlPanel.h"

class C_ObjectJumppad : public C_BaseObject
{
	DECLARE_CLASS( C_ObjectJumppad, C_BaseObject );
public:
	DECLARE_CLIENTCLASS();

	C_ObjectJumppad();

	virtual void OnPreDataChanged( DataUpdateType_t updateType );
	virtual void OnDataChanged( DataUpdateType_t updateType );

	virtual void GetStatusText( wchar_t *pStatus, int iMaxStatusLen );
	virtual void GetTargetIDDataString( wchar_t *sDataString, int iMaxLenInBytes );

	virtual void UpdateOnRemove();

	virtual CStudioHdr *OnNewModel( void );

	int GetState( void ) { return m_iState; }

	int GetTimesUsed( void );

	void StartActiveEffects( void );
	void StopActiveEffects( void );

	virtual void UpdateDamageEffects( BuildingDamageLevel_t damageLevel );

private:
	int m_iState;
	int m_iOldState;
	int m_iTimesUsed;

	CNewParticleEffect			*m_pFanSpinEffect;

	CNewParticleEffect			*m_pDamageEffects;

	CSoundPatch		*m_pSpinSound;

private:
	C_ObjectJumppad( const C_ObjectJumppad & ); // not defined, not accessible
};

#endif	//C_OBJ_JUMPPAD_H