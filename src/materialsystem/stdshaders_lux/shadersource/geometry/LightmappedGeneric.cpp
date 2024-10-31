//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	LUX_Lightmappedgeneric Shader for brushes.
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
#include "lux_brush_vs30.inc"
#include "lux_lightmappedgeneric_simple_ps30.inc"
#include "lux_lightmappedgeneric_bump_ps30.inc"
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
void StaticShader_LuxLMG_Simple(IShaderShadow *pShaderShadow,
	bool bHasDetailTexture, bool bHasEnvMapMask, bool bHasFlashlight, int iSelfIllumMode,
	int iEnvMapMode, bool bPCC, bool bBlendTintByBaseAlpha, bool bEnvMapFresnel,
	bool bSeamlessBase, int iDetailUV, int iBumpUV, int iEnvMapMaskUV, bool bHasVertexColor,
	bool bHasSSBump, bool bHasParallaxMap, bool bEnvMapAnisotropy)
{
	DECLARE_STATIC_VERTEX_SHADER(lux_brush_vs30);
	SET_STATIC_VERTEX_SHADER_COMBO(VERTEX_RGBA, bHasVertexColor);
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, bSeamlessBase);
	SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iDetailUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0);			// 0 Regardless of iBumpUV
	SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, iEnvMapMaskUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER(lux_brush_vs30);

	if (bHasFlashlight)
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
		SET_STATIC_PIXEL_SHADER_COMBO(WORLDVERTEXTRANSITION, 0);
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
		DECLARE_STATIC_PIXEL_SHADER(lux_lightmappedgeneric_simple_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUMMODE, iSelfIllumMode);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, iEnvMapMode);
		SET_STATIC_PIXEL_SHADER_COMBO(PCC, bPCC);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPFRESNEL, bEnvMapFresnel);
		SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(PARALLAX, bHasParallaxMap);
		SET_STATIC_PIXEL_SHADER(lux_lightmappedgeneric_simple_ps30);
	}
}

void StaticShader_LuxLMG_Bump(IShaderShadow *pShaderShadow,
	bool bHasDetailTexture, bool bHasEnvMapMask, bool bHasFlashlight, int iSelfIllumMode,
	int iEnvMapMode, bool bPCC, bool bBlendTintByBaseAlpha, bool bEnvMapFresnel,
	bool bSeamlessBase, int iDetailUV, int iBumpUV, int iEnvMapMaskUV, bool bHasVertexColor,
	bool bHasSSBump, bool bHasParallaxMap, bool bEnvMapAnisotropy)
{
	DECLARE_STATIC_VERTEX_SHADER(lux_brush_vs30);
	SET_STATIC_VERTEX_SHADER_COMBO(VERTEX_RGBA, bHasVertexColor);
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, bSeamlessBase);
	SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iDetailUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, iBumpUV);		// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, iEnvMapMaskUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER(lux_brush_vs30);

	if (bHasFlashlight)
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
		SET_STATIC_PIXEL_SHADER_COMBO(WORLDVERTEXTRANSITION, 0);
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
		DECLARE_STATIC_PIXEL_SHADER(lux_lightmappedgeneric_bump_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUMMODE, iSelfIllumMode);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, iEnvMapMode);
		SET_STATIC_PIXEL_SHADER_COMBO(PCC, bPCC);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPFRESNEL, bEnvMapFresnel);
		SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(SSBUMP, bHasSSBump); // 
		SET_STATIC_PIXEL_SHADER_COMBO(PARALLAX, bHasParallaxMap);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPANISOTROPY, bEnvMapAnisotropy);
		SET_STATIC_PIXEL_SHADER(lux_lightmappedgeneric_bump_ps30);
	}
}

//==================================================================================================
// Dynamic Shader Declarations ( this is done to avoid a bazillion if statements for dynamic state.)
//
//==================================================================================================
void DynamicShader_LuxLMG_Simple(IShaderDynamicAPI *pShaderAPI, int ifogIndex, int iLightingPreview, bool bFlashlightShadows,
	int iPixelFogCombo, bool bWriteDepthToAlpha, bool bWriteWaterFogToAlpha, bool bHasFlashlight)
{
	DECLARE_DYNAMIC_VERTEX_SHADER(lux_brush_vs30);
//	SET_DYNAMIC_VERTEX_SHADER_COMBO(DOWATERFOG, ifogIndex);
	SET_DYNAMIC_VERTEX_SHADER(lux_brush_vs30);

	if (bHasFlashlight)
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_lightmappedgeneric_simple_ps30);
		//	SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, iPixelFogCombo);
		//	SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
		SET_DYNAMIC_PIXEL_SHADER(lux_lightmappedgeneric_simple_ps30);
	}
}

void DynamicShader_LuxLMG_Bump(IShaderDynamicAPI *pShaderAPI, int ifogIndex, int iLightingPreview, bool bFlashlightShadows,
	int iPixelFogCombo, bool bWriteDepthToAlpha, bool bWriteWaterFogToAlpha, bool bHasFlashlight)
{
	DECLARE_DYNAMIC_VERTEX_SHADER(lux_brush_vs30);
//	SET_DYNAMIC_VERTEX_SHADER_COMBO(DOWATERFOG, ifogIndex);
	SET_DYNAMIC_VERTEX_SHADER(lux_brush_vs30);

	if (bHasFlashlight)
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_lightmappedgeneric_bump_ps30);
		//	SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, iPixelFogCombo);
		//	SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
		SET_DYNAMIC_PIXEL_SHADER(lux_lightmappedgeneric_bump_ps30);
	}
}

