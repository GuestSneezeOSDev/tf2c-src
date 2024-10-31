//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	LUX_WorldVertexTransition Shader for Displacements.
//
//===========================================================================//

// File in which you can determine what to use.
// We do not use preprocessor definitions for this because this is honestly faster and easier to set up.
#include "../../ACROHS_Defines.h"

// All Shaders use the same struct and sampler definitions. This way multipass CSM doesn't need to re-setup the struct
#include "../../ACROHS_Shared.h"

// We need all of these
#include "../CommonFunctions.h"
#include "../../cpp_shader_constant_register_map.h"

// this is required for our Function Array's
#include <functional>

// Includes for Shaderfiles...
#include "lux_worldvertextransition_simple_ps30.inc"
#include "lux_worldvertextransition_bump_ps30.inc"
#include "lux_displacement_vs30.inc"
#include "lux_flashlight_simple_ps30.inc"

extern ConVar mat_fullbright;
extern ConVar mat_printvalues;
extern ConVar mat_oldshaders;
extern ConVar mat_specular;

#ifdef DEBUG_LUXELS
extern ConVar mat_luxels;
#endif

#ifdef LIGHTWARP
extern ConVar mat_disable_lightwarp;
#endif

#ifdef BRUSH_PHONG
extern ConVar mat_enable_lightmapped_phong;
extern ConVar mat_force_lightmapped_phong;
extern ConVar mat_force_lightmapped_phong_boost;
extern ConVar mat_force_lightmapped_phong_exp;
#endif

#ifdef MAPBASE_FEATURED
extern ConVar mat_specular_disable_on_missing;
#endif


//==================================================================================================
// Static Shader Declarations. this is done to avoid a bazillion if statements for dynamic state.
// Also static state ( It doesn't quite matter there )
//==================================================================================================
void StaticShader_LuxWVT_Simple(IShaderShadow *pShaderShadow,
	bool bHasDetailTexture, bool bHasEnvMapMask, bool bHasFlashlight, int iSelfIllumMode,
	int iEnvMapMode, bool bPCC, bool bBlendTintByBaseAlpha, bool bEnvMapFresnel,
	bool bSeamlessBase, int iDetailUV, int iBumpUV, int iEnvMapMaskUV, bool bHasSeamlessSecondary, int iBlendModulateUV, bool bHasDetailTexture2, bool bHasSSBump)
{
	DECLARE_STATIC_VERTEX_SHADER(lux_displacement_vs30);
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, bSeamlessBase);
	SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iDetailUV);		// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0);				// 0 Regardless of iBumpUV
	SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, iEnvMapMaskUV);		// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(BLENDMODULATE_UV, iBlendModulateUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_SECONDARY, bHasSeamlessSecondary);
	SET_STATIC_VERTEX_SHADER(lux_displacement_vs30);

	if (bHasFlashlight)
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
		SET_STATIC_PIXEL_SHADER_COMBO(WORLDVERTEXTRANSITION, 1);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE2, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAPPED, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(SSBUMP, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONG, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONGEXPONENTTEXTURE, 0);
		SET_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_worldvertextransition_simple_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, iEnvMapMode);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPFRESNEL, bEnvMapFresnel);
		SET_STATIC_PIXEL_SHADER_COMBO(PCC, bPCC);
		SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUMMODE, iSelfIllumMode);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE2, bHasDetailTexture2);
		SET_STATIC_PIXEL_SHADER(lux_worldvertextransition_simple_ps30);
	}
}

void StaticShader_LuxWVT_Bump(IShaderShadow *pShaderShadow,
	bool bHasDetailTexture, bool bHasEnvMapMask, bool bHasFlashlight, int iSelfIllumMode,
	int iEnvMapMode, bool bPCC, bool bBlendTintByBaseAlpha, bool bEnvMapFresnel,
	bool bSeamlessBase, int iDetailUV, int iBumpUV, int iEnvMapMaskUV, bool bHasSeamlessSecondary, int iBlendModulateUV, bool bHasDetailTexture2, bool bHasSSBump)
{
	DECLARE_STATIC_VERTEX_SHADER(lux_displacement_vs30);
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, bSeamlessBase);
	SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iDetailUV);		// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, iBumpUV);			// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, iEnvMapMaskUV);		// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(BLENDMODULATE_UV, iBlendModulateUV); // 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_SECONDARY, bHasSeamlessSecondary);
	SET_STATIC_VERTEX_SHADER(lux_displacement_vs30);

	if (bHasFlashlight)
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
		SET_STATIC_PIXEL_SHADER_COMBO(WORLDVERTEXTRANSITION, 1);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE2, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAPPED, 1); // Yes
		SET_STATIC_PIXEL_SHADER_COMBO(SSBUMP, bHasSSBump);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONG, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONGEXPONENTTEXTURE, 0);
		SET_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_worldvertextransition_bump_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, iEnvMapMode);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPFRESNEL, bEnvMapFresnel);
		SET_STATIC_PIXEL_SHADER_COMBO(PCC, bPCC);
		SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUMMODE, iSelfIllumMode);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE2, bHasDetailTexture2);
		SET_STATIC_PIXEL_SHADER_COMBO(SSBUMP, bHasSSBump);
		SET_STATIC_PIXEL_SHADER(lux_worldvertextransition_bump_ps30);
	}
}

//==================================================================================================
// Dynamic Shader Declarations ( this is done to avoid a bazillion if statements for dynamic state.)
//
//==================================================================================================
void DynamicShader_LuxWVT_Simple(IShaderDynamicAPI *pShaderAPI, int ifogIndex, int iLightingPreview, bool bFlashlightShadows,
	int iPixelFogCombo, bool bWriteDepthToAlpha, bool bWriteWaterFogToAlpha, bool bHasFlashlight)
{
	DECLARE_DYNAMIC_VERTEX_SHADER(lux_displacement_vs30);
	SET_DYNAMIC_VERTEX_SHADER(lux_displacement_vs30);

	if (bHasFlashlight)
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_worldvertextransition_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, iPixelFogCombo);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
		SET_DYNAMIC_PIXEL_SHADER(lux_worldvertextransition_simple_ps30);
	}
}

void DynamicShader_LuxWVT_Bump(IShaderDynamicAPI *pShaderAPI, int ifogIndex, int iLightingPreview, bool bFlashlightShadows,
	int iPixelFogCombo, bool bWriteDepthToAlpha, bool bWriteWaterFogToAlpha, bool bHasFlashlight)
{
	DECLARE_DYNAMIC_VERTEX_SHADER(lux_displacement_vs30);
	SET_DYNAMIC_VERTEX_SHADER(lux_displacement_vs30);

	if (bHasFlashlight)
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_worldvertextransition_bump_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, iPixelFogCombo);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
		SET_DYNAMIC_PIXEL_SHADER(lux_worldvertextransition_bump_ps30);
	}
}

//==================================================================================================
// Putting these into arrays. Yes you heard right, function array!
// Could do it with int's but that will execute the code from the array and pointers dumb
//==================================================================================================
// pShaderShadow, bHasLightWarpTexture, bHasDetailTexture, bHasEnvMapMask, bHasFlashlight, iSelfIllumMode, iEnvMapMode, bPCC
std::function<void(IShaderShadow*, bool, bool, bool, int, int, bool, bool, bool, bool, int, int, int, bool, int, bool, bool)> DeclareWVTStatics[]
{
	StaticShader_LuxWVT_Simple, StaticShader_LuxWVT_Bump
};

