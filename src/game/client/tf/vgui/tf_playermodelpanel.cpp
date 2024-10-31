//=============================================================================
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_playermodelpanel.h"
#include "tf_playerclass_shared.h"
#include "tf_inventory.h"
#include "c_tf_player.h"
#include "c_te_effect_dispatch.h"
#include <engine/IEngineSound.h>
#include "scenefilecache/ISceneFileCache.h"
#include "c_sceneentity.h"
#include "c_baseflex.h"
#include <sentence.h>
#include "animation.h"
#include <bone_setup.h>
#include "matsys_controls/matsyscontrols.h"
#include <vgui/ISurface.h>
#include "cl_animevent.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFPlayerModelPanel );

CTFPlayerModelPanel::CTFPlayerModelPanel( Panel *pParent, const char *pName ) : BaseClass( pParent, pName ),
m_LocalToGlobal( 0, 0, FlexSettingLessFunc )
{
	SetKeyBoardInputEnabled( false );
	SetIgnoreDoubleClick( true );

	m_nBody = 0;
	m_iTeamNum = TF_TEAM_RED;
	m_iClass = TF_CLASS_UNDEFINED;
	m_iActiveWeaponSlot = TF_LOADOUT_SLOT_INVALID;
#ifdef ITEM_TAUNTING
	m_iTauntMDLIndex = -1;
#endif
	m_pStudioHdr = NULL;
	memset( m_bCustomClassData, 0, sizeof( m_bCustomClassData ) );

	m_pScene = NULL;
	m_flCurrentTime = 0.0f;
	m_bFlexEvents = false;

	memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );
	memset( m_flexWeight, 0, sizeof( m_flexWeight ) );

	for ( int i = TF_LOADOUT_SLOT_PRIMARY; i < TF_LOADOUT_SLOT_COUNT; i++ )
	{
		m_aMergeMDLMap[i] = -1;
		m_aMergeExtraWearableMDLMap[i].iModelHandle = -1;
		m_aMergeExtraWearableMDLMap[i].bHideOnActiveWeapon = false;
	}

	// Team Fortress 2 Classic
	m_iPlayerEffectsFlags = 0;
	m_bSoundEventAllowed = true;

	for ( int iClass = TF_FIRST_NORMAL_CLASS; iClass < TF_CLASS_COUNT_ALL; iClass++ )
	{
		TFPlayerClassData_t *pClassData = GetPlayerClassData( iClass );

		// Load the models for each class.
		const char *pszClassModel = pClassData->m_szModelName;
		if ( pszClassModel[0] != '\0' )
		{
			engine->LoadModel( pszClassModel );
		}

		const char *pszClassHWMModel = pClassData->m_szHWMModelName;
		if ( pszClassHWMModel[0] != '\0' )
		{
			engine->LoadModel( pszClassHWMModel );
		}
	}

	// Load phonemes for MP3s.
	engine->AddPhonemeFile( "scripts/game_sounds_vo_phonemes.txt" );
	engine->AddPhonemeFile( NULL );
}


CTFPlayerModelPanel::~CTFPlayerModelPanel()
{
	StopVCD();
	UnlockStudioHdr();
	m_LocalToGlobal.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Load in the model portion of the panel's resource file.
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	static ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
	if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
	{
		inResourceData->ProcessResolutionKeys( "_minmode" );
	}

	// Parse per class model settings.
	KeyValues *pClassKeys = inResourceData->FindKey( "customclassdata" );
	if ( pClassKeys )
	{
		for ( KeyValues *pSubData = pClassKeys->GetFirstSubKey(); pSubData; pSubData = pSubData->GetNextKey() )
		{
			int iClass = UTIL_StringFieldToInt( pSubData->GetName(), g_aPlayerClassNames_NonLocalized, TF_CLASS_COUNT_ALL );
			if ( iClass == -1 )
				continue;

			m_ClassResData[iClass].m_flFOV = pSubData->GetFloat( "fov" );
			m_ClassResData[iClass].m_angModelPoseRot.Init( pSubData->GetFloat( "angles_x", 0.0f ), pSubData->GetFloat( "angles_y", 0.0f ), pSubData->GetFloat( "angles_z", 0.0f ) );
			m_ClassResData[iClass].m_vecOriginOffset.Init( pSubData->GetFloat( "origin_x", 110.0 ), pSubData->GetFloat( "origin_y", 5.0 ), pSubData->GetFloat( "origin_z", 5.0 ) );
			m_bCustomClassData[iClass] = true;
		}
	}

	if ( m_bCustomClassData[m_iClass] )
	{
		SetCameraFOV( m_ClassResData[m_iClass].m_flFOV );
		m_vecPlayerPos = m_ClassResData[m_iClass].m_vecOriginOffset;
		m_angPlayer = m_ClassResData[m_iClass].m_angModelPoseRot;
	}

	// Team Fortress 2 Classic
	m_iPlayerEffectsFlags = inResourceData->GetInt( "display_player_effects", 0 );
	m_bDisableFrameAdvancement = inResourceData->GetBool( "disable_frame_advancement", false );
}


void CTFPlayerModelPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();
}


void CTFPlayerModelPanel::OnPaint3D( void )
{
	if ( m_RootMDL.m_MDL.GetMDL() == MDLHANDLE_INVALID )
		return;

	StudioRenderConfig_t oldStudioRenderConfig;
	StudioRender()->GetCurrentConfig( oldStudioRenderConfig );

	UpdateStudioRenderConfig();

	CMatRenderContextPtr pRenderContext( vgui::MaterialSystem() );
	if ( vgui::MaterialSystemHardwareConfig()->GetHDRType() == HDR_TYPE_NONE )
	{
		ITexture *pMyCube = HasLightProbe() ? GetLightProbeCubemap( false ) : m_DefaultEnvCubemap;
		pRenderContext->BindLocalCubemap( pMyCube );
	}
	else
	{
		ITexture *pMyCube = HasLightProbe() ? GetLightProbeCubemap( true ) : m_DefaultHDREnvCubemap;
		pRenderContext->BindLocalCubemap( pMyCube );
	}

	PrePaint3D( pRenderContext );

	if ( m_bGroundGrid )
	{
		DrawGrid();
	}

	if ( m_bLookAtCamera )
	{
		matrix3x4_t worldToCamera;
		ComputeCameraTransform( &worldToCamera );

		Vector vecPosition;
		MatrixGetColumn( worldToCamera, 3, vecPosition );
		m_RootMDL.m_MDL.m_bWorldSpaceViewTarget = true;
		m_RootMDL.m_MDL.m_vecViewTarget = vecPosition;
	}

	// Draw the MDL
	CStudioHdr studioHdr( g_pMDLCache->GetStudioHdr( m_RootMDL.m_MDL.GetMDL() ), g_pMDLCache );

	SetupFlexWeights();

	matrix3x4_t *pBoneToWorld = g_pStudioRender->LockBoneMatrices( studioHdr.numbones() );
	m_RootMDL.m_MDL.SetUpBones( m_RootMDL.m_MDLToWorld, studioHdr.numbones(), pBoneToWorld, m_PoseParameters, m_SequenceLayers, m_nNumSequenceLayers );
	g_pStudioRender->UnlockBoneMatrices();

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	bool bInvuln = ( pPlayer && ShouldDisplayPlayerEffect( kInvuln ) && ( pPlayer->m_Shared.IsInvulnerable() && !pPlayer->m_Shared.InCond( TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED ) ) );

#ifndef TF2C_BETA
	if (m_iTeamNum == TF_TEAM_GLOBAL)
	{
		CMaterialReference m_TestMaterial;
		m_TestMaterial.Init("dev/global_disguise_gray", TEXTURE_GROUP_CLIENT_EFFECTS);
		g_pStudioRender->ForcedMaterialOverride(m_TestMaterial);
		bInvuln = false;
	}
#endif

	if ( bInvuln )
	{
		SetTeam( m_iTeamNum, true );
	}

	m_RootMDL.m_MDL.Draw( m_RootMDL.m_MDLToWorld, pBoneToWorld );

	if ( bInvuln )
	{
		SetTeam( m_iTeamNum );
	}

	// Draw the merge MDLs.
	matrix3x4_t matMergeBoneToWorld[MAXSTUDIOBONES];
	for ( int iMerge = 0; iMerge < m_aMergeMDLs.Count(); ++iMerge )
	{
		if ( m_aMergeMDLs[iMerge].m_bDisabled )
			continue;

		// Get the merge studio header.
		studiohdr_t *pStudioHdr = g_pMDLCache->GetStudioHdr( m_aMergeMDLs[iMerge].m_MDL.GetMDL() );
		matrix3x4_t *pMergeBoneToWorld = &matMergeBoneToWorld[0];

		// If we have a valid mesh, bonemerge it.
		// If we have an invalid mesh we can't bonemerge because it'll crash trying to pull data from the missing header.
		if ( pStudioHdr )
		{
			CStudioHdr mergeHdr( pStudioHdr, g_pMDLCache );
			m_aMergeMDLs[iMerge].m_MDL.SetupBonesWithBoneMerge( &mergeHdr, pMergeBoneToWorld, &studioHdr, pBoneToWorld, m_RootMDL.m_MDLToWorld );		

			if ( bInvuln )
			{
#ifdef TF2C_BETA
				g_pStudioRender->ForcedMaterialOverride( *pPlayer->GetInvulnMaterialRefPlayerModelPanel() );
#else
				g_pStudioRender->ForcedMaterialOverride(*pPlayer->GetInvulnMaterialRef());
#endif
			}
			/*else if ( m_iActiveWeaponSlot != TF_LOADOUT_SLOT_INVALID && iMerge == m_aMergeMDLMap[m_iActiveWeaponSlot] )
			{
			}*/
			

			m_aMergeMDLs[iMerge].m_MDL.Draw( m_aMergeMDLs[iMerge].m_MDLToWorld, pMergeBoneToWorld );

			if ( bInvuln )
			{
				g_pStudioRender->ForcedMaterialOverride( NULL );
			}
		}
	}

#ifndef TF2C_BETA
	if (m_iTeamNum == TF_TEAM_GLOBAL)
		g_pStudioRender->ForcedMaterialOverride(NULL);
#endif

	PostPaint3D( pRenderContext );

	if ( m_bDrawCollisionModel )
	{
		DrawCollisionModel();
	}

	pRenderContext->Flush();
	StudioRender()->UpdateConfig( oldStudioRenderConfig );
}