//==================================================================================================
// Putting these into arrays. Yes you heard right, function array!
// Could do it with int's but that will execute the code from the array and pointers dumb
//==================================================================================================
// pShaderShadow, bHasLightWarpTexture, bHasDetailTexture, bHasEnvMapMask, bHasFlashlight, iSelfIllumMode, iEnvMapMode, bPCC, bHasSSBump, bHasParallaxMap, bEnvMapAnisotropy
std::function<void(IShaderShadow*, bool, bool, bool, int, int, bool, bool, bool, bool, int, int, int, bool, bool, bool, bool)> DeclareStatics[]
{
	StaticShader_LuxLMG_Simple, StaticShader_LuxLMG_Bump
};

// fogIndex, (numBones > 0), ( pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING) != 0 ),  (int)vertexCompression, lightState.m_nNumLights, bCSM,
// bWriteWaterFogToAlpha, bWriteDepthToAlpha, ( pShaderAPI->GetPixelFogCombo() ), bFlashlightShadows, bLightMappedModel, bUberlight
std::function<void(IShaderDynamicAPI*, int, int, bool, int, bool, bool, bool)> DeclareDynamics[]
{
	DynamicShader_LuxLMG_Simple, DynamicShader_LuxLMG_Bump
};

// COLOR is for brushes. COLOR2 for models
#define LuxLightMappedGeneric_Params() void LuxLightMappedGeneric_Link_Params(Brush_Vars_t &info)\
{									   \
	info.m_nColor = COLOR;				\
	info.m_nMultipurpose7 = MATERIALNAME;\
	info.m_nMultipurpose6 = DEBUG;		  \
	Link_DetailTextureParameters()		   \
	Link_EnvironmentMapParameters()		    \
	Link_EnvMapFresnelReflection()			 \
	Link_EnvMapMaskParameters()				  \
	Link_GlobalParameters()					   \
	Link_MiscParameters()					    \
	Link_NormalTextureParameters()				 \
	Link_ParallaxCorrectionParameters()			  \
	Link_SelfIlluminationParameters()			   \
	Link_SelfIllumTextureParameters()			    \
	Link_SeamlessParameters()						 \
	Link_ParallaxTextureParameters()				  \
}//----------------------------------------------------\

//==================================================================================================
void LuxLightMappedGeneric_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Brush_Vars_t &info)
{
	// I wanted to debug print some values and needed to know which material they belong to. However pMaterialName is not available to the draw!
	// Well, now it is...
	params[info.m_nMultipurpose7]->SetStringValue(pMaterialName);

	// The usual...
	Flashlight_BorderColorSupportCheck();

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

#ifdef PARALLAX_MAPPING
	// Not supported with Lightwarp because we need a sampler on like ALL the shaders that want to use this
	if (IsParamDefined(info.m_nLightWarpTexture) && IsParamDefined(info.m_nParallaxMap))
	{
		params[info.m_nParallaxMap]->SetUndefined();
	}

	FloatParameterDefault(info.m_nParallaxMapScale, 0.25f) // ParallaxMapScale
	FloatParameterDefault(info.m_nParallaxMapMaxOffset, 0.5f) // ParallaxMapMaxOffset
#endif

#ifdef CUBEMAPS
#ifdef CUBEMAPS_FRESNEL
	// These default values make no sense.
	// And the name is also terrible
	// Its Scale, Bias, Exponent... and not "min" ( which implies a minimum ), "max" ( which implies a maximum ), exp ( exponent )
	// You know what it actually is? Scale, Add, Fresnel Exponent... First value scales ( multiplies ), second value just... gets added ontop, and the last value is actually an exponent...
	// Yet Scale here is at 0 and it always adds 1.0
	// Is it supposed to be 1.0f, 0.0f, 2.0f ?
	Vec3ParameterDefault(info.m_nEnvMapFresnelMinMaxExp, 0.0f, 1.0f, 2.0f)
#else
	params[info.m_nEnvMapFresnel]->SetFloatValue(0.0f);
#endif
#endif

	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nDetailBlendFactor, 1.0f)

	// Default Value is supposed to be 4.0f
	FloatParameterDefault(info.m_nDetailScale, 4.0f)

	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nSelfIllumMaskScale, 1.0f);

	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS(MATERIAL_VAR_NO_DEBUG_OVERRIDE);
	}

	// No BaseTexture ? None of these.
	if (!params[info.m_nBaseTexture]->IsDefined())
	{
		CLEAR_FLAGS(MATERIAL_VAR_SELFILLUM);
		CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK);
	}

	SET_FLAGS2(MATERIAL_VAR2_LIGHTING_LIGHTMAP);
	if (g_pConfig->UseBumpmapping() && params[info.m_nBumpMap]->IsDefined())
	{
		SET_FLAGS2(MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP);
	}

#ifdef CUBEMAPS

	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nEnvMapSaturation, 1.0f)

	// If mat_specular 0, then get rid of envmap
	// TODO: What does this have to do with the Basetexture?
	// ---There is a section on Valve's shader where they bind a blacktexture instead of a white one if the material uses an envmap but not a basetexture.
	// Perhaps that is related? Who knows really, however for consistency we keep it like this.
	if (params[info.m_nEnvMap]->IsDefined())
	{
		if (!g_pConfig->UseSpecular() && params[info.m_nBaseTexture]->IsDefined())
		{
			params[info.m_nEnvMap]->SetUndefined();
#ifdef PARALLAXCORRECTEDCUBEMAPS
			params[info.m_nEnvMapParallax]->SetUndefined();
#endif
		}

		// ShiroDkxtro2 : I skipped PCC and EnvMapAnisotropy, 
		if (IsParamDefined(info.m_nEnvMapAnisotropy) && GetBoolParamValue(info.m_nEnvMapAnisotropy))
		{
			// Default Value is supposed to be 1.0f
			FloatParameterDefault(info.m_nEnvMapAnisotropyScale, 1.0f)

			if (GetBoolParamValue(info.m_nEnvMapParallax))
			{
				Warning("Material %s wants to use $EnvMapAnisotropy but is also Parallax Corrected, \n this combination was disabled as the results... didn't make much sense. \n Material will not be using $EnvMapAnisotropy. \n", params[info.m_nMultipurpose7]->GetStringValue());
			}

			// EnvMap Anisotropy requires NormalMaps
			if (!IsParamDefined(info.m_nBumpMap))
			{
				Warning("Material %s wants to use $EnvMapAnisotropy, \n but it does not have a $BumpMap to derive its Information from. \n Deactivating... \n", params[info.m_nMultipurpose7]->GetStringValue());
				SetIntParamValue(info.m_nEnvMapAnisotropy, 0);
			}
		}
	}