// Todo put here what is what
std::function<void(IShaderDynamicAPI*, int, int, bool, int, bool, bool, bool)> DeclareWVTDynamics[]
{
	DynamicShader_LuxWVT_Simple, DynamicShader_LuxWVT_Bump
};

// COLOR is for brushes. COLOR2 for models
#define LuxWorldVertexTransition_Params() void LuxWorldVertexTransition_Link_Params(Displacement_Vars_t &info)  \
{																												\
	info.m_nColor2	= COLOR2;																					\
	info.m_nColor	= COLOR;																					\
	info.m_nMultipurpose7				= MATERIALNAME;															\
	info.m_nMultipurpose6				= SEAMLESS_SECONDARY;													\
	info.m_nMultipurpose5				= BLENDMODULATETRANSPARENCY;											\
	info.m_nMultipurpose4				= SEAMLESS_BLENDMASK;													\
	info.m_nMultipurpose3				= SEAMLESS_BLENDMASKSCALE;												\
	info.m_nEnvMapMask2					= ENVMAPMASK2;															\
	info.m_nEnvMapMaskFrame2			= ENVMAPMASKFRAME2;														\
	info.m_nSelfIllumTexture2			= SELFILLUMTEXTURE2;													\
	info.m_nSelfIllumMask2				= SELFILLUMMASK2;														\
	info.m_nSelfIllumTextureFrame2		= SELFILLUMTEXTUREFRAME2;												\
	info.m_nSelfIllumMaskFrame2			= SELFILLUMMASKFRAME2;													\
	info.m_nSelfIllumTint2				= SELFILLUMTINT2;														\
	info.m_nBlendTintColorOverBase2		= BLENDTINTCOLOROVERBASE2;												\
	Link_DetailTextureParameters()																				\
	Link_Detail2TextureParameters()																				\
	Link_EnvironmentMapParameters()																				\
	Link_EnvMapFresnelReflection()																				\
	Link_EnvMapMaskParameters()																					\
	Link_GlobalParameters()																						\
	Link_MiscParameters()																						\
	Link_NormalTextureParameters()																				\
	Link_ParallaxCorrectionParameters()																			\
	Link_SelfIlluminationParameters()																			\
	Link_SelfIllumTextureParameters()																			\
	Link_SeamlessParameters()																					\
	Link_DisplacementParameters()																				\
}//-------------------------------------------------------------------------------------------------------------|

void LuxWorldVertexTransition_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Displacement_Vars_t &info)
{
	// I wanted to debug print some values and needed to know which material they belong to. However pMaterialName is not available to the draw!
	// Well, now it is...
	params[info.m_nMultipurpose7]->SetStringValue(pMaterialName);

	// The usual...
	Flashlight_BorderColorSupportCheck();

	// We put $bumpmap on $normaltexture, then undefine it.
	// We trick the engine into thinking we don't have a normal map. This is required for Model Lightmapping ( with bump ) but its everywhere for consistency!
	// ShiroDkxtro2, 03.03.2022 : We don't have an m_nNormalTexture2, so we just go with bumps again.
	// Bumpmap_to_NormalTexture(info.m_nBumpMap, info.m_nNormalTexture);

	// Consistency with stock LMG
	// 07.02.2023 ShiroDkxtro2 : This will always spit out errors.
	// Are we using graphics? Yes this is a shader... Do we have envmap? YES. ITS SUPPOSED TO HAVE THEM. Can NOT use Editor Materials? YES this is not hammer!!!! 
	//if (pShader->IsUsingGraphics() && params[info.m_nEnvMap]->IsDefined() && !pShader->CanUseEditorMaterials())
	//{
	//	if (stricmp(params[info.m_nEnvMap]->GetStringValue(), "env_cubemap") == 0)
	//	{
	//		Warning("env_cubemap used on world geometry without rebuilding map. . ignoring: %s\n", pMaterialName);
	//		params[info.m_nEnvMap]->SetUndefined();
	//	}
	//}

#ifdef LIGHTWARP
	// Only try to undefine if defined...
	if (mat_disable_lightwarp.GetBool() && params[info.m_nLightWarpTexture]->IsDefined())
	{
		params[info.m_nLightWarpTexture]->SetUndefined();
	}
#endif

#ifdef CUBEMAPS_FRESNEL
	// These default values make no sense.
	// And the name is also terrible
	// Its Scale, Bias, Exponent... and not "min" ( which implies a minimum ), "max" ( which implies a maximum ), exp ( exponent )
	// You know what it actually is? Scale, Add, Fresnel Exponent... First value scales ( multiplies ), second value just... gets added ontop, and the last value is actually an exponent...
	// Yet Scale here is at 0 and it always adds 1.0
	// Is it supposed to be 1.0f, 0.0f, 2.0f ?
	Vec3ParameterDefault(info.m_nEnvMapFresnelMinMaxExp, 0.0f, 1.0f, 2.0f)
#else
	params[info.m_nEnvMapFresnel]->SetFloatValue(0);
#endif 

	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nDetailBlendFactor, 1.0f)
	FloatParameterDefault(info.m_nDetailBlendFactor2, 1.0f)

	// Default Value is supposed to be 4.0f
	FloatParameterDefault(info.m_nDetailScale, 4.0f)
	FloatParameterDefault(info.m_nDetailScale2, 4.0f)

	// Default Value is supposed to be 1.0f, 1.0f, 1.0f
	Vec3ParameterDefault(info.m_nEnvMapSaturation, 1.0f, 1.0f, 1.0f)


	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS(MATERIAL_VAR_NO_DEBUG_OVERRIDE);
	}

	// No BaseTexture ? None of these.
	// ShiroDkxtro2 : You could use a $basetexture2 without using $basetexture...
	//if (!params[info.m_nBaseTexture]->IsDefined())
	//{
	//	CLEAR_FLAGS(MATERIAL_VAR_SELFILLUM);
	//	CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK);
	//}

	SET_FLAGS2(MATERIAL_VAR2_LIGHTING_LIGHTMAP);
	if (g_pConfig->UseBumpmapping() && (params[info.m_nBumpMap]->IsDefined() || params[info.m_nBumpMap2]->IsDefined()))
	{
		SET_FLAGS2(MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP);

		// We have a $bumpmap2 but no $bumpmap. Will just bind a default one later but we should warn the Users about this...
		if (!params[info.m_nBumpMap]->IsDefined())
		{
			Warning("%s has a $BumpMap2 but no $BumpMap. Binding a default one instead.\n", pMaterialName);
		}
	}

	// If mat_specular 0, then get rid of envmap
	if (!g_pConfig->UseSpecular() && params[info.m_nEnvMap]->IsDefined() && params[info.m_nBaseTexture]->IsDefined())
	{
		params[info.m_nEnvMap]->SetUndefined();
#ifdef PARALLAXCORRECTEDCUBEMAPS
		params[info.m_nEnvMapParallax]->SetUndefined();
#endif
	}