void CTFPlayerModelPanel::SetupFlexWeights( void )
{
	if ( m_RootMDL.m_MDL.m_MDLHandle != MDLHANDLE_INVALID && m_pStudioHdr )
	{
		if ( m_pStudioHdr->pFlexcontroller( LocalFlexController_t( 0 ) )->localToGlobal == -1 )
		{
			// Base class should have initialized flex controllers.
			Assert( false );
			for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
			{
				int j = C_BaseFlex::AddGlobalFlexController( m_pStudioHdr->pFlexcontroller( i )->pszName() );
				m_pStudioHdr->pFlexcontroller( i )->localToGlobal = j;
			}
		}

		memset( m_RootMDL.m_MDL.m_pFlexControls, 0, sizeof( m_RootMDL.m_MDL.m_pFlexControls ) );
		float *pFlexWeights = m_RootMDL.m_MDL.m_pFlexControls;

		if ( m_pScene )
		{
			// slowly decay to neutral expression
			for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
			{
				SetFlexWeight( i, GetFlexWeight( i ) * 0.95 );
			}

			// Run choreo scene.
			m_bFlexEvents = true;
			m_pScene->Think( m_flCurrentTime );
		}

		// Convert local weights from 0..1 to real dynamic range.
		// FIXME: Possibly get rid of this?
		for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
		{
			mstudioflexcontroller_t *pflex = m_pStudioHdr->pFlexcontroller( i );
			pFlexWeights[pflex->localToGlobal] = RemapValClamped( m_flexWeight[i], 0.0f, 1.0f, pflex->min, pflex->max );
		}

		if ( m_pScene )
		{
			// Run choreo scene.
			m_bFlexEvents = false;
			m_pScene->Think( m_flCurrentTime );
			m_flCurrentTime += gpGlobals->frametime;

			if ( m_pScene->SimulationFinished() )
			{
				StopVCD();
			}
		}

		// Drive the mouth from .wav file playback...
		ProcessVisemes( m_PhonemeClasses );

#if 0
		// Convert back to normalized weights
		for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
		{
			mstudioflexcontroller_t *pflex = m_pStudioHdr->pFlexcontroller( i );

			// rescale
			if ( pflex->max != pflex->min )
			{
				pFlexWeights[pflex->localToGlobal] = ( pFlexWeights[pflex->localToGlobal] - pflex->min ) / ( pflex->max - pflex->min );
			}
		}
#endif
	}
}


void CTFPlayerModelPanel::FireEvent( const char *pszEventName, const char *pszEventOptions )
{
	if ( V_stricmp( pszEventName, "AE_WPN_HIDE" ) == 0 )
	{
#ifdef ITEM_TAUNTING
		if ( m_iTauntMDLIndex != -1 )
		{
			m_aMergeMDLs[m_iTauntMDLIndex].m_bDisabled = true;
		}
		else
#endif
		{
			SetCarriedItemVisibilityInSlot( m_iActiveWeaponSlot, false );

			// Reset the bodygroups when we hide our weapon.
			m_nBody = 0;
			UpdateBodygroups();
		}
	}
	else if ( V_stricmp( pszEventName, "AE_WPN_UNHIDE" ) == 0 )
	{
#ifdef ITEM_TAUNTING
		if ( m_iTauntMDLIndex != -1 )
		{
			m_aMergeMDLs[m_iTauntMDLIndex].m_bDisabled = false;
		}
		else
#endif
		{
			SetCarriedItemVisibilityInSlot( m_iActiveWeaponSlot, true );
			
			// Reset the bodygroups when we unhide our weapon.
			m_nBody = 0;
			UpdateBodygroups();
		}
	}
	else if ( SoundEventAllowed() && ( V_stricmp( pszEventName, "AE_CL_PLAYSOUND" ) == 0 || atoi( pszEventName ) == CL_EVENT_SOUND ) )
	{
		C_RecipientFilter filter;
		C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, pszEventOptions );
	}
}


void CTFPlayerModelPanel::LockStudioHdr()
{
	MDLHandle_t hModel = m_RootMDL.m_MDL.m_MDLHandle;

	Assert( m_pStudioHdr == NULL );
	AUTO_LOCK( m_StudioHdrInitLock );

	if ( m_pStudioHdr )
	{
		Assert( m_pStudioHdr->GetRenderHdr() == mdlcache->GetStudioHdr( hModel ) );
		return;
	}

	if ( hModel == MDLHANDLE_INVALID )
		return;

	const studiohdr_t *pStudioHdr = mdlcache->LockStudioHdr( hModel );
	if ( !pStudioHdr )
		return;

	CStudioHdr *pNewWrapper = new CStudioHdr( pStudioHdr, mdlcache );
	Assert( pNewWrapper->IsValid() );

	if ( pNewWrapper->GetVirtualModel() )
	{
		MDLHandle_t hVirtualModel = (MDLHandle_t)(int)( pStudioHdr->virtualModel ) & 0xffff;
		mdlcache->LockStudioHdr( hVirtualModel );
	}

	m_pStudioHdr = pNewWrapper; // must be last to ensure virtual model correctly set up
}


void CTFPlayerModelPanel::UnlockStudioHdr()
{
	MDLHandle_t hModel = m_RootMDL.m_MDL.m_MDLHandle;
	if ( hModel == MDLHANDLE_INVALID )
		return;

	studiohdr_t *pStudioHdr = mdlcache->GetStudioHdr( hModel );
	Assert( m_pStudioHdr && m_pStudioHdr->GetRenderHdr() == pStudioHdr );

	if ( pStudioHdr->GetVirtualModel() )
	{
		MDLHandle_t hVirtualModel = (MDLHandle_t)(int)pStudioHdr->virtualModel & 0xffff;
		mdlcache->UnlockStudioHdr( hVirtualModel );
	}

	mdlcache->UnlockStudioHdr( hModel );

	delete m_pStudioHdr;
	m_pStudioHdr = NULL;
}


