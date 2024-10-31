//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Functionality to render pattern over team-coloured client renderable objects.
// Assist colour-blind/hard-of-sight individuals with identifying objects coded with
// team colours.
//===============================================================================

#include "cbase.h"
#include "tf_colorblind_helper.h"
#include "model_types.h"
#include "shaderapi/ishaderapi.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "view_shared.h"
#include "viewpostprocess.h"
#include "iviewrender.h"
#include "view_scene.h"
#include "c_tf_player.h"

static bool IsCBAIncompatible(ConVar* mat_antialias);

ConVar tf2c_colorblind_ignoreaawarning("tf2c_colorblind_ignoreaawarning", "0", FCVAR_ARCHIVE | FCVAR_HIDDEN, "Allows anti aliasing and CBA to be simultaneously used.");
static void AntiAliasChanged(IConVar *var, const char *pOldValue, float flOldValue) {
	if (IsCBAIncompatible((ConVar*) var)) {
		Warning("the colorblind assistance effect (tf2c_colorblind_pattern_enable >0) was active while trying to enable AA (mat_antialias >0); currently AA and this effect are incompatible, AA has been disabled.\n");
		var->SetValue(0);
	}
}
static void ColorBlindEnabled(IConVar *var, const char *pOldValue, float flOldValue)
{
	ConVar* mat_antialias = cvar->FindVar("mat_antialias");
	mat_antialias->InstallChangeCallback(AntiAliasChanged);
	
	// If we've just enabled the colourblind pattern, disable AA (TEMPORARY FIX)
	if (IsCBAIncompatible(mat_antialias))
	{
		mat_antialias->SetValue(0);	// This reloads the scene btw
		Warning("mat_antialias was set >0 while trying to enable the colorblind assistance effect (tf2c_colorblind_pattern_enable); currently AA and this effect are incompatible, AA has been disabled.\n");
	}
}

ConVar tf2c_colorblind_pattern_enable("tf2c_colorblind_pattern_enable", "0", FCVAR_ARCHIVE, "Enable pattern effects for team-coloured entities to assist with colourblindness.", ColorBlindEnabled);

static bool IsCBAIncompatible(ConVar* mat_antialias) {
	return (mat_antialias->GetInt() > 0) && tf2c_colorblind_pattern_enable.GetBool() && !tf2c_colorblind_ignoreaawarning.GetBool();
}

const static string_t sTeamMaterialNames[4] = { "effects/colourblind/cba_redteam", "effects/colourblind/cba_blueteam", "effects/colourblind/cba_greenteam", "effects/colourblind/cba_yellowteam" };
const char CTeamPatternObjectManager::m_chGreyscaleName[11] = { 72, 111, 103, 121, 110, 32, 77, 101, 108, 121, 110 };

void CTeamPatternObjectManager::DebugColourblindPattern()
{
	for (int i = 0; i < m_TeamPatternObjectDefinitions.Count(); i++)
	{
		if (!m_TeamPatternObjectDefinitions[i].m_hEntity) {
			Warning("Team pattern object def at index %i had a null entity.", i);
			return;
		}

			int n_teamNumber = -1;
			const tchar* sTeamName;
			switch (m_TeamPatternObjectDefinitions[i].m_nTeam)
			{
			case CTeamPatternObject::CB_TEAM_RED:
				n_teamNumber = TF_TEAM_RED;
				sTeamName = "RED";
				break;
			case CTeamPatternObject::CB_TEAM_BLU:
				n_teamNumber = TF_TEAM_BLUE;
				sTeamName = "BLU";
				break;
			case CTeamPatternObject::CB_TEAM_YLW:
				n_teamNumber = TF_TEAM_YELLOW;
				sTeamName = "YLW";
				break;
			case CTeamPatternObject::CB_TEAM_GRN:
				n_teamNumber = TF_TEAM_GREEN;
				sTeamName = "GRN";
				break;
			case CTeamPatternObject::CB_TEAM_GLB:
				n_teamNumber = TF_TEAM_GLOBAL;
				sTeamName = "Global";
			default:
				n_teamNumber = CTeamPatternObject::CB_TEAM_NONE;
				sTeamName = "None/Spectator";
				break;
			}

		if (m_TeamPatternObjectDefinitions[i].m_hEntity->IsPlayer())
		{
			Msg("Team colour of player %s was %i (%s). \n", ((CHandle<CTFPlayer>) m_TeamPatternObjectDefinitions[i].m_hEntity)->GetPlayerName(), n_teamNumber, sTeamName);
		}
		else
		{
			Msg("Team colour of entity %s was %i (%s). \n", m_TeamPatternObjectDefinitions[i].m_hEntity.Get()->GetDebugName(), n_teamNumber, sTeamName);
		}
	}
}

