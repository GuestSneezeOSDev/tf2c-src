//=============================================================================//
//
// Purpose: 
//
//=============================================================================//
#ifndef TF_ITEMMODELPANEL_H
#define TF_ITEMMODELPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "econ_item_schema.h"
#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"

class CEmbeddedItemModelPanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CEmbeddedItemModelPanel, vgui::EditablePanel );

	CEmbeddedItemModelPanel( Panel *parent, const char *name );

	virtual void Paint( void );
	void SetItem( CEconItemDefinition *pItemDef );

private:
	CEconItemDefinition *m_pItemDef;
	int m_iWeaponTextureID;

};

class CItemModelPanel : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CItemModelPanel, vgui::EditablePanel );

	CItemModelPanel( Panel *parent, const char* name );

	enum
	{
		ITEMMODEL_TEXT_AUTO,
		ITEMMODEL_TEXT_LARGE,
		ITEMMODEL_TEXT_SMALL,
		ITEMMODEL_TEXT_SMALLEST,
		ITEMMODEL_TEXT_LARGER,
		ITEMMODEL_TEXT_COUNT
	};

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );

	void SetItem( CEconItemDefinition *pItemDefinition, int iSlot = -1, bool bNoAmmo = false, bool bEquipped = false );
	void SetNumberSettings( int x, int y, vgui::HFont hFont, Color col, int a );

	void SetNameVisibility( bool bVisible );

private:
	CPanelAnimationVarAliasType( int, m_iModelXPos, "model_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iModelYPos, "model_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iModelWide, "model_wide", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iModelTall, "model_tall", "0", "proportional_int" );
	CPanelAnimationVar( bool, m_bModelCenterX, "model_center_x", "0" );
	CPanelAnimationVar( bool, m_bModelCenterY, "model_center_y", "0" );
	CPanelAnimationVar( bool, m_bModelOnly, "model_only", "0" );

	CPanelAnimationVar( bool, m_bTextCenter, "text_center", "0" );
	CPanelAnimationVar( bool, m_bTextCenterX, "text_center_x", "0" );

	CPanelAnimationVar( bool, m_bNameOnly, "name_only", "0" );

	CPanelAnimationVarAliasType( int, m_iTextXPos, "text_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextYPos, "text_ypos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextWide, "text_wide", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextYOffset, "text_yoffset", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iHPadding, "padding_height", "0", "proportional_int" );
	CPanelAnimationVar( bool, m_bStandardTextColor, "standard_text_color", "0" );
	CPanelAnimationVar( int, m_iForceTextSize, "text_forcesize", "0" );

	CEconItemDefinition *m_pItemDef;

	vgui::EditablePanel	*m_pMainContainer;
	CEmbeddedItemModelPanel *m_pModelPanel;
	CExLabel			*m_pNameLabel;
	CExLabel			*m_pAttribLabel;
	CExLabel			*m_pEquippedLabel;
	CExLabel			*m_pSlotID;

	vgui::HFont			m_hLargeFont;
	vgui::HFont			m_hSmallFont;
	vgui::HFont			m_hSmallestFont;
	vgui::HFont			m_hLargerFont;
	Color				m_clrDefaultNameColor;

};

#endif // TF_ITEMMODELPANEL_H