int CTFPlayerModelPanel::LookupSequence( const char *pszName )
{
	return ::LookupSequence( GetModelPtr(), pszName );
}


float CTFPlayerModelPanel::GetSequenceFrameRate( int nSequence )
{
	// Team Fortress 2 Classic
	if ( m_bDisableFrameAdvancement )
		return 0.0f;
	
	static float flPoseParameters[MAXSTUDIOPOSEPARAM] = {};
	return Studio_FPS( GetModelPtr(), nSequence, flPoseParameters );
}


void CTFPlayerModelPanel::SetToPlayerClass( int iClass )
{
	if ( m_iClass == iClass )
		return;

	m_iClass = iClass;

	ClearCarriedItems();
	UnlockStudioHdr();
	SetMDL( GetPlayerClassData( m_iClass )->GetModelName(), GetClientRenderable() );
	LockStudioHdr();
	
	ResetFlexWeights();
	InitPhonemeMappings();

	SetLookAtCamera( false );

	// Use custom class settings if we have them.
	if ( m_bCustomClassData[m_iClass] )
	{
		SetCameraFOV( m_ClassResData[m_iClass].m_flFOV );
		SetModelAnglesAndPosition( m_ClassResData[m_iClass].m_angModelPoseRot, m_ClassResData[m_iClass].m_vecOriginOffset );
	}
	else
	{
		SetCameraFOV( m_BMPResData.m_flFOV );
		SetModelAnglesAndPosition( m_BMPResData.m_angModelPoseRot, m_BMPResData.m_vecOriginOffset );
	}
}

//-----------------------------------------------------------------------------
// Purpose: For class select menu.
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::SetToRandomClass( int iTeam )
{
	m_iClass = TF_CLASS_UNDEFINED;

	ClearCarriedItems();
	UnlockStudioHdr();
	SetMDL( "models/class_menu/random_class_icon.mdl", GetClientRenderable() );
	LockStudioHdr();

	SetSequence( LookupSequence( "selection" ) );
	SetSkin( GetTeamSkin( iTeam ) );

	SetCameraFOV( m_BMPResData.m_flFOV );
	SetModelAnglesAndPosition( m_BMPResData.m_angModelPoseRot, m_BMPResData.m_vecOriginOffset );
}


void CTFPlayerModelPanel::SetTeam( int iTeam, bool bInvuln /*= false*/ )
{
	m_iTeamNum = iTeam;

	// Update skins.
	int nSkin = 0;

	switch ( m_iTeamNum )
	{
		// Global uses red skins
		// it gets overriden with materials anyways
#ifndef TF2C_BETA
		case TF_TEAM_GLOBAL:
#endif
		case TF_TEAM_RED:
			nSkin = 0;
			break;
		case TF_TEAM_BLUE:
			nSkin = 1;
			break;
		case TF_TEAM_GREEN:
			nSkin = 4;
			break;
		case TF_TEAM_YELLOW:
			nSkin = 5;
			break;
#ifdef TF2C_BETA
		case TF_TEAM_GLOBAL:
			nSkin = 8;
			break;
#endif
	}

	if ( bInvuln && ( m_iClass > TF_CLASS_UNDEFINED && m_iClass < TF_CLASS_COUNT_ALL ) )
	{
		nSkin += 2;
#ifdef TF2C_BETA
		if ( m_iTeamNum == TF_TEAM_GLOBAL )
			nSkin = 9; // odd number of teams and i dont want to add a blank one in the model. that would be ugly. we are never adding new teams in a billion trillion years
#endif
	}

	SetSkin( nSkin );

#ifdef TF2C_BETA
	int nAttachmentSkin = m_iTeamNum;
#else
	int nAttachmentSkin = m_iTeamNum == TF_TEAM_GLOBAL ? TF_TEAM_RED : m_iTeamNum;
#endif

	FOR_EACH_VEC( m_aMergeMDLs, i )
	{
		m_aMergeMDLs[i].m_MDL.m_nSkin = GetTeamSkin( nAttachmentSkin );
	}
}

static ETFWeaponType g_iRemapLoadoutToAnim[TF_LOADOUT_SLOT_COUNT] =
{
	TF_WPN_TYPE_PRIMARY,
	TF_WPN_TYPE_SECONDARY,
	TF_WPN_TYPE_MELEE,
	TF_WPN_TYPE_GRENADE,
	TF_WPN_TYPE_BUILDING,
	TF_WPN_TYPE_PDA,
	TF_WPN_TYPE_PDA,
};


ETFWeaponType CTFPlayerModelPanel::GetAnimSlot( CEconItemView *pItem, int iClass )
{
	CEconItemDefinition *pStatic = pItem->GetStaticData();
	if ( !pStatic )
		return TF_WPN_TYPE_INVALID;

	if ( pStatic->IsAWearable() )
		return TF_WPN_TYPE_NOT_USED;

	ETFWeaponType iSlot = pStatic->anim_slot;
	if ( iSlot == TF_WPN_TYPE_INVALID )
	{
		// If the schema did not set anim type pick it according to loadout slot.
		iSlot = g_iRemapLoadoutToAnim[pStatic->GetLoadoutSlot( iClass )];
	}

	return iSlot;
}


void CTFPlayerModelPanel::LoadItems( ETFLoadoutSlot iHoldSlot /*= TF_LOADOUT_SLOT_INVALID*/ )
{
	ClearCarriedItems();

	// Iterate through all items and add their models.
	ETFLoadoutSlot iSlot;
	int iPreset;
	for ( int i = TF_LOADOUT_SLOT_PRIMARY; i < TF_LOADOUT_SLOT_COUNT; i++ )
	{
		iSlot = (ETFLoadoutSlot)i;
		iPreset = GetTFInventory()->GetItemPreset( m_iClass, iSlot );
		AddCarriedItem( GetTFInventory()->GetItem( m_iClass, iSlot, iPreset ) );
	}

	if ( iHoldSlot != TF_LOADOUT_SLOT_INVALID )
	{
		if ( !HoldItemInSlot( iHoldSlot ) )
		{
			HoldFirstValidItem();
		}
	}
	else
	{
		HoldFirstValidItem();
	}
}


CEconItemView *CTFPlayerModelPanel::GetItemInSlot( ETFLoadoutSlot iSlot )
{
	FOR_EACH_VEC( m_Items, i )
	{
		if ( m_Items[i]->GetLoadoutSlot( m_iClass ) == iSlot )
			return m_Items[i];
	}

	return NULL;
}


void CTFPlayerModelPanel::HoldFirstValidItem( void )
{
	for ( int i = TF_LOADOUT_SLOT_PRIMARY; i < TF_LOADOUT_SLOT_COUNT; i++ )
	{
		if ( HoldItemInSlot( (ETFLoadoutSlot)i ) )
			break;
	}
}


bool CTFPlayerModelPanel::HoldItemInSlot( ETFLoadoutSlot iSlot )
{
	CEconItemView *pItem = GetItemInSlot( iSlot );
	if ( !pItem )
		return false;

	int iAnimSlot = GetAnimSlot( pItem, m_iClass );
	if ( iAnimSlot < 0 )
		return false;

	// Hide the previous item.
	SetCarriedItemVisibilityInSlot( m_iActiveWeaponSlot, false );

	// Set the model animation.
	SetModelAnim( FindAnimByName( g_AnimSlots[iAnimSlot] ) );
	m_RootMDL.m_MDL.m_flPlaybackRate = GetSequenceFrameRate( m_RootMDL.m_MDL.m_nSequence );
	SetPoseParameterByName( "r_hand_grip", 0.0f );

	// Now show the current item.
	m_iActiveWeaponSlot = iSlot;
	SetCarriedItemVisibilityInSlot( m_iActiveWeaponSlot, true );

	// Update our bodygroups when we switch weapons.
	m_nBody = 0;
	UpdateBodygroups();
	return true;
}