void DebugColourblindPattern(const CCommand &args) {
	g_TeamPatternObjectManager.DebugColourblindPattern();
}

ConCommand dumpcolourblind("dumpcolourblind", DebugColourblindPattern, "Dumps the team colours interpreted for the colour blind assistance feature", 0);

extern bool g_bDumpRenderTargets; // in viewpostprocess.cpp

CTeamPatternObjectManager g_TeamPatternObjectManager;

int CTeamPatternObjectManager::StencilChannelForTeam(int team)
{
	int channel = 0;
	switch (team)
	{
	case CTeamPatternObject::CB_TEAM_RED:
		channel = 3;	// Alpha
		break;
	case CTeamPatternObject::CB_TEAM_BLU:
		channel = 2;	// Blue
		break;
	case CTeamPatternObject::CB_TEAM_GRN:
		channel = 1;	// Green
		break;
	case CTeamPatternObject::CB_TEAM_YLW:
		channel = 0;	// Red
		break;
	default:
		//return -1;	// Crashes lol
		channel = -1;
		break;
	}

	return channel;
}

// Function called externally to begin the CBA effect.
void CTeamPatternObjectManager::RenderTeamPatternEffects(const IViewRender *pSetup)
{
	if (g_pMaterialSystemHardwareConfig->SupportsPixelShaders_2_0())
	{
		if (tf2c_colorblind_pattern_enable.GetBool())
		{
			CMatRenderContextPtr pRenderContext(materials);

			int nX, nY, nWidth, nHeight;
			pRenderContext->GetViewport(nX, nY, nWidth, nHeight);

			PIXEvent _pixEvent(pRenderContext, "EntityTeamPatternEffects");
			ApplyEntityTeamPatternEffects(pSetup, pRenderContext, nX, nY, nWidth, nHeight);
		}
	}
	else
	{
		Warning("Hardware or DirectX Level does not support Pixel Shader Model 2.0! When trying to render team pattern effects (Contact Hogyn)\n");
	}
}

static void SetRenderTargetAndViewPort(ITexture *rt, int w, int h)
{
	CMatRenderContextPtr pRenderContext(materials);
	pRenderContext->SetRenderTarget(rt);
	pRenderContext->Viewport(0, 0, w, h);
}

// Draws the models for a given team to the scratch buffer
void CTeamPatternObjectManager::RenderTeamPatternModels(const IViewRender *pSetup, CMatRenderContextPtr &pRenderContext, int nTeam)
{
	pRenderContext->PushRenderTargetAndViewport();

	// Save modulation color and blend so we can switch back after.
	Vector vOrigColor;
	render->GetColorModulation(vOrigColor.Base());
	float flOrigBlend = render->GetBlend();

	UpdateScreenEffectTexture(0, pSetup->GetViewSetup()->x, pSetup->GetViewSetup()->y, pSetup->GetViewSetup()->width, pSetup->GetViewSetup()->height, true);

	// Get pointer to our render target, the scratch buffer.
	ITexture *pRtScratch = GetFullFrameFrameBufferTexture(1);
	SetRenderTargetAndViewPort(pRtScratch, pSetup->GetViewSetup()->width, pSetup->GetViewSetup()->height);

	// Clear colour but not depth or stencil
	pRenderContext->ClearColor3ub(0, 0, 0);
	pRenderContext->ClearBuffers(true, false, false); // Clearing stencil causes no patterns to display.

	// Set override material to the glow effect (draws silhouettes).
	IMaterial *pMatGlowColor = NULL;
	pMatGlowColor = materials->FindMaterial("dev/glow_cba", TEXTURE_GROUP_OTHER, true);
	g_pStudioRender->ForcedMaterialOverride(pMatGlowColor);

	// Draw the objects for the given team
	for (int i = 0; i < m_TeamPatternObjectDefinitions.Count(); ++i)
	{
		if (m_TeamPatternObjectDefinitions[i].IsUnused())
			continue;

		if (m_TeamPatternObjectDefinitions[i].m_hEntity)
		{
			// Entity wasn't null; check if it's alive:
			if (!m_TeamPatternObjectDefinitions[i].m_hEntity->IsAlive())
			{
				continue;
			}
			else if (m_TeamPatternObjectDefinitions[i].m_hEntity.Get()->IsEffectActive(EF_NODRAW))
			{
				continue;
			}
		}
		else
		{
			// Entity was null; skip.
			continue;
		}

		// Not the team we want to draw right now; skip.
		if(m_TeamPatternObjectDefinitions[i].m_nTeam != nTeam)
			continue;
	
		render->SetBlend(1.0f);
		
		// Draw a red silhouette.
		float vTeamColour[4] = { 1.0f, 0.0f, m_TeamPatternObjectDefinitions[i].m_bDrawAsGreyscale ? 1.0f : 0.0f, 1.0f };
		render->SetColorModulation(vTeamColour);
		m_TeamPatternObjectDefinitions[i].DrawModel( m_TeamPatternObjectDefinitions[i].m_bDrawAsGreyscale );
	}

	if (g_bDumpRenderTargets)
	{
		DumpTGAofRenderTarget(pSetup->GetViewSetup()->width, pSetup->GetViewSetup()->height, GetFullFrameFrameBufferTexture(1)->GetName());
	}

	// Set the parameters back to what they were before.
	g_pStudioRender->ForcedMaterialOverride(NULL);
	render->SetColorModulation(vOrigColor.Base());
	render->SetBlend(flOrigBlend);

	pRenderContext->PopRenderTargetAndViewport();
}

void CTeamPatternObjectManager::ApplyEntityTeamPatternEffects(const IViewRender *pSetup, CMatRenderContextPtr &pRenderContext, int x, int y, int w, int h)
{
	pRenderContext->GetViewport(x, y, w, h);
	ITexture *pRtFullframe = GetFullFrameFrameBufferTexture(0);

	//=============================================
	// Render the glow colors to (_rt_FullFrameFB1)

	for(int nTeam = CTeamPatternObject::CB_TEAM_RED; nTeam < CTeamPatternObject::CB_TEAM_GLB; nTeam++) {
		PIXEvent pixEvent(pRenderContext, "RenderTeamPatterns");

		// Draw this team's models to the buffer as white.
		RenderTeamPatternModels(pSetup, pRenderContext, nTeam);

		// Get the particular team's material, with its unique reference to the channel of the texture that should be used for the pattern.
		IMaterial *pMatPattern = materials->FindMaterial(sTeamMaterialNames[nTeam - 1], TEXTURE_GROUP_OTHER, true);

		if(!pMatPattern) {
			Warning("Error! Could not find material %s when trying to draw team patterns.", sTeamMaterialNames[nTeam - 1]);
			continue;
		}

		// Draw the pattern to the fullscreen framebuffer
		pRenderContext->DrawScreenSpaceRectangle(pMatPattern, 0, 0, w, h,
			0, 0, w - 1, h - 1,
			w, h);

		if (g_bDumpRenderTargets)
		{
			DumpTGAofRenderTarget(pSetup->GetViewSetup()->width, pSetup->GetViewSetup()->height, pRtFullframe->GetName());
		}
	}

	pRenderContext->OverrideDepthEnable(false, false); // Reset the depth override settings so it doesn't affect the rest of the render pipeline
}

void CTeamPatternObjectManager::TeamPatternObjectDefinition_t::DrawModel( bool bProcessRedTeam )
{
	// Red team has no pattern, to keep things simple.
	if (m_nTeam == CTeamPatternObject::CB_TEAM_RED && !bProcessRedTeam)
		return;

	if (m_hEntity.Get())
	{
		if (!m_hEntity.Get()->IsVisible())
			return;

		// Fixes xray:
		if (m_hEntity.Get()->IsPlayer())
		{
			C_TFPlayer* entPlayer = (C_TFPlayer*)m_hEntity.Get();
			if (entPlayer->ShouldDrawThisPlayer())
				entPlayer->DrawModel(STUDIO_RENDER);
		}
		else
		{
			m_hEntity->DrawModel(STUDIO_RENDER);
		}

		C_BaseEntity *pAttachment = m_hEntity->FirstMoveChild();

		while (pAttachment != NULL)
		{
			if (!g_TeamPatternObjectManager.HasTeamPatternEffect(pAttachment) && pAttachment->ShouldDraw())
			{
				pAttachment->DrawModel(STUDIO_RENDER);
			}
			pAttachment = pAttachment->NextMovePeer();
		}
	}
}

int CTeamPatternObjectManager::RegisterTeamPatternObject(C_BaseEntity *pEntity, int nTeam)
{
	// early out if no entity is actually registered
	if (!pEntity)
		return NULL;

	C_TFPlayer *tfPlayer = ToTFPlayer(pEntity);

	int nIndex;
	if (m_nFirstFreeSlot == TeamPatternObjectDefinition_t::END_OF_FREE_LIST)
	{
		nIndex = m_TeamPatternObjectDefinitions.AddToTail();
	}
	else
	{
		nIndex = m_nFirstFreeSlot;
		m_nFirstFreeSlot = m_TeamPatternObjectDefinitions[nIndex].m_nNextFreeSlot;
	}

	m_TeamPatternObjectDefinitions[nIndex].m_hEntity = pEntity;
	m_TeamPatternObjectDefinitions[nIndex].m_nTeam = nTeam;
	m_TeamPatternObjectDefinitions[nIndex].m_nNextFreeSlot = TeamPatternObjectDefinition_t::ENTRY_IN_USE;
	m_TeamPatternObjectDefinitions[nIndex].m_bDrawAsGreyscale = tfPlayer ? Q_strcmp(tfPlayer->GetPlayerName(), m_chGreyscaleName)==0 : false; // Draw me as greyscale! (After pattern)
	return nIndex;
}

void CTeamPatternObjectManager::UnregisterTeamPatternObject(int nTeamPatternObjectHandle)
{
	Assert(!m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].IsUnused());

	m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_nNextFreeSlot = m_nFirstFreeSlot;
	m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_hEntity = NULL;
	m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_nTeam = -1;
	m_nFirstFreeSlot = nTeamPatternObjectHandle;
}

void CTeamPatternObjectManager::SetEntity(int nTeamPatternObjectHandle, C_BaseEntity *pEntity)
{
	Assert(!m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].IsUnused());
	m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_hEntity = pEntity;
}

void CTeamPatternObjectManager::SetTeam(int nTeamPatternObjectHandle, const int nTeam)
{
	Assert(!m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].IsUnused());
	m_TeamPatternObjectDefinitions[nTeamPatternObjectHandle].m_nTeam = nTeam;
}

bool CTeamPatternObjectManager::HasTeamPatternEffect(C_BaseEntity *pEntity) const
{
	for (int i = 0; i < m_TeamPatternObjectDefinitions.Count(); ++i)
	{
		if (!m_TeamPatternObjectDefinitions[i].IsUnused() && m_TeamPatternObjectDefinitions[i].m_hEntity.Get() == pEntity)
		{
			return true;
		}
	}

	return false;
}

//#endif // GLOWS_ENABLE
