//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Description :	What is this?
//					This is a total rewrite of the SDK Shaders.
//					Under the prefix of lux_ shaders.
//					The goal is to make the most possible combinations work,
//					add new parameters and functionality and...
//					Most importantly.-Decrease Compile times. A lot.
//					For ACROHS specifically, we also add CSM support.
//					This nukes ps20b and below. No Linux support.
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	04.03.2023 DMY
//
//	Purpose of this File :	Because of ACROHS's CSM Implementation,
//							all Geometry Shaders must use the same struct
//							I also wanted to cut down on unnecessary Headers
//							So all Structs will be found in this file.
//							One exception of this might be the Uberlight_t Struct
//
//===========================================================================//

#include <string.h> // Required for memset()
#include "BaseVSShader.h"

// Shaders can access the flashlight from here.
#include "shadersource/flashlight/flashlight.h"

//===========================================================================//
// Sampler definitions for all shaders
//===========================================================================//

// For Stock Shaders :
const Sampler_t SAMPLER_BASETEXTURE		= SHADER_SAMPLER0;
const Sampler_t SAMPLER_BASETEXTURE2	= SHADER_SAMPLER1;
const Sampler_t SAMPLER_NORMALTEXTURE	= SHADER_SAMPLER2;
const Sampler_t SAMPLER_NORMALTEXTURE2	= SHADER_SAMPLER3;
const Sampler_t SAMPLER_DETAILTEXTURE	= SHADER_SAMPLER4;
const Sampler_t SAMPLER_ENVMAPMASK		= SHADER_SAMPLER5;	// Not rendered under the flashlight
const Sampler_t SAMPLER_LIGHTWARP		= SHADER_SAMPLER6;	// Not rendered under the flashlight
const Sampler_t SAMPLER_PARALLAXMAP		= SHADER_SAMPLER6;  // Same as Lightwarp, not compatible. Needed a Sampler
const Sampler_t SAMPLER_PHONGWARP		= SHADER_SAMPLER7;
const Sampler_t SAMPLER_PHONGEXPONENT	= SHADER_SAMPLER8;
const Sampler_t SAMPLER_ENVMAPMASK2		= SHADER_SAMPLER9;	// Not rendered under the flashlight
const Sampler_t SAMPLER_DETAILTEXTURE2	= SHADER_SAMPLER10;
const Sampler_t SAMPLER_LIGHTMAP		= SHADER_SAMPLER11;
const Sampler_t SAMPLER_BLENDMODULATE	= SHADER_SAMPLER12;
const Sampler_t SAMPLER_SELFILLUM		= SHADER_SAMPLER13; // Not rendered under the flashlight
const Sampler_t SAMPLER_ENVMAPTEXTURE	= SHADER_SAMPLER14; // Not rendered under the flashlight

// CRITICAL NOTE FOR SAMPLER 15 :
// So. APPARENTLY, and don't quote me on this, Valve always binds a Linear->Gamma 1D Lookup Table Texture to this Sampler.
// This didn't appear to be an issue AT ALL. Except when suddenly ( on models ) they would just replace envmaps with pure white(?!)
// So basically we moved EnvMap from 15 to 14, this fixes the issue 
const Sampler_t SAMPLER_SELFILLUM2 = SHADER_SAMPLER15; // Not rendered under the flashlight

// For the Flashlight :
const Sampler_t SAMPLER_SHADOWDEPTH			= SHADER_SAMPLER13;
const Sampler_t SAMPLER_RANDOMROTATION		= SHADER_SAMPLER14;
const Sampler_t SAMPLER_FLASHLIGHTCOOKIE	= SHADER_SAMPLER15;

#ifdef ACROHS_CSM
// For CSM : -- Same as Flashlight... ( Because no flashlight on CSM rendering )
const Sampler_t SAMPLER_CASCADED_SHADOW = SHADER_SAMPLER13;
const Sampler_t SAMPLER_STATIC_SHADOW = SHADER_SAMPLER14;
const Sampler_t SAMPLER_CLOUD_SHADOW = SHADER_SAMPLER15;
#endif

// For Screenrendered Shaders :


//===========================================================================//
// Our structs. We define first the contents and then the struct.
// That way we have way less lines of code and this becomes a little bit more readable
// We reuse the Geometry_Vars_t for all Geometry Shaders. For CSM on ACROHS
// And because it means we have less headers everywhere with unique Structs...
//===========================================================================//

// These are globally accessible and their parameters don't have to be defined in a shader
// To make the struct more readable its a define x().
#define Declare_GloballyDefinedStructVars()		\
int m_nBaseTexture;								\
int m_nBaseTextureFrame;						\
int m_nBaseTextureTransform;					\
int m_nFlashlightTexture;						\
int m_nFlashlightTextureFrame;					\
int m_nColor;//---------------------------------\
// int m_nsRGBtint;

// This is commented because I couldn't think of an effective way to not have to check flags
// int m_nColor;
//-------------------------------------------------------------------------------------
#define Declare_NormalTextureStructVars()		\
int m_nBumpMap;									\
int m_nBumpFrame;								\
int m_nBumpTransform;							\
int m_nNormalTexture;							\
int m_nSSBump;									\
int m_nSSBumpMathFix;							\
int m_nLightWarpTexture;						\
int m_nLightWarpNoBump;//-----------------------|
//-------------------------------------------------------------------------------------
#define Declare_ParallaxTextureStructVars()		\
int m_nParallaxMap;								\
int m_nParallaxMapScale;						\
int m_nParallaxMapMaxOffset;//------------------|
//-------------------------------------------------------------------------------------
#define Declare_DetailTextureStructVars()		\
int m_nDetailTexture;							\
int m_nDetailBlendMode;							\
int m_nDetailBlendFactor;						\
int m_nDetailScale;								\
int m_nDetailTextureTransform;					\
int m_nDetailTint;								\
int m_nDetailFrame;//---------------------------|
//-------------------------------------------------------------------------------------
#define Declare_Detail2TextureStructVars()		\
int m_nDetailTexture2;							\
int m_nDetailBlendMode2;						\
int m_nDetailBlendFactor2;						\
int m_nDetailScale2;							\
int m_nDetailTint2;								\
int m_nDetailFrame2;//--------------------------|
//-------------------------------------------------------------------------------------
#ifdef MODEL_LIGHTMAPPING
#define Declare_LightmappingStructVars()		\
int m_nLightMap;								\
int m_nLightMapUVs;//---------------------------|
// Don't remove this comment
#endif
//-------------------------------------------------------------------------------------
#define Declare_DistanceAlphaStructVars()		\
int m_nSoftEdges;								\
int m_nScaleEdgeSoftnessBasedOnScreenRes;		\
int m_nEdgeSoftnessStart;						\
int m_nEdgeSoftnessEnd;							\
int m_nOutline;									\
int m_nOutlineColor;							\
int m_nOutlineAlpha;							\
int m_nOutlineStart0;							\
int m_nOutlineStart1;							\
int m_nOutlineEnd0;								\
int m_nOutlineEnd1;								\
int m_nScaleOutlineSoftnessBasedOnScreenRes;//--|
//-------------------------------------------------------------------------------------
#ifdef CUBEMAPS
#define Declare_EnvironmentMapStructVars()		\
int m_nEnvMap;									\
int m_nEnvMapFrame;								\
int m_nEnvMapMaskFlip;							\
int m_nEnvMapTint;								\
int m_nEnvMapContrast;							\
int m_nEnvMapSaturation;						\
int m_nEnvMapLightScale;						\
int m_nEnvMapFresnel;							\
int m_nEnvMapFresnelMinMaxExp;					\
int m_nEnvMapAnisotropy;						\
int m_nEnvMapAnisotropyScale;//-----------------|
//-------------------------------------------------------------------------------------
// Note that $normalmapalphaenvmapmask and $basealphaenvmapmask are global vars.-Doesn't need to be defined here.
#define Declare_EnvMapMaskStructVars()			\
int m_nEnvMapMask;								\
int m_nEnvMapMaskFrame;							\
int m_nEnvMapMaskTransform;//-------------------|
//-------------------------------------------------------------------------------------
#ifdef PARALLAXCORRECTEDCUBEMAPS
#define Declare_ParallaxCorrectionStructVars()	\
int m_nEnvMapParallax;							\
int m_nEnvMapParallaxOBB1;						\
int m_nEnvMapParallaxOBB2;						\
int m_nEnvMapParallaxOBB3;						\
int m_nEnvMapOrigin;//--------------------------|
// Don't remove this comment
#endif
#else
// This avoids compile errors when CUBEMAPS is not defined
#define Declare_ParallaxCorrectionStructVars() ;
#endif
//-------------------------------------------------------------------------------------
#define Declare_DisplacementStructVars()		\
int m_nBaseTexture2;							\
int m_nBaseTextureFrame2;						\
int m_nBaseTextureTransform2;					\
int m_nBumpMap2;								\
int m_nBumpFrame2;								\
int m_nBumpTransform2;							\
int m_nBlendModulateTexture;					\
int m_nBlendModulateFrame;						\
int m_nBlendModulateTransform;//----------------|
//-------------------------------------------------------------------------------------