void CTFPlayerModelPanel::AddCarriedItem( CEconItemView *pItem )
{
	if ( !pItem )
		return;

	CEconItemDefinition *pStatic = pItem->GetStaticData();
	if ( !pStatic )
		return;

	ETFLoadoutSlot iSlot = pStatic->GetLoadoutSlot( m_iClass );
	if ( iSlot == TF_LOADOUT_SLOT_INVALID )
		return;

	m_Items.AddToTail( pItem );

	const char *pszModel = pItem->GetWorldDisplayModel( m_iClass );
	if ( pszModel && pszModel[0] )
	{
#ifdef ASYNC_MODEL_LOADING
		int iModelIndex = modelinfo->RegisterDynamicModel( pszModel, true );
		if ( iModelIndex == -1 )
		{
			m_aMergeMDLMap[iSlot] = -1;
			return;
		}

		LoadingModel_t newlyLoadingModel;
		newlyLoadingModel.m_pItem = new CEconItemView;
		newlyLoadingModel.m_iModelIndex = iModelIndex;
		newlyLoadingModel.m_iSlot = iSlot;

		*newlyLoadingModel.m_pItem = *pItem;
		m_vecLoadingModels[m_vecLoadingModels.AddToTail()] = newlyLoadingModel;

		modelinfo->RegisterModelLoadCallback( iModelIndex, this, true );
#else
		// Add the model and remember its index in merge MDLs list.
#ifdef TF2C_BETA
		int nDesiredSkin = m_iTeamNum;
#else
		int nDesiredSkin = m_iTeamNum == TF_TEAM_GLOBAL ? TF_TEAM_RED : m_iTeamNum;
#endif
		MDLHandle_t hItemModel = SetMergeMDL( pszModel, GetClientRenderable(), GetTeamSkin( nDesiredSkin ) );
		m_aMergeMDLMap[iSlot] = GetMergeMDLIndex( hItemModel );

		// Set if attached models are viewable in thirdperson.
		int iNumAttachedModels = pStatic->GetNumAttachedModels( nDesiredSkin );
		m_aMergeAttachedMDLMap[iSlot].EnsureCapacity( iNumAttachedModels );

		attachedmodel_t	*pModel;
		for ( int i = 0; i < iNumAttachedModels; ++i )
		{
			pModel = pStatic->GetAttachedModelData( nDesiredSkin, i );
			if ( !pModel || !pModel->m_pszModelName || !( pModel->m_iModelDisplayFlags & kAttachedModelDisplayFlag_WorldModel ) )
				continue;
	
			MDLHandle_t hAttachmentModel = SetMergeMDL( pModel->m_pszModelName, GetClientRenderable(), GetTeamSkin( nDesiredSkin ) );
			m_aMergeAttachedMDLMap[iSlot].AddToTail( GetMergeMDLIndex( hAttachmentModel ) );
		}

#ifndef ASYNC_MODEL_LOADING
		// This is an extra wearable, like regular ones, it's never hidden.
		const char *pszExtraWearableModel = pItem->GetExtraWearableModel();
		if ( pszExtraWearableModel && pszExtraWearableModel[0] )
		{
			MDLHandle_t hExtraWearableModel = SetMergeMDL( pszExtraWearableModel, GetClientRenderable(), GetTeamSkin( nDesiredSkin ) );
			m_aMergeExtraWearableMDLMap[iSlot].iModelHandle = GetMergeMDLIndex( hExtraWearableModel );

			bool bHideOnActive = pItem->GetExtraWearableModelVisibilityRules();
			m_aMergeExtraWearableMDLMap[iSlot].bHideOnActiveWeapon = bHideOnActive;
		}
		else
		{
			m_aMergeExtraWearableMDLMap[iSlot].iModelHandle = -1;
			m_aMergeExtraWearableMDLMap[iSlot].bHideOnActiveWeapon = false;
		}
#endif
		// If this is not a wearable then hide the model.
		if ( !pStatic->IsAWearable() )
		{
			SetCarriedItemVisibilityInSlot( iSlot, false );
		}
#endif
	}
	else
	{
		m_aMergeMDLMap[iSlot] = -1;
	}
}


bool CTFPlayerModelPanel::UpdateBodygroups( void )
{
	if ( m_pStudioHdr )
	{
		ETFLoadoutSlot iSlot;
		const char *pszModelName;

		CEconItemView *pItem;
		CEconItemDefinition *pStatic;
		EconItemVisuals *pVisuals;

		unsigned int j;
		const char *pszBodyGroupName;
		int iBodygroup;
		FOR_EACH_VEC( m_Items, i )
		{
			pItem = m_Items[i];
			if ( !pItem )
				continue;

			iSlot = pItem->GetLoadoutSlot( m_iClass );
			pszModelName = pItem->GetExtraWearableModel();
			if ( pszModelName && !pszModelName[0] )
			{
				if ( m_aMergeMDLMap[iSlot] != -1 && m_aMergeMDLs[m_aMergeMDLMap[iSlot]].m_bDisabled )
					continue;

				pszModelName = pItem->GetWorldDisplayModel( m_iClass );
				if ( ( pszModelName && !pszModelName[0] )
#ifdef ASYNC_MODEL_LOADING
					|| modelinfo->IsDynamicModelLoading( modelinfo->GetModelIndex( pszModelName ) )
#endif
				)
					continue;
			}

			pStatic = pItem->GetStaticData();
			if ( !pStatic )
				continue;
		
			if ( pStatic->hide_bodygroups_deployed_only )
			{
				if ( !pStatic->IsAWearable() && m_iActiveWeaponSlot != iSlot )
					continue;
			}
#ifdef TF2C_BETA
			pVisuals = pStatic->GetVisuals( m_iTeamNum );
#else
			pVisuals = pStatic->GetVisuals(m_iTeamNum == TF_TEAM_GLOBAL ? TF_TEAM_RED : m_iTeamNum);
#endif
			if ( pVisuals )
			{
				for ( j = 0; j < pVisuals->player_bodygroups.Count(); j++ )
				{
					pszBodyGroupName = pVisuals->player_bodygroups.GetElementName( j );
					if ( pszBodyGroupName )
					{
						iBodygroup = ::FindBodygroupByName( m_pStudioHdr, pszBodyGroupName );
						if ( iBodygroup == -1 )
							continue;

						::SetBodygroup( m_pStudioHdr, m_nBody, iBodygroup, pVisuals->player_bodygroups.Element( j ) );
					}
				}
			}
		}

		SetBody( m_nBody );
		return true;
	}

	return false;
}


#ifdef ASYNC_MODEL_LOADING