#endif

	//TODO: remove
#ifdef SIGNED_DISTANCE_FIELD_ALPHA
	FloatParameterDefault(info.m_nEdgeSoftnessStart, 0.5)
	FloatParameterDefault(info.m_nEdgeSoftnessEnd, 0.5)
	FloatParameterDefault(info.m_nOutlineAlpha, 1.0)
#endif

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
	if (params[info.m_nPhong]->GetIntValue() != 0 )
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

//==================================================================================================
void LuxLightMappedGeneric_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Brush_Vars_t &info)
{
	// Always needed...
	pShader->LoadTexture(info.m_nFlashlightTexture, TEXTUREFLAGS_SRGB);
	SET_FLAGS2(MATERIAL_VAR2_NEEDS_TANGENT_SPACES); // Supposedly you need this for the flashlight... Doubt X but ok
//	SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_FLASHLIGHT);	// Yes, Yes we support
//	SET_FLAGS2(MATERIAL_VAR2_USE_FLASHLIGHT);		// Yes, Yes we use it, what did you think? "Yeah we support it but don't use it, please"(?)
	SET_FLAGS2(MATERIAL_VAR2_LIGHTING_LIGHTMAP);	// We want lightmaps

	// We don't check for Transparency we did so earlier and do so later...
	LoadTextureWithCheck(info.m_nBaseTexture, TEXTUREFLAGS_SRGB)
	LoadNormalTextureLightmapFlag(info.m_nBumpMap)

	if (IsParamDefined(info.m_nBumpMap))
	{
		ITexture *pBumpMap = params[info.m_nBumpMap]->GetTextureValue();
		bool bIsSSBump = pBumpMap->GetFlags() & TEXTUREFLAGS_SSBUMP ? true : false;
		SetIntParamValue(info.m_nSSBump, bIsSSBump)
	}

	// I can't make sense of whatever they tried to do on the Stock Shaders
	// They load Detail as sRGB but then don't do sRGB Read... and vice versa.
	// We will just check if this  Texture has the flag and do sRGB read based on that
		LoadTextureWithCheck(info.m_nDetailTexture, 0)
#ifdef LIGHTWARP
		LoadTextureWithCheck(info.m_nLightWarpTexture, 0)
#endif
		LoadTextureWithCheck(info.m_nSelfIllumMask, 0)
#ifdef SELFILLUMTEXTURING
		LoadTextureWithCheck(info.m_nSelfIllumTexture, TEXTUREFLAGS_SRGB)
#endif

#ifdef PARALLAX_MAPPING
		LoadTextureWithCheck(info.m_nParallaxMap, 0)
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
		if (params[info.m_nEnvMapMask]->IsDefined())
		{
			pShader->LoadTexture(info.m_nEnvMapMask, 0);

			// We already have an envmapmask now, so discard the others!
			CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK);
			CLEAR_FLAGS(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK);
		}
		else
		{
			// NormalMapAlphaEnvMapMask takes priority, I decided thats sensible because its the go to one
			if (IS_FLAG_SET(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK))
			{
				if (params[info.m_nBumpMap]->GetTextureValue()->IsError())
					CLEAR_FLAGS(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK); // No normal map, no masking.

				CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK); // If we use normal map alpha, don't use basetexture alpha.
			}
			else
			{
				if (params[info.m_nBaseTexture]->GetTextureValue()->IsError())
					CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK); // If we have no Basetexture, can't use its alpha.
			}
		}

		// Tell the Shader to flip the EnvMap when set on AlphaEnvMapMask ( Consistency with Stock Shaders )
		if (IsParamDefined(info.m_nEnvMapMaskFlip) && !GetBoolParamValue(info.m_nEnvMapMaskFlip) && IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
		{
			SetIntParamValue(info.m_nEnvMapMaskFlip, 1);
		}
	}
#endif // CUBEMAPS
	else
	{ // No EnvMap == No Masking.
		CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK);
		CLEAR_FLAGS(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK);
	}

	// No Alphatesting/blendtint when we use the Alpha for other things
	// Valves Shaders ignore the fact you can use $envmapmask's alpha for selfillum...
	if	(
#ifdef SELFILLUMTEXTURING
		(IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) && !params[info.m_nSelfIllumTexture]->IsTexture()) ||
#endif
		(IS_FLAG_SET(MATERIAL_VAR_SELFILLUM) && !params[info.m_nSelfIllum_EnvMapMask_Alpha]->GetIntValue() == 1) || IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK)
		)
	{
		CLEAR_FLAGS(MATERIAL_VAR_ALPHATEST);
		SetIntParamValue(info.m_nBlendTintByBaseAlpha, 0)
	}
}