// Note that $selfillum is a global var.-Doesn't need to be defined here..
#define Declare_SelfIlluminationStructVars()	\
int m_nSelfIllumMask;							\
int m_nSelfIllumMaskScale;						\
int m_nSelfIllumMaskFrame;						\
int m_nSelfIllum_EnvMapMask_Alpha;				\
int m_nSelfIllumFresnel;						\
int m_nSelfIllumFresnelMinMaxExp;				\
int m_nSelfIllumTint;//-------------------------|
//-------------------------------------------------------------------------------------
#ifdef SELFILLUMTEXTURING
#define Declare_SelfIllumTextureStructVars()	\
int m_nSelfIllumTexture;						\
int m_nSelfIllumTextureFrame;//-----------------|
// Don't remove this comment
#endif
//-------------------------------------------------------------------------------------
#define Declare_MiscStructVars()				\
int m_nAlphaTestReference;						\
int m_nBlendTintByBaseAlpha;					\
int m_nBlendTintColorOverBase;					\
int m_nMultipurpose1;							\
int m_nMultipurpose2;							\
int m_nMultipurpose3;							\
int m_nMultipurpose4;							\
int m_nMultipurpose5;							\
int m_nMultipurpose6;							\
int m_nMultipurpose7;//-------------------------|
// NOTE: these are stuff like $time, $forward etc.
// We save a ton of ints on the struct by reusing these instead of declaring new ones.
//-------------------------------------------------------------------------------------
#ifdef PHONG_REFLECTIONS
#define Declare_PhongStructVars()				\
int m_nPhongExponent;							\
int m_nPhongTint;								\
int m_nPhongAlbedoTint;							\
int m_nPhongAlbedoBoost;						\
int m_nPhongWarpTexture;						\
int m_nPhongFresnelRanges;						\
int m_nPhongBoost;								\
int m_nPhongExponentTexture;					\
int m_nPhongExponentFactor;						\
int m_nPhong;									\
int m_nFakePhongParam;							\
int m_nBaseMapAlphaPhongMask;					\
int m_nBaseMapLuminancePhongMask;				\
int m_nInvertPhongMask;							\
int m_nPhongDisableHalfLambert;					\
int m_nPhongExponentTextureMask;				\
int m_nPhongNewBehaviour;						\
int m_nPhongNoEnvMap;							\
int m_nPhongNoEnvMapMask;//---------------------|
// $PhongAlbedoBoost used to be CS:GO+ only, but its just a multiplier for Albedo, so its very easy to implement
// $BaseMapLuminancePhongMask used to be L4D1+ only, but the code is in ASW's Phong Shader. ( And its also very simple... )
// $PhongExponentFactor used to be sdk2013mp/tf2+ only, but oh well it just replaces the *149... eaaasy
#define Declare_RimLightStructVars()			\
int m_nRimLight;								\
int m_nRimLightExponent;						\
int m_nRimLightBoost;							\
int m_nRimMask;//-------------------------------|
// Don't remove this comment
#endif
//-------------------------------------------------------------------------------------
#ifdef TREESWAYING
#define Declare_TreeswayStructVars()			\
int m_nTreeSway;								\
int m_nTreeSwayHeight;							\
int m_nTreeSwayStartHeight;						\
int m_nTreeSwayRadius;							\
int m_nTreeSwayStartRadius;						\
int m_nTreeSwaySpeed;							\
int m_nTreeSwaySpeedHighWindMultiplier;			\
int m_nTreeSwayStrength;						\
int m_nTreeSwayScrumbleSpeed;					\
int m_nTreeSwayScrumbleStrength;				\
int m_nTreeSwayScrumbleFrequency;				\
int m_nTreeSwayFalloffExp;						\
int m_nTreeSwayScrumbleFalloffExp;				\
int m_nTreeSwaySpeedLerpStart;					\
int m_nTreeSwaySpeedLerpEnd;					\
int m_nTreeSwayStatic;							\
int m_nTreeSwayStaticValues;//------------------|
// Don't remove this comment
#endif
//-------------------------------------------------------------------------------------
#define Declare_SeamlessStructVars()			\
int m_nSeamless_Base;							\
int m_nSeamless_Detail;							\
int m_nSeamless_Bump;							\
int m_nSeamless_EnvMapMask;						\
int m_nSeamless_Scale;							\
int m_nSeamless_DetailScale;					\
int m_nSeamless_BumpScale;						\
int m_nSeamless_EnvMapMaskScale;//--------------|

// This exists because I'm lazy, remove it later, thanks!
struct Geometry_Vars_t
{
	Geometry_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
		Declare_GloballyDefinedStructVars()

		// I'm just vibing, bumping, normal mapping
		Declare_NormalTextureStructVars()

		Declare_MiscStructVars()
		// Add the Glowy stuff
		Declare_SelfIlluminationStructVars()

		// ======================================= //
#ifdef DETAILTEXTURING
		// Add DetailTextures
		Declare_DetailTextureStructVars()
#endif
		// ======================================= //
#ifdef CUBEMAPS
		Declare_EnvironmentMapStructVars()
		Declare_EnvMapMaskStructVars()
#endif
		// ======================================= //
#ifdef SELFILLUMTEXTURING
		Declare_SelfIllumTextureStructVars()
#endif
		// ======================================= //
#ifdef PHONG_REFLECTIONS
		Declare_PhongStructVars()
		Declare_RimLightStructVars() // TODO: Should this not be part of Phong?
#endif
		// ======================================= //
		int m_nCloak;
		int m_nEmissiveBlend;
		int m_nDistanceAlpha;
		int m_nFleshInterior;
		// ======================================= //
#ifdef MODEL_LIGHTMAPPING
		Declare_LightmappingStructVars()
#endif
		// ======================================= //
#ifdef TREESWAYING
		Declare_TreeswayStructVars()
#endif
		// ======================================= //
#ifdef PARALLAXCORRECTEDCUBEMAPS
		Declare_ParallaxCorrectionStructVars()
#endif
};

struct Brush_Vars_t
{
	Brush_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars()
	Declare_SeamlessStructVars()

	// I'm just vibing, bumping, normal mapping
	Declare_NormalTextureStructVars()