#ifdef BRUSH_PHONG
		if (params[info.m_nPhong]->IsDefined() && !mat_enable_lightmapped_phong.GetBool())
		{
			params[info.m_nPhong]->SetUndefined();
		}

	FloatParameterDefault(info.m_nPhongBoost, 1.0f)
		FloatParameterDefault(info.m_nPhongExponent, 5.0f)
		Vec3ParameterDefault(info.m_nPhongFresnelRanges, 0.0f, 0.5f, 1.0f)

		if mat_enable_lightmapped_phong.GetBool()
		{

		}
	if (params[info.m_nPhong]->GetIntValue() != 0)
	{
		params[info.m_nPhong]->SetUndefined();
	}
	else if (mat_force_lightmapped_phong.GetBool() && params[info.m_nEnvmapMaskTransform]->MatrixIsIdentity())
	{
		params[info.m_nPhong]->SetIntValue(1);
		params[info.m_nPhongBoost]->SetFloatValue(mat_force_lightmapped_phong_boost.GetFloat());
		params[info.m_nPhongFresnelRanges]->SetVecValue(0.0, 0.5, 1.0);
		params[info.m_nPhongExponent]->SetFloatValue(mat_force_lightmapped_phong_exp.GetFloat());
	}
#endif

	// Seamless 
	if (GetFloatParamValue(info.m_nSeamless_Scale) != 0.0f)
	{
		// if we don't have DetailScale we want Seamless_Scale
		if (GetFloatParamValue(info.m_nSeamless_DetailScale) == 0.0f)
		{
			SetFloatParamValue(info.m_nSeamless_DetailScale, GetFloatParamValue(info.m_nSeamless_Scale))
		}
		// Using Seamless_Scale will enable Seamless_Base
		// IMPORTANT: **Not on VLG**. Stock behaviour demands you use $seamless_base
		SetIntParamValue(info.m_nSeamless_Base, 1)
	}
}

void LuxWorldVertexTransition_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Displacement_Vars_t &info)
{
	// Always needed...
	pShader->LoadTexture(info.m_nFlashlightTexture, TEXTUREFLAGS_SRGB);
	//	SET_FLAGS2(MATERIAL_VAR2_NEEDS_TANGENT_SPACES); // Supposedly you need this for the flashlight... Doubt X but ok
	SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_FLASHLIGHT);	// Yes, Yes we support
	SET_FLAGS2(MATERIAL_VAR2_USE_FLASHLIGHT);		// Yes, Yes we use it, what did you think? "Yeah we support it but don't use it, please"(?)
	SET_FLAGS2(MATERIAL_VAR2_LIGHTING_LIGHTMAP);	// We want lightmaps

	LoadTextureWithCheck(info.m_nBaseTexture, TEXTUREFLAGS_SRGB)
	LoadTextureWithCheck(info.m_nBaseTexture2, TEXTUREFLAGS_SRGB)
	LoadTextureWithCheck(info.m_nBumpMap, 0)
	LoadTextureWithCheck(info.m_nBumpMap2, 0)

	if (IsParamDefined(info.m_nBumpMap))
	{
		ITexture *pBumpMap = params[info.m_nBumpMap]->GetTextureValue();
		bool bIsSSBump = pBumpMap->GetFlags() & TEXTUREFLAGS_SSBUMP ? true : false;
		SetIntParamValue(info.m_nSSBump, bIsSSBump)
	}

	if (IsParamDefined(info.m_nBumpMap) || IsParamDefined(info.m_nBumpMap2))
	{
		SET_FLAGS2(MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP);	// We want lightmaps
		Warning("Set Bumped Lightmap flag on LUX_WVT");
	}
	LoadTextureWithCheck(info.m_nBlendModulateTexture, 0)

	// I can't make sense of whatever they tried to do on the Stock Shaders
	// They load Detail as sRGB but then don't do sRGB Read... and vice versa.
	// We will just check if this  Texture has the flag and do sRGB read based on that
		LoadTextureWithCheck(info.m_nDetailTexture, 0)
		LoadTextureWithCheck(info.m_nDetailTexture2, 0)

#ifdef LIGHTWARP
		LoadTextureWithCheck(info.m_nLightWarpTexture, 0)
#endif
	if (params[info.m_nSelfIllumMask]->IsDefined() || params[info.m_nSelfIllumMask2]->IsDefined())
	{
		LoadTextureWithCheck(info.m_nSelfIllumMask, 0)
		LoadTextureWithCheck(info.m_nSelfIllumMask2, 0)
	}
#ifdef SELFILLUMTEXTURING
	else
	{
		LoadTextureWithCheck(info.m_nSelfIllumTexture, TEXTUREFLAGS_SRGB)
		LoadTextureWithCheck(info.m_nSelfIllumTexture2, TEXTUREFLAGS_SRGB)
	}
#endif

#ifdef CUBEMAPS
	// ShiroDkxtro2 : Valve loads the cubemap as sRGB based on whether or not you are on HDR_TYPE_NONE
	// This makes no sense! You can use custom sRGB Cubemaps or have a fallback to LDR.
	// I don't support the continious usage of LDR, however it does not hurt to check at all
	LoadEnvMap(info.m_nEnvMap, 0)

#ifdef MAPBASE_FEATURED
		if (mat_specular_disable_on_missing.GetBool())
		{
			// Revert to defaultcubemap when the envmap texture is missing
			// (should be equivalent to toolsblack in Mapbase)
			if (params[info.m_nEnvMap]->GetTextureValue()->IsError())
			{
				params[info.m_nEnvMap]->SetStringValue("engine/defaultcubemap");
				pShader->LoadCubeMap(info.m_nEnvMap, 0);
			}
		}
#endif


	// This big block of if-statements is to determine if we even have any envmapmasking.
	// We don't want EnvMapMasking if we don't even have an envmap
	if (!params[info.m_nEnvMap]->GetTextureValue()->IsError())
	{
		if (params[info.m_nEnvMapMask]->IsDefined() || params[info.m_nEnvMapMask2]->IsDefined())
		{
			LoadTextureWithCheck(info.m_nEnvMapMask, 0)
			LoadTextureWithCheck(info.m_nEnvMapMask2, 0)

			// We already have an envmapmask now, so discard the others!
			CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK);
			CLEAR_FLAGS(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK);
		}
		else
		{
			// NormalMapAlphaEnvMapMask takes priority, I decided thats sensible because its the go to one
			if (IS_FLAG_SET(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK))
			{
				if (params[info.m_nBumpMap]->GetTextureValue()->IsError() || params[info.m_nBumpMap2]->GetTextureValue()->IsError())
					CLEAR_FLAGS(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK); // No normal map, no masking.

				CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK); // If we use normal map alpha, don't use basetexture alpha.
			}
			else
			{
				if (params[info.m_nBaseTexture]->GetTextureValue()->IsError() || params[info.m_nBaseTexture2]->GetTextureValue()->IsError())
					CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK); // If we have no Basetexture, can't use its alpha.
			}
		}

		// Tell the Shader to flip the EnvMap when set on AlphaEnvMapMask ( Consistency with Stock Shaders )
		if (!GetBoolParamValue(info.m_nEnvMapMaskFlip) && IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
		{
			SetIntParamValue(info.m_nEnvMapMaskFlip, 1);
		}
	}
#endif // #ifdef CUBEMAPS #endif
	else
	{ // No EnvMap == No Masking.
		CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK);
		CLEAR_FLAGS(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK);
	}

	// No Alphatesting/blendtint when we use the Alpha for other things
	// Valves Shaders ignore the fact you can use $envmapmask's alpha for selfillum...
	if (
#ifdef SELFILLUMTEXTURING
		// Must make sure we use $selfillum without selfillumtextures or alpha can't be used.
		(IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) && (!params[info.m_nSelfIllumTexture]->IsTexture() || !params[info.m_nSelfIllumTexture2]->IsTexture())) ||
