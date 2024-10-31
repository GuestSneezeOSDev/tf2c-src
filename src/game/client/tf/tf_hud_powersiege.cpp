#include "cbase.h"
#include "tf_hud_powersiege.h"
#include "../../shared/tf/tf_generator.h"
#include <igameresources.h>

using namespace vgui;

CTFHudPowerSiege::CTFHudPowerSiege( vgui::Panel* parent, const char* name ) : EditablePanel( parent, name )
{
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

void CTFHudPowerSiege::ApplySchemeSettings( IScheme* pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/hudobjectivepowersiege.res" );

	m_pLabel = dynamic_cast<CExLabel*>( FindChildByName( "StatusLabel" ) );
}

void CTFHudPowerSiege::OnTick() 
{
	char buffer[100] = "";
	for ( CTeamGenerator* pGenerator : CTeamGenerator::AutoList() )
	{
		int iGeneratorPercentage = (int)(ceil(pGenerator->m_flHealthPercentage * 100.0f));
		int iShieldPercentage = -1;
		CBaseEntity* pEnt = pGenerator->GetShield().Get();
		if ( pEnt ) 
		{
			CTeamShield *pShield = dynamic_cast<CTeamShield*>( pEnt );
			iShieldPercentage = (int)(ceil(pShield->m_flHealthPercentage * 100.0f));
		}

		char str[100];
		V_sprintf_safe( str, "G: %i%% S: %i%% (%s) ", iGeneratorPercentage, iShieldPercentage, GameResources()->GetTeamName( pGenerator->GetTeamNumber() ) );
		V_strcat( buffer, str, sizeof(buffer) );
	}
	if(m_pLabel) m_pLabel->SetText( buffer );
}