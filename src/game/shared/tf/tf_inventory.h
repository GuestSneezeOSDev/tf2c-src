//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple Inventory
// by MrModez
// $NoKeywords: $
//=============================================================================//
#ifndef TF_INVENTORY_H
#define TF_INVENTORY_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h"
#include "tf_shareddefs.h"

#define INVENTORY_WEAPONS		5
#define INVENTORY_COLNUM		5
#define INVENTORY_ROWNUM		4

#define DEFAULT_ITEM_SCHEMA_FILE "scripts/items/items_game.txt"

// #define ITEM_ACKNOWLEDGEMENTS

class CTFInventory : public CAutoGameSystem
{
public:
	CTFInventory();
	~CTFInventory();

	virtual char const *Name() { return "CTFInventory"; }

	void			PostInit()					override;
	void			Shutdown()					override;
	void			LevelInitPreEntity()		override;
	void			LevelShutdownPostEntity()	override;

	void			InitInventorySys();
	void			CheckForCustomSchema( bool bAlwaysReload = false, bool bSkipInitialization = false );

	CEconItemView	*GetItem( int iClass, ETFLoadoutSlot iSlot, int iNum );
	bool			CheckValidSlot( int iClass, ETFLoadoutSlot iSlot );
	bool			CheckValidItem( int iClass, ETFLoadoutSlot iSlot, int iPreset );
	bool			CheckValidItemLoose(int iClass, ETFLoadoutSlot iSlot, int iPreset);
	int				GetNumItems( int iClass, ETFLoadoutSlot iSlot );

#if defined( CLIENT_DLL )
	bool			GetAllItemPresets( KeyValues *pKV );
	int				GetItemPreset( int iClass, ETFLoadoutSlot iSlot );
	void			SetItemPreset( int iClass, ETFLoadoutSlot iSlot, int iPreset );
	const char		*GetSlotName( ETFLoadoutSlot iSlot );

#ifdef ITEM_ACKNOWLEDGEMENTS
	bool			ItemIsAcknowledged( int iItemID );
	bool			MarkItemAsAcknowledged( int iItemID );
#endif

	void			ResetInventory( void );
#endif

	void			SetOverriddenSchema( bool bOverridden ) { m_bOverriddenSchema = bOverridden; }

private:
	CUtlVectorAutoPurge<CEconItemView *> m_Items[TF_CLASS_COUNT_ALL][TF_LOADOUT_SLOT_COUNT];

#if defined( CLIENT_DLL )
	void LoadInventory();
	void SaveInventory();

	KeyValues *m_pInventory;
#endif
	
	bool m_bInitialInitialization;
	bool m_bOverriddenSchema;
};

CTFInventory* GetTFInventory();

#endif // TF_INVENTORY_H