	Declare_MiscStructVars()
	// Add the Glowy stuff
	Declare_SelfIlluminationStructVars()

// ======================================= //
#ifdef DETAILTEXTURING
	// Add DetailTextures
	Declare_DetailTextureStructVars()
#endif
// ======================================= //
#ifdef CUBEMAPS
	Declare_EnvironmentMapStructVars()
	Declare_EnvMapMaskStructVars()
#endif
// ======================================= //
#ifdef SELFILLUMTEXTURING
	Declare_SelfIllumTextureStructVars()
#endif
// ======================================= //
#ifdef PHONG_REFLECTIONS
	Declare_PhongStructVars()
	Declare_RimLightStructVars() // TODO: Should this not be part of Phong?
#endif
// ======================================= //
	int m_nDistanceAlpha; // Used to set the flag...
// ======================================= //
#ifdef PARALLAXCORRECTEDCUBEMAPS
	Declare_ParallaxCorrectionStructVars()
#endif
// ======================================= //
#ifdef PARALLAX_MAPPING
Declare_ParallaxTextureStructVars()
#endif
};

struct Model_Vars_t
{
	Model_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars()

		// I'm just vibing, bumping, normal mapping
		Declare_NormalTextureStructVars()

		Declare_MiscStructVars()
		// Add the Glowy stuff
		Declare_SelfIlluminationStructVars()

		// ======================================= //
#ifdef DETAILTEXTURING
		// Add DetailTextures
		Declare_DetailTextureStructVars()
#endif
		// ======================================= //
#ifdef CUBEMAPS
		Declare_EnvironmentMapStructVars()
		Declare_EnvMapMaskStructVars()
#endif
		// ======================================= //
#ifdef SELFILLUMTEXTURING
		Declare_SelfIllumTextureStructVars()
#endif
		// ======================================= //
#ifdef PHONG_REFLECTIONS
		Declare_PhongStructVars()
		Declare_RimLightStructVars() // TODO: Should this not be part of Phong?
#endif
		// ======================================= //
		int m_nCloak;
		int m_nEmissiveBlend;
		int m_nDistanceAlpha;
		int m_nFleshInterior;
		// ======================================= //
#ifdef MODEL_LIGHTMAPPING
		Declare_LightmappingStructVars()
#endif
		// ======================================= //
#ifdef TREESWAYING
		Declare_TreeswayStructVars()
#endif
		Declare_SeamlessStructVars()
#ifdef PARALLAX_MAPPING
		Declare_ParallaxTextureStructVars()
#endif
};

struct Displacement_Vars_t
{
	Displacement_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
		Declare_GloballyDefinedStructVars()
		int m_nColor2;
		int m_nBlendTintColorOverBase2;

		// I'm just vibing, bumping, normal mapping
		Declare_NormalTextureStructVars()

		Declare_MiscStructVars()
		Declare_DisplacementStructVars()
		// Add the Glowy stuff
		Declare_SelfIlluminationStructVars()
		int m_nSelfIllumMask2;
		int m_nSelfIllumMaskFrame2;
		int m_nSelfIllumMaskScale2;
		int m_nSelfIllumTint2;

		// ======================================= //
#ifdef DETAILTEXTURING
		// Add DetailTextures
		Declare_DetailTextureStructVars()
		Declare_Detail2TextureStructVars()
#endif
		// ======================================= //
#ifdef CUBEMAPS
		Declare_EnvironmentMapStructVars()
		Declare_EnvMapMaskStructVars()
		int m_nEnvMapMask2;
		int m_nEnvMapMaskFrame2;
	#ifdef PARALLAXCORRECTEDCUBEMAPS
		Declare_ParallaxCorrectionStructVars()
	#endif
#endif
		// ======================================= //
#ifdef SELFILLUMTEXTURING
		Declare_SelfIllumTextureStructVars()
		int m_nSelfIllumTexture2;
		int m_nSelfIllumTextureFrame2;
#endif
		// ======================================= //
		Declare_SeamlessStructVars()
};

struct Unlit_Vars_t
{
	Unlit_Vars_t() { memset(this, 0xFF, sizeof(*this)); }

	Declare_GloballyDefinedStructVars()
	Declare_SeamlessStructVars()
	Declare_MiscStructVars()
#ifdef DETAILTEXTURING
	// Add DetailTextures
	Declare_DetailTextureStructVars()
#endif
	int m_nDistanceAlpha;
#ifdef CUBEMAPS
	// Add first envmapping then masking
	Declare_EnvironmentMapStructVars()
	Declare_EnvMapMaskStructVars()
	#ifdef PARALLAXCORRECTEDCUBEMAPS
		Declare_ParallaxCorrectionStructVars()
	#endif
#endif
};

struct Screenspace_Vars_t
{
	Screenspace_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars();
	Declare_EnvironmentMapStructVars();

	int m_nPixelShader;
	int m_nDisableColorWrites;
	int m_nAlphatested;
	int m_nLinearRead_BaseTexture;
	int m_nLinearRead_Texture1;
	int m_nLinearRead_Texture2;
	int m_nLinearRead_Texture3;
	int m_nLinearWrite;
	int m_nC0_X;
	int m_nC0_Y;
	int m_nC0_Z;
	int m_nC0_W;
	int m_nC1_X;
	int m_nC1_Y;
	int m_nC1_Z;
	int m_nC1_W;
	int m_nC2_X;
	int m_nC2_Y;
	int m_nC2_Z;
	int m_nC2_W;
	int m_nC3_X;
	int m_nC3_Y;
	int m_nC3_Z;
	int m_nC3_W;
	int m_nTexture1;
	int m_nTexture2;
	int m_nTexture3;
	int m_nTexture4;
	int m_nTexture5;
	int m_nTexture6;
	int m_nTexture7;

	int m_nAlphaBlendingEnabled;
	int m_nDeferred;
};

struct Misc_Vars_t
{
	Misc_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars();

};

struct EmissiveBlend_Vars_t
{
	EmissiveBlend_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars();

	int m_nBlendStrength; // Amount this layer is blended in globally
	int m_nFlowTexture;
	int m_nEmissiveTexture;
	int m_nEmissiveTint;
	int m_nEmissiveScrollVector;
	int m_nTime;
};

// TODO: Do CloakPass Shader
struct Cloak_Vars_t
{
	Cloak_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars();

	int m_nCloakFactor;
	int m_nCloakColorTint;
	int m_nRefractAmount;

	int m_nBumpMap;
	int m_nBumpFrame;
	int m_nBumpTransform;
};

// TODO: Do FleshInterior Pass Shader
struct FleshInterior_Vars_t
{
	FleshInterior_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars();

	int m_nFleshTexture;
	int m_nFleshNoiseTexture;
	int m_nFleshBorderTexture1D;
	int m_nFleshNormalTexture;
	int m_nFleshSubsurfaceTexture;
	int m_nFleshCubeTexture;

	int m_nBorderNoiseScale;
	int m_nDebugForceFleshOn;
	int m_nEffectCenterRadius1;
	int m_nEffectCenterRadius2;
	int m_nEffectCenterRadius3;
	int m_nEffectCenterRadius4;

	int m_nSubsurfaceTint;
	int m_nBorderWidth;
	int m_nBorderSoftness; // > 0.0f && < 0.5f !
	int m_nBorderTint;
	int m_nGlobalOpacity;
	int m_nGlossBrightness;
	int m_nScrollSpeed;

	int m_nTime;
};

// TODO: Do DistanceAlpha Shader
struct DistanceAlpha_Vars_t
{
	DistanceAlpha_Vars_t() { memset(this, 0xFF, sizeof(*this)); }
	Declare_GloballyDefinedStructVars();
	Declare_DistanceAlphaStructVars();
	Declare_DetailTextureStructVars();
	Declare_MiscStructVars();
};

//===========================================================================//
// Parameter Declarations. Used on Shader param Declaration to enable Features
// If you use one of these, you must use the Var Linker Declarations too!!!
//===========================================================================//