void CTFPlayerModelPanel::OnModelLoadComplete( const model_t *pModel )
{
	CEconItemView *pItem = NULL;
	ETFLoadoutSlot iSlot = TF_LOADOUT_SLOT_INVALID;
	FOR_EACH_VEC_BACK( m_vecLoadingModels, i )
	{
		if ( modelinfo->GetModel( m_vecLoadingModels[i].m_iModelIndex ) == pModel )
		{
			pItem = m_vecLoadingModels[i].m_pItem;
			iSlot = m_vecLoadingModels[i].m_iSlot;
			m_vecLoadingModels.FastRemove( i );
			break;
		}
	}

	Assert( pItem );
	if ( pItem && iSlot != TF_LOADOUT_SLOT_INVALID )
	{
		CEconItemDefinition *pStatic = pItem->GetStaticData();
		if ( !pStatic )
			return;

		m_Items.AddToTail( pItem );

		MDLHandle_t hModel = modelinfo->GetCacheHandle( pModel );
		Assert( hModel != MDLHANDLE_INVALID );
		if ( hModel != MDLHANDLE_INVALID )
		{
#ifdef TF2C_BETA
			int nDesiredSkin = m_iTeamNum;
#else
			int nDesiredSkin = m_iTeamNum == TF_TEAM_GLOBAL ? TF_TEAM_RED : m_iTeamNum;
#endif
			// Add the model and remember its index in merge MDLs list.
			SetMergeMDL( hModel, GetClientRenderable(), GetTeamSkin( nDesiredSkin ) );
			m_aMergeMDLMap[iSlot] = GetMergeMDLIndex( hModel );

			// Set if attached models are viewable in thirdperson.
			int iNumAttachedModels = pStatic->GetNumAttachedModels( nDesiredSkin );
			m_aMergeAttachedMDLMap[iSlot].EnsureCapacity( iNumAttachedModels );

			attachedmodel_t	*pModel;
			for ( int i = 0; i < iNumAttachedModels; ++i )
			{
				pModel = pStatic->GetAttachedModelData( nDesiredSkin, i );
				if ( !pModel || !pModel->m_pszModelName || !( pModel->m_iModelDisplayFlags & kAttachedModelDisplayFlag_WorldModel ) )
					continue;
	
				MDLHandle_t hAttachmentModel = SetMergeMDL( pModel->m_pszModelName, GetClientRenderable(), GetTeamSkin( nDesiredSkin ) );
				m_aMergeAttachedMDLMap[iSlot].AddToTail( GetMergeMDLIndex( hAttachmentModel ) );
			}

			// If this is not a wearable then hide the model.
			if ( !pStatic->IsAWearable() && iSlot != m_iActiveWeaponSlot )
			{
				SetCarriedItemVisibilityInSlot( iSlot, false );
			}

			// Update our bodygroups when an item loads in.
			m_nBody = 0;
			UpdateBodygroups();
		}

		// This is an extra wearable, like regular ones, it's never hidden.
		const char *pszExtraWearableModel = pItem->GetExtraWearableModel();
		if ( pszExtraWearableModel && pszExtraWearableModel[0] )
		{
#ifdef TF2C_BETA
			SetMergeMDL( pszExtraWearableModel, GetClientRenderable(), GetTeamSkin( m_iTeamNum ) );
#else
			SetMergeMDL(pszExtraWearableModel, GetClientRenderable(), GetTeamSkin(m_iTeamNum == TF_TEAM_GLOBAL ? TF_TEAM_RED : m_iTeamNum));
#endif
		}
	}
}
#endif


void CTFPlayerModelPanel::ClearCarriedItems( void )
{
	StopVCD();
	ClearMergeMDLs();
#ifdef ASYNC_MODEL_LOADING
	m_vecLoadingModels.Purge();
#endif
	m_Items.RemoveAll();
	m_iActiveWeaponSlot = TF_LOADOUT_SLOT_INVALID;
	m_nBody = 0;

	for ( int i = TF_LOADOUT_SLOT_PRIMARY; i < TF_LOADOUT_SLOT_COUNT; i++ )
	{
		m_aMergeMDLMap[i] = -1;
		m_aMergeAttachedMDLMap[i].Purge();
		m_aMergeExtraWearableMDLMap[i].iModelHandle = -1;
		m_aMergeExtraWearableMDLMap[i].bHideOnActiveWeapon = false;
	}
}


void CTFPlayerModelPanel::SetCarriedItemVisibilityInSlot( int iSlot, bool bVisible )
{
	if ( iSlot > TF_LOADOUT_SLOT_INVALID )
	{
		if ( m_aMergeMDLMap[iSlot] != -1 )
		{
			m_aMergeMDLs[m_aMergeMDLMap[iSlot]].m_bDisabled = !bVisible;
		}

		FOR_EACH_VEC( m_aMergeAttachedMDLMap[iSlot], i )
		{
			m_aMergeMDLs[m_aMergeAttachedMDLMap[iSlot].Element( i )].m_bDisabled = !bVisible;
		}

		if ( m_aMergeExtraWearableMDLMap[iSlot].iModelHandle != -1 )
		{
			if ( m_aMergeExtraWearableMDLMap[iSlot].bHideOnActiveWeapon )
			{
				m_aMergeMDLs[m_aMergeExtraWearableMDLMap[iSlot].iModelHandle].m_bDisabled = bVisible;
			}
			else
			{
				m_aMergeMDLs[m_aMergeExtraWearableMDLMap[iSlot].iModelHandle].m_bDisabled = false;
			}
		}
	}
}

#ifdef ITEM_TAUNTING

void CTFPlayerModelPanel::PlayTauntFromItem( CEconItemView *pItem )
{
	CEconItemDefinition *pStatic = pItem->GetStaticData();
	if ( !pStatic )
		return;

	if ( pStatic->taunt.custom_taunt_scene_per_class[m_iClass].Count() == 0 )
		return;

	// Get class-specific scene.
	const char *pszScene = pStatic->taunt.custom_taunt_scene_per_class[m_iClass].Random();
	if ( !pszScene[0] )
		return;

	if ( pStatic->taunt.taunt_force_weapon_slot != TF_LOADOUT_SLOT_INVALID )
	{
		// If taunt wants us to switch to a weapon and we fail then cancel taunt.
		if ( !HoldItemInSlot( pStatic->taunt.taunt_force_weapon_slot ) )
			return;
	}

	PlayVCD( pszScene );

	// Attach taunt prop if needed.
	const char *pszTauntModel = pStatic->taunt.custom_taunt_prop_per_class[m_iClass];
	const char *pszTauntPropScene = pStatic->taunt.custom_taunt_prop_scene_per_class[m_iClass];

	// Not supporting props with VCD scenes.
	if ( pszTauntModel[0] && !pszTauntPropScene[0] )
	{
		MDLHandle_t hModel = SetMergeMDL( pszTauntModel, GetClientRenderable(), GetTeamSkin( m_iTeamNum ) );
		
		// Hide the active weapon.
		SetCarriedItemVisibilityInSlot( m_iActiveWeaponSlot, false );

		m_iTauntMDLIndex = GetMergeMDLIndex( hModel );
	}
}
#endif

extern CFlexSceneFileManager g_FlexSceneFileManager;


void CTFPlayerModelPanel::InitPhonemeMappings( void )
{
	CStudioHdr *pStudio = GetModelPtr();
	if ( pStudio )
	{
		char szBasename[MAX_PATH];
		V_StripExtension( pStudio->pszName(), szBasename, sizeof( szBasename ) );
		char szExpressionName[MAX_PATH];
		V_sprintf_safe( szExpressionName, "%s/phonemes/phonemes", szBasename );
		if ( g_FlexSceneFileManager.FindSceneFile( this, szExpressionName, true ) )
		{
			// Fill in phoneme class lookup
			memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );

			Emphasized_Phoneme *normal = &m_PhonemeClasses[PHONEME_CLASS_NORMAL];
			V_strcpy_safe( normal->classname, szExpressionName );
			normal->required = true;

			Emphasized_Phoneme *weak = &m_PhonemeClasses[PHONEME_CLASS_WEAK];
			V_sprintf_safe( weak->classname, "%s_weak", szExpressionName );
			Emphasized_Phoneme *strong = &m_PhonemeClasses[PHONEME_CLASS_STRONG];
			V_sprintf_safe( strong->classname, "%s_strong", szExpressionName );
		}
		else
		{
			// Fill in phoneme class lookup
			memset( m_PhonemeClasses, 0, sizeof( m_PhonemeClasses ) );

			Emphasized_Phoneme *normal = &m_PhonemeClasses[PHONEME_CLASS_NORMAL];
			V_strcpy_safe( normal->classname, "phonemes" );
			normal->required = true;

			Emphasized_Phoneme *weak = &m_PhonemeClasses[PHONEME_CLASS_WEAK];
			V_strcpy_safe( weak->classname, "phonemes_weak" );
			Emphasized_Phoneme *strong = &m_PhonemeClasses[PHONEME_CLASS_STRONG];
			V_strcpy_safe( strong->classname, "phonemes_strong" );
		}
	}
}


void CTFPlayerModelPanel::SetFlexWeight( LocalFlexController_t index, float value )
{
	if ( !m_pStudioHdr )
		return;

	if ( index >= 0 && index < m_pStudioHdr->numflexcontrollers() )
	{
		mstudioflexcontroller_t *pflexcontroller = m_pStudioHdr->pFlexcontroller( index );
		m_flexWeight[index] = RemapValClamped( value, pflexcontroller->min, pflexcontroller->max, 0.0f, 1.0f );
	}
}


