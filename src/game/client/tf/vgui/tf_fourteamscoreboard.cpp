//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_fourteamscoreboard.h"
#include "vgui_controls/SectionedListPanel.h"

using namespace vgui;

CTFFourTeamScoreBoardDialog::CTFFourTeamScoreBoardDialog( IViewPort *pViewPort, const char *pszName ) : CTFClientScoreBoardDialog( pViewPort, pszName, TF_TEAM_COUNT )
{
	for ( int i = m_iTeamCount - 1; i >= FIRST_GAME_TEAM; i-- )
	{
		m_pPlayerListsVIP[i]->MarkForDeletion();
		m_pPlayerListsVIP[i] = NULL;
	}
}
