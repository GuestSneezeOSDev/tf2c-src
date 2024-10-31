//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Description :	What is this?
//					PBR. Look it up!
//
//	Initial D.	:	25.09.2022 DMY
//	Last Change :	05.04.2023 DMY
//
//	Purpose of this File :	PBR for Brushes, Models and displacements.
//
//===========================================================================//
//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CBaseVSShader;
class IMaterialVar;
class IShaderDynamicAPI;
class IShaderShadow;

struct Infected_Vars_t
{
	Infected_Vars_t() { memset(this, 0xFF, sizeof(*this)); }

	Declare_GloballyDefinedStructVars()

	// Not using the Declare function because it has some unused things.
	//int m_nBumpMap;
	//int m_nBumpFrame;
	//int m_nBumpTransform;
	//int m_nNormalTexture;
	Declare_NormalTextureStructVars()
	Declare_MiscStructVars()
	Declare_DetailTextureStructVars()
	Declare_PhongStructVars()

	// Infected Specific Vars
	int m_nDisableVariation;
	int m_nGradientTexture;
	int m_nSheetIndex;
	int m_nColorTintGradient;
	int m_nDefaultPhongExponent;
	int m_nSkinTintGradient;
	int m_nSkinPhongExponent;
	int m_nBloodColor;
	int m_nBloodPhongExponent;
	int m_nBloodSpecBoost;
	int m_nBloodMaskRange;
	int m_nBurning;
	int m_nBurnStrength;
	int m_nBurnDetailTexture;
	int m_nEyeGlow;
	int m_nEyeGlowColor;
	int m_nEyeGlowFlashlightBoost;
	int m_nDetailPhongExponent;

	// Used for randomisation
	int m_nRandom1;
	int m_nRandom2;
	int m_nRandom3;
	int m_nRandom4;
};