//==================================================================================================
void LuxLightMappedGeneric_Shader_Draw(
	CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow *pShaderShadow, Brush_Vars_t &info, VertexCompressionType_t vertexCompression,
	CBasePerMaterialContextData **pContextDataPtr)
{
	// This is used for the function array that declares and sets static/dynamic shaders
	// 0 is the default. lux_lightmappedgeneric_simple_ps30
	// 1 is the bumped shader. lux_lmg_bump_ps30
	// 2 will probably be the phonged shader? I haven't looked into it yet...
	int iShaderCombo = 0;
	// Flagstuff
	// NOTE: We already made sure we don't have conflicting flags on Shader Init ( see above )
	bool bHasFlashlight				=	pShader->UsingFlashlight(params);
	bool bSelfIllum					=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_SELFILLUM); // No SelfIllum under the flashlight
	bool bAlphatest					=						IS_FLAG_SET(MATERIAL_VAR_ALPHATEST);
	bool bNormalMapAlphaEnvMapMask	=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK); // No Envmapping under the flashlight
	bool bBaseAlphaEnvMapMask		=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK); // No Envmapping under the flashlight
	bool bBlendTintByBaseAlpha		=						GetBoolParamValue(info.m_nBlendTintByBaseAlpha);
	bool bHasSSBump					=						GetBoolParamValue(info.m_nSSBump);

	// Texture related Boolean. We check for existing bools first because its faster
	bool	bHasBaseTexture			=						IsTextureLoaded(info.m_nBaseTexture);
	bool	bHasNormalTexture		=						IsTextureLoaded(info.m_nBumpMap);
#ifdef LIGHTWARP
	bool	bHasLightWarpTexture	=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nLightWarpTexture); // No Lightwarp under the flashlight
#else
	bool	bHasLightWarpTexture	=	false;
#endif
	bool	bHasDetailTexture		=						IsTextureLoaded(info.m_nDetailTexture);
#ifdef CUBEMAPS
	bool	bHasEnvMap				=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nEnvMap); // No Envmapping under the flashlight
	bool	bHasEnvMapMask			=	bHasEnvMap		&&	IsTextureLoaded(info.m_nEnvMapMask); // No Envmapping under the flashlight
	bool	bHasEnvMapFresnel		=	bHasEnvMap		&&	GetBoolParamValue(info.m_nEnvMapFresnel);
	#ifdef PARALLAXCORRECTEDCUBEMAPS
	bool	bPCC					=	bHasEnvMap		&& GetIntParamValue(info.m_nEnvMapParallax) != 0;
	#else
	bool	bPCC = false;
	#endif
#else
	bool	bHasEnvMap				=	false;
	bool	bHasEnvMapMask			=	false;
	bool	bHasEnvMapFresnel		=	false;
	bool	bPCC					=	false;
#endif
	bool	bHasSelfIllumMask		=	bSelfIllum		&&	IsTextureLoaded(info.m_nSelfIllumMask);
#ifdef SELFILLUMTEXTURING
	bool	bHasSelfIllumTexture	=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nSelfIllumTexture);
#endif

#ifdef PARALLAX_MAPPING
	bool	bHasParallaxMap			=	!bHasLightWarpTexture && IsTextureLoaded(info.m_nParallaxMap);
#else
	bool	bHasParallaxMap			=	false;
#endif

	//	We check these later when we are sure we even have an EnvMapMask, Detail/NormalTexture
	//	NOTE: We don't check for BaseTextureTransform. Its the default fallback and if not changed, won't change anything.
	bool bDetailTextureTransform	= false;
	bool bEnvMapMaskTransform		= false;
	bool bNormalTextureTransform	= false;
	// Seamless Data
	bool bHasSeamlessBase			= GetIntParamValue(info.m_nSeamless_Base) != 0;
	bool bHasSeamlessDetail			= GetIntParamValue(info.m_nSeamless_Detail) != 0;
	bool bHasSeamlessBump			= GetIntParamValue(info.m_nSeamless_Bump) != 0;
	bool bHasSeamlessEnvMapMask		= GetIntParamValue(info.m_nSeamless_EnvMapMask) != 0;
	// we need to tell the VertexShader if we want any Seamless Data at all
	bool bUsesSeamlessData			= bHasSeamlessBase || bHasSeamlessDetail || bHasSeamlessBump || bHasSeamlessEnvMapMask;
	int	 iModeDetail				= 0;
	int	 iModeBump					= 0;
	int	 iModeEnvMapMask			= 0;
	// Cannot use VertexColors in conjunction with SeamlessData due to Vertex Shader Output limits.
	// Would need to sacrifice something... or ugly packing
	bool bHasVertexColor = bUsesSeamlessData ? false : IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);

	// When we don't have these, we don't compute them.
	if (bHasDetailTexture)
	{
		bDetailTextureTransform = params[info.m_nDetailTextureTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nDetailTextureTransform);
		iModeDetail = CheckSeamless(bHasSeamlessDetail);
	}

#ifdef CUBEMAPS
	if (bHasEnvMapMask)
	{
		bEnvMapMaskTransform = params[info.m_nEnvMapMaskTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nEnvMapMaskTransform);
		iModeEnvMapMask = CheckSeamless(bHasSeamlessEnvMapMask);
	}
#endif

	if (bHasNormalTexture)
	{
		iShaderCombo = 1;
		bNormalTextureTransform = params[info.m_nBumpTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nBumpTransform); // NOTE: Name is BumpTransform and not NormalTextureTransform
		iModeBump = CheckSeamless(bHasSeamlessBump);
	}

	// Purpose : Int to tell the Shader what Mask to use.
	//	0 = No Cubemap
	//	1 = $EnvMap no Masking
	//	2 = $EnvMapMask
	//	3 = $BaseAlphaEnvMapMask
	//	4 = $NormalMapAlphaEnvMapMask
	//	Order is important, we set the Combo to 3 for the non-bumped Shader and avoid writing a SKIP:
	//	bHasEnvMap will cause 0 if false, 1 if true. We then override based on the rest.
	int iEnvMapMode = ComputeEnvMapMode(bHasEnvMap, bHasEnvMapMask, bBaseAlphaEnvMapMask, bNormalMapAlphaEnvMapMask);

	int iSelfIllumMode = bSelfIllum; // 0 if no selfillum, 1 if selfillum is on. 2 is $SelfIllum_EnvMapMask_Alpha. 3 is $SelfIllumTexture
	int iDetailBlendMode = GetIntParamValue(info.m_nDetailBlendMode);

	// All of these conditions MUST be true.
	if (bHasEnvMapMask && bSelfIllum && (GetIntParamValue(info.m_nSelfIllum_EnvMapMask_Alpha) == 1))
		iSelfIllumMode = 2;