#define Declare_NormalTextureParameters()\
SHADER_PARAM(BUMPMAP, SHADER_PARAM_TYPE_TEXTURE, "", "bump map")\
SHADER_PARAM(BUMPFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap")\
SHADER_PARAM(BUMPTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform" )\
SHADER_PARAM(NORMALTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "models/shadertest/shader1_normal", "bump map but... Ok yeah its just a bump map ok? gets replaced when $bumpmap")\
SHADER_PARAM(SSBUMP, SHADER_PARAM_TYPE_BOOL, "0", "If 1, $bumpmap is Self-Shadowing. Note that if you enable this, both bumpmaps for Blendtextures will be SS-Bumps!") \
SHADER_PARAM(SSBUMPMATHFIX, SHADER_PARAM_TYPE_BOOL, "0", "Supposed to replicate whatever l4d2 does... Idk ")\
SHADER_PARAM(LIGHTWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "light munging lookup texture")\
SHADER_PARAM(LIGHTWARPNOBUMP, SHADER_PARAM_TYPE_BOOL, "0", "Enable Lightwarp for Materials that don't have a bumpmap and don't want the default one either. Not that useful on Vertex Lighting...")

#define Declare_ParallaxTextureParameters()\
SHADER_PARAM(PARALLAXMAP			,	SHADER_PARAM_TYPE_TEXTURE	, "", "Heightmap on .x")\
SHADER_PARAM(PARALLAXMAPSCALE		,	SHADER_PARAM_TYPE_FLOAT		, "", "Heightmap Scale. Play around, doesn't make much sense")\
SHADER_PARAM(PARALLAXMAPMAXOFFSET	,	SHADER_PARAM_TYPE_FLOAT		, "", "Heightmap Maximum Offset. Only works on Parallax Interval Mapping")

