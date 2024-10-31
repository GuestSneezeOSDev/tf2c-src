//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_FOURTEAMSCOREBOARD_H
#define TF_FOURTEAMSCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_clientscoreboard.h"

//-----------------------------------------------------------------------------
// Purpose: displays the MapInfo menu
//-----------------------------------------------------------------------------

class CTFFourTeamScoreBoardDialog : public CTFClientScoreBoardDialog
{
	DECLARE_CLASS_SIMPLE( CTFFourTeamScoreBoardDialog, CTFClientScoreBoardDialog );

public:
	CTFFourTeamScoreBoardDialog( IViewPort *pViewPort, const char *pszName );

protected:
	virtual const char *GetName( void ) { return PANEL_FOURTEAMSCOREBOARD; }
	virtual const char *GetResFilename( void ) { return "Resource/UI/FourTeamScoreBoard.res"; }

};

#endif // TF_FOURTEAMSCOREBOARD_H
