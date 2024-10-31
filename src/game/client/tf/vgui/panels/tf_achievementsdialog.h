#ifndef TFACHIEVEMENTSDIALOG_H
#define TFACHIEVEMENTSDIALOG_H

#include "tf_dialogpanelbase.h"
#include "vgui/controls/tf_advbutton.h"
#include "vgui_controls/PanelListPanel.h"
#include "vgui_controls/Label.h"
#include "tier1/KeyValues.h"

#include "achievementmgr.h"
#include "achievements_tf.h"
#include "achievements_220_tf.h"

class IAchievement;

#define ACHIEVED_ICON_PATH "hud/icon_check.vtf"
#define LOCK_ICON_PATH "hud/icon_locked.vtf"

typedef struct
{
	bool m_bReduced;
	bool m_bExpanded;
} achievement_item_settings_t;

// Loads an achievement's icon into a specified image panel, or turns the panel off if no achievement icon was found.
bool LoadAchievementIcon( vgui::ImagePanel* pIconPanel, IAchievement *pAchievement, const char *pszExt = NULL );

// Updates a listed achievement item's progress bar. 
void UpdateProgressBar( vgui::EditablePanel* pPanel, IAchievement *pAchievement, Color clrProgressBar );


class CTFAchievementsDialog : public CTFDialogPanelBase
{
	DECLARE_CLASS_SIMPLE( CTFAchievementsDialog, CTFDialogPanelBase );

public:
	CTFAchievementsDialog( vgui::Panel* parent, const char *panelName );
	virtual ~CTFAchievementsDialog();

	void Show();
	void Hide();

	void CreateAchievementList( void );

	void RefreshAchievementsProgressBar( int iCount, int iNumAchievements, int iNumUnlocked, int iTotalScore );
	void RefreshAchievementPackList( void );
	void RefreshAchievementList( void );
	void Reset( void );
	
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void UpdateAchievementDialogInfo( void );
	void OnCommand( const char* command );

	virtual void AppearAnimation();

	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_PARAMS( OnButtonChecked, "CheckButtonChecked", data );
	MESSAGE_FUNC( OnWarningAccepted, "OnWarningAccepted" );

	void CreateNewAchievementGroup( int iMinRange, int iMaxRange );

	bool					m_bInitialized;

	vgui::IScheme			*m_pScheme;

	vgui::PanelListPanel	*m_pAchievementsList;
	vgui::ImagePanel		*m_pListBG;

	vgui::ImagePanel		*m_pPercentageBarBackground;
	vgui::ImagePanel		*m_pPercentageBar;

	vgui::ComboBox			*m_pAchievementPackCombo;

	vgui::CheckButton		*m_pHideAchievedCheck;

	int m_nTotalScore;

	int m_iFixedWidth;

	typedef struct 
	{
		int m_iMinRange;
		int m_iMaxRange;
		int m_iNumAchievements;
		int m_iNumUnlocked;
	} achievement_group_t;

	int m_iNumAchievementGroups;

	achievement_group_t m_AchievementGroups[15];

	achievement_item_settings_t m_AchievementItemSettings[TF2C_ACHIEVEMENT_COUNT];

};

//////////////////////////////////////////////////////////////////////////
// Individual item panel, displaying stats for one achievement
class CTFAchievementDialogItemPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFAchievementDialogItemPanel, vgui::EditablePanel );

public:
	CTFAchievementDialogItemPanel( vgui::PanelListPanel *parent, const char* name, int iListItemID, achievement_item_settings_t *settings );
	~CTFAchievementDialogItemPanel();

	void OnCommand( const char* command );

	void SetAchievementInfo( IAchievement* pAchievement );
	void UpdateAchievementInfo();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void ToggleShowOnHUD( void );

	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );

private:
	achievement_item_settings_t *m_AchievementSettings;

	CBaseAchievement* m_pSourceAchievement;
	int	m_iSourceAchievementIndex;

	vgui::PanelListPanel	*m_pParent;

	vgui::ImagePanel		*m_pAchievementIcon;
	vgui::ImagePanel		*m_pPercentageBar;
	vgui::PanelListPanel	*m_pComponentsList;

	vgui::CheckButton		*m_pShowOnHUDCheck;

	CTFButton				*m_pExpandButton;

	CPanelAnimationVar( Color, m_clrProgressBar, "ProgressBarColor", "140 140 140 255" );

	CPanelAnimationVar( int, m_iTall, "tall", "0" );
	CPanelAnimationVar( int, m_iTall_Reduced, "tall_reduced", "0" );
	CPanelAnimationVar( int, m_iTall_Expanded, "tall_expanded", "0" );

	int m_iListItemID;

};

#endif // TFTESTMENU_H