float CTFPlayerModelPanel::GetFlexWeight( LocalFlexController_t index )
{
	if ( !m_pStudioHdr )
		return 0.0f;

	if ( index >= 0 && index <  m_pStudioHdr->numflexcontrollers() )
	{
		mstudioflexcontroller_t *pflexcontroller = m_pStudioHdr->pFlexcontroller( index );
		return RemapValClamped( m_flexWeight[index], 0.0f, 1.0f, pflexcontroller->min, pflexcontroller->max );
	}

	return 0.0f;
}


void CTFPlayerModelPanel::ResetFlexWeights( void )
{
	if ( !m_pStudioHdr || m_pStudioHdr->numflexdesc() == 0 )
		return;

	// Reset the flex weights to their starting position.
	for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); ++i )
	{
		SetFlexWeight( i, 0.0f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Look up instance specific mapping
//-----------------------------------------------------------------------------
int CTFPlayerModelPanel::FlexControllerLocalToGlobal( const flexsettinghdr_t *pSettinghdr, int key )
{
	FS_LocalToGlobal_t entry( pSettinghdr );

	int idx = m_LocalToGlobal.Find( entry );
	if ( idx == m_LocalToGlobal.InvalidIndex() )
	{
		// This should never happen!!!
		Assert( 0 );
		Warning( "Unable to find mapping for flexcontroller %i, settings %p on CTFPlayerModelPanel\n", key, pSettinghdr );
		EnsureTranslations( pSettinghdr );
		idx = m_LocalToGlobal.Find( entry );
		if ( idx == m_LocalToGlobal.InvalidIndex() )
		{
			Error( "CTFPlayerModelPanel::FlexControllerLocalToGlobal failed!\n" );
		}
	}

	FS_LocalToGlobal_t& result = m_LocalToGlobal[idx];
	// Validate lookup
	Assert( result.m_nCount != 0 && key < result.m_nCount );
	int index = result.m_Mapping[key];
	return index;
}


LocalFlexController_t CTFPlayerModelPanel::FindFlexController( const char *szName )
{
	for ( LocalFlexController_t i = LocalFlexController_t( 0 ); i < m_pStudioHdr->numflexcontrollers(); i++ )
	{
		if ( V_stricmp( m_pStudioHdr->pFlexcontroller( i )->pszName(), szName ) == 0 )
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "flexcontroller %s couldn't be mapped!!!\n", szName ) );
	return LocalFlexController_t( -1 );
}

#define STRONG_CROSSFADE_START		0.60f
#define WEAK_CROSSFADE_START		0.40f

//-----------------------------------------------------------------------------
// Purpose: 
// Here's the formula
// 0.5 is neutral 100 % of the default setting
// Crossfade starts at STRONG_CROSSFADE_START and is full at STRONG_CROSSFADE_END
// If there isn't a strong then the intensity of the underlying phoneme is fixed at 2 x STRONG_CROSSFADE_START
//  so we don't get huge numbers
// Input  : *classes - 
//			emphasis_intensity - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ComputeBlendedSetting( Emphasized_Phoneme *classes, float emphasis_intensity )
{
	// See which blends are available for the current phoneme
	bool has_weak = classes[PHONEME_CLASS_WEAK].valid;
	bool has_strong = classes[PHONEME_CLASS_STRONG].valid;

	// Better have phonemes in general
	Assert( classes[PHONEME_CLASS_NORMAL].valid );

	if ( emphasis_intensity > STRONG_CROSSFADE_START )
	{
		if ( has_strong )
		{
			// Blend in some of strong
			float dist_remaining = 1.0f - emphasis_intensity;
			float frac = dist_remaining / ( 1.0f - STRONG_CROSSFADE_START );

			classes[PHONEME_CLASS_NORMAL].amount = (frac)* 2.0f * STRONG_CROSSFADE_START;
			classes[PHONEME_CLASS_STRONG].amount = 1.0f - frac;
		}
		else
		{
			emphasis_intensity = Min( emphasis_intensity, STRONG_CROSSFADE_START );
			classes[PHONEME_CLASS_NORMAL].amount = 2.0f * emphasis_intensity;
		}
	}
	else if ( emphasis_intensity < WEAK_CROSSFADE_START )
	{
		if ( has_weak )
		{
			// Blend in some weak
			float dist_remaining = WEAK_CROSSFADE_START - emphasis_intensity;
			float frac = dist_remaining / ( WEAK_CROSSFADE_START );

			classes[PHONEME_CLASS_NORMAL].amount = ( 1.0f - frac ) * 2.0f * WEAK_CROSSFADE_START;
			classes[PHONEME_CLASS_WEAK].amount = frac;
		}
		else
		{
			emphasis_intensity = Max( emphasis_intensity, WEAK_CROSSFADE_START );
			classes[PHONEME_CLASS_NORMAL].amount = 2.0f * emphasis_intensity;
		}
	}
	else
	{
		// Assume 0.5 (neutral) becomes a scaling of 1.0f
		classes[PHONEME_CLASS_NORMAL].amount = 2.0f * emphasis_intensity;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			phoneme - 
//			scale - 
//			newexpression - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddViseme( Emphasized_Phoneme *classes, float emphasis_intensity, int phoneme, float scale, bool newexpression )
{
	int type;

	// Setup weights for any emphasis blends
	bool skip = SetupEmphasisBlend( classes, phoneme );
	// Uh-oh, missing or unknown phoneme???
	if ( skip )
	{
		return;
	}

	// Compute blend weights
	ComputeBlendedSetting( classes, emphasis_intensity );

	for ( type = 0; type < NUM_PHONEME_CLASSES; type++ )
	{
		Emphasized_Phoneme *info = &classes[type];
		if ( !info->valid || info->amount == 0.0f )
			continue;

		const flexsettinghdr_t *actual_flexsetting_header = info->base;
		const flexsetting_t *pSetting = actual_flexsetting_header->pIndexedSetting( phoneme );
		if ( !pSetting )
		{
			continue;
		}

		flexweight_t *pWeights = NULL;

		int truecount = pSetting->psetting( (byte *)actual_flexsetting_header, 0, &pWeights );
		if ( pWeights )
		{
			for ( int i = 0; i < truecount; i++ )
			{
				// Translate to global controller number
				int j = FlexControllerLocalToGlobal( actual_flexsetting_header, pWeights->key );
				// Add scaled weighting in
				m_RootMDL.m_MDL.m_pFlexControls[j] += info->amount * scale * pWeights->weight;
				// Go to next setting
				pWeights++;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A lot of the one time setup and also resets amount to 0.0f default
//  for strong/weak/normal tracks
// Returning true == skip this phoneme
// Input  : *classes - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFPlayerModelPanel::SetupEmphasisBlend( Emphasized_Phoneme *classes, int phoneme )
{
	int i;

	bool skip = false;

	for ( i = 0; i < NUM_PHONEME_CLASSES; i++ )
	{
		Emphasized_Phoneme *info = &classes[i];

		// Assume it's bogus
		info->valid = false;
		info->amount = 0.0f;

		// One time setup
		if ( !info->basechecked )
		{
			info->basechecked = true;
			info->base = (flexsettinghdr_t *)g_FlexSceneFileManager.FindSceneFile( this, info->classname, true );
		}
		info->exp = NULL;
		if ( info->base )
		{
			Assert( info->base->id == ( 'V' << 16 ) + ( 'F' << 8 ) + ( 'E' ) );
			info->exp = info->base->pIndexedSetting( phoneme );
		}

		if ( info->required && ( !info->base || !info->exp ) )
		{
			skip = true;
			break;
		}

		if ( info->exp )
		{
			info->valid = true;
		}
	}

	return skip;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//			*sentence - 
//			t - 
//			dt - 
//			juststarted - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddVisemesForSentence( Emphasized_Phoneme *classes, float emphasis_intensity, CSentence *sentence, float t, float dt, bool juststarted )
{
	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
	{
		return;
	}

	int pcount = sentence->GetRuntimePhonemeCount();
	for ( int k = 0; k < pcount; k++ )
	{
		const CBasePhonemeTag *phoneme = sentence->GetRuntimePhoneme( k );

		if ( t > phoneme->GetStartTime() && t < phoneme->GetEndTime() )
		{
			// Let's just always do crossfade.
			if ( k < pcount - 1 )
			{
				const CBasePhonemeTag *next = sentence->GetRuntimePhoneme( k + 1 );
				// if I have a neighbor
				if ( next )
				{
					//  and they're touching
					if ( next->GetStartTime() == phoneme->GetEndTime() )
					{
						// no gap, so increase the blend length to the end of the next phoneme, as long as it's not longer than the current phoneme
						dt = Max( dt, Min( next->GetEndTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
					}
					else
					{
						// dead space, so increase the blend length to the start of the next phoneme, as long as it's not longer than the current phoneme
						dt = Max( dt, Min( next->GetStartTime() - t, phoneme->GetEndTime() - phoneme->GetStartTime() ) );
					}
				}
				else
				{
					// last phoneme in list, increase the blend length to the length of the current phoneme
					dt = Max( dt, phoneme->GetEndTime() - phoneme->GetStartTime() );
				}
			}
		}

		float t1 = ( phoneme->GetStartTime() - t ) / dt;
		float t2 = ( phoneme->GetEndTime() - t ) / dt;

		if ( t1 < 1.0 && t2 > 0 )
		{
			float scale;

			// clamp
			if ( t2 > 1 )
				t2 = 1;
			if ( t1 < 0 )
				t1 = 0;

			// FIXME: simple box filter.  Should use something fancier
			scale = ( t2 - t1 );

			AddViseme( classes, emphasis_intensity, phoneme->GetPhonemeCode(), scale, juststarted );
		}
	}
}

extern ConVar g_CV_PhonemeDelay;
extern ConVar g_CV_PhonemeFilter;
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *classes - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::ProcessVisemes( Emphasized_Phoneme *classes )
{
	// Any sounds being played?
	CMouthInfo *pMouth = clientdll->GetClientUIMouthInfo();
	if ( !pMouth->IsActive() )
		return;

	// Multiple phoneme tracks can overlap, look across all such tracks.
	for ( int source = 0; source < pMouth->GetNumVoiceSources(); source++ )
	{
		CVoiceData *vd = pMouth->GetVoiceSource( source );
		if ( !vd || vd->ShouldIgnorePhonemes() )
			continue;

		CSentence *sentence = engine->GetSentence( vd->GetSource() );
		if ( !sentence )
			continue;

		float	sentence_length = engine->GetSentenceLength( vd->GetSource() );
		float	timesincestart = vd->GetElapsedTime();

		// This sound should be done...why hasn't it been removed yet???
		if ( timesincestart >= ( sentence_length + 2.0f ) )
			continue;

		// Adjust actual time
		float t = timesincestart - g_CV_PhonemeDelay.GetFloat();

		// Get box filter duration
		float dt = g_CV_PhonemeFilter.GetFloat();

		// Streaming sounds get an additional delay...
		/*
		// Tracker 20534:  Probably not needed any more with the async sound stuff that
		//  we now have (we don't have a disk i/o hitch on startup which might have been
		//  messing up the startup timing a bit )
		bool streaming = engine->IsStreaming( vd->m_pAudioSource );
		if ( streaming )
		{
		t -= g_CV_PhonemeDelayStreaming.GetFloat();
		}
		*/

		// Assume sound has been playing for a while...
		bool juststarted = false;

		// Get intensity setting for this time (from spline)
		float emphasis_intensity = sentence->GetIntensity( t, sentence_length );

		// Blend and add visemes together
		AddVisemesForSentence( classes, emphasis_intensity, sentence, t, dt, juststarted );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Since everyone shared a pSettinghdr now, we need to set up the localtoglobal mapping per entity, but 
//  we just do this in memory with an array of integers (could be shorts, I suppose)
// Input  : *pSettinghdr - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::EnsureTranslations( const flexsettinghdr_t *pSettinghdr )
{
	Assert( pSettinghdr );

	FS_LocalToGlobal_t entry( pSettinghdr );

	unsigned short idx = m_LocalToGlobal.Find( entry );
	if ( idx != m_LocalToGlobal.InvalidIndex() )
		return;

	entry.SetCount( pSettinghdr->numkeys );

	for ( int i = 0; i < pSettinghdr->numkeys; ++i )
	{
		entry.m_Mapping[i] = C_BaseFlex::AddGlobalFlexController( pSettinghdr->pLocalName( i ) );
	}

	m_LocalToGlobal.Insert( entry );
}

extern CChoreoStringPool g_ChoreoStringPool;


CChoreoScene *CTFPlayerModelPanel::LoadScene( const char *filename )
{
	char loadfile[512];
	V_strcpy_safe( loadfile, filename );
	V_SetExtension( loadfile, ".vcd", sizeof( loadfile ) );
	V_FixSlashes( loadfile );

	char *pBuffer = NULL;
	size_t bufsize = scenefilecache->GetSceneBufferSize( loadfile );
	if ( bufsize <= 0 )
		return NULL;

	pBuffer = new char[bufsize];
	if ( !scenefilecache->GetSceneData( filename, (byte *)pBuffer, bufsize ) )
	{
		delete[] pBuffer;
		return NULL;
	}

	CChoreoScene *pScene;
	if ( IsBufferBinaryVCD( pBuffer, bufsize ) )
	{
		pScene = new CChoreoScene( this );
		CUtlBuffer buf( pBuffer, bufsize, CUtlBuffer::READ_ONLY );
		if ( !pScene->RestoreFromBinaryBuffer( buf, loadfile, &g_ChoreoStringPool ) )
		{
			Warning( "Unable to restore binary scene '%s'\n", loadfile );
			delete pScene;
			pScene = NULL;
		}
		else
		{
			pScene->SetPrintFunc( Scene_Printf );
			pScene->SetEventCallbackInterface( this );
		}
	}
	else
	{
		g_TokenProcessor.SetBuffer( pBuffer );
		pScene = ChoreoLoadScene( loadfile, this, &g_TokenProcessor, Scene_Printf );
	}

	delete[] pBuffer;
	return pScene;
}


void CTFPlayerModelPanel::PlayVCD( const char *pszFile )
{
	// Stop the previous scene.
	StopVCD();

	m_flCurrentTime = 0.0f;
	m_pScene = LoadScene( pszFile );

	if ( m_pScene )
	{
		int types[6];
		types[0] = CChoreoEvent::FLEXANIMATION;
		types[1] = CChoreoEvent::EXPRESSION;
		types[2] = CChoreoEvent::GESTURE;
		types[3] = CChoreoEvent::SEQUENCE;
		types[4] = CChoreoEvent::SPEAK;
		types[5] = CChoreoEvent::LOOP;
		m_pScene->RemoveEventsExceptTypes( types, 6 );

		m_pScene->ResetSimulation();
	}
}


void CTFPlayerModelPanel::StopVCD( void )
{
	if ( !m_pScene )
		return;

	delete m_pScene;
	m_pScene = NULL;
	SetSequenceLayers( NULL, 0 );

	ResetFlexWeights();

#ifdef ITEM_TAUNTING
	// Remove taunt prop.
	if ( m_iTauntMDLIndex != -1 )
	{
		m_aMergeMDLs.Remove( m_iTauntMDLIndex );
		m_iTauntMDLIndex = -1;
	}
#endif

	// Always unhide the weapon because some stupid taunts don't fire AE_WPN_UNHIDE.
	SetCarriedItemVisibilityInSlot( m_iActiveWeaponSlot, true );

	if ( m_pStudioHdr )
	{
		m_RootMDL.m_MDL.m_flPlaybackRate = GetSequenceFrameRate( m_RootMDL.m_MDL.m_nSequence );
	}
}


void CTFPlayerModelPanel::ProcessLoop( CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event->GetType() == CChoreoEvent::LOOP );

	float backtime = (float)atof( event->GetParameters() );

	bool process = true;
	int counter = event->GetLoopCount();
	if ( counter != -1 )
	{
		int remaining = event->GetNumLoopsRemaining();
		if ( remaining <= 0 )
		{
			process = false;
		}
		else
		{
			event->SetNumLoopsRemaining( --remaining );
		}
	}

	if ( !process )
		return;

	scene->LoopToTime( backtime );

	// Reset the sequences.
	for ( int i = 0; i < m_nNumSequenceLayers; i++ )
	{
		m_SequenceLayers[i].m_flCycleBeganAt += m_flCurrentTime - backtime;
	}

	m_flCurrentTime = backtime;
}


void CTFPlayerModelPanel::ProcessSequence( CChoreoScene *scene, CChoreoEvent *event )
{
	// Make sure sequence exists.
	int nSequence = LookupSequence( event->GetParameters() );
	if ( nSequence < 0 )
		return;

	MDLSquenceLayer_t seqLayer;
	seqLayer.m_nSequenceIndex = nSequence;
	seqLayer.m_flWeight = 1.0f;
	seqLayer.m_bNoLoop = true;
	seqLayer.m_flCycleBeganAt = 0.0f;

	SetSequenceLayers( &seqLayer, 1 );
	m_RootMDL.m_MDL.m_flPlaybackRate = GetSequenceFrameRate( nSequence );
}


void CTFPlayerModelPanel::ProcessFlexAnimation( CChoreoScene *scene, CChoreoEvent *event )
{
	if ( !event->GetTrackLookupSet() )
	{
		// Create lookup data
		for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
		{
			CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
			if ( !track )
				continue;

			if ( track->IsComboType() )
			{
				char name[512];
				V_strcpy_safe( name, "right_" );
				V_strcat_safe( name, track->GetFlexControllerName() );

				track->SetFlexControllerIndex( Max( FindFlexController( name ), LocalFlexController_t( 0 ) ), 0, 0 );

				V_strcpy_safe( name, "left_" );
				V_strcat_safe( name, track->GetFlexControllerName() );

				track->SetFlexControllerIndex( Max( FindFlexController( name ), LocalFlexController_t( 0 ) ), 0, 1 );
			}
			else
			{
				track->SetFlexControllerIndex( Max( FindFlexController( (char *)track->GetFlexControllerName() ), LocalFlexController_t( 0 ) ), 0 );
			}
		}

		event->SetTrackLookupSet( true );
	}

	float scenetime = scene->GetTime();

	float weight = event->GetIntensity( scenetime );

	// Compute intensity for each track in animation and apply
	// Iterate animation tracks
	for ( int i = 0; i < event->GetNumFlexAnimationTracks(); i++ )
	{
		CFlexAnimationTrack *track = event->GetFlexAnimationTrack( i );
		if ( !track )
			continue;

		// Disabled
		if ( !track->IsTrackActive() )
			continue;

		// Map track flex controller to global name
		if ( track->IsComboType() )
		{
			for ( int side = 0; side < 2; side++ )
			{
				LocalFlexController_t controller = track->GetRawFlexControllerIndex( side );

				// Get spline intensity for controller
				float flIntensity = track->GetIntensity( scenetime, side );
				if ( controller >= LocalFlexController_t( 0 ) )
				{
					float orig = GetFlexWeight( controller );
					float value = orig * ( 1 - weight ) + flIntensity * weight;
					SetFlexWeight( controller, value );
				}
			}
		}
		else
		{
			LocalFlexController_t controller = track->GetRawFlexControllerIndex( 0 );

			// Get spline intensity for controller
			float flIntensity = track->GetIntensity( scenetime, 0 );
			if ( controller >= LocalFlexController_t( 0 ) )
			{
				float orig = GetFlexWeight( controller );
				float value = orig * ( 1 - weight ) + flIntensity * weight;
				SetFlexWeight( controller, value );
			}
		}
	}
}


void CTFPlayerModelPanel::ProcessFlexSettingSceneEvent( CChoreoScene *scene, CChoreoEvent *event )
{
	// Look up the actual strings
	const char *scenefile = event->GetParameters();
	const char *name = event->GetParameters2();

	// Have to find both strings
	if ( scenefile && name )
	{
		// Find the scene file
		const flexsettinghdr_t *pExpHdr = (const flexsettinghdr_t *)g_FlexSceneFileManager.FindSceneFile( this, scenefile, true );
		if ( pExpHdr )
		{
			// Add the named expression
			AddFlexSetting( name, event->GetIntensity( scene->GetTime() ), pExpHdr );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *expr - 
//			scale - 
//			*pSettinghdr - 
//			newexpression - 
//-----------------------------------------------------------------------------
void CTFPlayerModelPanel::AddFlexSetting( const char *expr, float scale, const flexsettinghdr_t *pSettinghdr )
{
	int i;
	const flexsetting_t *pSetting = NULL;

	// Find the named setting in the base
	for ( i = 0; i < pSettinghdr->numflexsettings; i++ )
	{
		pSetting = pSettinghdr->pSetting( i );
		if ( !pSetting )
			continue;

		if ( !V_stricmp( pSetting->pszName(), expr ) )
			break;
	}

	if ( i >= pSettinghdr->numflexsettings )
		return;

	flexweight_t *pWeights = NULL;
	int truecount = pSetting->psetting( (byte *)pSettinghdr, 0, &pWeights );
	if ( !pWeights )
		return;

	for ( i = 0; i < truecount; i++, pWeights++ )
	{
		// Translate to local flex controller
		// this is translating from the settings's local index to the models local index
		int index = FlexControllerLocalToGlobal( pSettinghdr, pWeights->key );

		// blend scaled weighting in to total (post networking g_flexweight!!!!)
		float s = clamp( scale * pWeights->influence, 0.0f, 1.0f );
		m_RootMDL.m_MDL.m_pFlexControls[index] = m_RootMDL.m_MDL.m_pFlexControls[index] * ( 1.0f - s ) + pWeights->weight * s;
	}
}


void CTFPlayerModelPanel::StartEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	Assert( event );

	if ( !V_stricmp( event->GetName(), "NULL" ) )
	{
		Scene_Printf( "%s : %8.2f:  ignored %s\n", scene->GetFilename(), currenttime, event->GetDescription() );
		return;
	}

	switch ( event->GetType() )
	{
		case CChoreoEvent::SPEAK:
		{
			float time_in_past = m_flCurrentTime - event->GetStartTime();
			float soundtime = gpGlobals->curtime - time_in_past;

			EmitSound_t es;
			es.m_nChannel = CHAN_VOICE;
			es.m_flVolume = 1.0f;
			es.m_SoundLevel = SNDLVL_TALKING;
			es.m_flSoundTime = soundtime;

			es.m_bEmitCloseCaption = false;
			es.m_pSoundName = event->GetParameters();

			C_RecipientFilter filter;
			C_BaseEntity::EmitSound( filter, SOUND_FROM_UI_PANEL, es );
			break;
		}
		case CChoreoEvent::LOOP:
			Assert( scene );
			Assert( event );
		
			ProcessLoop( scene, event );
			break;

		case CChoreoEvent::SEQUENCE:
			Assert( scene );
			Assert( event );

			ProcessSequence( scene, event );
			break;
	}

	event->m_flPrevTime = currenttime;
}


void CTFPlayerModelPanel::EndEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
}


void CTFPlayerModelPanel::ProcessEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	switch ( event->GetType() )
	{
		case CChoreoEvent::FLEXANIMATION:
			if ( m_bFlexEvents )
			{
				ProcessFlexAnimation( scene, event );
			}
			break;
		case CChoreoEvent::EXPRESSION:
			if ( !m_bFlexEvents )
			{
				ProcessFlexSettingSceneEvent( scene, event );
			}
			break;
	}

	event->m_flPrevTime = currenttime;
}


bool CTFPlayerModelPanel::CheckEvent( float currenttime, CChoreoScene *scene, CChoreoEvent *event )
{
	return true;
}
