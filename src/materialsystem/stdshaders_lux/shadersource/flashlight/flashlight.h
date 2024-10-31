//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	26.05.2023 DMY
//	Last Change :	26.05.2023 DMY
//
//	Purpose of this File :	Header for Flashlight setup functions. So that other files may use it
//
//===========================================================================//

#include <string.h> // Required for memset()

struct Flashlight_Vars_t
{
	// Set default values via constructor...
	Flashlight_Vars_t() { memset(this, 0xFF, sizeof(*this)); }

	// We cannot access shared header from here otherwise we will cause redefinition of various things.
	// This isn't very complicated either so this little mess should be fine.

	// Global
	int m_nBaseTexture;
	int m_nBaseTextureFrame;
	int m_nBaseTextureTransform;
	int m_nFlashlightTexture;
	int m_nFlashlightTextureFrame;
	int m_nColor;

	// Necessary Bump Params
	int m_nBumpMap;
	int m_nBumpFrame;
	int m_nBumpTransform;
	int m_nNormalTexture;
	int m_nSSBump;
	int m_nSSBumpMathFix;
	int m_nLightWarpTexture;
	int m_nLightWarpNoBump;

	// Detail Textures
	int m_nDetailTexture;
	int m_nDetailBlendMode;
	int m_nDetailBlendFactor;
	int m_nDetailScale;
	int m_nDetailTextureTransform;
	int m_nDetailTint;
	int m_nDetailFrame;
	
	// These just go unused when not on WVT
	int m_nDetailTexture2;
	int m_nDetailBlendMode2;
	int m_nDetailBlendFactor2;
	int m_nDetailScale2;
	int m_nDetailTint2;
	int m_nDetailFrame2;

	// We don't care for lightmaps but we do want to know if we have Lightmap UV's
	// So that we can tell the Shader whether or not it should unpack the UV
	int m_nLightMapUVs;

	// More WVT params
	int m_nBaseTexture2;
	int m_nBaseTextureFrame2;
	int m_nBaseTextureTransform2;
	int m_nBumpMap2;
	int m_nBumpFrame2;
	int m_nBumpTransform2;
	int m_nBlendModulateTexture;
	int m_nBlendModulateFrame;
	int m_nBlendModulateTransform;

	// We definitely need these...
	int m_nAlphaTestReference;
	int m_nBlendTintByBaseAlpha;
	int m_nBlendTintColorOverBase;

	// This will be nice to have for some stuff ( including debugging )
	int m_nMultipurpose1;
	int m_nMultipurpose2;
	int m_nMultipurpose3;
	int m_nMultipurpose4;
	int m_nMultipurpose5;
	int m_nMultipurpose6;
	int m_nMultipurpose7;

	// Phong related info we need
	int m_nPhongExponent;
	int m_nPhongTint;
	int m_nPhongAlbedoTint;
	int m_nPhongAlbedoBoost;
	int m_nPhongWarpTexture;
	int m_nPhongFresnelRanges;
	int m_nPhongBoost;
	int m_nPhongExponentTexture;
	int m_nPhongExponentFactor;
	int m_nPhong;
	int m_nBaseMapAlphaPhongMask;
	int m_nBaseMapLuminancePhongMask;
	int m_nInvertPhongMask;
	int m_nPhongDisableHalfLambert;
	int m_nPhongExponentTextureMask;
	int m_nPhongNewBehaviour;

	// Flashlight renders no envmaps or rimlighting.
//	int m_nPhongNoEnvMap;
//	int m_nPhongNoEnvMapMask;

	// Does the model sway? Yeah kinda important.
	int m_nTreeSway;
	int m_nTreeSwayHeight;
	int m_nTreeSwayStartHeight;
	int m_nTreeSwayRadius;
	int m_nTreeSwayStartRadius;
	int m_nTreeSwaySpeed;
	int m_nTreeSwaySpeedHighWindMultiplier;
	int m_nTreeSwayStrength;
	int m_nTreeSwayScrumbleSpeed;
	int m_nTreeSwayScrumbleStrength;
	int m_nTreeSwayScrumbleFrequency;
	int m_nTreeSwayFalloffExp;
	int m_nTreeSwayScrumbleFalloffExp;
	int m_nTreeSwaySpeedLerpStart;
	int m_nTreeSwaySpeedLerpEnd;
	int m_nTreeSwayStatic;
	int m_nTreeSwayStaticValues;
};

// Setting up the vars.
#define LuxFlashlight_Simple_Params() void LuxFlashlight_Simple_Link_Params(Flashlight_Vars_t &info)\
{									   \
	info.m_nColor = COLOR;				\
	info.m_nMultipurpose7 = MATERIALNAME;\
	info.m_nMultipurpose6 = DEBUG;		  \
	Link_DetailTextureParameters()		   \
	Link_GlobalParameters()					\
	Link_MiscParameters()					 \
	Link_NormalTextureParameters()			  \
}//----------------------------------------------------\

// Flashlight drawing functions ( depending on which shader you are using )

// Simple shader, no bumpmapping, models and brushes
void LuxFlashlight_Simple_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr, Flashlight_Vars_t &info, bool bIsModel);