#ifdef SELFILLUMTEXTURING
	if (bHasSelfIllumTexture) iSelfIllumMode = 3;
#endif

	bool IsOpaque(bIsFullyOpaque, info.m_nBaseTexture, bAlphatest);
	bIsFullyOpaque = bIsFullyOpaque && !bHasFlashlight;

	//===========================================================================//
	// Snapshotting State
	// This gets setup basically once, or again when using the flashlight
	// Once this is done we use dynamic state.
	//===========================================================================//
	if (pShader->IsSnapshotting())
	{	
		if (bHasFlashlight)
		{
			pShader->SetInitialShadowState();
			pShaderShadow->EnableDepthWrites(false);
		}

		pShaderShadow->EnableAlphaTest(bAlphatest);
		if (params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f) // 0 is default.
		{
			pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue());
		}

		pShaderShadow->EnableAlphaWrites(bIsFullyOpaque);

		EnableSampler(SAMPLER_BASETEXTURE, true) // We always have a basetexture, and yes they should always be sRGB
		EnableSamplerWithCheck((bHasNormalTexture), SAMPLER_NORMALTEXTURE, false)	// No sRGB
		

		if (bSelfIllum)
		{
#ifdef SELFILLUMTEXTURING
			EnableSamplerWithCheck(bHasSelfIllumTexture, SAMPLER_SELFILLUM, true) else
#endif
			EnableSamplerWithCheck(bHasSelfIllumMask, SAMPLER_SELFILLUM, false) // We don't have to bind a standard texture. Sampler will return 0 0 0 0. Too bad!
		}

#ifdef PARALLAX_MAPPING
		EnableSamplerWithCheck(bHasParallaxMap, SAMPLER_PARALLAXMAP, false)
