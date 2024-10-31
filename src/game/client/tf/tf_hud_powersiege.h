#ifndef TF_HUD_POWERSIEGE_H
#define TF_HUD_POWERSIEGE_H

#include "cbase.h"
#include "tf_controls.h"

class CTFHudPowerSiege : public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFHudPowerSiege, vgui::EditablePanel );

	CTFHudPowerSiege( vgui::Panel* parent, const char* name );
	void ApplySchemeSettings( vgui::IScheme* pScheme );
	void OnTick();
private:
	CExLabel *m_pLabel;
};

#endif
