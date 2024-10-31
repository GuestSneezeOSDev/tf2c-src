//=============================================================================
//
// Purpose: 
//
//=============================================================================
#ifndef TF_PLAYERMODELPANEL_H
#define TF_PLAYERMODELPANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "basemodel_panel.h"
#include "ichoreoeventcallback.h"
#include "choreoscene.h"

enum ePlayerEffects
{
	kNone	= 0,
	kCloak	= ( 1 << 0 ),
	kCrit	= ( 1 << 1 ),
	kInvuln	= ( 1 << 2 ),
};

// #define ASYNC_MODEL_LOADING

#ifdef ASYNC_MODEL_LOADING
struct LoadingModel_t
{
	CEconItemView *m_pItem = NULL;
	CRefCountedModelIndex m_iModelIndex;
	ETFLoadoutSlot m_iSlot = TF_LOADOUT_SLOT_INVALID;
};
#endif

struct ExtraWearableModelData
{
	int iModelHandle;
	bool bHideOnActiveWeapon;
};

class CTFPlayerModelPanel : public CBaseModelPanel, public CDefaultClientRenderable, public IChoreoEventCallback, public IHasLocalToGlobalFlexSettings
#ifdef ASYNC_MODEL_LOADING
	, public IModelLoadCallback
#endif
{
public:
	DECLARE_CLASS_SIMPLE( CTFPlayerModelPanel, CBaseModelPanel );

	CTFPlayerModelPanel( vgui::Panel *pParent, const char *pName );
	~CTFPlayerModelPanel();

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void PerformLayout( void );
	virtual void RequestFocus( int direction = 0 ) {}

	void LockStudioHdr();
	void UnlockStudioHdr();
	CStudioHdr *GetModelPtr( void ) { return m_pStudioHdr; }
	int LookupSequence( const char *pszName );
	float GetSequenceFrameRate( int nSequence );

	virtual void OnPaint3D() OVERRIDE;
	virtual void SetupFlexWeights( void );
	virtual void FireEvent( const char *pszEventName, const char *pszEventOptions );

	void SetToPlayerClass( int iClass );
	void SetToRandomClass( int iTeam );
	void SetTeam( int iTeam, bool bInvuln = false );
	ETFWeaponType GetAnimSlot( CEconItemView *pItem, int iClass );
	void LoadItems( ETFLoadoutSlot iHoldSlot = TF_LOADOUT_SLOT_INVALID );
	CEconItemView *GetItemInSlot( ETFLoadoutSlot iSlot );
	void HoldFirstValidItem( void );
	bool HoldItemInSlot( ETFLoadoutSlot iSlot );
	void AddCarriedItem( CEconItemView *pItem );
	void ClearCarriedItems( void );
	void SetCarriedItemVisibilityInSlot( int iSlot, bool bVisible );
#ifdef ITEM_TAUNTING
	void PlayTauntFromItem( CEconItemView *pItem );
#endif

	bool UpdateBodygroups( void );

#ifdef ASYNC_MODEL_LOADING
	// IModelLoadCallback
	virtual void	OnModelLoadComplete( const model_t *pModel );
#endif

	void InitPhonemeMappings( void );
	void SetFlexWeight( LocalFlexController_t index, float value );
	float GetFlexWeight( LocalFlexController_t index );
	void ResetFlexWeights( void );
	int FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key );
	LocalFlexController_t FindFlexController( const char *szName );

	void ProcessVisemes( Emphasized_Phoneme *classes );
	void AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted );
	void AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression );
	bool SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme );
	void ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity );
	
	// IHasLocalToGlobalFlexSettings
	virtual void EnsureTranslations( const flexsettinghdr_t *pSettinghdr );

	CChoreoScene *LoadScene( const char *filename );
	void PlayVCD( const char *pszFile );
	void StopVCD();
	void ProcessLoop( CChoreoScene *scene, CChoreoEvent *event );
	void ProcessSequence( CChoreoScene *scene, CChoreoEvent *event );
	void ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event );
	void ProcessFlexSettingSceneEvent( CChoreoScene *scene, CChoreoEvent *event );
	void AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr );

	// IChoreoEventCallback
	virtual void StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual void ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );
	virtual bool CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event );

	// IClientRenderable
	virtual const Vector&			GetRenderOrigin( void ) { return vec3_origin; }
	virtual const QAngle&			GetRenderAngles( void ) { return vec3_angle; }
	virtual void					GetRenderBounds( Vector& mins, Vector& maxs )
	{
		GetBoundingBox( mins, maxs );
	}
	virtual const matrix3x4_t &		RenderableToWorldTransform()
	{
		static matrix3x4_t mat;
		SetIdentityMatrix( mat );
		return mat;
	}
	virtual bool					ShouldDraw( void ) { return false; }
	virtual bool					IsTransparent( void ) { return false; }
	virtual bool					ShouldReceiveProjectedTextures( int flags ) { return false; }

	// Team Fortress 2 Classic
	bool							ShouldDisplayPlayerEffect( ePlayerEffects eEffect ) { return ( m_iPlayerEffectsFlags & eEffect ); }
	bool							SoundEventAllowed( void ) { return m_bSoundEventAllowed; }
	void							SetSoundEventAllowed( bool bAllowed ) { m_bSoundEventAllowed = bAllowed; }

private:
	CStudioHdr *m_pStudioHdr;
	CThreadFastMutex m_StudioHdrInitLock;

	int	m_nBody;
	int m_iTeamNum;
	int m_iClass;
	bool m_bCustomClassData[TF_CLASS_COUNT_ALL];
	BMPResData_t m_ClassResData[TF_CLASS_COUNT_ALL];
	CUtlVector<CEconItemView *> m_Items;

	int m_aMergeMDLMap[TF_LOADOUT_SLOT_COUNT];
	CUtlVector<int> m_aMergeAttachedMDLMap[TF_LOADOUT_SLOT_COUNT];
	ExtraWearableModelData m_aMergeExtraWearableMDLMap[TF_LOADOUT_SLOT_COUNT];
	ETFLoadoutSlot m_iActiveWeaponSlot;
#ifdef ITEM_TAUNTING
	int m_iTauntMDLIndex;
#endif

#ifdef ASYNC_MODEL_LOADING
	CUtlVector<LoadingModel_t> m_vecLoadingModels;
#endif

	CChoreoScene *m_pScene;
	float m_flCurrentTime;
	bool m_bFlexEvents;

	CUtlRBTree<FS_LocalToGlobal_t, unsigned short> m_LocalToGlobal;
	Emphasized_Phoneme m_PhonemeClasses[NUM_PHONEME_CLASSES];
	float m_flexWeight[MAXSTUDIOFLEXCTRL];

	// Team Fortress 2 Classic
	int m_iPlayerEffectsFlags;
	bool m_bDisableFrameAdvancement;
	bool m_bSoundEventAllowed;

};

#endif // TF_PLAYERMODELPANEL_H