#endif


		// ShiroDkxtro2: Stock Shaders do some cursed Ternary stuff with the detailblendmode
		// Which... If I interpret it correctly makes it read non-sRGB maps as sRGB and vice versa...
		if (bHasDetailTexture)
		{
			ITexture *pDetailTexture = params[info.m_nDetailTexture]->GetTextureValue();
			EnableSampler(SAMPLER_DETAILTEXTURE, (pDetailTexture->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
			
			// While we are at it...
			// Stock Shader consistency : Valve's shader forces this, so we do too to not break any existing materials.
			// p2's panel texture uses blendmode 10 with an ssbump, however stock shaders didn't allow for this.
			// it would only work on those when it was a non-self-shadowed bumpmap.
			if (pDetailTexture->GetFlags() & TEXTUREFLAGS_SSBUMP && bHasNormalTexture)
				iDetailBlendMode = 10;
			else
				iDetailBlendMode = 11;
		}

		// ShiroDkxtro2: Stock Shaders will enable sRGB based on HDR_TYPE_NONE
		// Yes. sRGB cubemaps would be read incorrectly on HDR.
		bool	bHasEnvMapAnisotropy = false;
#ifdef CUBEMAPS
		if (bHasEnvMap)
		{
			ITexture *pEnvMap = params[info.m_nEnvMap]->GetTextureValue();
			EnableSampler(SAMPLER_ENVMAPTEXTURE, (pEnvMap->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
			EnableSamplerWithCheck(bHasEnvMapMask, SAMPLER_ENVMAPMASK, false) // Stock Consistency, no sRGB
			bHasEnvMapAnisotropy = bHasNormalTexture && GetBoolParamValue(info.m_nEnvMapAnisotropy);
		}
#endif

		EnableFlashlightSamplers(bHasFlashlight, SAMPLER_SHADOWDEPTH, SAMPLER_RANDOMROTATION, SAMPLER_FLASHLIGHTCOOKIE, info.m_nBaseTexture)

		// H++ only runs LDR. We check for Brushes too in case we ever get model lightmapping preview...
		EnableSampler(SAMPLER_LIGHTMAP, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? true : false)
		
		// Valve's shader does not check for this, which is odd. Aka no VertexAlpha if not also using VertexColor? Is that a "bug"? Should we enable that behaviour?
		// IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA)

		unsigned int flags = VERTEX_POSITION;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
		{
			// Enables Vertex Color
			flags |= VERTEX_COLOR;
		}

		if (bHasEnvMap || bHasNormalTexture)
		{
			flags |= VERTEX_TANGENT_S | VERTEX_TANGENT_T | VERTEX_NORMAL;
		}

		pShaderShadow->VertexShaderVertexFormat(flags, bHasNormalTexture ? 3 : 2, 0, 0);

 		pShaderShadow->EnableSRGBWrite(true); // We always do sRGB Writes. We use ps30.
		auto DeclareStaticShaders = DeclareStatics[iShaderCombo];
		DeclareStaticShaders(pShaderShadow, bHasDetailTexture, bHasEnvMapMask, bHasFlashlight,
							iSelfIllumMode, iEnvMapMode, bPCC, bBlendTintByBaseAlpha, bHasEnvMapFresnel,
							bHasSeamlessBase, iModeDetail, iModeBump, iModeEnvMapMask, bHasVertexColor, bHasSSBump, bHasParallaxMap, bHasEnvMapAnisotropy);
	}
	else // End of Snapshotting ------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{
		//===========================================================================//
		// Bind Textures
		//===========================================================================//
		BindTextureWithCheckAndFallback(bHasBaseTexture, SAMPLER_BASETEXTURE, info.m_nBaseTexture, info.m_nBaseTextureFrame, TEXTURE_WHITE)

		// if mat_fullbright 2. Bind a standard white texture...
#ifdef DEBUG_FULLBRIGHT2 
		if (mat_fullbright.GetInt() == 2 && !IS_FLAG_SET(MATERIAL_VAR_NO_DEBUG_OVERRIDE))
			BindTextureStandard(SAMPLER_BASETEXTURE, TEXTURE_GREY)
#endif

		BindTextureWithCheck(bHasDetailTexture, SAMPLER_DETAILTEXTURE, info.m_nDetailTexture, info.m_nDetailFrame)

		BindTextureWithCheck(bHasSelfIllumMask, SAMPLER_SELFILLUM, info.m_nSelfIllumMask, info.m_nSelfIllumMaskFrame)
#ifdef SELFILLUMTEXTURING
		// SelfIllumTexture has priority over $selfillum via base alpha and SelfillumMask
		BindTextureWithCheck(bHasSelfIllumTexture, SAMPLER_SELFILLUM, info.m_nSelfIllumTexture, info.m_nSelfIllumTextureFrame)
#endif
#ifdef LIGHTWARP
		BindTextureWithCheck(bHasLightWarpTexture, SAMPLER_LIGHTWARP, info.m_nLightWarpTexture, 0) // $lightwarptextureframe? :/? Should that exist...?
#endif
#ifdef PARALLAX_MAPPING
		BindTextureWithCheck(bHasParallaxMap, SAMPLER_PARALLAXMAP, info.m_nParallaxMap, 0)
#endif

#ifdef CUBEMAPS
		if (bHasEnvMap)
		{
			BindTextureWithCheckAndFallback(mat_specular.GetBool(), SAMPLER_ENVMAPTEXTURE, info.m_nEnvMap, info.m_nEnvMapFrame, TEXTURE_BLACK)


			BindTextureWithCheck(bHasEnvMapMask, SAMPLER_ENVMAPMASK, info.m_nEnvMapMask, info.m_nEnvMapMaskFrame)
		}
#endif

		if (bHasNormalTexture)
		{
			BindTextureWithoutCheck(SAMPLER_NORMALTEXTURE, info.m_nBumpMap, info.m_nBumpFrame)
			BindTextureStandard(SAMPLER_LIGHTMAP, TEXTURE_LIGHTMAP_BUMPED)
		}
		else
		{
			BindTextureStandard(SAMPLER_LIGHTMAP, TEXTURE_LIGHTMAP)
		}

#ifdef DEBUG_LUXELS
		// Debug Luxel Texture
		if (mat_luxels.GetBool())
		{
			pShaderAPI->BindStandardTexture(SAMPLER_LIGHTMAP, TEXTURE_DEBUG_LUXELS);
		}
#endif

		//===========================================================================//
		// Prepare floats for Constant Registers
		//===========================================================================//

		// f4Empty is just float4 Name = {1,1,1,1};
		// Yes I have some excessive problems with macro overusage...
		f4Empty(f4BaseTextureTint_Factor)
		f4Empty(f4DetailTint_BlendFactor)
		f4Empty(f4SelfIllumTint_Scale)
		f4Empty(f4SelfIllumFresnelMinMaxExp)
		f4Empty(f4EnvMapTint_LightScale)
		f4Empty(f4EnvMapFresnelRanges_)
		f4Empty(f4EnvMapSaturation_Contrast)
		f4Empty(f4DetailBlendMode) // yzw empty
		f4Empty(f4EnvMapControls)
		
		BOOL BBools[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };

		// $color/$color2 and $BlendTintColorOverBase
		GetVec3ParamValue(info.m_nColor, f4BaseTextureTint_Factor);
		f4BaseTextureTint_Factor[3] = GetFloatParamValue(info.m_nBlendTintColorOverBase); // Consider writing a macro for this

#ifdef DETAILTEXTURING
		if (bHasDetailTexture)
		{
			// $DetailTint and $DetailBlendFactor
			GetVec3ParamValue(info.m_nDetailTint, f4DetailTint_BlendFactor);
			f4DetailTint_BlendFactor[3] = GetFloatParamValue(info.m_nDetailBlendFactor); // Consider writing a macro for this
			f4DetailBlendMode[0] = iDetailBlendMode; // We trunc it later and do some dynamic branching. Saves a ton of combos but makes rendering slower. but oh well...
		}
#endif

		if (bSelfIllum)
		{
			GetVec3ParamValue(info.m_nSelfIllumFresnelMinMaxExp, f4SelfIllumFresnelMinMaxExp); // .w empty
			GetVec3ParamValue(info.m_nSelfIllumTint, f4SelfIllumTint_Scale);
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
			f4EnvMapControls[3] = GetFloatParamValue(info.m_nEnvMapAnisotropyScale);

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
		SetupStockConstantRegisters(false, SAMPLER_FLASHLIGHTCOOKIE, SAMPLER_RANDOMROTATION, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame, bFlashlightShadows)

			if (bHasFlashlight)
			{
				VMatrix worldToTexture;
				ITexture *pFlashlightDepthTexture;
				FlashlightState_t flashlightState = pShaderAPI->GetFlashlightStateEx(worldToTexture, &pFlashlightDepthTexture);

				SetFlashLightColorFromState(flashlightState, pShaderAPI);

				pShader->BindTexture(SAMPLER_FLASHLIGHTCOOKIE, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame);

				if (pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && flashlightState.m_bEnableShadows)
				{
					pShader->BindTexture(SAMPLER_SHADOWDEPTH, pFlashlightDepthTexture, 0);
					pShaderAPI->BindStandardTexture(SAMPLER_RANDOMROTATION, TEXTURE_SHADOW_NOISE_2D);

					float tweaks[4];
					tweaks[0] = ShadowFilterFromState(flashlightState);
					tweaks[1] = ShadowAttenFromState(flashlightState);
					pShader->HashShadow2DJitter(flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3]);
					pShaderAPI->SetPixelShaderConstant(PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1);

					// Dimensions of screen, used for screen-space noise map sampling
					float vScreenScale[4] = { 1280.0f / 32.0f, 720.0f / 32.0f, 0, 0 };
					int nWidth, nHeight;
					pShaderAPI->GetBackBufferDimensions(nWidth, nHeight);
					vScreenScale[0] = (float)nWidth / 32.0f;
					vScreenScale[1] = (float)nHeight / 32.0f;
					pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_SCREEN_SCALE, vScreenScale, 1);
				}

				bFlashlightShadows = flashlightState.m_bEnableShadows && (pFlashlightDepthTexture != NULL);

				float atten[4];										// Set the flashlight attenuation factors
				atten[0] = flashlightState.m_fConstantAtten;
				atten[1] = flashlightState.m_fLinearAtten;
				atten[2] = flashlightState.m_fQuadraticAtten;
				atten[3] = flashlightState.m_FarZ;
				pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_ATTENUATION, atten, 1);
			}

		int iFogIndex = 0;
		bool bWriteDepthToAlpha = false;
		bool bWriteWaterFogToAlpha = false;
		SetupDynamicComboVariables(iFogIndex, bWriteDepthToAlpha, bWriteWaterFogToAlpha, bIsFullyOpaque)

		//==================================================================================================
		// Set Pixelshader Constant Registers (PSREG's)
		//==================================================================================================
		pShaderAPI->SetPixelShaderConstant(32, f4BaseTextureTint_Factor, 1);
		pShaderAPI->SetPixelShaderConstant(33, f4DetailTint_BlendFactor, 1);
		pShaderAPI->SetPixelShaderConstant(34, f4SelfIllumTint_Scale, 1);
		pShaderAPI->SetPixelShaderConstant(35, f4EnvMapTint_LightScale, 1);
		pShaderAPI->SetPixelShaderConstant(36, f4EnvMapFresnelRanges_, 1);
		pShaderAPI->SetPixelShaderConstant(37, f4DetailBlendMode, 1);
//										   38, PCC EnvMap Origin
//										   39, PCC Bounding	Box
//										   40, PCC Bounding	Box
//										   41, PCC Bounding	Box
//		pShaderAPI->SetPixelShaderConstant(42, f4PhongTint_Boost, 1);
//		pShaderAPI->SetPixelShaderConstant(43, f4PhongFresnelRanges_Exponent, 1);
//		pShaderAPI->SetPixelShaderConstant(44, f4RimLightControls, 1);
		pShaderAPI->SetPixelShaderConstant(45, f4SelfIllumFresnelMinMaxExp, 1);
		pShaderAPI->SetPixelShaderConstant(46, f4EnvMapSaturation_Contrast, 1);

		pShaderAPI->SetPixelShaderConstant(51, f4EnvMapControls, 1);


#ifdef PARALLAX_MAPPING
		f4Empty(f4ParallaxControls)
		f4ParallaxControls[0] = GetFloatParamValue(info.m_nParallaxMapScale);
		f4ParallaxControls[1] = GetFloatParamValue(info.m_nParallaxMapMaxOffset);
		pShaderAPI->SetPixelShaderConstant(50, f4ParallaxControls, 1);
#endif
		//==================================================================================================
		// Set Vertexshader Constant Registers (VSREG's...?)
		//==================================================================================================
		// Always having this
		pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_13, info.m_nBaseTextureTransform);

		if (!bNormalTextureTransform)
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_15, info.m_nBumpTransform);
		else
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_15, info.m_nBaseTextureTransform);

		if (!bDetailTextureTransform)
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nDetailTextureTransform, info.m_nDetailScale);
		else
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nBaseTextureTransform, info.m_nDetailScale);