#endif
		(IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) && !params[info.m_nSelfIllum_EnvMapMask_Alpha]->GetIntValue() == 1) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK)
		)
	{
		CLEAR_FLAGS(MATERIAL_VAR_ALPHATEST);
		SetIntParamValue(info.m_nBlendTintByBaseAlpha, 0)
	}
}

void LuxWorldVertexTransition_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, Displacement_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{

	//////////////////////////////////////////////////////////////////////////
	//		  BOOLEAN OPTIONS - USED THROUGH THE REST OF FUNCTION			//
	//////////////////////////////////////////////////////////////////////////

	// This is used for the function array that declares and sets static/dynamic shaders
	// 0 is the default. lux_worldvertextransition_simple_ps30
	// 1 is the bumped shader. lux_wvt_bump_ps30
	// 2 will probably be the phonged shader? I haven't looked into it yet...
	int iShaderCombo = 0;
	// Flagstuff
	// NOTE: We already made sure we don't have conflicting flags on Shader Init ( see above )
	bool bHasFlashlight				=						pShader->UsingFlashlight(params);
	bool bSelfIllum					=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_SELFILLUM); // No SelfIllum under the flashlight
	bool bAlphatest					=						IS_FLAG_SET(MATERIAL_VAR_ALPHATEST);
	bool bNormalMapAlphaEnvMapMask	=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK); // No Envmapping under the flashlight
	bool bBaseAlphaEnvMapMask		=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK); // No Envmapping under the flashlight
	bool bBlendTintByBaseAlpha		=						GetIntParamValue(info.m_nBlendTintByBaseAlpha) != 0;
	bool bHasEnvMapFresnel			=	!bHasFlashlight	&&	GetBoolParamValue(info.m_nEnvMapFresnel) != 0;

	// Texture related Boolean. We check for existing bools first because its faster
	bool	bHasBaseTexture			=						IsTextureLoaded(info.m_nBaseTexture);
	bool	bHasBaseTexture2		=						IsTextureLoaded(info.m_nBaseTexture2);
	bool	bHasNormalTexture		=						IsTextureLoaded(info.m_nBumpMap);
	bool	bHasNormalTexture2		=						IsTextureLoaded(info.m_nBumpMap2);
	bool	bHasAnyNormalTexture	=						(bHasNormalTexture || bHasNormalTexture2);
	bool	bHasSSBump				=	bHasNormalTexture && GetBoolParamValue(info.m_nSSBump);
#ifdef LIGHTWARP
	bool	bHasLightWarpTexture	=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nLightWarpTexture); // No Lightwarp under the flashlight
#endif
#ifdef DETAILTEXTURING
	bool	bHasDetailTexture		=						IsTextureLoaded(info.m_nDetailTexture);
	bool	bHasDetailTexture2		=						IsTextureLoaded(info.m_nDetailTexture2);
#endif
#ifdef CUBEMAPS
	bool	bHasEnvMap				=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nEnvMap); // No Envmapping under the flashlight
	bool	bHasEnvMapMask			=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nEnvMapMask); // No Envmapping under the flashlight
	bool	bHasEnvMapMask2			=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nEnvMapMask2); // No Envmapping under the flashlight
	bool	bHasAnyEnvMapMask		=	bHasEnvMapMask || bHasEnvMapMask2;
#ifdef PARALLAXCORRECTEDCUBEMAPS
	bool	bPCC					=	bHasEnvMap		&&	GetIntParamValue(info.m_nEnvMapParallax) != 0;
#else
	bool	bPCC					=	false;
#endif
#endif
	bool	bHasSelfIllumMask		=	bSelfIllum		&&	IsTextureLoaded(info.m_nSelfIllumMask);
	bool	bHasSelfIllumMask2		=	bSelfIllum		&&	IsTextureLoaded(info.m_nSelfIllumMask2);
	bool	bHasAnySelfIllumMask	=	bHasSelfIllumMask || bHasSelfIllumMask2;
	bool	bHasSelfIllumTexture	=	false;
	bool	bHasSelfIllumTexture2	=	false;
	bool	bHasAnySelfIllumTexture =	false;
#ifdef SELFILLUMTEXTURING
	if (!bHasAnySelfIllumMask)
	{
			bHasSelfIllumTexture	=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nSelfIllumTexture);
			bHasSelfIllumTexture2	=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nSelfIllumTexture2);
			bHasAnySelfIllumTexture =	bHasSelfIllumTexture || bHasSelfIllumTexture2;
	}
#endif
	bool	bHasBlendModulateTexture=						IsTextureLoaded(info.m_nBlendModulateTexture);

	//////////////////////////////////////////////////////////////////////////
	//				TEXTURE TRANSFORMATIONS & $Seamless						//
	//////////////////////////////////////////////////////////////////////////

	//	We check these later when we are sure we even have an EnvMapMask, Detail/NormalTexture
	//	NOTE: We don't check for BaseTextureTransform. Its the default fallback and if not changed, won't change anything.
	bool bBaseTextureTransform2		= !params[info.m_nBaseTextureTransform2]->MatrixIsIdentity();
	bool bNormalTextureTransform	= false;
	bool bNormalTexture2Transform	= false;
	bool bDetailTextureTransform	= false;
	bool bEnvMapMaskTransform		= false;
	bool bBlendMaskTransform		= bHasBlendModulateTexture && !params[info.m_nBlendModulateTransform]->MatrixIsIdentity();

	// Seamless Data
	bool bHasSeamlessSecondary		= GetIntParamValue(info.m_nMultipurpose6) != 0;
	bool bHasSeamlessBase			= GetIntParamValue(info.m_nSeamless_Base) != 0;
	bool bHasSeamlessDetail			= GetIntParamValue(info.m_nSeamless_Detail) != 0;
	bool bHasSeamlessBump			= GetIntParamValue(info.m_nSeamless_Bump) != 0;
	bool bHasSeamlessEnvMapMask		= GetIntParamValue(info.m_nSeamless_EnvMapMask) != 0;
	bool bHasSeamlessBlendMask		= GetIntParamValue(info.m_nMultipurpose4) != 0;
	// we need to tell the VertexShader if we want any Seamless Data at all
	bool bUsesSeamlessData			= bHasSeamlessSecondary || bHasSeamlessBase || bHasSeamlessDetail || bHasSeamlessBump || bHasSeamlessEnvMapMask;
	int	 iModeDetail				= 0;
	int	 iModeBump					= 0;
	int	 iModeEnvMapMask			= 0;
	int	 iModeBlendModulate			= 0;

	// When we don't have these, we don't compute them.

#ifdef DETAILTEXTURING
	if (bHasDetailTexture)
	{
		bDetailTextureTransform = !params[info.m_nDetailTextureTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nDetailTextureTransform);
		iModeDetail = CheckSeamless(bHasSeamlessDetail);
	}
#endif

#ifdef CUBEMAPS
	if (bHasEnvMapMask)
	{
		bEnvMapMaskTransform = !params[info.m_nEnvMapMaskTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nEnvMapMaskTransform);
		iModeEnvMapMask = CheckSeamless(bHasSeamlessEnvMapMask);
	}