#ifdef DETAILTEXTURING
#define Declare_DetailTextureParameters()\
SHADER_PARAM(DETAIL, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "first detail texture")\
SHADER_PARAM(DETAILBLENDMODE, SHADER_PARAM_TYPE_INTEGER, "0", "Usually 0-11. No SS Bumping on models. Reference the VDC to see what each do")\
SHADER_PARAM(DETAILBLENDFACTOR, SHADER_PARAM_TYPE_FLOAT, "", "how much detail you want?")\
SHADER_PARAM(DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture")\
SHADER_PARAM(DETAILTEXTURETRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$detail & $detail2 texcoord transform")\
SHADER_PARAM(DETAILTINT, SHADER_PARAM_TYPE_COLOR, "", "Tints $Detail")\
SHADER_PARAM(DETAILFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail")

#define Declare_Detail2TextureParameters()\
SHADER_PARAM(DETAIL2, SHADER_PARAM_TYPE_TEXTURE, "shadertest/detail", "Second detail texture, parameternames are consistens with but CSGO does not have $detailblendmode2")\
SHADER_PARAM(DETAILBLENDMODE2, SHADER_PARAM_TYPE_INTEGER, "0", "Usually 0-11. No SS Bumping on models. Reference the VDC to see what each do")\
SHADER_PARAM(DETAILBLENDFACTOR2, SHADER_PARAM_TYPE_FLOAT, "", "how much detail you want?")\
SHADER_PARAM(DETAILSCALE2, SHADER_PARAM_TYPE_FLOAT, "4", "scale of the detail texture")\
SHADER_PARAM(DETAILTINT2, SHADER_PARAM_TYPE_COLOR, "", "Tints $Detail")\
SHADER_PARAM(DETAILFRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $detail")
#endif

#ifdef MODEL_LIGHTMAPPING
#define Declare_LightmappingParameters()\
SHADER_PARAM(LIGHTMAP, SHADER_PARAM_TYPE_TEXTURE, "", "lightmap texture--will be bound by the engine but can be bound manually")\
SHADER_PARAM(LIGHTMAPUVS, SHADER_PARAM_TYPE_BOOL, "0", "Use specially encoded UV Information to have a separate UV for Lightmap use")
#endif

#ifdef SIGNED_DISTANCE_FIELD_ALPHA
#define Declare_DistanceAlphaParameters()\
SHADER_PARAM(DISTANCEALPHA, SHADER_PARAM_TYPE_INTEGER, "0", "Signed Distance Fields. Basically, Transparency but it doesn't look bad? Apparently has to be used with $translucent")\
SHADER_PARAM(SOFTEDGES, SHADER_PARAM_TYPE_INTEGER, "0", "Literally the exact same ____ing thing as $distancealpha. Even in Valves Code. $distancealpha and $softedges ")\
SHADER_PARAM(SCALEEDGESOFTNESSBASEDONSCREENRES, SHADER_PARAM_TYPE_INTEGER, "0", "Scale the size of the soft edges based upon resolution. 1024x768 = nominal.")\
SHADER_PARAM(EDGESOFTNESSSTART, SHADER_PARAM_TYPE_FLOAT, "0.6", "Start value for soft edges for distancealpha.")\
SHADER_PARAM(EDGESOFTNESSEND, SHADER_PARAM_TYPE_FLOAT, "0.5", "End value for soft edges for distancealpha.")\
SHADER_PARAM(OUTLINE, SHADER_PARAM_TYPE_INTEGER, "0", "Enable outline for distance coded textures.") \
SHADER_PARAM(OUTLINECOLOR, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "color of outline for distance coded images.") \
SHADER_PARAM(OUTLINEALPHA, SHADER_PARAM_TYPE_FLOAT, "", "alpha value for outline") \
SHADER_PARAM(OUTLINESTART0, SHADER_PARAM_TYPE_FLOAT, "", "outer start value for outline") \
SHADER_PARAM(OUTLINESTART1, SHADER_PARAM_TYPE_FLOAT, "", "inner start value for outline") \
SHADER_PARAM(OUTLINEEND0, SHADER_PARAM_TYPE_FLOAT, "", "inner end value for outline") \
SHADER_PARAM(OUTLINEEND1, SHADER_PARAM_TYPE_FLOAT, "", "outer end value for outline") \
SHADER_PARAM(SCALEOUTLINESOFTNESSBASEDONSCREENRES, SHADER_PARAM_TYPE_INTEGER, "0", "Scale the size of the soft part of the outline based upon resolution. 1024x768 = nominal.")
#endif

#ifdef CUBEMAPS
#define Declare_EnvironmentMapParameters()\
SHADER_PARAM(ENVMAP, SHADER_PARAM_TYPE_ENVMAP, "", "Set the cubemap for this material.")\
SHADER_PARAM(ENVMAPFRAME, SHADER_PARAM_TYPE_INTEGER, "", "The frame to start an animated envmap on")\
SHADER_PARAM(ENVMAPMASKFLIP, SHADER_PARAM_TYPE_BOOL, "", "Flips the EnvMapMask. This is done automatically when using $basealphaenvmapmask but can be overriden using this parameter!")\
SHADER_PARAM(ENVMAPTINT, SHADER_PARAM_TYPE_COLOR, "", "envmap tint. Works like vector3 scale where each number scales the specific color by that much")\
SHADER_PARAM(ENVMAPCONTRAST, SHADER_PARAM_TYPE_FLOAT, "", "0-1 where 1 is full contrast and 0 is pure cubemap")\
SHADER_PARAM(ENVMAPSATURATION, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "0-1 where 1 is default pure cubemap and 0 is b/w.")\
SHADER_PARAM(ENVMAPLIGHTSCALE, SHADER_PARAM_TYPE_FLOAT, "0.0", "Value of 0 will disable the behavior and is set by default")\
SHADER_PARAM(FRESNELREFLECTION, SHADER_PARAM_TYPE_FLOAT, "1.0", "1.0 == mirror, 0.0 == water")\
SHADER_PARAM(ENVMAPFRESNEL, SHADER_PARAM_TYPE_FLOAT, "0", "Degree to which Fresnel should be applied to env map")\
SHADER_PARAM(ENVMAPFRESNELMINMAXEXP, SHADER_PARAM_TYPE_VEC3, "[0.0 1.0 2.0]", "Min/max fresnel range and exponent for vertexlitgeneric")\
SHADER_PARAM(ENVMAPANISOTROPY, SHADER_PARAM_TYPE_BOOL, "0", "Enables modification of the Reflection Vector. Uses Normal Maps Alpha Channel for bending it, thus requires a $BumpMap")\
SHADER_PARAM(ENVMAPANISOTROPYSCALE, SHADER_PARAM_TYPE_FLOAT, "0", "Multiplication Factor of the Normal Map Alpha, which is used for lerping between the Regular Normal and Warped Normal")

// Note that $normalmapalphaenvmapmask and $basealphaenvmapmask are global vars.-Doesn't need to be defined here.
#define Declare_EnvMapMaskParameters()\
SHADER_PARAM(ENVMAPMASK, SHADER_PARAM_TYPE_TEXTURE, "", "specular texture for pbr_override")\
SHADER_PARAM(ENVMAPMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $envmapmask")\
SHADER_PARAM(ENVMAPMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$envmapmask transformation. Must include all values")
#endif

#ifdef PARALLAXCORRECTEDCUBEMAPS
#define Declare_ParallaxCorrectionParameters()\
SHADER_PARAM(ENVMAPPARALLAX, SHADER_PARAM_TYPE_BOOL, "0", "Enables parallax correction code for env_cubemaps")\
SHADER_PARAM(ENVMAPPARALLAXOBB1, SHADER_PARAM_TYPE_VEC4, "[1 0 0 0]", "The first line of the parallax correction OBB matrix")\
SHADER_PARAM(ENVMAPPARALLAXOBB2, SHADER_PARAM_TYPE_VEC4, "[0 1 0 0]", "The second line of the parallax correction OBB matrix")\
SHADER_PARAM(ENVMAPPARALLAXOBB3, SHADER_PARAM_TYPE_VEC4, "[0 0 1 0]", "The third line of the parallax correction OBB matrix")\
SHADER_PARAM(ENVMAPORIGIN, SHADER_PARAM_TYPE_VEC3, "[0 0 0]", "The world space position of the env_cubemap being corrected")
#endif

#define Declare_DisplacementParameters()\
SHADER_PARAM(BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "Second Basetexture, obviously, god why do I put these comments even, a toddler would understand this")\
SHADER_PARAM(FRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $basetexture2")\
SHADER_PARAM(BASETEXTURETRANSFORM2, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$basetexture2 texcoord transform")\
SHADER_PARAM(BUMPMAP2, SHADER_PARAM_TYPE_TEXTURE, "", "bump map")\
SHADER_PARAM(BUMPFRAME2, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $bumpmap2")\
SHADER_PARAM(BUMPTRANSFORM2, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$bumpmap texcoord transform")\
SHADER_PARAM(BLENDMODULATETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "texture to use r/g channels for blend range for")\
SHADER_PARAM(BLENDMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $blendmodulatetexture")\
SHADER_PARAM(BLENDMASKTRANSFORM, SHADER_PARAM_TYPE_MATRIX, "center .5 .5 scale 1 1 rotate 0 translate 0 0", "$blendmodulatetexture texcoord transform")\
SHADER_PARAM(BLENDTINTCOLOROVERBASE2, SHADER_PARAM_TYPE_FLOAT, "0", "blend between tint acting as a multiplication versus a replace, requires $blendtintbybasealpha")

// Note that $selfillum is a global var.-Doesn't need to be defined here..
#define Declare_SelfIlluminationParameters()\
SHADER_PARAM(SELFILLUMMASK, SHADER_PARAM_TYPE_TEXTURE, "", "Acts as a seperate mask instead of using an alpha channel")\
SHADER_PARAM(SELFILLUMMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "", "Range from 0.01 to 9.99, trying any other values might affect other effects")\
SHADER_PARAM(SELFILLUMMASKFRAME, SHADER_PARAM_TYPE_INTEGER, "", "")\
SHADER_PARAM(SELFILLUM_ENVMAPMASK_ALPHA, SHADER_PARAM_TYPE_BOOL, "", "Use the Envmapmask Alpha as Selfillum Mask")\
SHADER_PARAM(SELFILLUMFRESNEL, SHADER_PARAM_TYPE_BOOL, "", "Allows the material to use Fresnel Ranges for Selfillum")\
SHADER_PARAM(SELFILLUMFRESNELMINMAXEXP, SHADER_PARAM_TYPE_VEC3, "0.0 1.0 1.0", "Minimum Illumination, Maximum Illumination, Fresnel Exponent")\
SHADER_PARAM(SELFILLUMTINT, SHADER_PARAM_TYPE_COLOR, "[1.0 1.0 1.0]", "Tint the entire Selfillum")

#ifdef SELFILLUMTEXTURING
#define Declare_SelfIllumTextureParameters()\
SHADER_PARAM(SELFILLUMTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Emission/Selfillum Texture")\
SHADER_PARAM(SELFILLUMTEXTUREFRAME, SHADER_PARAM_TYPE_INTEGER, "0", "frame number for $Emissiontexture")
#endif

#define Declare_MiscParameters()\
SHADER_PARAM(ALPHATESTREFERENCE, SHADER_PARAM_TYPE_FLOAT, "0.0", "")\
SHADER_PARAM(BLENDTINTBYBASEALPHA, SHADER_PARAM_TYPE_BOOL, "0", "Use the base alpha to blend in the $color modulation")\
SHADER_PARAM(BLENDTINTCOLOROVERBASE, SHADER_PARAM_TYPE_FLOAT, "0", "blend between tint acting as a multiplication versus a replace, requires $blendtintbybasealpha")

#ifdef PHONG_REFLECTIONS
#define Declare_PhongParameters()\
SHADER_PARAM(PHONGEXPONENT, SHADER_PARAM_TYPE_FLOAT, "5.0", "Phong exponent for local specular lights" )\
SHADER_PARAM(PHONGTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Phong tint for local specular lights, funfact. This used to be a vec4... then a vec3... Now its what it should be!")\
SHADER_PARAM(PHONGALBEDOTINT, SHADER_PARAM_TYPE_BOOL, "1.0", "Apply tint by albedo (controlled by spec exponent texture")\
SHADER_PARAM(PHONGWARPTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "warp the specular term")\
SHADER_PARAM(PHONGFRESNELRANGES, SHADER_PARAM_TYPE_VEC3, "[0  0.5  1]", "Parameters for remapping fresnel output")\
SHADER_PARAM(PHONGBOOST, SHADER_PARAM_TYPE_FLOAT, "1.0", "Phong overbrightening factor (specular mask channel should be authored to account for this)")\
SHADER_PARAM(PHONGEXPONENTTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "shadertest/BaseTexture", "Phong Exponent map")\
SHADER_PARAM(PHONGEXPONENTFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "When using a phong exponent texture, this will be multiplied by the 0..1 that comes out of the texture.")\
SHADER_PARAM(PHONG, SHADER_PARAM_TYPE_STRING, "", "enables phong lighting. This is a String-Param on purpose!")\
SHADER_PARAM(BASEMAPALPHAPHONGMASK, SHADER_PARAM_TYPE_BOOL, "0", "indicates that there is no normal map and that the phong mask is in base alpha")\
SHADER_PARAM(INVERTPHONGMASK, SHADER_PARAM_TYPE_BOOL, "0", "invert the phong mask (0=full phong, 1=no phong)")\
SHADER_PARAM(PHONGDISABLEHALFLAMBERT, SHADER_PARAM_TYPE_BOOL, "0", "Disable half lambert for phong")\
SHADER_PARAM(REALPHONG, SHADER_PARAM_TYPE_BOOL, "0", "Internal Value used for determining whether Phong is set to anything but 0")\
SHADER_PARAM(PHONGALBEDOBOOST, SHADER_PARAM_TYPE_FLOAT, "0", "Replication of CS:GO+'s same Parameter. Will multiply $PhongAlbedoTint by this value.")\
SHADER_PARAM(BASEMAPLUMINANCEPHONGMASK, SHADER_PARAM_TYPE_BOOL, "0", "Multiply Phong by average luminance. Note this is not 'r+g+b/3', it uses dot(rgb, luminance weights)")\
SHADER_PARAM(PHONGNEWBEHAVIOUR, SHADER_PARAM_TYPE_BOOL, "0", "Use new masking behaviour. Aka what truly is in your vmt and not weird logic.")\
SHADER_PARAM(PHONGEXPONENTTEXTUREMASK, SHADER_PARAM_TYPE_BOOL, "0", "The blue channel of the exponenttexture is used for masking Phong.")\
SHADER_PARAM(PHONGNOENVMAP, SHADER_PARAM_TYPE_BOOL, "0", "Disable Envmapping on Phong.")\
SHADER_PARAM(PHONGNOENVMAPMASK, SHADER_PARAM_TYPE_BOOL, "0", "Disable Envmapping on Phong.")

#define Declare_RimLightParameters()\
SHADER_PARAM(RIMLIGHT, SHADER_PARAM_TYPE_BOOL, "0", "enables rim lighting")\
SHADER_PARAM(RIMLIGHTEXPONENT, SHADER_PARAM_TYPE_FLOAT, "4.0", "Exponent for rim lights")\
SHADER_PARAM(RIMLIGHTBOOST, SHADER_PARAM_TYPE_FLOAT, "1.0", "Boost for rim lights")\
SHADER_PARAM(RIMMASK, SHADER_PARAM_TYPE_BOOL, "0", "Indicates whether or not to use alpha channel of exponent texture to mask the rim term")
#endif

#ifdef TREESWAYING
#define Declare_TreeswayParameters()\
SHADER_PARAM(TREESWAY, SHADER_PARAM_TYPE_INTEGER, "0", "" )\
SHADER_PARAM(TREESWAYHEIGHT, SHADER_PARAM_TYPE_FLOAT, "1000", "")\
SHADER_PARAM(TREESWAYSTARTHEIGHT, SHADER_PARAM_TYPE_FLOAT, "0.2", "")\
SHADER_PARAM(TREESWAYRADIUS, SHADER_PARAM_TYPE_FLOAT, "300", "")\
SHADER_PARAM(TREESWAYSTARTRADIUS, SHADER_PARAM_TYPE_FLOAT, "0.1", "")\
SHADER_PARAM(TREESWAYSPEED, SHADERf_PARAM_TYPE_FLOAT, "1", "")\
SHADER_PARAM(TREESWAYSPEEDHIGHWINDMULTIPLIER, SHADER_PARAM_TYPE_FLOAT, "2", "")\
SHADER_PARAM(TREESWAYSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "10", "")\
SHADER_PARAM(TREESWAYSCRUMBLESPEED, SHADER_PARAM_TYPE_FLOAT, "0.1", "")\
SHADER_PARAM(TREESWAYSCRUMBLESTRENGTH, SHADER_PARAM_TYPE_FLOAT, "0.1", "")\
SHADER_PARAM(TREESWAYSCRUMBLEFREQUENCY, SHADER_PARAM_TYPE_FLOAT, "0.1", "")\
SHADER_PARAM(TREESWAYFALLOFFEXP, SHADER_PARAM_TYPE_FLOAT, "1.5", "")\
SHADER_PARAM(TREESWAYSCRUMBLEFALLOFFEXP, SHADER_PARAM_TYPE_FLOAT, "1.0", "")\
SHADER_PARAM(TREESWAYSPEEDLERPSTART, SHADER_PARAM_TYPE_FLOAT, "3", "")\
SHADER_PARAM(TREESWAYSPEEDLERPEND, SHADER_PARAM_TYPE_FLOAT, "6", "")\
SHADER_PARAM(TREESWAYSTATIC, SHADER_PARAM_TYPE_BOOL, "0", "")\
SHADER_PARAM(TREESWAYSTATICVALUES, SHADER_PARAM_TYPE_VEC2, "[0.5 0.5]", "")
#endif

#define Declare_SeamlessParameters()\
SHADER_PARAM(SEAMLESS_BASE, SHADER_PARAM_TYPE_BOOL, "0", "Enable Seamless Behaviour for the respective Texture")\
SHADER_PARAM(SEAMLESS_DETAIL, SHADER_PARAM_TYPE_BOOL, "0", "Enable Seamless Behaviour for the respective Texture")\
SHADER_PARAM(SEAMLESS_BUMP, SHADER_PARAM_TYPE_BOOL, "0", "Enable Seamless Behaviour for the respective Texture")\
SHADER_PARAM(SEAMLESS_ENVMAPMASK, SHADER_PARAM_TYPE_BOOL, "0", "Enable Seamless Behaviour for the respective Texture")\
SHADER_PARAM(SEAMLESS_SCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "The scale of the Seamless Behaviour")\
SHADER_PARAM(SEAMLESS_DETAILSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "The scale of the Seamless Behaviour")\
SHADER_PARAM(SEAMLESS_BUMPSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "The scale of the Seamless Behaviour")\
SHADER_PARAM(SEAMLESS_ENVMAPMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "1.0", "The scale of the Seamless Behaviour")

#define Declare_CloakParameters()																   \
SHADER_PARAM(CLOAKPASSENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enables cloak render in a second pass")\
SHADER_PARAM(CLOAKFACTOR, SHADER_PARAM_TYPE_FLOAT, "0.0", "")										 \
SHADER_PARAM(CLOAKCOLORTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Cloak color tint")				  \
SHADER_PARAM(REFRACTAMOUNT, SHADER_PARAM_TYPE_FLOAT, "2", "")

// Emissive Scroll Pass
#define Declare_EmissiveBlendParameters()														   \
SHADER_PARAM(EMISSIVEBLENDENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enable emissive blend pass")		\
SHADER_PARAM(EMISSIVEBLENDBASETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map")		 \
SHADER_PARAM(EMISSIVEBLENDSCROLLVECTOR, SHADER_PARAM_TYPE_VEC2, "[0.11 0.124]", "Emissive scroll vec")\
SHADER_PARAM(EMISSIVEBLENDSTRENGTH, SHADER_PARAM_TYPE_FLOAT, "1.0", "Emissive blend strength")		   \
SHADER_PARAM(EMISSIVEBLENDTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "self-illumination map")				\
SHADER_PARAM(EMISSIVEBLENDTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Self-illumination tint")			 \
SHADER_PARAM(EMISSIVEBLENDFLOWTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "flow map")						  \
SHADER_PARAM(TIME, SHADER_PARAM_TYPE_FLOAT, "0.0", "Needs CurrentTime Proxy")			


#define Declare_FleshInteriorParameters()														   \
SHADER_PARAM(FLESHINTERIORENABLED, SHADER_PARAM_TYPE_BOOL, "0", "Enable Flesh interior blend pass" )\
SHADER_PARAM(FLESHINTERIORTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh color texture")			 \
SHADER_PARAM(FLESHINTERIORNOISETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh noise texture")		  \
SHADER_PARAM(FLESHBORDERTEXTURE1D, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh border 1D texture")		   \
SHADER_PARAM(FLESHNORMALTEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh normal texture")					\
SHADER_PARAM(FLESHSUBSURFACETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh subsurface texture")			 \
SHADER_PARAM(FLESHCUBETEXTURE, SHADER_PARAM_TYPE_TEXTURE, "", "Flesh cubemap texture")					  \
SHADER_PARAM(FLESHBORDERNOISESCALE, SHADER_PARAM_TYPE_FLOAT, "1.5", "Flesh Noise UV scalar for border")	   \
SHADER_PARAM(FLESHDEBUGFORCEFLESHON, SHADER_PARAM_TYPE_BOOL, "0", "Flesh Debug full flesh")					\
SHADER_PARAM(FLESHSUBSURFACETINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Subsurface Color")					 \
SHADER_PARAM(FLESHBORDERWIDTH, SHADER_PARAM_TYPE_FLOAT, "0.3", "Flesh border")								  \
SHADER_PARAM(FLESHBORDERSOFTNESS, SHADER_PARAM_TYPE_FLOAT, "0.42", "Flesh border softness (> 0.0 && <= 0.5)")  \
SHADER_PARAM(FLESHBORDERTINT, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Flesh border Color")							\
SHADER_PARAM(FLESHEFFECTCENTERRADIUS1, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius")\
SHADER_PARAM(FLESHEFFECTCENTERRADIUS2, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius") \
SHADER_PARAM(FLESHEFFECTCENTERRADIUS3, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius")  \
SHADER_PARAM(FLESHEFFECTCENTERRADIUS4, SHADER_PARAM_TYPE_VEC4, "[0 0 0 0.001]", "Flesh effect center and radius")	\
SHADER_PARAM(FLESHGLOBALOPACITY, SHADER_PARAM_TYPE_FLOAT, "1.0", "Flesh global opacity")							 \
SHADER_PARAM(FLESHGLOSSBRIGHTNESS, SHADER_PARAM_TYPE_FLOAT, "0.66", "Flesh gloss brightness")						  \
SHADER_PARAM(FLESHSCROLLSPEED, SHADER_PARAM_TYPE_FLOAT, "1.0", "Flesh scroll speed")


//===========================================================================//
// 'Linker' Declarations. I don't know what this would be called
// But you 'link' the internal values to a parameter. So I call it linker ( because thats what they do )
// These will be used on the cpp's and not in the header, because
// we don't need to set up some of this on some of the shaders.
//===========================================================================//

#define Link_GlobalParameters()\
info.m_nBaseTexture = BASETEXTURE;\
info.m_nBaseTextureFrame = FRAME;\
info.m_nBaseTextureTransform = BASETEXTURETRANSFORM;\
info.m_nFlashlightTexture = FLASHLIGHTTEXTURE;\
info.m_nFlashlightTextureFrame = FLASHLIGHTTEXTUREFRAME;

// m_nsRGBtint= SRGBTINT; // TODO: Make this useless?
// This doesn't work because it wants to check MATERIAL_VAR_MODEL before its available in the function!
//m_nColor = IS_FLAG_SET(MATERIAL_VAR_MODEL) ? COLOR2 : COLOR;

#define Link_NormalTextureParameters()\
info.m_nBumpMap = BUMPMAP;\
info.m_nBumpFrame = BUMPFRAME;\
info.m_nBumpTransform = BUMPTRANSFORM;\
info.m_nNormalTexture = NORMALTEXTURE;\
info.m_nSSBump = SSBUMP;\
info.m_nSSBumpMathFix = SSBUMPMATHFIX;\
info.m_nLightWarpTexture = LIGHTWARPTEXTURE;\
info.m_nLightWarpNoBump = LIGHTWARPNOBUMP;

#ifdef PARALLAX_MAPPING
#define Link_ParallaxTextureParameters()\
info.m_nParallaxMap = PARALLAXMAP;\
info.m_nParallaxMapScale = PARALLAXMAPSCALE;\
info.m_nParallaxMapMaxOffset = PARALLAXMAPMAXOFFSET;

#else
#define Link_ParallaxTextureParameters() ;
// This exists so we don't Error out
#endif

#ifdef DETAILTEXTURING
#define Link_DetailTextureParameters()\
info.m_nDetailTexture = DETAIL;\
info.m_nDetailBlendMode = DETAILBLENDMODE;\
info.m_nDetailBlendFactor = DETAILBLENDFACTOR;\
info.m_nDetailScale = DETAILSCALE;\
info.m_nDetailTextureTransform = DETAILTEXTURETRANSFORM;\
info.m_nDetailTint = DETAILTINT;\
info.m_nDetailFrame = DETAILFRAME;

#define Link_Detail2TextureParameters()\
info.m_nDetailTexture2 = DETAIL2;\
info.m_nDetailBlendMode2 = DETAILBLENDMODE2;\
info.m_nDetailBlendFactor2 = DETAILBLENDFACTOR2;\
info.m_nDetailScale2 = DETAILSCALE2;\
info.m_nDetailTint2 = DETAILTINT2;\
info.m_nDetailFrame2 = DETAILFRAME2;
#else
#define Link_DetailTextureParameters() ;
#define Link_Detail2TextureParameters() ;
// This exists so we don't Error out
#endif

#define Link_MiscParameters()\
info.m_nAlphaTestReference = ALPHATESTREFERENCE;\
info.m_nBlendTintByBaseAlpha = BLENDTINTBYBASEALPHA;\
info.m_nBlendTintColorOverBase = BLENDTINTCOLOROVERBASE;
// Note : We don't link the multipurpose ones here. That is done on the shader! ( That is why they are multipurpose... )

#ifdef MODEL_LIGHTMAPPING
#define Link_LightmappingParameters()\
info.m_nLightMap = LIGHTMAP;\
info.m_nLightMapUVs = LIGHTMAPUVS;
#else
#define Link_LightmappingParameters() ;
// This exists so we don't Error out
#endif

#ifdef SELFILLUMTEXTURING
#define Link_SelfIllumTextureParameters()\
info.m_nSelfIllumTexture = SELFILLUMTEXTURE;\
info.m_nSelfIllumTextureFrame = SELFILLUMTEXTUREFRAME;
#else
#define Link_SelfIllumTextureParameters() ;
// This exists so we don't Error out
#endif

#ifdef SIGNED_DISTANCE_FIELD_ALPHA
#define Link_DistanceAlphaParameters()\
info.m_nDistanceAlpha = DISTANCEALPHA;\
info.m_nSoftEdges = SOFTEDGES;\
info.m_nScaleEdgeSoftnessBasedOnScreenRes = SCALEEDGESOFTNESSBASEDONSCREENRES;\
info.m_nEdgeSoftnessStart = EDGESOFTNESSSTART;\
info.m_nEdgeSoftnessEnd = EDGESOFTNESSEND;\
info.m_nOutline = OUTLINE;\
info.m_nOutlineColor = OUTLINECOLOR;\
info.m_nOutlineAlpha = OUTLINEALPHA; \
info.m_nOutlineStart0 = OUTLINESTART0; \
info.m_nOutlineStart1 = OUTLINESTART1; \
info.m_nOutlineEnd0 = OUTLINEEND0; \
info.m_nOutlineEnd1 = OUTLINEEND1; \
info.m_nScaleOutlineSoftnessBasedOnScreenRes = SCALEOUTLINESOFTNESSBASEDONSCREENRES;
#else
#define Link_DistanceAlphaParameters() ;
// This exists so we don't Error out
#endif

#ifdef CUBEMAPS
// We intentionally don't look for EnvMapFresnel because brushes don't use it.
#define Link_EnvironmentMapParameters()\
info.m_nEnvMap = ENVMAP;\
info.m_nEnvMapFrame = ENVMAPFRAME;\
info.m_nEnvMapMaskFlip = ENVMAPMASKFLIP;\
info.m_nEnvMapTint= ENVMAPTINT;\
info.m_nEnvMapContrast = ENVMAPCONTRAST;\
info.m_nEnvMapSaturation = ENVMAPSATURATION;\
info.m_nEnvMapLightScale = ENVMAPLIGHTSCALE;\
info.m_nEnvMapFresnelMinMaxExp = ENVMAPFRESNELMINMAXEXP;\
info.m_nEnvMapAnisotropy = ENVMAPANISOTROPY;\
info.m_nEnvMapAnisotropyScale = ENVMAPANISOTROPYSCALE;


#define Link_EnvMapFresnel()\
info.m_nEnvMapFresnel = ENVMAPFRESNEL;

#define Link_EnvMapFresnelReflection()\
info.m_nEnvMapFresnel = FRESNELREFLECTION;

#define Link_EnvMapMaskParameters()\
info.m_nEnvMapMask = ENVMAPMASK;\
info.m_nEnvMapMaskFrame = ENVMAPMASKFRAME;\
info.m_nEnvMapMaskTransform = ENVMAPMASKTRANSFORM;
#else
#define Link_EnvironmentMapParameters() ;
#define Link_EnvMapMaskParameters()		;
#define Link_EnvMapFresnel()			;
#define Link_EnvMapFresnelReflection()	;
// This exists so we don't Error out
#endif

#ifdef PARALLAXCORRECTEDCUBEMAPS
#define Link_ParallaxCorrectionParameters()\
info.m_nEnvMapParallax		= ENVMAPPARALLAX;\
info.m_nEnvMapParallaxOBB1	= ENVMAPPARALLAXOBB1;\
info.m_nEnvMapParallaxOBB2	= ENVMAPPARALLAXOBB2;\
info.m_nEnvMapParallaxOBB3	= ENVMAPPARALLAXOBB3;\
info.m_nEnvMapOrigin		= ENVMAPORIGIN;
#else
#define Link_ParallaxCorrectionParameters() ;
// This exists so we don't Error out
#endif

#define Link_DisplacementParameters()\
info.m_nBaseTexture2 = BASETEXTURE2;\
info.m_nBaseTextureFrame2 = FRAME2;\
info.m_nBaseTextureTransform2 = BASETEXTURETRANSFORM2;\
info.m_nBumpMap2 = BUMPMAP2;\
info.m_nBumpFrame2 = BUMPFRAME2;\
info.m_nBumpTransform2 = BUMPTRANSFORM2;\
info.m_nBlendModulateTexture = BLENDMODULATETEXTURE;\
info.m_nBlendModulateFrame = BLENDMASKFRAME;\
info.m_nBlendModulateTransform = BLENDMASKTRANSFORM;\

#define Link_SelfIlluminationParameters()\
info.m_nSelfIllumMask = SELFILLUMMASK;\
info.m_nSelfIllumMaskScale = SELFILLUMMASKSCALE;\
info.m_nSelfIllumMaskFrame = SELFILLUMMASKFRAME;\
info.m_nSelfIllum_EnvMapMask_Alpha = SELFILLUM_ENVMAPMASK_ALPHA;\
info.m_nSelfIllumFresnel = SELFILLUMFRESNEL;\
info.m_nSelfIllumFresnelMinMaxExp = SELFILLUMFRESNELMINMAXEXP;\
info.m_nSelfIllumTint = SELFILLUMTINT;

#ifdef PHONG_REFLECTIONS
#define Link_PhongParameters()\
info.m_nPhongExponent = PHONGEXPONENT;\
info.m_nPhongTint = PHONGTINT;\
info.m_nPhongAlbedoTint = PHONGALBEDOTINT;\
info.m_nPhongWarpTexture = PHONGWARPTEXTURE;\
info.m_nPhongFresnelRanges = PHONGFRESNELRANGES;\
info.m_nPhongBoost = PHONGBOOST;\
info.m_nPhongExponentTexture = PHONGEXPONENTTEXTURE;\
info.m_nPhongExponentFactor = PHONGEXPONENTFACTOR;\
info.m_nFakePhongParam = PHONG;\
info.m_nPhong = PHONG;\
info.m_nBaseMapAlphaPhongMask = BASEMAPALPHAPHONGMASK;\
info.m_nBaseMapLuminancePhongMask = BASEMAPLUMINANCEPHONGMASK;\
info.m_nInvertPhongMask = INVERTPHONGMASK;\
info.m_nPhongDisableHalfLambert = PHONGDISABLEHALFLAMBERT;\
info.m_nPhongExponentTextureMask = PHONGEXPONENTTEXTUREMASK;\
info.m_nPhongAlbedoBoost = PHONGALBEDOBOOST;\
info.m_nPhongNewBehaviour = PHONGNEWBEHAVIOUR;\
info.m_nPhongNoEnvMap = PHONGNOENVMAP;\
info.m_nPhongNoEnvMapMask = PHONGNOENVMAPMASK;
// REALPHONG


#define Link_RimLightParameters()\
info.m_nRimLight = RIMLIGHT;\
info.m_nRimLightExponent = RIMLIGHTEXPONENT;\
info.m_nRimLightBoost = RIMLIGHTBOOST;\
info.m_nRimMask = RIMMASK;
#else
#define Link_PhongParameters()		;
#define Link_RimLightParameters()	;
// This exists so we don't Error out
#endif

#ifdef TREESWAYING
#define Link_TreeswayParameters()\
info.m_nTreeSway = TREESWAY;\
info.m_nTreeSwayHeight = TREESWAYHEIGHT;\
info.m_nTreeSwayStartHeight TREESWAYSTARTHEIGHT;\
info.m_nTreeSwayRadius = TREESWAYRADIUS;\
info.m_nTreeSwayStartRadius = TREESWAYSTARTRADIUS;\
info.m_nTreeSwaySpeed = TREESWAYSPEED;\
info.m_nTreeSwaySpeedHighWindMultiplier = TREESWAYSPEEDHIGHWINDMULTIPLIER;\
info.m_nTreeSwayStrength = TREESWAYSTRENGTH,\
info.m_nTreeSwayScrumbleSpeed  = TREESWAYSCRUMBLESPEED;\
info.m_nTreeSwayScrumbleStrength = TREESWAYSCRUMBLESTRENGTH;\
info.m_nTreeSwayScrumbleFrequency = TREESWAYSCRUMBLEFREQUENCY;\
info.m_nTreeSwayFalloffExp = TREESWAYFALLOFFEXP;\
info.m_nTreeSwayScrumbleFalloffExp = TREESWAYSCRUMBLEFALLOFFEXP;\
info.m_nTreeSwaySpeedLerpStart = TREESWAYSPEEDLERPSTART;\
info.m_nTreeSwaySpeedLerpEnd = TREESWAYSPEEDLERPEND;\
info.m_nTreeSwayStatic = TREESWAYSTATIC;\
info.m_nTreeSwayStaticValues = TREESWAYSTATICVALUES;
#else
#define Link_TreeswayParameters()	;
// This exists so we don't Error out
#endif

#define Link_SeamlessParameters()\
info.m_nSeamless_Base				= SEAMLESS_BASE;\
info.m_nSeamless_Detail				= SEAMLESS_DETAIL;\
info.m_nSeamless_Bump				= SEAMLESS_BUMP;\
info.m_nSeamless_EnvMapMask			= SEAMLESS_ENVMAPMASK;\
info.m_nSeamless_Scale				= SEAMLESS_SCALE;\
info.m_nSeamless_DetailScale		= SEAMLESS_DETAILSCALE;\
info.m_nSeamless_BumpScale			= SEAMLESS_BUMPSCALE;\
info.m_nSeamless_EnvMapMaskScale	= SEAMLESS_ENVMAPMASKSCALE;