#ifdef CUBEMAPS
		if (!bEnvMapMaskTransform)
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_19, info.m_nEnvMapMaskTransform);
		else
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_19, info.m_nBaseTextureTransform);
#endif

		f4Empty(SeamlessScales)
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
			// I tried shader specific ones. 0-3 and those didn't work. So we are going to use our new registers instead.
			pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_30, SeamlessScales);
		}

		BBools[13] = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
		BBools[14] = IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA);
		BBools[15] = bWriteDepthToAlpha;

		// Always! Required for Lightwarp
		BBools[1] = bHasLightWarpTexture || (bHasFlashlight && bFlashlightShadows);
		pShaderAPI->SetBooleanPixelShaderConstant(0, BBools, 16, true);

		//==================================================================================================
		// Declare Dynamic Shaders ( Using FunctionArray Lookup Table )
		//==================================================================================================

			auto DeclareDynamic = DeclareDynamics[iShaderCombo];
			DeclareDynamic(pShaderAPI, iFogIndex, 0, bFlashlightShadows,
				(pShaderAPI->GetPixelFogCombo()), bWriteDepthToAlpha, bWriteWaterFogToAlpha, bHasFlashlight);

			// Used for debugging materials
			// Doesn't have to be pretty, and it won't be pretty to debug anyways.
			if (mat_printvalues.GetBool())
			{
				Warning("--- LightmappedGeneric Material : %s ---", params[info.m_nMultipurpose7]->GetStringValue());
				Warning("Static Vertex Shader Combo \n");
				Warning("%i %i %i %i %i \n", bHasVertexColor, bHasSeamlessBase, bHasDetailTexture, bHasNormalTexture, bHasEnvMapMask);
				Warning("Static Pixel Shader Combo \n");
				Warning("%s", bHasFlashlight ? "Flashlight Pass \n" : "Regular Pass \n");
				if (bHasFlashlight)
					Warning("%i %i %i %i %i %i %i \n", g_pHardwareConfig->GetShadowFilterMode(), bBlendTintByBaseAlpha, bHasDetailTexture, 0, bHasNormalTexture, 0, 0);
				else
					Warning("%i %i %i %i %i %i %i %i \n", bHasDetailTexture, iSelfIllumMode, iEnvMapMode, bPCC, bBlendTintByBaseAlpha, bHasEnvMapFresnel, 0, bHasParallaxMap);
				Warning("Dynamic Vertex Shader Combo \n");
				Warning("None. \n");
				Warning("Dynamic Pixel Shader Combo \n");
				if (bHasFlashlight)
					Warning("%i %i \n", bFlashlightShadows, pShaderAPI->GetPixelFogCombo());
				else
					Warning("%i, %i \n", pShaderAPI->GetPixelFogCombo(), bWriteWaterFogToAlpha);
				Warning("---------------------------------------\n");
			}
	} // End of Snapshot/Dynamic state

	if (mat_printvalues.GetBool())
	{
		mat_printvalues.SetValue(0);
	}

	// ShiroDkxtro2:	I had it happen a bunch of times...
	//		pShader->Draw(); MUST BE above the final }
	//		It is very easy to mess this up ( done so several times )
	//		Game will just crash and debug leads you to a bogus function.
	pShader->Draw();

	if ((IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) != 0) && bIsFullyOpaque)
	{
		//Alpha testing makes it so we can't write to dest alpha
		//Writing to depth makes it so later polygons can't write to dest alpha either
		//This leads to situations with garbage in dest alpha.

		//Fix it now by converting depth to dest alpha for any pixels that just wrote.
		pShader->DrawEqualDepthToDestAlpha();
	}

} // End of lux*_Shader_Draw

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_LightmappedGeneric,		LUX_LightMappedGeneric)
DEFINE_FALLBACK_SHADER(SDK_LightmappedGeneric_DX9,	LUX_LightMappedGeneric)
DEFINE_FALLBACK_SHADER(SDK_LightmappedGeneric_DX8,	LUX_LightMappedGeneric)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_LightMappedGeneric, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
	Declare_MiscParameters()
	Declare_NormalTextureParameters()
	Declare_SelfIlluminationParameters()
	Declare_SeamlessParameters()
	SHADER_PARAM(MATERIALNAME, SHADER_PARAM_TYPE_STRING, "", "") // Can't use pMaterialName on Draw so we put it on this parameter instead...
	SHADER_PARAM(DISTANCEALPHA, SHADER_PARAM_TYPE_BOOL, "", "")
	SHADER_PARAM(BASETEXTURE2, SHADER_PARAM_TYPE_TEXTURE, "", "") // Used for Fallback
	SHADER_PARAM(DEBUG, SHADER_PARAM_TYPE_INTEGER, "0", "")