#endif

	if (bHasNormalTexture)
	{
		iShaderCombo = 1;
		bNormalTextureTransform = !params[info.m_nBumpTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nBumpTransform);
		iModeBump = CheckSeamless(bHasSeamlessBump);
	}

	if (bHasNormalTexture2)
	{
		iShaderCombo = 1;
		bNormalTexture2Transform = !params[info.m_nBumpTransform2]->MatrixIsIdentity();//  IsParamDefined(info.m_nBumpTransform);
	}

	if (bHasBlendModulateTexture)
	{
		iModeBlendModulate = CheckSeamless(bHasSeamlessBlendMask);
	}

	//////////////////////////////////////////////////////////////////////////
	//			OTHER TEXTURE MODES - ENVMAP, SELFILLUM, DETAIL				//
	//////////////////////////////////////////////////////////////////////////

	// Purpose : Int to tell the Shader what Mask to use.
	//	0 = No Cubemap
	//	1 = $EnvMap no Masking
	//	2 = $EnvMapMask
	//	3 = $BaseAlphaEnvMapMask
	//	4 = $NormalMapAlphaEnvMapMask
	//	Order is important, we set the Combo to 3 for the non-bumped Shader and avoid writing a SKIP:
	//	bHasEnvMap will cause 0 if false, 1 if true. We then override based on the rest.
	//	If you have bBaseAlphaEnvMapMask, the EnvMapMask for each Texture will come from their respective Basetextures, etcetera 
	int iEnvMapMode = ComputeEnvMapMode(bHasEnvMap, bHasAnyEnvMapMask, bBaseAlphaEnvMapMask, bNormalMapAlphaEnvMapMask);
	int iSelfIllumMode = bSelfIllum; // 0 if no selfillum, 1 if selfillum is on. 2 is $SelfIllum_EnvMapMask_Alpha. 3 is $SelfIllumTexture

	// All of these conditions MUST be true.
	if (bHasAnyEnvMapMask && bSelfIllum && (GetIntParamValue(info.m_nSelfIllum_EnvMapMask_Alpha) == 1))
		iSelfIllumMode = 2;

#ifdef SELFILLUMTEXTURING
	if (bHasAnySelfIllumTexture) iSelfIllumMode = 3;
#endif

	bool IsOpaque(bIsFullyOpaque_1, info.m_nBaseTexture, bAlphatest);
	// FIXME: Determine both
//	bool IsOpaque(bIsFullyOpaque_2, info.m_nBaseTexture2, bAlphatest);
	// I have no idea what Destination Alpha has to do with all of this and why the EnvMapMask gets checked on stock shaders :(?
	bool bIsFullyOpaque = bIsFullyOpaque_1;// && bIsFullyOpaque_2;


	//===========================================================================//
	// Snapshotting State
	// This gets setup basically once, or again when using the flashlight
	// Once this is done we use dynamic state.
	//===========================================================================//
	if (pShader->IsSnapshotting())
	{

		pShaderShadow->EnableAlphaTest(bAlphatest);
		if (params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f) // 0 is default.
		{
			pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue());
		}

		pShaderShadow->EnableAlphaWrites(bIsFullyOpaque);

		//////////////////////////////////////////////////////////////////////////
		//							ENABLING SAMPLERS 							//
		//////////////////////////////////////////////////////////////////////////
		//if (pShaderAPI->InEditorMode())
		//{
		//	if (bHasDetailTexture2)
		//	{
		//		bHasDetailTexture2 = bHasDetailTexture;
		//		bHasDetailTexture = true;
		//	}
		//	else
		//	{
		//		bHasDetailTexture2 = bHasDetailTexture;
		//		bHasDetailTexture = false;
		//	}
		//}

/*s0*/	EnableSampler(SAMPLER_BASETEXTURE, true) // We always have a basetexture, and yes they should always be sRGB
/*s1*/	EnableSampler(SAMPLER_BASETEXTURE2, true) // We always have a basetexture2, and yes they should always be sRGB
		if (bHasAnyNormalTexture)
		{
/*s2*/		EnableSampler(SAMPLER_NORMALTEXTURE, false)	// No sRGB
/*s3*/		EnableSampler(SAMPLER_NORMALTEXTURE2, false)	// No sRGB
		}
		
