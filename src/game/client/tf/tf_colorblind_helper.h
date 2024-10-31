//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Functionality to render a pattern over team-coloured client renderable objects.
// Helps with visibility of players, buildings, and projectiles.
//
//===============================================================================

#ifndef CBA_EFFECT_H
#define CBA_EFFECT_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "mathlib/vector.h"

//#ifdef GLOWS_ENABLE

class C_BaseEntity;
class IViewRender;
class CMatRenderContextPtr;

// This is typically the un-processed framebuffer after the main rendering loop - the order of post processing effects
// will affect this framebuffer.

static inline const char* TeamIndexToString(int n_teamNumber) {
	switch (n_teamNumber) {
		case TF_TEAM_BLUE:
			return "BLU";
		case TF_TEAM_RED:
			return "RED";
		case TF_TEAM_GREEN:
			return "GRN";
		case TF_TEAM_YELLOW:
			return "YLW";
		case TF_TEAM_GLOBAL:
			return "Global";
		default:
			return "Default ### ! ###";
	}
}

class CTeamPatternObjectManager
{
public:

	//const static float s_rgflStencilTeams[4];

	int StencilChannelForTeam(int team);
	
	CTeamPatternObjectManager() :
		m_nFirstFreeSlot(TeamPatternObjectDefinition_t::END_OF_FREE_LIST)
	{
	}

	int RegisterTeamPatternObject(C_BaseEntity *pEntity, int nTeam);

	void UnregisterTeamPatternObject(int nTeamPatternObjectHandle);

	void SetEntity(int nTeamPatternObjectHandle, C_BaseEntity *pEntity);

	void SetTeam(int nTeamPatternObjectHandle, const int nTeam);

	bool HasTeamPatternEffect(C_BaseEntity *pEntity) const;

	void RenderTeamPatternEffects(const IViewRender *pSetup/*, CMatRenderContextPtr &pRenderContext*/);
	void DebugColourblindPattern();

private:

	void RenderTeamPatternModels(const IViewRender *pSetup, CMatRenderContextPtr &pRenderContext, int nTeam);
	void ApplyEntityTeamPatternEffects(const IViewRender *pSetup, CMatRenderContextPtr &pRenderContext, int x, int y, int w, int h);
	static void OnChangeMyVariable(IConVar *var, const char *pOldValue, float flOldValue);

	struct TeamPatternObjectDefinition_t
	{
		bool ShouldDraw(int nSlot) const
		{
			return m_hEntity.Get() &&
				m_hEntity->ShouldDraw() &&
				!m_hEntity->IsDormant();
		}

		bool IsUnused() const { return m_nNextFreeSlot != TeamPatternObjectDefinition_t::ENTRY_IN_USE; }
		void DrawModel(bool bProcessRedTeam = false);

		bool m_bDrawAsGreyscale;

		EHANDLE m_hEntity;

		// The team that is used for this object in its stencil pass
		int m_nTeam;

		// Linked list of free slots
		int m_nNextFreeSlot;

		// Special values for TeamPatternObjectDefinition_t::m_nNextFreeSlot
		static const int END_OF_FREE_LIST = -1;
		static const int ENTRY_IN_USE = -2;
	};
	
	static const char m_chGreyscaleName[11];

	CUtlVector< TeamPatternObjectDefinition_t > m_TeamPatternObjectDefinitions;
	int m_nFirstFreeSlot;
};

// TODO is this defined in the .cpp?
extern CTeamPatternObjectManager g_TeamPatternObjectManager;

class CTeamPatternObject
{
public:

	// Teams are encoded in the stencil buffer in the red channel.
	// This enum is used to index the stencil buffer colour to use
	enum COLORBLIND_TEAMS {
		CB_TEAM_NONE,	// = 0, no effect (texture 0, white)
		CB_TEAM_RED,	// = 1, texture 1
		CB_TEAM_BLU,	// = 2, texture 2
		CB_TEAM_GRN,	// = 3, texture 3
		CB_TEAM_YLW,	// = 4, texture 4
		CB_TEAM_GLB		// = 5, Global team
	};

	CTeamPatternObject(C_BaseEntity *pEntity, const int nTeam = CB_TEAM_RED)
	{
		m_nTeamPatternObjectHandle = g_TeamPatternObjectManager.RegisterTeamPatternObject(pEntity, nTeam);
	}

	~CTeamPatternObject()
	{
		g_TeamPatternObjectManager.UnregisterTeamPatternObject(m_nTeamPatternObjectHandle);
	}

	void Destroy(void)
	{
		g_TeamPatternObjectManager.UnregisterTeamPatternObject(m_nTeamPatternObjectHandle);
	}

	void SetEntity(C_BaseEntity *pEntity)
	{
		g_TeamPatternObjectManager.SetEntity(m_nTeamPatternObjectHandle, pEntity);
	}

	void SetTeam(const int nTeam)
	{
		g_TeamPatternObjectManager.SetTeam(m_nTeamPatternObjectHandle, nTeam);
	}

	// Add more accessors/mutators here as needed

private:
	int m_nTeamPatternObjectHandle;

	// Assignment & copy-construction disallowed
	CTeamPatternObject(const CTeamPatternObject &other);
	CTeamPatternObject& operator=(const CTeamPatternObject &other);
};

#endif //CBA_EFFECT_H