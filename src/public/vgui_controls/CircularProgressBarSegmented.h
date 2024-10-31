//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CIRCULARPROGRESSBARSEGMENTED_H
#define CIRCULARPROGRESSBARSEGMENTED_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/ProgressBar.h>

enum progress_textures_t
{
	PROGRESS_TEXTURE_FG,
	PROGRESS_TEXTURE_BG,

	NUM_PROGRESS_TEXTURES,
};

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Progress Bar in the shape of a pie graph
//-----------------------------------------------------------------------------
class CircularProgressBarSegmented : public ProgressBar 
{
	DECLARE_CLASS_SIMPLE( CircularProgressBarSegmented, ProgressBar );

public:
	CircularProgressBarSegmented(Panel *parent, const char *panelName);
	~CircularProgressBarSegmented();

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void ApplySchemeSettings(IScheme *pScheme);

	void SetFgImage(const char *imageName) { SetImage( imageName, PROGRESS_TEXTURE_FG ); }
	void SetBgImage(const char *imageName) { SetImage( imageName, PROGRESS_TEXTURE_BG ); }

	enum CircularProgressDir_e
	{
		PROGRESS_CW,
		PROGRESS_CCW
	};
	int GetProgressDirection() const { return m_iProgressDirection; }
	void SetProgressDirection( int val ) { m_iProgressDirection = val; }
	void SetStartSegment( int val ) { m_iStartSegment = val; }

protected:
	virtual void Paint();
	virtual void PaintBackground();
	
	void DrawCircleSegment( Color c, float flEndDegrees, bool clockwise /* = true */ );
	void SetImage(const char *imageName, progress_textures_t iPos);

private:
	int m_iProgressDirection;
	int m_iStartSegment;

	int m_nTextureId[NUM_PROGRESS_TEXTURES];
	char *m_pszImageName[NUM_PROGRESS_TEXTURES];
	int   m_lenImageName[NUM_PROGRESS_TEXTURES];
};

} // namespace vgui

#endif // CIRCULARPROGRESSBAR_H