#ifdef DETAILTEXTURING
		// ShiroDkxtro2: Stock Shaders do some cursed Ternary stuff with the detailblendmode
		// Which... If I interpret it correctly makes it read non-sRGB maps as sRGB and vice versa...
		if (bHasDetailTexture)
		{
			ITexture *pDetailTexture = params[info.m_nDetailTexture]->GetTextureValue();
/*s4*/		EnableSampler(SAMPLER_DETAILTEXTURE, (pDetailTexture->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
		}

		if (bHasDetailTexture2)
		{
			ITexture *pDetailTexture2 = params[info.m_nDetailTexture2]->GetTextureValue();
/*s10*/		EnableSampler(SAMPLER_DETAILTEXTURE2, (pDetailTexture2->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
		}
#endif

#ifdef CUBEMAPS
		// ShiroDkxtro2: Stock Shaders will enable sRGB based on HDR_TYPE_NONE
		// Yes. sRGB cubemaps would be read incorrectly on HDR.
		if (bHasEnvMap)
		{
			ITexture *pEnvMap = params[info.m_nEnvMap]->GetTextureValue();
/*s15*/		EnableSampler(SAMPLER_ENVMAPTEXTURE, (pEnvMap->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
			if(bHasAnyEnvMapMask)
			{
			
			}
/*s5*/		EnableSampler(SAMPLER_ENVMAPMASK, true) // Yes, Yes we want sRGB Read
/*s9*/		EnableSampler(SAMPLER_ENVMAPMASK2, true) // Yes, Yes we want sRGB Read
		}
#endif

#ifdef LIGHTWARP
/*s6*/	EnableSamplerWithCheck(bHasLightWarpTexture, SAMPLER_LIGHTWARP, true)
#endif
/*s7*/ // PhongWarp
/*s8*/ // PhongExponent

/*s11*/ EnableSampler(SAMPLER_LIGHTMAP, false) // Always. No sRGB
/*s12*/ EnableSamplerWithCheck(bHasBlendModulateTexture, SAMPLER_BLENDMODULATE, false)

		if (bSelfIllum)
		{
			if (bHasAnySelfIllumMask || bHasAnySelfIllumTexture)
			{
/*s13*/			EnableSampler(SAMPLER_SELFILLUM, true)
/*s14*/			EnableSampler(SAMPLER_SELFILLUM2, true)
			}
		}

		// s13, s14, s15, when under flashlight
		// TODO: Shouldn't we check BaseTexture2 for BlendRequirements too...?
		EnableFlashlightSamplers(bHasFlashlight, SAMPLER_SHADOWDEPTH, SAMPLER_RANDOMROTATION, SAMPLER_FLASHLIGHTCOOKIE, info.m_nBaseTexture)

		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_COLOR; \
		pShaderShadow->VertexShaderVertexFormat(flags, bHasAnyNormalTexture ? 3 : 2, 0, 0);

		//////////////////////////////////////////////////////////////////////////
		//						SETTING STATIC SHADERS							//
		//////////////////////////////////////////////////////////////////////////

		pShaderShadow->EnableSRGBWrite(true); // We always do sRGB Writes. We use ps30.
		auto DeclareStaticShaders = DeclareWVTStatics[iShaderCombo];
		DeclareStaticShaders(pShaderShadow, bHasDetailTexture, bHasEnvMapMask, bHasFlashlight,
		iSelfIllumMode, iEnvMapMode, bPCC, bBlendTintByBaseAlpha, bHasEnvMapFresnel,
		bHasSeamlessBase, iModeDetail, iModeBump, iModeEnvMapMask, bHasSeamlessSecondary, iModeBlendModulate, bHasDetailTexture2, bHasSSBump);
	}
	else // End of Snapshotting ------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{
		//////////////////////////////////////////////////////////////////////////
		//					   BINDING TEXTURES TO SAMPLERS						//
		//////////////////////////////////////////////////////////////////////////

		BindTextureWithCheckAndFallback(bHasBaseTexture, SAMPLER_BASETEXTURE, info.m_nBaseTexture, info.m_nBaseTextureFrame, TEXTURE_WHITE)
		BindTextureWithCheckAndFallback(bHasBaseTexture2, SAMPLER_BASETEXTURE2, info.m_nBaseTexture2, info.m_nBaseTextureFrame2, TEXTURE_WHITE)

		if (bHasAnyNormalTexture)
		{
			// if we don't have a $bumpmap just bind the $bumpmap2 again. This will cancel out on the blend in the shader, reproducing stock shader behaviour
			if(bHasNormalTexture)
			{
				BindTextureWithoutCheck(SAMPLER_NORMALTEXTURE, info.m_nBumpMap, info.m_nBumpFrame)
			}
			else 
			{
				BindTextureWithoutCheck(SAMPLER_NORMALTEXTURE, info.m_nBumpMap2, info.m_nBumpFrame2)
			}
		
			// if we don't have a $bumpmap2 just bind the $bumpmap again. This will cancel out on the blend in the shader, reproducing stock shader behaviour
			if (bHasNormalTexture2)
			{
				BindTextureWithoutCheck(SAMPLER_NORMALTEXTURE2, info.m_nBumpMap2, info.m_nBumpFrame2)
			}
			else
			{
				BindTextureWithoutCheck(SAMPLER_NORMALTEXTURE2, info.m_nBumpMap, info.m_nBumpFrame)
			}
			
			BindTextureStandard(SAMPLER_LIGHTMAP, TEXTURE_LIGHTMAP_BUMPED)
		}
		else
		{
			BindTextureStandard(SAMPLER_LIGHTMAP, TEXTURE_LIGHTMAP)
		}

#ifdef DETAILTEXTURING
		BindTextureWithCheck(bHasDetailTexture, SAMPLER_DETAILTEXTURE, info.m_nDetailTexture, info.m_nDetailFrame)
		BindTextureWithCheck(bHasDetailTexture2, SAMPLER_DETAILTEXTURE2, info.m_nDetailTexture2, info.m_nDetailFrame2)
#endif
		if (bSelfIllum)
		{
			if (bHasAnySelfIllumMask)
			{
				BindTextureWithCheckAndFallback(bHasSelfIllumMask, SAMPLER_SELFILLUM, info.m_nSelfIllumMask, info.m_nSelfIllumMaskFrame, TEXTURE_BLACK)
				BindTextureWithCheckAndFallback(bHasSelfIllumMask2, SAMPLER_SELFILLUM2, info.m_nSelfIllumMask2, info.m_nSelfIllumMaskFrame2, TEXTURE_BLACK)
			}
#ifdef SELFILLUMTEXTURING
			else if (bHasAnySelfIllumTexture)
			{
				BindTextureWithCheckAndFallback(bHasSelfIllumTexture, SAMPLER_SELFILLUM, info.m_nSelfIllumTexture, info.m_nSelfIllumTextureFrame, TEXTURE_BLACK)
				BindTextureWithCheckAndFallback(bHasSelfIllumTexture2, SAMPLER_SELFILLUM2, info.m_nSelfIllumTexture2, info.m_nSelfIllumTextureFrame2, TEXTURE_BLACK)
			}
#endif
		}

		if (bHasAnyEnvMapMask)
		{
			BindTextureWithCheckAndFallback(bHasEnvMapMask, SAMPLER_ENVMAPMASK, info.m_nEnvMapMask, info.m_nEnvMapMaskFrame, TEXTURE_BLACK)
			BindTextureWithCheckAndFallback(bHasEnvMapMask2, SAMPLER_ENVMAPMASK2, info.m_nEnvMapMask2, info.m_nEnvMapMaskFrame2, TEXTURE_BLACK)
		}

#ifdef CUBEMAPS
		if (bHasEnvMap)
		{
			BindTextureWithCheckAndFallback(mat_specular.GetBool(), SAMPLER_ENVMAPTEXTURE, info.m_nEnvMap, info.m_nEnvMapFrame, TEXTURE_BLACK)
		}
#endif

#ifdef LIGHTWARP
		BindTextureWithCheck(bHasLightWarpTexture, SAMPLER_LIGHTWARP, info.m_nLightWarpTexture, 0)
#endif

		BindTextureWithCheck(bHasBlendModulateTexture, SAMPLER_BLENDMODULATE, info.m_nBlendModulateTexture, info.m_nBlendModulateFrame)



#ifdef DEBUG_FULLBRIGHT2 
		// if mat_fullbright 2. Bind a standard white texture...
if (mat_fullbright.GetInt() == 2 && !IS_FLAG_SET(MATERIAL_VAR_NO_DEBUG_OVERRIDE))
{
	BindTextureStandard(SAMPLER_BASETEXTURE, TEXTURE_GREY) BindTextureStandard(SAMPLER_BASETEXTURE2, TEXTURE_GREY)
}
#endif
#ifdef DEBUG_LUXELS
// Debug Luxel Texture
if (mat_luxels.GetBool())
{
	BindTextureStandard(SAMPLER_LIGHTMAP, TEXTURE_DEBUG_LUXELS);
}
#endif
		//////////////////////////////////////////////////////////////////////////
		//					   PREPARING CONSTANT REGISTERS						//
		//////////////////////////////////////////////////////////////////////////

		// f4Empty is just float4 Name = {1,1,1,1};
		// Yes I have some excessive problems with macro overusage...
		f4Empty(f4BaseTextureTint_Factor)
		f4Empty(f4BaseTexture2Tint_Factor)
		f4Empty(f4DetailTint_BlendFactor)
		f4Empty(f4Detail2Tint_BlendFactor)
		f4Empty(f4SelfIllumTint_Scale)
		f4Empty(f4SelfIllum2Tint_Scale)

		f4Empty(f4SelfIllumFresnelMinMaxExp)
		f4Empty(f4EnvMapTint_LightScale)
		f4Empty(f4EnvMapFresnelRanges_)
		f4Empty(f4EnvMapSaturation_Contrast)
		f4Empty(f4DetailBlendModes) // yzw empty
		f4Empty(f4EnvMapControls)
		BOOL BBools[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };

		// $color/$color2 and $BlendTintColorOverBase
		GetVec3ParamValue(info.m_nColor, f4BaseTextureTint_Factor);
		f4BaseTextureTint_Factor[3] = GetFloatParamValue(info.m_nBlendTintColorOverBase); // Consider writing a macro for this
		GetVec3ParamValue(info.m_nColor2, f4BaseTexture2Tint_Factor);
		f4BaseTexture2Tint_Factor[3] = GetFloatParamValue(info.m_nBlendTintColorOverBase2);

#ifdef DETAILTEXTURING
		if (bHasDetailTexture)
		{
			// $DetailTint and $DetailBlendFactor
			GetVec3ParamValue(info.m_nDetailTint, f4DetailTint_BlendFactor);
			f4DetailTint_BlendFactor[3] = GetFloatParamValue(info.m_nDetailBlendFactor); // Consider writing a macro for this
			f4DetailBlendModes[0] = GetIntParamValue(info.m_nDetailBlendMode); // We trunc it later and do some dynamic branching. Saves a ton of combos but makes rendering slower. but oh well...
		}

		if (bHasDetailTexture2)
		{
			// $DetailTint and $DetailBlendFactor
			GetVec3ParamValue(info.m_nDetailTint2, f4Detail2Tint_BlendFactor);
			f4Detail2Tint_BlendFactor[3] = GetFloatParamValue(info.m_nDetailBlendFactor2); // Consider writing a macro for this
			f4DetailBlendModes[1] = GetIntParamValue(info.m_nDetailBlendMode2); // We trunc it later and do some dynamic branching. Saves a ton of combos but makes rendering slower. but oh well...
		}
#endif

		if (bSelfIllum)
		{
			GetVec3ParamValue(info.m_nSelfIllumFresnelMinMaxExp, f4SelfIllumFresnelMinMaxExp); // .w empty
			GetVec3ParamValue(info.m_nSelfIllumTint, f4SelfIllumTint_Scale);
			GetVec3ParamValue(info.m_nSelfIllumTint2, f4SelfIllum2Tint_Scale);
			if (bHasSelfIllumMask) // Mask Scale is only for $SelfIllumMask and NOT for $SelfIllumTexture
			{
				f4SelfIllumTint_Scale[3] = GetFloatParamValue(info.m_nSelfIllumMaskScale);
			}
			else
			{
				f4SelfIllumTint_Scale[3] = 0.0f;
			}

		}

#ifdef CUBEMAPS
		if (bHasEnvMap)
		{
			f4EnvMapControls[0] = bBaseAlphaEnvMapMask;
			f4EnvMapControls[1] = bNormalMapAlphaEnvMapMask;
			f4EnvMapControls[2] = GetBoolParamValue(info.m_nEnvMapMaskFlip); // The shader will lerp between 1.0f - envmapmask and envmapmask based on this bool
			//				[3] = No EnvMapAnisotropy on WVT

			GetVec3ParamValue(info.m_nEnvMapTint, f4EnvMapTint_LightScale);
			f4EnvMapTint_LightScale[3] = GetFloatParamValue(info.m_nEnvMapLightScale); // We always need the LightScale.

			GetVec3ParamValue(info.m_nEnvMapSaturation, f4EnvMapSaturation_Contrast); // Yes. Yes this is a vec3 parameter.
			f4EnvMapSaturation_Contrast[3] = GetFloatParamValue(info.m_nEnvMapContrast);

			if (bHasEnvMapFresnel)
			{
				GetVec3ParamValue(info.m_nEnvMapFresnelMinMaxExp, f4EnvMapFresnelRanges_);
			}
		}
	#ifdef PARALLAXCORRECTEDCUBEMAPS
		SetUpPCC(bPCC, info.m_nEnvMapOrigin, info.m_nEnvMapParallaxOBB1, info.m_nEnvMapParallaxOBB2, info.m_nEnvMapParallaxOBB3, 39, 38)
	#endif
#endif

		// This does a lot of things for us!!
		// Just don't touch constants <32 and you should be fine c:
		bool bFlashlightShadows = false;
		SetupStockConstantRegisters(bHasFlashlight, SAMPLER_FLASHLIGHTCOOKIE, SAMPLER_RANDOMROTATION, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame, bFlashlightShadows)

		int iFogIndex = 0;
		bool bWriteDepthToAlpha = false;
		bool bWriteWaterFogToAlpha = false;
		SetupDynamicComboVariables(iFogIndex, bWriteDepthToAlpha, bWriteWaterFogToAlpha, bIsFullyOpaque)

		//////////////////////////////////////////////////////////////////////////
		//						SETTING CONSTANT REGISTERS (PS)					//
		//////////////////////////////////////////////////////////////////////////
		pShaderAPI->SetPixelShaderConstant(32, f4BaseTextureTint_Factor, 1);
		pShaderAPI->SetPixelShaderConstant(33, f4DetailTint_BlendFactor, 1);
		pShaderAPI->SetPixelShaderConstant(34, f4SelfIllumTint_Scale, 1);
		pShaderAPI->SetPixelShaderConstant(35, f4EnvMapTint_LightScale, 1);
		pShaderAPI->SetPixelShaderConstant(36, f4EnvMapFresnelRanges_, 1);
		pShaderAPI->SetPixelShaderConstant(37, f4DetailBlendModes, 1);
		//										   38, PCC EnvMap Origin
		//										   39, PCC Bounding	Box
		//										   40, PCC Bounding	Box
		//										   41, PCC Bounding	Box
		//		pShaderAPI->SetPixelShaderConstant(42, f4PhongTint_Boost, 1);
		//		pShaderAPI->SetPixelShaderConstant(43, f4PhongFresnelRanges_Exponent, 1);
		//		pShaderAPI->SetPixelShaderConstant(44, f4RimLightControls, 1);
		pShaderAPI->SetPixelShaderConstant(45, f4SelfIllumFresnelMinMaxExp, 1);
		pShaderAPI->SetPixelShaderConstant(46, f4EnvMapSaturation_Contrast, 1);
		pShaderAPI->SetPixelShaderConstant(47, f4BaseTexture2Tint_Factor, 1);
		pShaderAPI->SetPixelShaderConstant(48, f4Detail2Tint_BlendFactor, 1);
		pShaderAPI->SetPixelShaderConstant(49, f4SelfIllum2Tint_Scale, 1);

		pShaderAPI->SetPixelShaderConstant(51, f4EnvMapControls, 1);

		//////////////////////////////////////////////////////////////////////////
		//						SETTING CONSTANT REGISTERS (VS)					//
		//////////////////////////////////////////////////////////////////////////
		// Always having this
		pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_13, info.m_nBaseTextureTransform);

		if (bBaseTextureTransform2)
			pShader->SetVertexShaderTextureTransform(242, info.m_nBaseTextureTransform2);
		else
			pShader->SetVertexShaderTextureTransform(242, info.m_nBaseTextureTransform); // Fallback to Basetexture coords...

		if (bNormalTextureTransform)
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_15, info.m_nBumpTransform);
		else
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_15, info.m_nBaseTextureTransform);

		if (bNormalTexture2Transform)
			pShader->SetVertexShaderTextureTransform(244, info.m_nBumpTransform2);
		else if(bNormalTextureTransform) 
			pShader->SetVertexShaderTextureTransform(244, info.m_nBumpTransform); // Fallback to Bump coords...
		else
			pShader->SetVertexShaderTextureTransform(244, info.m_nBaseTextureTransform); // Fallback to Basetexture coords...

		if (bDetailTextureTransform)
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nDetailTextureTransform, info.m_nDetailScale);
		else
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nBaseTextureTransform, info.m_nDetailScale);

		if (bEnvMapMaskTransform)
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_19, info.m_nEnvMapMaskTransform);
		else
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_19, info.m_nBaseTextureTransform);

		if (bBlendMaskTransform)
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, info.m_nBlendModulateTransform);
		else
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, info.m_nBaseTextureTransform);

		f4Empty(SeamlessScales)
		f4Empty(SeamlessScales2) // only .x is used for Seamless BlendModulateTextures...
			if (bUsesSeamlessData) // Skip all of this if not used.
			{
				if (bHasSeamlessBase)
				{
					// All textures must use these coordinates or they will end up scuffed.
					// The way this works is that the Vertex Shader will output the weights and UV of the Basetexture for all textures with no regards to transforms
					// They can't get screwed up but you also can't transform them unless you also enable Seamless Scale for those Textures.
					// This might have undesired consequences on Custom Materials that might have abused the previous behaviour on purpose.
					SeamlessScales[0] = GetFloatParamValue(info.m_nSeamless_Scale);
					BBools[3] = true; // bHasSeamless_Base
					BBools[4] = true; // bHasSeamless_Detail
					BBools[5] = true; // bHasSeamless_Bump
					BBools[6] = true; // bHasSeamless_EnvMapMask
				}

				if (bHasSeamlessDetail)
				{
					BBools[4] = true; // bHasSeamless_Detail
					SeamlessScales[1] = GetFloatParamValue(info.m_nSeamless_DetailScale);
				}

				if (bHasSeamlessBump)
				{
					BBools[5] = true; // bHasSeamless_Bump
					SeamlessScales[2] = GetFloatParamValue(info.m_nSeamless_BumpScale);
				}

				if (bHasSeamlessEnvMapMask)
				{
					BBools[6] = true; // bHasSeamless_EnvMapMask
					SeamlessScales[3] = GetFloatParamValue(info.m_nSeamless_EnvMapMaskScale);
				}
				
				if (bHasSeamlessBlendMask)
				{
					SeamlessScales2[0] = GetFloatParamValue(info.m_nMultipurpose3); // Seamless Scale must be added
				}

				if (bHasSeamlessSecondary)
				{
					SeamlessScales[0] = GetFloatParamValue(info.m_nSeamless_Scale); // Always required...
					BBools[7] = true;
				}
				// I tried shader specific ones. 0-3 and those didn't work. So we are going to use our new registers instead.
				pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_30, SeamlessScales);
				pShaderAPI->SetVertexShaderConstant(241, SeamlessScales2); // Oops I didn't make any SPECIFIC_CONST above 30... c:
			}

		// Always! Required for Lightwarp
		BBools[1] = bHasLightWarpTexture;
		// Always! Required for BlendModulate.
		BBools[8] = bHasBlendModulateTexture;
		BBools[9] = bHasBlendModulateTexture && (GetIntParamValue(info.m_nMultipurpose5) != 0); // bHasBlendModulateTransparency
		BBools[10] = pShaderAPI->InEditorMode();

		BBools[15] = bWriteDepthToAlpha;
		pShaderAPI->SetBooleanPixelShaderConstant(0, BBools, 16, true);

		//////////////////////////////////////////////////////////////////////////
		//						SETTING DYNAMIC SHADERS							//
		//////////////////////////////////////////////////////////////////////////

		auto DeclareDynamic = DeclareWVTDynamics[iShaderCombo];
		DeclareDynamic(pShaderAPI, iFogIndex, 0, bFlashlightShadows,
			(pShaderAPI->GetPixelFogCombo()), bWriteDepthToAlpha, bWriteWaterFogToAlpha, bHasFlashlight);
	} // End of Snapshot/Dynamic state

	if (mat_printvalues.GetBool())
	{
		Warning("%d %d %d", bHasNormalTexture, bHasNormalTexture2, bHasAnyNormalTexture);
		mat_printvalues.SetValue(0);
	}

	// ShiroDkxtro2:	I had it happen a bunch of times...
	//		pShader->Draw(); MUST BE above the final }
	//		It is very easy to mess this up ( done so several times )
	//		Game will just crash and debug leads you to a bogus function.
	pShader->Draw();
}

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_WorldVertexTransition, LUX_WorldVertexTransition)
DEFINE_FALLBACK_SHADER(SDK_WorldVertexTransition_DX9, LUX_WorldVertexTransition)
DEFINE_FALLBACK_SHADER(SDK_WorldVertexTransition_DX8, LUX_WorldVertexTransition)
DEFINE_FALLBACK_SHADER(SDK_WorldVertexTransition_DX6, LUX_WorldVertexTransition)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_WorldVertexTransition, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
Declare_MiscParameters()
Declare_DisplacementParameters()
Declare_NormalTextureParameters()
Declare_SeamlessParameters()
Declare_SelfIlluminationParameters()
SHADER_PARAM(MATERIALNAME, SHADER_PARAM_TYPE_STRING, "", "") // Can't use pMaterialName on Draw so we put it on this parameter instead...
SHADER_PARAM(DISTANCEALPHA, SHADER_PARAM_TYPE_BOOL, "", "")
SHADER_PARAM(SEAMLESS_SECONDARY, SHADER_PARAM_TYPE_BOOL, "", "")
SHADER_PARAM(SEAMLESS_BLENDMASK, SHADER_PARAM_TYPE_BOOL, "0", "")
SHADER_PARAM(SEAMLESS_BLENDMASKSCALE, SHADER_PARAM_TYPE_FLOAT, "0.0", "")
SHADER_PARAM(SELFILLUMMASK2, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(SELFILLUMMASKFRAME2, SHADER_PARAM_TYPE_INTEGER, "", "frame number for $")
SHADER_PARAM(SELFILLUMTINT2, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "")
SHADER_PARAM(BLENDMODULATETRANSPARENCY, SHADER_PARAM_TYPE_BOOL, "0", "Use Blue and Alpha Channel of the Blendmodulate Texture for Transparency Output.")

#ifdef DETAILTEXTURING
Declare_DetailTextureParameters()
Declare_Detail2TextureParameters()
#endif
#ifdef CUBEMAPS
Declare_EnvironmentMapParameters()
Declare_EnvMapMaskParameters()
SHADER_PARAM(ENVMAPMASK2, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(ENVMAPMASKFRAME2, SHADER_PARAM_TYPE_INTEGER, "", "frame number for $")
#ifdef PARALLAXCORRECTEDCUBEMAPS
Declare_ParallaxCorrectionParameters()
#endif
#endif

#ifdef SELFILLUMTEXTURING
Declare_SelfIllumTextureParameters()
SHADER_PARAM(SELFILLUMTEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "")
SHADER_PARAM(SELFILLUMTEXTUREFRAME2, SHADER_PARAM_TYPE_INTEGER, "", "frame number for $")
#endif

END_SHADER_PARAMS

LuxWorldVertexTransition_Params()

SHADER_INIT_PARAMS()
{
	Displacement_Vars_t vars;
	LuxWorldVertexTransition_Link_Params(vars);
	LuxWorldVertexTransition_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	if (mat_oldshaders.GetBool())
	{
		return "WorldVertexTransition";
	}

	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
	Displacement_Vars_t vars;
	LuxWorldVertexTransition_Link_Params(vars);
	LuxWorldVertexTransition_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Displacement_Vars_t vars;
	LuxWorldVertexTransition_Link_Params(vars);
	LuxWorldVertexTransition_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER