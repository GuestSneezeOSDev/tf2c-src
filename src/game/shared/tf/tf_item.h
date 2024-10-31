//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
//
//=============================================================================//
#ifndef TF_ITEM_H
#define TF_ITEM_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
#define CTFPlayer C_TFPlayer
#define CTFItem C_TFItem
#endif

class CTFPlayer;

//=============================================================================
//
// TF Item
//
class CTFItem : public CBaseAnimating
{
public:
	DECLARE_CLASS( CTFItem, CBaseAnimating )
	DECLARE_NETWORKCLASS();

	// Unique identifier.
	virtual unsigned int GetItemID();
	
	// Pick up and drop.
	virtual void PickUp( CTFPlayer *pPlayer, bool bInvisible );
	virtual void Drop( CTFPlayer *pPlayer, bool bVisible, bool bThrown = false, bool bMessage = true );

#ifdef CLIENT_DLL
	virtual bool ShouldDraw();
	virtual ShadowType_t ShadowCastType();
#endif
};

#endif // TF_ITEM_H