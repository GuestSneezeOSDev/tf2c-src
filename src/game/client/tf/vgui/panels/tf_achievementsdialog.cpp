#include "cbase.h"
#include "tf_achievementsdialog.h"
#include "tf_mainmenu.h"
#include "tf_gamerules.h"

#include "ienginevgui.h"
#include "vgui/ILocalize.h"
#include <vgui_controls/AnimationController.h>
#include "filesystem.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/CheckButton.h"
#include <vgui_controls/QueryBox.h>
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Sets the parameter pIconPanel to display the specified achievement's icon file.
//-----------------------------------------------------------------------------
bool LoadAchievementIcon( vgui::ImagePanel* pIconPanel, IAchievement *pAchievement, const char *pszExt /*= NULL*/ )
{
	char imagePath[_MAX_PATH];
	Q_strncpy( imagePath, "achievements\\", sizeof( imagePath ) );
	Q_strncat( imagePath, pAchievement->GetName(), sizeof( imagePath ), COPY_ALL_CHARACTERS );
	if ( pszExt )
	{
		Q_strncat( imagePath, pszExt, sizeof( imagePath ), COPY_ALL_CHARACTERS );
	}
	Q_strncat( imagePath, ".vtf", sizeof( imagePath ), COPY_ALL_CHARACTERS );

	char checkFile[_MAX_PATH];
	Q_snprintf( checkFile, sizeof( checkFile ), "materials\\vgui\\%s", imagePath );
	if ( !g_pFullFileSystem->FileExists( checkFile ) )
	{
		Q_snprintf( imagePath, sizeof( imagePath ), "hud\\icon_locked.vtf" );
	}

	// pIconPanel->SetShouldScaleImage( true );
	pIconPanel->SetImage( imagePath );
	// pIconPanel->SetVisible( true );
	
	return true; // pIconPanel->IsVisible()
}

//-----------------------------------------------------------------------------
// The bias is to ensure the percentage bar gets plenty orange before it reaches the text,
// as the white-on-grey is hard to read.
//-----------------------------------------------------------------------------
Color LerpColors( Color cStart, Color cEnd, float flPercent )
{
	float r = ( float )( ( float )( cStart.r() ) + ( float )( cEnd.r() - cStart.r() ) * Bias( flPercent, 0.75 ) );
	float g = ( float )( ( float )( cStart.g() ) + ( float )( cEnd.g() - cStart.g() ) * Bias( flPercent, 0.75 ) );
	float b = ( float )( ( float )( cStart.b() ) + ( float )( cEnd.b() - cStart.b() ) * Bias( flPercent, 0.75 ) );
	float a = ( float )( ( float )( cStart.a() ) + ( float )( cEnd.a() - cStart.a() ) * Bias( flPercent, 0.75 ) );
	return Color( r, g, b, a );
}

//-----------------------------------------------------------------------------
// Purpose: Shares common percentage bar calculations/color settings between xbox and pc.
//			Not really intended for robustness or reuse across many panels.
// Input  : pFrame - assumed to have certain child panels ( see below )
//			*pAchievement - source achievement to poll for progress. Non progress achievements will not show a percentage bar.
//-----------------------------------------------------------------------------
void UpdateProgressBar( vgui::EditablePanel* pPanel, IAchievement *pAchievement, Color clrProgressBar )
{
	if ( pAchievement->GetGoal() > 1 )
	{
		bool bShowProgress = true;

		// if this achievement gets saved with game and we're not in a level and have not achieved it, then we do not have any state 
		// for this achievement, don't show progress
		if ( ( pAchievement->GetFlags() & ACH_SAVE_WITH_GAME ) && !engine->IsInGame() && !pAchievement->IsAchieved() )
		{
			bShowProgress = false;
		}

		float flCompletion = 0.0f;

		// Once achieved, we can't rely on count. If they've completed the achievement just set to 100%.
		int iCount = pAchievement->GetCount();
		if ( pAchievement->IsAchieved() )
		{
			flCompletion = 1.0f;
			iCount = pAchievement->GetGoal();
		}
		else if ( bShowProgress )
		{
			flCompletion = ( ( ( float )pAchievement->GetCount() ) / ( ( float )pAchievement->GetGoal() ) );
			// In rare cases count can exceed goal and not be achieved ( switch local storage on X360, take saved game from different user on PC ).
			// These will self-correct with continued play, but if we're in that state don't show more than 100% achieved.
			flCompletion = min( flCompletion, 1.0 );
		}

		char szPercentageText[ 256 ] = "";
		if ( bShowProgress )
		{
			Q_snprintf( szPercentageText, 256, "%d/%d", iCount, pAchievement->GetGoal() );			
		}

		pPanel->SetDialogVariable( "achievement_progress", iCount );
		pPanel->SetDialogVariable( "achievement_goal", pAchievement->GetGoal() );

		//pPanel->SetControlString( "PercentageText", szPercentageText );
		//pPanel->SetControlVisible( "PercentageText", true );
		//pPanel->SetControlVisible( "CompletionText", true );

		vgui::ImagePanel *pPercentageBar	= ( vgui::ImagePanel* )pPanel->FindChildByName( "PercentageBar" );
		vgui::ImagePanel *pPercentageBarBkg = ( vgui::ImagePanel* )pPanel->FindChildByName( "PercentageBarBackground" );

		if ( pPercentageBar && pPercentageBarBkg )
		{
			pPercentageBar->SetFillColor( clrProgressBar );
			pPercentageBar->SetWide( pPercentageBarBkg->GetWide() * flCompletion );

			// pPanel->SetControlVisible( "PercentageBarBackground", true );
			// pPanel->SetControlVisible( "PercentageBar", true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFAchievementsDialog::CTFAchievementsDialog( vgui::Panel* parent, const char *panelName ) : CTFDialogPanelBase( parent, panelName )
{
	m_bInitialized = false;

	m_pAchievementsList = new vgui::PanelListPanel( this, "listpanel_achievements" );
	m_pAchievementsList->SetFirstColumnWidth( 0 );

	m_pListBG = new vgui::ImagePanel( this, "listpanel_background" );

	m_pPercentageBarBackground = SETUP_PANEL( new ImagePanel( this, "PercentageBarBackground" ) );
	m_pPercentageBar = SETUP_PANEL( new ImagePanel( this, "PercentageBar" ) );

	m_pAchievementPackCombo = new ComboBox( this, "achievement_pack_combo", 10, false );

	m_pHideAchievedCheck = new CheckButton( this, "HideAchieved", "" );
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CTFAchievementsDialog::~CTFAchievementsDialog()
{
	m_pAchievementsList->DeleteAllItems();
	delete m_pAchievementsList;
	delete m_pPercentageBarBackground;
	delete m_pPercentageBar;
}


void CTFAchievementsDialog::Show()
{
	BaseClass::Show();

	if ( !m_bInitialized )
	{
		memset( m_AchievementItemSettings, 0, sizeof( m_AchievementItemSettings ) );

		for ( int i = 0; i < (sizeof( m_AchievementItemSettings ) / sizeof (achievement_item_settings_t)); i++ )
		{
			m_AchievementItemSettings[i].m_bReduced = false;
			m_AchievementItemSettings[i].m_bExpanded = false;
		}

		CreateAchievementList();
	}
	else
	{
		RefreshAchievementList();
	}
};


void CTFAchievementsDialog::Hide()
{
	BaseClass::Hide();
};

//-----------------------------------------------------------------------------
// Purpose: Reset the achievement list
//-----------------------------------------------------------------------------
void CTFAchievementsDialog::CreateAchievementList( void )
{
	Reset();

	// int that holds the highest number achievement id we've found
	int iHighestAchievementIDSeen = -1;
	int iNextGroupBoundary = 1000;

	// Base groups
	//CreateNewAchievementGroup( TF2C_ACHIEVEMENT_START - 1,							TF2C_ACHIEVEMENT_COUNT							);
	CreateNewAchievementGroup( TF2C_ACHIEVEMENT_PACK_TEAMFORTRESS2CLASSIC_BEGIN,	TF2C_ACHIEVEMENT_PACK_TEAMFORTRESS2CLASSIC_END	);
	CreateNewAchievementGroup(TF2C_ACHIEVEMENT_PACK_GAMEMODES_AND_MAPS_BEGIN,		TF2C_ACHIEVEMENT_PACK_GAMEMODES_AND_MAPS_END);
	CreateNewAchievementGroup(TF2C_ACHIEVEMENT_PACK_STOCK_BEGIN,					TF2C_ACHIEVEMENT_PACK_STOCK_END);
	CreateNewAchievementGroup(TF2C_ACHIEVEMENT_PACK_DEATH_AND_TAXES_BEGIN,			TF2C_ACHIEVEMENT_PACK_DEATH_AND_TAXES_END);
	CreateNewAchievementGroup(TF2C_ACHIEVEMENT_PACK_FIGHT_AND_FLIGHT_BEGIN,			TF2C_ACHIEVEMENT_PACK_FIGHT_AND_FLIGHT_END);
	CreateNewAchievementGroup(TF2C_ACHIEVEMENT_PACK_2POINT2_BEGIN,					TF2C_ACHIEVEMENT_PACK_2POINT2_END);

	int iActiveDropDownItem = m_pAchievementPackCombo->GetActiveItem();

	if ( engine->GetAchievementMgr() )
	{
		int iCount = engine->GetAchievementMgr()->GetAchievementCount();
		for ( int i = 0; i < iCount; ++i )
		{		
			IAchievement* pCur = engine->GetAchievementMgr()->GetAchievementByIndex( i );

			if ( !pCur )
				continue;

			int iAchievementID = pCur->GetAchievementID();

			if ( iAchievementID > iHighestAchievementIDSeen )
			{
				// if its crossed the next group boundary, create a new group
				if ( iAchievementID >= iNextGroupBoundary )
				{
					int iNewGroupBoundary = 100 * ( ( int )( ( float )iAchievementID / 100 ) );
					CreateNewAchievementGroup( iNewGroupBoundary, iNewGroupBoundary + 99 );

					iNextGroupBoundary = iNewGroupBoundary + 100;
				}

				iHighestAchievementIDSeen = iAchievementID;
			}

			// don't show hidden achievements if not achieved
			if ( pCur->ShouldHideUntilAchieved() && !pCur->IsAchieved() )
				continue;

			bool bAchieved = pCur->IsAchieved();

			if ( bAchieved )
			{
				m_nTotalScore += pCur->GetPointValue();
			}

			for ( int j = 0; j < m_iNumAchievementGroups; j++ )
			{
				if ( iAchievementID >= m_AchievementGroups[j].m_iMinRange &&
					iAchievementID <= m_AchievementGroups[j].m_iMaxRange )
				{
					if ( bAchieved )
					{
						m_AchievementGroups[j].m_iNumUnlocked++;
					}

					if ( ( pCur->IsAchieved() && pCur->ShouldHideUntilAchieved() ) || !pCur->ShouldHideUntilAchieved() )
					{
						m_AchievementGroups[j].m_iNumAchievements++;
					}
				}
			}

			if ( bAchieved && m_pHideAchievedCheck->IsSelected() )
			{
				continue;
			}
			
			CTFAchievementDialogItemPanel *achievementItemPanel = new CTFAchievementDialogItemPanel( m_pAchievementsList, "AchievementDialogItemPanel", i, &m_AchievementItemSettings[i] );
			achievementItemPanel->SetAchievementInfo( pCur );
			m_pAchievementsList->AddItem( NULL, achievementItemPanel );
		}

		RefreshAchievementPackList();

		m_pAchievementPackCombo->ActivateItem( 0 );
		m_pAchievementPackCombo->ActivateItem( iActiveDropDownItem );

		m_bInitialized = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Refresh the achievement list
//-----------------------------------------------------------------------------
void CTFAchievementsDialog::RefreshAchievementsProgressBar( int iCount, int iNumAchievements, int iNumUnlocked, int iTotalScore )
{
	// Set up total completion percentage bar
	float flCompletion = 0.0f;
	if ( iCount > 0 )
	{
		flCompletion = ( ( ( float )iNumUnlocked ) / ( ( float )( iNumAchievements ) ) );
	}

	char szPercentageText[256];
	/*Q_snprintf(szPercentageText, 256, "%d | %d / %d ( %d%% )",
		iTotalScore, iNumUnlocked, iNumAchievements, ( int )( flCompletion * 100.0f ) );*/ // no more total score
	V_snprintf(szPercentageText, sizeof(szPercentageText), "%d / %d ( %d%% )",
		iNumUnlocked, iNumAchievements, (int)(flCompletion * 100.0f));

	SetControlString( "PercentageText", szPercentageText );
	SetControlVisible( "PercentageText", true );
	SetControlVisible( "CompletionText", true );

	Color clrHighlight = m_pScheme->GetColor( "NewGame.SelectionColor", Color( 255, 255, 255, 255 ) );
	Color clrWhite( 255, 255, 255, 255 );

	Color cProgressBar = Color( static_cast<float>( clrHighlight.r() ) * ( 1.0f - flCompletion ) + static_cast<float>( clrWhite.r() ) * flCompletion,
		static_cast<float>( clrHighlight.g() ) * ( 1.0f - flCompletion ) + static_cast<float>( clrWhite.g() ) * flCompletion,
		static_cast<float>( clrHighlight.b() ) * ( 1.0f - flCompletion ) + static_cast<float>( clrWhite.b() ) * flCompletion,
		static_cast<float>( clrHighlight.a() ) * ( 1.0f - flCompletion ) + static_cast<float>( clrWhite.a() ) * flCompletion );

	m_pPercentageBar->SetFgColor( cProgressBar );
	m_pPercentageBar->SetWide( m_pPercentageBarBackground->GetWide() * flCompletion );

	SetControlVisible( "PercentageBarBackground", true );
	SetControlVisible( "PercentageBar", true );
}

//-----------------------------------------------------------------------------
// Purpose: Refresh the achievement pack list
//-----------------------------------------------------------------------------
void CTFAchievementsDialog::RefreshAchievementPackList( void )
{
	m_pAchievementPackCombo->DeleteAllItems();

	int iActiveItem = m_pAchievementPackCombo->GetActiveItem();

	for ( int i = 0; i < m_iNumAchievementGroups; i++ )
	{
		char buf[128];

		/*if (i == 0)
		{
			Q_snprintf( buf, sizeof( buf ), "#Achievement_Group_All" );
		}
		else*/
		{
			Q_snprintf( buf, sizeof( buf ), "#Achievement_Group_%d", m_AchievementGroups[i].m_iMinRange );
		}		

		bool bLocalizationError = false;
		const wchar_t *wzGroupName = g_pVGuiLocalize->Find( buf );

		if ( !wzGroupName )
		{
			wzGroupName = L"#%s1 ( %s2 of %s3 )";
			bLocalizationError = true;
		}

		wchar_t wzGroupTitle[128];

		if ( wzGroupName )
		{
			wchar_t wzGroupID[8];
			V_swprintf_safe( wzGroupID, L"%d", m_AchievementGroups[i].m_iMinRange );

			wchar_t wzNumUnlocked[8];
			V_swprintf_safe( wzNumUnlocked, L"%d", m_AchievementGroups[i].m_iNumUnlocked );

			wchar_t wzNumAchievements[8];
			V_swprintf_safe( wzNumAchievements, L"%d", m_AchievementGroups[i].m_iNumAchievements );

			if ( bLocalizationError )
			{
				g_pVGuiLocalize->ConstructString( wzGroupTitle, sizeof( wzGroupTitle ), wzGroupName, 3, wzGroupID, wzNumUnlocked, wzNumAchievements );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( wzGroupTitle, sizeof( wzGroupTitle ), wzGroupName, 2, wzNumUnlocked, wzNumAchievements );
			}
		}

		KeyValues *pKV = new KeyValues( "grp" );
		pKV->SetInt( "minrange", m_AchievementGroups[i].m_iMinRange );
		pKV->SetInt( "maxrange", m_AchievementGroups[i].m_iMaxRange );
		m_pAchievementPackCombo->AddItem( wzGroupTitle, pKV );
	}

	// m_pAchievementPackCombo->ActivateItemByRow( 0 );
	m_pAchievementPackCombo->ActivateItem( iActiveItem );
}

//-----------------------------------------------------------------------------
// Purpose: Sort achievements alphabetically and by progress
//-----------------------------------------------------------------------------
class CSortAchievements
{
public:
	bool Less( IAchievement* src1, IAchievement* src2, void* pCtx )
	{
		// Sort by putting achieved at the bottom
		if ( src1->IsAchieved() && !src2->IsAchieved() )
			return false;

		if ( !src1->IsAchieved() && src2->IsAchieved() )
			return true;

		// Sort by progress percent?

		// Sort by alphabet
		auto name1 = g_pVGuiLocalize->Find( CFmtStr( "#%s_NAME", src1->GetName() ) );
		auto name2 = g_pVGuiLocalize->Find( CFmtStr( "#%s_NAME", src2->GetName() ) );

		if ( wcsicmp( name1, name2 ) < 0 )
			return true;

		return false;
	}
};

//-----------------------------------------------------------------------------
// Purpose: Refresh the achievement list
//-----------------------------------------------------------------------------
void CTFAchievementsDialog::RefreshAchievementList( void )
{
	if ( engine->GetAchievementMgr() )
	{
		// Re-populate the achievement list with the selected group
		m_pAchievementsList->DeleteAllItems();
		
		int iCurrentGroup = m_pAchievementPackCombo->GetActiveItem();

		int iGroupUnlocked = 0;
		int iGroupAchievements = 0;

		int iCount = engine->GetAchievementMgr()->GetAchievementCount();
		int iNumAchievements = 0;
		int iNumUnlocked = 0;
		int iTotalScore = 0;

		KeyValues *pData = m_pAchievementPackCombo->GetActiveItemUserData();

		if ( pData )
		{
			int iMinRange = pData->GetInt( "minrange" );
			int iMaxRange = pData->GetInt( "maxrange" );

			CUtlSortVector<IAchievement*, CSortAchievements> vecSortedAchievements;

			for ( int i = 0; i < iCount; ++i )
			{		
				IAchievement* pCur = engine->GetAchievementMgr()->GetAchievementByIndex( i );

				if ( !pCur )
					continue;

				bool bAchieved = pCur->IsAchieved();

				// don't show hidden achievements if not achieved
				if ( pCur->ShouldHideUntilAchieved() && !bAchieved )
				{
					continue;
				}

				if ( bAchieved )
				{
					iNumUnlocked++;
					iTotalScore += pCur->GetPointValue();
				}

				iNumAchievements++;
					
				int iAchievementID = pCur->GetAchievementID();

				if ( iAchievementID < iMinRange || iAchievementID > iMaxRange )
					continue;

				iGroupAchievements++;

				if ( bAchieved )
				{
					iGroupUnlocked++;
				}

				// don't show if we don't want them to show
				if ( m_pHideAchievedCheck->IsSelected() && pCur->IsAchieved() )
					continue;

				vecSortedAchievements.Insert( pCur );
			}

			for ( int i = 0; i < vecSortedAchievements.Count(); i++ )
			{
				CTFAchievementDialogItemPanel* achievementItemPanel = new CTFAchievementDialogItemPanel( m_pAchievementsList, "AchievementDialogItemPanel", i, &m_AchievementItemSettings[i] );
				achievementItemPanel->SetAchievementInfo( vecSortedAchievements[i] );
				m_pAchievementsList->AddItem( NULL, achievementItemPanel );
			}
		}

		m_AchievementGroups[iCurrentGroup].m_iNumUnlocked = iGroupUnlocked;
		m_AchievementGroups[iCurrentGroup].m_iNumAchievements = iGroupAchievements;

		RefreshAchievementPackList();
		RefreshAchievementsProgressBar( iCount, iNumAchievements, iNumUnlocked, iTotalScore );

		// Reset expanded/reduced achievements so they don't affect any in a newly selected pack.
		for ( int i = 0; i < ( sizeof( m_AchievementItemSettings ) / sizeof( achievement_item_settings_t ) ); i++ )
		{
			m_AchievementItemSettings[i].m_bReduced = false;
			m_AchievementItemSettings[i].m_bExpanded = false;
		}

		if ( m_pAchievementPackCombo->IsVisible() && m_iNumAchievementGroups <= 2 )
		{
			// we have no achievement packs. Hide the combo and bump the achievement list up a bit
			m_pAchievementPackCombo->SetVisible( false );

			// do some work to preserve the pincorner and resizing

			int comboX, comboY;
			m_pAchievementPackCombo->GetPos( comboX, comboY );

			int x, y, w, h;
			m_pAchievementsList->GetBounds( x, y, w, h );

			PinCorner_e corner = m_pAchievementsList->GetPinCorner();
			int pinX, pinY;
			m_pAchievementsList->GetPinOffset( pinX, pinY );

			int resizeOffsetX, resizeOffsetY;
			m_pAchievementsList->GetResizeOffset( resizeOffsetX, resizeOffsetY );

			m_pAchievementsList->SetAutoResize( corner, AUTORESIZE_DOWN, pinX, comboY, resizeOffsetX, resizeOffsetY );

			m_pAchievementsList->SetBounds( x, comboY, w, h + ( y - comboY ) );

			m_pListBG->SetAutoResize( corner, AUTORESIZE_DOWN, pinX, comboY, resizeOffsetX, resizeOffsetY );
			m_pListBG->SetBounds( x, comboY, w, h + ( y - comboY ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reset
//-----------------------------------------------------------------------------
void CTFAchievementsDialog::Reset( void )
{
	m_nTotalScore = 0;

	m_pAchievementPackCombo->DeleteAllItems();
	m_pAchievementsList->DeleteAllItems();

	Q_memset( m_AchievementGroups, 0, sizeof( m_AchievementGroups ) );
	m_iNumAchievementGroups = 0;
}


void CTFAchievementsDialog::CreateNewAchievementGroup( int iMinRange, int iMaxRange )
{
	m_AchievementGroups[m_iNumAchievementGroups].m_iMinRange = iMinRange;
	m_AchievementGroups[m_iNumAchievementGroups].m_iMaxRange = iMaxRange;
	m_iNumAchievementGroups++;
}


void CTFAchievementsDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pScheme = pScheme;

	LoadControlSettings( "resource/UI/main_menu/AchievementsDialog.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Each sub-panel gets its data updated
//-----------------------------------------------------------------------------
void CTFAchievementsDialog::UpdateAchievementDialogInfo( void )
{
	for ( int i = 0; i < m_pAchievementsList->GetItemCount(); i++ )
	{
		CTFAchievementDialogItemPanel *pPanel = ( CTFAchievementDialogItemPanel * )m_pAchievementsList->GetItemPanel( i );
		if ( pPanel )
		{
			pPanel->UpdateAchievementInfo();
		}
	}
}


void CTFAchievementsDialog::OnCommand( const char* command )
{
	if ( !Q_strcasecmp( command, "ongameuiactivated" ) )
	{
		Msg( "Updating achievement info\n" );
		UpdateAchievementDialogInfo();
	}
	else if ( !Q_strcasecmp( command, "ResetAchievements" ) )
	{
		// Notify the user that this will reset all of their achievements (Dear Lord!)
		QueryBox *pResetAchievementsQuery = new QueryBox( "#TF_ConfirmResetAchievements_Title", "#TF_ConfirmResetAchievements_Message", guiroot->GetParent() );
		if ( pResetAchievementsQuery )
		{
			pResetAchievementsQuery->AddActionSignalTarget( this );

			pResetAchievementsQuery->SetOKCommand( new KeyValues( "OnWarningAccepted" ) );
			pResetAchievementsQuery->SetOKButtonText( "#TF_ConfirmResetAchievements_OK" );

			pResetAchievementsQuery->DoModal();
		}
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

//----------------------------------------------------------
// Purpose: New group was selected in the dropdown, recalc what achievements to show
//----------------------------------------------------------
void CTFAchievementsDialog::OnTextChanged( KeyValues *data )
{
	Panel *pPanel = ( Panel * )data->GetPtr( "panel", NULL );

	// first check which control had its text changed!
	if ( pPanel == m_pAchievementPackCombo || pPanel == m_pHideAchievedCheck )
	{
		RefreshAchievementList();
	}
}


void CTFAchievementsDialog::OnButtonChecked( KeyValues *data )
{
	OnTextChanged( data );
}


void CTFAchievementsDialog::OnWarningAccepted( void )
{
	CAchievementMgr *pAchievementMgr = dynamic_cast<CAchievementMgr *>( engine->GetAchievementMgr() );
	if ( !pAchievementMgr )
		return;

	pAchievementMgr->ResetAchievements();
	CreateAchievementList();
}


void CTFAchievementsDialog::AppearAnimation()
{
	// Offset the dialog and make it slide back into normal position.
	int x, y;
	GetPos( x, y );
	SetPos( x, y + XRES( 512 ) );

	GetAnimationController()->RunAnimationCommand( this, "ypos", y, 0.0f, 0.5f, AnimationController::INTERPOLATOR_SIMPLESPLINE, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: creates child panels, passes down name to pick up any settings from res files.
//-----------------------------------------------------------------------------
CTFAchievementDialogItemPanel::CTFAchievementDialogItemPanel( vgui::PanelListPanel *parent, const char* name, int iListItemID, achievement_item_settings_t *settings ) : BaseClass( parent, name )
{
	m_AchievementSettings = settings;

	m_iListItemID = iListItemID;
	m_pParent = parent;

	m_pAchievementIcon = NULL;
	m_pPercentageBar = SETUP_PANEL( new ImagePanel( this, "PercentageBar" ) );
	m_pComponentsList = NULL;
}

CTFAchievementDialogItemPanel::~CTFAchievementDialogItemPanel()
{
	if ( m_pComponentsList )
	{
		m_pComponentsList->DeleteAllItems();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Loads settings from hl2/resource/ui/achievementitem.res
//			Sets display info for this achievement item.
//-----------------------------------------------------------------------------
void CTFAchievementDialogItemPanel::ApplySchemeSettings( IScheme *pScheme )
{
	char customResourceFile[_MAX_PATH];
	Q_snprintf( customResourceFile, sizeof( customResourceFile ), "resource\\ui\\main_menu\\achievements\\AchievementsDialogItem_%d", m_pSourceAchievement->GetAchievementID() );
	Q_strncat( customResourceFile, ".res", sizeof( customResourceFile ), COPY_ALL_CHARACTERS );

	KeyValues *pConditions = new KeyValues( "conditions" );

	// Size Status

	if ( m_AchievementSettings->m_bReduced )
	{
		AddSubKeyNamed( pConditions, "if_reduced" );
	}

	// Achieved Status

	if ( m_pSourceAchievement->IsAchieved() )
	{
		AddSubKeyNamed( pConditions, "if_achieved" );

#if 0
		AddSubKeyNamed( pConditions, "if_recently_achieved" );
#endif
	}
	else
	{
		AddSubKeyNamed( pConditions, "if_not_achieved" );
	}

	if ( m_pSourceAchievement->GetPointValue() <= 0 )
	{
		AddSubKeyNamed( pConditions, "if_has_no_value" );
	}

	// Progress Status

	if ( m_pSourceAchievement->GetGoal() > 1 )
	{
		AddSubKeyNamed( pConditions, "if_has_progress" );
	}

	if ( m_pSourceAchievement->GetCount() > ( float )( m_pSourceAchievement->GetGoal() / 2.0f ) )
	{
		AddSubKeyNamed( pConditions, "if_progress_is_past_halfway" );
	}
	else
	{
		AddSubKeyNamed( pConditions, "if_progress_is_below_halfway" );
	}

	// Component Status

	if ( m_pSourceAchievement->GetFlags() & ACH_HAS_COMPONENTS )
	{
		AddSubKeyNamed( pConditions, "if_uses_components" );

		int iPresentComponents = 0;
		int iMissingComponents = 0;

		for ( int i = 0; i < m_pSourceAchievement->GetGoal(); i++ )
		{
			if ( ( m_pSourceAchievement->GetComponentBits() & ( (uint64)1 ) << i ) )
			{
				char buf[128];
				Q_snprintf( buf, sizeof( buf ), "if_component_%d_present", i + 1 );

				AddSubKeyNamed( pConditions, buf );
				iPresentComponents++;
			}
			else
			{
				char buf[128];
				Q_snprintf( buf, sizeof( buf ), "if_component_%d_missing", i + 1 );

				AddSubKeyNamed( pConditions, buf );
				iMissingComponents++;
			}
		}

		if ( iMissingComponents == m_pSourceAchievement->GetGoal() )
		{
			AddSubKeyNamed( pConditions, "if_missing_all_components" );
		}
		else if ( iPresentComponents == m_pSourceAchievement->GetGoal() )
		{
			AddSubKeyNamed( pConditions, "if_has_all_components" );
		}
	}

	if ( g_pFullFileSystem->FileExists( customResourceFile ) )
	{
		LoadControlSettings( customResourceFile, NULL, NULL, pConditions );
	}
	else
	{
		LoadControlSettings( "resource/ui/main_menu/achievements/AchievementsDialogItem.res", NULL, NULL, pConditions );
	}

	pConditions->deleteThis();
	
	if ( !m_pSourceAchievement )
		return;

	BaseClass::ApplySchemeSettings( pScheme );

	m_pAchievementIcon = dynamic_cast<vgui::ImagePanel *>( FindChildByName( "AchievementIcon" ) );
	m_pComponentsList = dynamic_cast<vgui::PanelListPanel *>( FindChildByName( "listpanel_components" ) );
	m_pShowOnHUDCheck = dynamic_cast<vgui::CheckButton*>(FindChildByName("ShowOnHUD"));
	if ( m_pShowOnHUDCheck )
	{
		m_pShowOnHUDCheck->SetMouseInputEnabled( true );
		m_pShowOnHUDCheck->SetEnabled( true );
		m_pShowOnHUDCheck->SetCheckButtonCheckable( true );
		m_pShowOnHUDCheck->AddActionSignalTarget( this );
	}

	m_pExpandButton = dynamic_cast<CTFButton*>( FindChildByName( "ExpandButton" ) );

	UpdateAchievementInfo();
}

//-----------------------------------------------------------------------------
// Purpose: Updates displayed achievement data. In applyschemesettings, and when gameui activates.
//-----------------------------------------------------------------------------
void CTFAchievementDialogItemPanel::UpdateAchievementInfo()
{
	if ( m_pSourceAchievement )
	{
		// Set name, description and unlocked state text
		SetDialogVariable( "achievement_name", ACHIEVEMENT_LOCALIZED_NAME( m_pSourceAchievement ) );
		SetDialogVariable( "achievement_desc", ACHIEVEMENT_LOCALIZED_DESC( m_pSourceAchievement ) );

		char szPointValue[16];
		Q_snprintf( szPointValue, sizeof( szPointValue ), "%d", m_pSourceAchievement->GetPointValue() );

		SetDialogVariable( "score", szPointValue );

		if ( m_AchievementSettings->m_bReduced )
		{
			SetTall( YRES( m_iTall_Reduced ) );
		}
		else
		{
			if ( m_AchievementSettings->m_bExpanded )
			{
				SetTall( YRES( m_iTall_Expanded ) );

				// Show the requirements (Generic)
				if ( m_pComponentsList && m_pSourceAchievement->GetFlags() & ACH_HAS_COMPONENTS )
				{
					m_pComponentsList->SetFirstColumnWidth( 0 );

					for ( int i = 0; i < m_pSourceAchievement->GetGoal(); i++ )
					{
						vgui::Label *pComponentLabel = new vgui::Label( this, "AchievementComponents", m_pSourceAchievement->GetComponentNames()[i] );

						pComponentLabel->SetEnabled( ( m_pSourceAchievement->GetComponentBits() & ( ( uint64 ) 1 ) << i ) );

						m_pComponentsList->AddItem( NULL, pComponentLabel );
					}
				}
			}
			else
			{
				SetTall( YRES( m_iTall ) );
			}
		}

		// Display percentage completion for progressive achievements
		// Set up total completion percentage bar. Goal > 1 means its a progress achievement.
		UpdateProgressBar( this, m_pSourceAchievement, m_clrProgressBar );

		// Setup icon
		// get the vtfFilename from the path.
		if ( m_pSourceAchievement->IsAchieved() )
		{
			LoadAchievementIcon( m_pAchievementIcon, m_pSourceAchievement );

			if ( m_pShowOnHUDCheck )
			{
				m_pShowOnHUDCheck->SetVisible( false );
				m_pShowOnHUDCheck->SetSelected( false );
			}
		}
		else
		{
			LoadAchievementIcon( m_pAchievementIcon, m_pSourceAchievement, "_bw" );

			if ( m_pShowOnHUDCheck )
			{
				m_pShowOnHUDCheck->SetVisible( !m_pSourceAchievement->ShouldHideUntilAchieved() );	// m_pSourceAchievement->GetGoal() > 1 && 
				m_pShowOnHUDCheck->SetSelected( m_pSourceAchievement->ShouldShowOnHUD() );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes a local copy of a pointer to the achievement entity stored on the client.
//-----------------------------------------------------------------------------
void CTFAchievementDialogItemPanel::SetAchievementInfo( IAchievement* pAchievement )
{
	if ( !pAchievement )
	{
		Assert( 0 );
		return;
	}

	m_pSourceAchievement = dynamic_cast< CBaseAchievement * >( pAchievement );
	m_iSourceAchievementIndex = pAchievement->GetAchievementID();
}


void CTFAchievementDialogItemPanel::OnCommand( const char* command )
{
	CTFAchievementsDialog *pParentPanel = GET_MAINMENUPANEL( CTFAchievementsDialog );

	if ( !pParentPanel )
		return BaseClass::OnCommand( command );

	if ( !Q_strcasecmp( command, "reduce" ) )
	{
		m_AchievementSettings->m_bReduced = !m_AchievementSettings->m_bReduced;
		m_AchievementSettings->m_bExpanded = false;

		InvalidateLayout( false, true );
		m_pParent->InvalidateLayout();
	}
	else if ( !Q_strcasecmp( command, "expand" ) )
	{
		m_AchievementSettings->m_bReduced = false;
		m_AchievementSettings->m_bExpanded = !m_AchievementSettings->m_bExpanded;

		if ( m_pSourceAchievement->GetAchievementID() != 1 ) // Achievement ID holds an easter egg the strings should only be visible on all other achievements
			m_pExpandButton->SetText( m_AchievementSettings->m_bExpanded ? "#TF_Achievement_HideDetails" : "#TF_Achievement_ShowDetails" );
		
		UpdateAchievementInfo();
		m_pParent->InvalidateLayout();
	}
	else if ( V_stristr( command, "play " ) )
	{
		engine->ClientCmd_Unrestricted( command );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CTFAchievementDialogItemPanel::ToggleShowOnHUD( void )
{
	m_pShowOnHUDCheck->SetSelected( !m_pShowOnHUDCheck->IsSelected() );
}

void CTFAchievementDialogItemPanel::OnCheckButtonChecked( Panel *panel )
{
	if ( panel == m_pShowOnHUDCheck && m_pSourceAchievement )
	{
		m_pSourceAchievement->SetShowOnHUD( m_pShowOnHUDCheck->IsSelected() );
	}
}