#ifdef DETAILTEXTURING
	Declare_DetailTextureParameters()
#endif

#ifdef SELFILLUMTEXTURING
	Declare_SelfIllumTextureParameters()
#endif

#ifdef CUBEMAPS
	Declare_EnvironmentMapParameters()
	Declare_EnvMapMaskParameters()
#endif

#ifdef PARALLAXCORRECTEDCUBEMAPS
	Declare_ParallaxCorrectionParameters()
#endif

#ifdef PARALLAX_MAPPING
	Declare_ParallaxTextureParameters()
#endif

END_SHADER_PARAMS

LuxLightMappedGeneric_Params()
LuxFlashlight_Simple_Params()

SHADER_INIT_PARAMS()
{
	Brush_Vars_t vars;
	LuxLightMappedGeneric_Link_Params(vars);
	LuxLightMappedGeneric_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK	
{
	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	else
	{
		// This has to come before routing to WVT. Previously LMG included WVT
		if (mat_oldshaders.GetBool())
		{
			return "LightmappedGeneric";
		}

		// ShiroDkxtro2: 100% someone used WVT stuff on LMG before. I sure know, I have.
		// We don't support those parameters anymore, so send them to the dedicated Shader
		if (IsParamDefined(BASETEXTURE2))
		{
			return "LUX_WorldVertexTransition";
		}

		if (IS_FLAG_SET(MATERIAL_VAR_MODEL))
		{
			Warning("Material Using LUX_LightMappedGeneric with $Model flag!\n Rendering Missing Texture instead...\n");
			return "LUX_ErrorShader";
		}
	}
	return 0;
}

SHADER_INIT
{
	Brush_Vars_t vars;
	LuxLightMappedGeneric_Link_Params(vars);
	LuxLightMappedGeneric_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Brush_Vars_t vars;
	LuxLightMappedGeneric_Link_Params(vars);
	LuxLightMappedGeneric_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER


// Beginning of LightMappedReflective

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
/*
BEGIN_VS_SHADER(LUX_LightMappedReflective, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
Declare_NormalTextureParameters()
Declare_DetailTextureParameters()
Declare_SelfIlluminationParameters()
#ifdef SELFILLUMTEXTURING
Declare_SelfIllumTextureParameters()
#endif
Declare_EnvMapMaskParameters()
Declare_EnvironmentMapParameters()
Declare_ParallaxCorrectionParameters()
Declare_MiscParameters()
END_SHADER_PARAMS

LuxLightMappedGeneric_Params()

SHADER_INIT_PARAMS()
{
	Geometry_Vars_t vars;
	LuxLightMappedGeneric_Link_Params(vars);
	LuxLightMappedGeneric_Init_Params(this, params, pMaterialName, vars);
}

SHADER_FALLBACK
{
	if (g_pHardwareConfig->GetDXSupportLevel() < 90)
	{
		Warning("Game run at DXLevel < 90 \n");
		return "Wireframe";
	}
	return 0;
}

SHADER_INIT
{
	Geometry_Vars_t vars;
	LuxLightMappedGeneric_Link_Params(vars);
	LuxLightMappedGeneric_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Geometry_Vars_t vars;
	LuxLightMappedGeneric_Link_Params(vars);
	LuxLightMappedGeneric_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER
*/