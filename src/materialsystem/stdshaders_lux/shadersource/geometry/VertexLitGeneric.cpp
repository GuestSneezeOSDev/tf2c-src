//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	LUX_VertexLitGeneric Shader for Models.
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

// This allows for additional passes for other effects
#include "../renderpasses/Cloak.h"
#include "../renderpasses/EmissiveBlend.h"
#include "../renderpasses/FleshInterior.h"
#include "../renderpasses/DistanceAlpha.h"

// this is required for our Function Array's
#include <functional>

// Includes for Shaderfiles...
#include "lux_model_vs30.inc"
#include "lux_vertexlitgeneric_simple_ps30.inc"
#include "lux_vertexlitgeneric_bump_ps30.inc"
#include "lux_vertexlitgeneric_phong_ps30.inc"
#include "lux_flashlight_simple_ps30.inc"

extern ConVar mat_fullbright;
extern ConVar mat_printvalues;
extern ConVar mat_oldshaders;
extern ConVar mat_specular;
#ifdef LIGHTWARP
extern ConVar mat_disable_lightwarp;
#endif

#ifdef MAPBASE_FEATURED
extern ConVar mat_specular_disable_on_missing;
#endif

//==================================================================================================
// Static Shader Declarations. this is done to avoid a bazillion if statements for dynamic state.
// Also static state ( It doesn't quite matter there )
//==================================================================================================
void StaticShader_LuxVLG_Simple(IShaderShadow *pShaderShadow,
	bool bHasDetailTexture, bool bHasEnvMapMask, bool bHasFlashlight, int iSelfIllumMode,
	int iEnvMapMode, bool bBlendTintByBaseAlpha, bool bEnvMapFresnel, bool bLightMapUV,
	bool bIsDecal, bool bHalfLambert, int iTreeSway, bool bHasPhongExponentTexture,
	bool bSeamlessBase, int iDetailUV, int iBumpUV, int iEnvMapMaskUV, bool bEnvMapAnisotropy)
{
	DECLARE_STATIC_VERTEX_SHADER(lux_model_vs30);
	SET_STATIC_VERTEX_SHADER_COMBO(DECAL, bIsDecal);
	SET_STATIC_VERTEX_SHADER_COMBO(HALFLAMBERT, bHalfLambert);
#ifdef MODEL_LIGHTMAPPING
	SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, bLightMapUV);
#else
	SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, 0);
#endif
	SET_STATIC_VERTEX_SHADER_COMBO(TREESWAY, iTreeSway);
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, bSeamlessBase);
	SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iDetailUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0);			// 0 Regardless of iBumpUV
	SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, iEnvMapMaskUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER(lux_model_vs30);

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
	DECLARE_STATIC_PIXEL_SHADER(lux_vertexlitgeneric_simple_ps30);
	SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
	SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUMMODE, iSelfIllumMode);
#ifdef CUBEMAPS
	SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, iEnvMapMode);
#else
	SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, 0);
#endif
	SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
	SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPFRESNEL, bEnvMapFresnel);
	SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
	SET_STATIC_PIXEL_SHADER(lux_vertexlitgeneric_simple_ps30);
	}
}

void StaticShader_LuxVLG_Bump(IShaderShadow *pShaderShadow,
	bool bHasDetailTexture, bool bHasEnvMapMask, bool bHasFlashlight, int iSelfIllumMode,
	int iEnvMapMode, bool bBlendTintByBaseAlpha, bool bEnvMapFresnel, bool bLightMapUV,
	bool bIsDecal, bool bHalfLambert, int iTreeSway, bool bHasPhongExponentTexture,
	bool bSeamlessBase, int iDetailUV, int iBumpUV, int iEnvMapMaskUV, bool bEnvMapAnisotropy)
{
	DECLARE_STATIC_VERTEX_SHADER(lux_model_vs30);
	SET_STATIC_VERTEX_SHADER_COMBO(DECAL, bIsDecal);
	SET_STATIC_VERTEX_SHADER_COMBO(HALFLAMBERT, bHalfLambert);
#ifdef MODEL_LIGHTMAPPING
	SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, bLightMapUV);
#else
	SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, 0);
#endif
	SET_STATIC_VERTEX_SHADER_COMBO(TREESWAY, iTreeSway);
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, bSeamlessBase);
	SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iDetailUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, iBumpUV);		// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, iEnvMapMaskUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER(lux_model_vs30);

	if (bHasFlashlight)
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
		SET_STATIC_PIXEL_SHADER_COMBO(WORLDVERTEXTRANSITION, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE2, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAPPED, 1);
		SET_STATIC_PIXEL_SHADER_COMBO(SSBUMP, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONG, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONGEXPONENTTEXTURE, 0);
		SET_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_vertexlitgeneric_bump_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUMMODE, iSelfIllumMode);
#ifdef CUBEMAPS
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, iEnvMapMode);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPANISOTROPY, bEnvMapAnisotropy);
#else
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPANISOTROPY, 0);
#endif
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPFRESNEL, bEnvMapFresnel);
		SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER(lux_vertexlitgeneric_bump_ps30);
	}
}

void StaticShader_LuxVLG_Phong(IShaderShadow *pShaderShadow,
	bool bHasDetailTexture, bool bHasEnvMapMask, bool bHasFlashlight, int iSelfIllumMode,
	int iEnvMapMode, bool bBlendTintByBaseAlpha, bool bEnvMapFresnel, bool bLightMapUV,
	bool bIsDecal, bool bHalfLambert, int iTreeSway, bool bHasPhongExponentTexture,
	bool bSeamlessBase, int iDetailUV, int iBumpUV, int iEnvMapMaskUV, bool bEnvMapAnisotropy)
{
	DECLARE_STATIC_VERTEX_SHADER(lux_model_vs30);
	SET_STATIC_VERTEX_SHADER_COMBO(DECAL, bIsDecal);
	SET_STATIC_VERTEX_SHADER_COMBO(HALFLAMBERT, 0); // No static lighting.
	SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, 0);	// This Shader uses Phong, so no lightmaps.
	SET_STATIC_VERTEX_SHADER_COMBO(TREESWAY, iTreeSway);
	SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, bSeamlessBase);
	SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iDetailUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, iBumpUV);		// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, iEnvMapMaskUV);	// 1 = normal, 2 = seamless
	SET_STATIC_VERTEX_SHADER(lux_model_vs30);

	if (bHasFlashlight)
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
		SET_STATIC_PIXEL_SHADER_COMBO(WORLDVERTEXTRANSITION, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE2, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(BUMPMAPPED, 1);
		SET_STATIC_PIXEL_SHADER_COMBO(SSBUMP, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONG, 1);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONGEXPONENTTEXTURE, bHasPhongExponentTexture);
		SET_STATIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
		DECLARE_STATIC_PIXEL_SHADER(lux_vertexlitgeneric_phong_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(SELFILLUMMODE, iSelfIllumMode);
#ifdef CUBEMAPS
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, iEnvMapMode);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPANISOTROPY, bEnvMapAnisotropy);
#else
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPMODE, 0);
		SET_STATIC_PIXEL_SHADER_COMBO(ENVMAPANISOTROPY, 0);
#endif
		SET_STATIC_PIXEL_SHADER_COMBO(BLENDTINTBYBASEALPHA, bBlendTintByBaseAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(PHONGEXPONENTTEXTURE, bHasPhongExponentTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(DESATURATEBYBASEALPHA, 0);
		SET_STATIC_PIXEL_SHADER(lux_vertexlitgeneric_phong_ps30);
	}
}

//==================================================================================================
// Dynamic Shader Declarations ( this is done to avoid a bazillion if statements for dynamic state.)
//
//==================================================================================================
void DynamicShader_LuxVLG_Simple(IShaderDynamicAPI *pShaderAPI, int ifogIndex, int iLightingPreview, bool bFlashlightShadows,
	int iPixelFogCombo, bool bWriteDepthToAlpha, bool bWriteWaterFogToAlpha, bool bLightMappedModel, int iNumLights,
	int iBones, int iVertexCompression, bool bStaticPropLighting, bool bDynamicPropLighting, bool bAmbientLight, bool bHasFlashlight)
{
	DECLARE_DYNAMIC_VERTEX_SHADER(lux_model_vs30);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, g_pHardwareConfig->HasFastVertexTextures() && pShaderAPI->IsHWMorphingEnabled());
	SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, iBones);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(STATICPROPLIGHTING, bStaticPropLighting);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(DYNAMICPROPLIGHTING, bDynamicPropLighting);
	SET_DYNAMIC_VERTEX_SHADER(lux_model_vs30);

	if (bHasFlashlight)
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
	DECLARE_DYNAMIC_PIXEL_SHADER(lux_vertexlitgeneric_simple_ps30);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, iPixelFogCombo);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(LIGHTMAPPED_MODEL, bLightMappedModel); // ShiroDkxtro2 : The name of this DYNAMIC seems to have to be this way...
	SET_DYNAMIC_PIXEL_SHADER(lux_vertexlitgeneric_simple_ps30);
	}
}

void DynamicShader_LuxVLG_Bump(IShaderDynamicAPI *pShaderAPI, int ifogIndex, int iLightingPreview, bool bFlashlightShadows,
	int iPixelFogCombo, bool bWriteDepthToAlpha, bool bWriteWaterFogToAlpha, bool bLightMappedModel, int iNumLights,
	int iBones, int iVertexCompression, bool bStaticPropLighting, bool bDynamicPropLighting, bool bAmbientLight, bool bHasFlashlight)
{
	DECLARE_DYNAMIC_VERTEX_SHADER(lux_model_vs30);
//	SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSED_VERTS, iVertexCompression);
//	SET_DYNAMIC_VERTEX_SHADER_COMBO(DOWATERFOG, ifogIndex);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, g_pHardwareConfig->HasFastVertexTextures() && pShaderAPI->IsHWMorphingEnabled());
//	SET_DYNAMIC_VERTEX_SHADER_COMBO(NUM_LIGHTS, 0); // iNumLights... This isn't even necessary!
	SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, iBones);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(STATICPROPLIGHTING, 0); // Using Bumped Lighting instead.
	SET_DYNAMIC_VERTEX_SHADER_COMBO(DYNAMICPROPLIGHTING, 0); // Using Bumped Lighting instead.
	SET_DYNAMIC_VERTEX_SHADER(lux_model_vs30);

	if (bHasFlashlight)
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
	DECLARE_DYNAMIC_PIXEL_SHADER(lux_vertexlitgeneric_bump_ps30);
//	SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, iPixelFogCombo);
//	SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(LIGHTMAPPED_MODEL, bLightMappedModel); // ShiroDkxtro2 : The name of this DYNAMIC seems to have to be this way...
	SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_LIGHTS, iNumLights);
	SET_DYNAMIC_PIXEL_SHADER(lux_vertexlitgeneric_bump_ps30);
	}
}

void DynamicShader_LuxVLG_Phong(IShaderDynamicAPI *pShaderAPI, int ifogIndex, int iLightingPreview, bool bFlashlightShadows,
	int iPixelFogCombo, bool bWriteDepthToAlpha, bool bWriteWaterFogToAlpha, bool bLightMappedModel, int iNumLights,
	int iBones, int iVertexCompression, bool bStaticPropLighting, bool bDynamicPropLighting, bool bAmbientLight, bool bHasFlashlight)
{
	DECLARE_DYNAMIC_VERTEX_SHADER(lux_model_vs30);
//	SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSED_VERTS, iVertexCompression);
//	SET_DYNAMIC_VERTEX_SHADER_COMBO(DOWATERFOG, ifogIndex);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, g_pHardwareConfig->HasFastVertexTextures() && pShaderAPI->IsHWMorphingEnabled());
//	SET_DYNAMIC_VERTEX_SHADER_COMBO(NUM_LIGHTS, iNumLights); // iNumLights... This isn't even necessary!
	SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, iBones);
	SET_DYNAMIC_VERTEX_SHADER_COMBO(STATICPROPLIGHTING, 0); // Using Bumped Lighting instead.
	SET_DYNAMIC_VERTEX_SHADER_COMBO(DYNAMICPROPLIGHTING, 0); // Using Bumped Lighting instead.
	SET_DYNAMIC_VERTEX_SHADER(lux_model_vs30);

	if (bHasFlashlight)
	{
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}
	else
	{
	DECLARE_DYNAMIC_PIXEL_SHADER(lux_vertexlitgeneric_phong_ps30);
//	SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, iPixelFogCombo);
//	SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_LIGHTS, iNumLights);
	SET_DYNAMIC_PIXEL_SHADER_COMBO(AMBIENT_LIGHT, !bStaticPropLighting && bAmbientLight);
	SET_DYNAMIC_PIXEL_SHADER(lux_vertexlitgeneric_phong_ps30);
	}
}

//==================================================================================================
// Putting these into arrays. Yes you heard right, function array!
// Could do it with int's but that will execute the code from the array and pointers dumb
//==================================================================================================
// pShaderShadow, bHasDetailTexture, bHasEnvMapMask, bHasFlashlight, iSelfIllumMode,
// iEnvMapMode, bBlendTintByBaseAlpha, bEnvMapFresnel, bLightMapUV,
// bIsDecal, bHalfLambert, iTreeSway, bHasPhongExponentTexture,
// bSeamlessBase, iDetailUV, iBumpUV, iEnvMapMaskUV, bEnvMapAnisotropy)
std::function<void(IShaderShadow*, bool, bool, bool, int, int, bool, bool, bool, bool, bool, int, bool, bool, int, int, int, bool)> DeclareStatics[]
{
	StaticShader_LuxVLG_Simple, StaticShader_LuxVLG_Bump, StaticShader_LuxVLG_Phong
};

// fogIndex, (numBones > 0), ( pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING) != 0 ),  (int)vertexCompression, lightState.m_nNumLights, bCSM,
// bWriteWaterFogToAlpha, bWriteDepthToAlpha, ( pShaderAPI->GetPixelFogCombo() ), bFlashlightShadows, bLightMappedModel, bUberlight, bHasFlashlight)
std::function<void(IShaderDynamicAPI*, int, int, bool, int, bool, bool, bool, int, int, int, bool, bool, bool, bool)> DeclareDynamics[]
{
	DynamicShader_LuxVLG_Simple, DynamicShader_LuxVLG_Bump, DynamicShader_LuxVLG_Phong
};

// COLOR is for brushes. COLOR2 for models
#define LuxVertexLitGeneric_Params()   void LuxVertexLitGeneric_Link_Params(Model_Vars_t &info)\
{									   \
	info.m_nColor =  COLOR2;			\
	info.m_nMultipurpose7 = MATERIALNAME;\
	Link_EnvMapFresnel()				  \
	Link_MiscParameters()				   \
	Link_PhongParameters()					\
	Link_GlobalParameters()					 \
	Link_SeamlessParameters()				  \
	Link_RimLightParameters()				   \
	Link_EnvMapMaskParameters()				    \
	Link_LightmappingParameters()				 \
	Link_DistanceAlphaParameters()				  \
	Link_DetailTextureParameters()				   \
	Link_NormalTextureParameters()				    \
	Link_EnvironmentMapParameters()				     \
	Link_SelfIlluminationParameters()				  \
	Link_SelfIllumTextureParameters()				   \
}//-----------------------------------------------------\

//==================================================================================================
void LuxVertexLitGeneric_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Model_Vars_t &info)
{
	// I wanted to debug print some values and needed to know which material they belong to. However pMaterialName is not available to the draw!
	// Well, now it is...
	params[info.m_nMultipurpose7]->SetStringValue(pMaterialName);

	// See ACROHS_Defines.h for more information.
	if (GetIntParamValue(info.m_nDistanceAlpha) != 0)
	{
		SET_FLAGS2(MATERIAL_VAR2_DISTANCEALPHA);
	}

	if (GetIntParamValue(info.m_nCloak) != 0)
	{
		SET_FLAGS2(MATERIAL_VAR2_CLOAK);
	}

	if (GetIntParamValue(info.m_nEmissiveBlend) != 0)
	{
		SET_FLAGS2(MATERIAL_VAR2_EMISSIVEBLEND);
	}

	if (GetIntParamValue(info.m_nFleshInterior) != 0)
	{
		SET_FLAGS2(MATERIAL_VAR2_FLESHINTERIOR);
	}

	SET_FLAGS(MATERIAL_VAR_MODEL);

	// The usual...
	Flashlight_BorderColorSupportCheck();

	// We trick the engine into thinking we don't have a normal map. This is required for Model Lightmapping ( with bumpmaps )
	if (IsParamDefined(info.m_nBumpMap))
	{
		params[info.m_nNormalTexture]->SetStringValue(params[info.m_nBumpMap]->GetStringValue());

		// TODO: Figure out if this makes any sense. Presumably you can only have $lightmap set if you don't have $bumpmap?
		if (IsParamDefined(info.m_nLightMap))
		{
			// 12.02.2023 ShiroDkxtro2 : NOTE, if you don't have data on $bumpmap, the engine will not send static light data or sunlight data to the model!!! 
			params[info.m_nBumpMap]->SetUndefined();
		}
	}
	else
	{
		// If someone uses $NormalTexture now instead of $BumpMap, make sure $BumpMap has some kind of data, so we can actually get static light data!
		if (IsParamDefined(info.m_nNormalTexture) && !IsParamDefined(info.m_nLightMap) || GetBoolParamValue(info.m_nPhong))
		{
			params[info.m_nBumpMap]->SetStringValue("..."); // Whats on $Bumpmap doesn't matter, it just has to... exist...
		}
	}

#ifdef LIGHTWARP
	// Only try to undefine if defined...
	if (IsParamDefined(info.m_nLightWarpTexture) && mat_disable_lightwarp.GetBool())
	{
		params[info.m_nLightWarpTexture]->SetUndefined();
	}

	// If lightwarp but no nobump and bumpmap, must add data to $bumpmap. Same reason as above   ^
	// This fixes models not receiving light data when you have a default bumpmap ( TODO: was this a real issue or does ShaderAPI enable lights again when binding a default normal map? )
	if (!IsParamDefined(info.m_nBumpMap) && (GetIntParamValue(info.m_nLightWarpNoBump) == 0) && IsParamDefined(info.m_nLightWarpTexture))
	{
		params[info.m_nBumpMap]->SetStringValue("..."); // Whats on $Bumpmap doesn't matter, it just has to... exist...
	}
#endif

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
			}

			// ShiroDkxtro2 : I skipped PCC and EnvMapAnisotropy, 
			if (IsParamDefined(info.m_nEnvMapAnisotropy) && GetBoolParamValue(info.m_nEnvMapAnisotropy))
			{
				// Default Value is supposed to be 1.0f
				FloatParameterDefault(info.m_nEnvMapAnisotropyScale, 1.0f)

				// EnvMap Anisotropy requires NormalMaps, if you use LightWarpNoBump with it, you are probably doing something wrong.
				if (!IsParamDefined(info.m_nBumpMap) || GetIntParamValue(info.m_nLightWarpNoBump) == 1)
				{
					Warning("Material %s wants to use $EnvMapAnisotropy, \n but it does not have a $BumpMap to derive its Information from. \n Deactivating... \n", params[info.m_nMultipurpose7]->GetStringValue());
					SetIntParamValue(info.m_nEnvMapAnisotropy, 0);
				}
			}
		}
#endif

#ifdef CUBEMAPS

	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nEnvMapSaturation, 1.0f)

#ifdef CUBEMAPS_FRESNEL
	// These default values make no sense.
	// And the name is also terrible
	// Its Scale, Bias, Exponent... and not "min" ( which implies a minimum ), "max" ( which implies a maximum ), exp ( exponent )
	// You know what it actually is? Scale, Add, Fresnel Exponent... First value scales ( multiplies ), second value just... gets added ontop, and the last value is actually an exponent...
	// Yet Scale here is at 0 and it always adds 1.0
	// Is it supposed to be 1.0f, 0.0f, 2.0f ?
	// Vec3ParameterDefault(info.m_nEnvMapFresnelMinMaxExp, 0.0f, 1.0f, 2.0f)

	// Default values are now 1.0f, 0.0f, 1.0f and thats that.
	Vec3ParameterDefault(info.m_nEnvMapFresnelMinMaxExp, 1.0f, 0.0f, 1.0f) 
#else
	params[info.m_nEnvMapFresnel]->SetFloatValue(0);
#endif
#endif

	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nDetailBlendFactor, 1.0f)

	// Default Value is supposed to be 4.0f
	FloatParameterDefault(info.m_nDetailScale, 4.0f)

	// Default Values are supposed to be 0.0f, 1.0f, 1.0f
	Vec3ParameterDefault(info.m_nSelfIllumFresnelMinMaxExp, 1.0f, 0.0f, 1.0f)

	// Funny issue we had, combine elite vmt uses selfillum tint [2 1 1] and caused the model to be rendered as pink
	// Turns out, Valve forced maskscale of 1.0 and I forgot to default it here
	// Apparently $SelfIllumMaskScale was a lie and it doesn't actually exist on any of the stock shaders.
	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nSelfIllumMaskScale, 1.0f);

	// If in decal mode, no debug override...
	if (IS_FLAG_SET(MATERIAL_VAR_DECAL))
	{
		SET_FLAGS(MATERIAL_VAR_NO_DEBUG_OVERRIDE);
	}

	// No BaseTexture ? None of these.
	if (!IsParamDefined(info.m_nBaseTexture))
	{
		CLEAR_FLAGS(MATERIAL_VAR_SELFILLUM);
		CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK);
	}

#ifdef PHONG_REFLECTIONS
	if (IsParamDefined(info.m_nPhong)) // strncmp(params[info.m_nFakePhongParam]->GetStringValue(), "", 1) && strncmp(params[info.m_nFakePhongParam]->GetStringValue(), "0", 1)
	{
		// g_pConfig says "No Phong"? No Phong it is!
		//if (!g_pConfig->UsePhong())
		//{
		//	params[info.m_nPhong]->SetIntValue(0);
		//}
		//else //-we have Phong
		{
			params[info.m_nPhong]->SetIntValue(1); // This will tell the Drawfunction to use the Phong Shader

			// Default Value is supposed to be 5.0f
			FloatParameterDefault(info.m_nPhongExponent, 5.0f)

			// Default Value is supposed to be ... Well in SDK2013mp its 0...
			// It replaces the *149 of the calculation so that is what its default value SHOULD be
			FloatParameterDefault(info.m_nPhongExponentFactor, 149.0f)

			// Default values are supposed to be 0, 0.5, 1
			Vec3ParameterDefault(info.m_nPhongFresnelRanges, 1.0f, 0.5f, 1.0f)
		}
		params[info.m_nFakePhongParam]->SetUndefined(); // Giga Important!
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
		// IMPORTANT: **Not on VLG**. Stock behaviour demands you use $seamless_base!
		// SetIntParamValue(info.m_nSeamless_Base, 1)
	}
}

//==================================================================================================
void LuxVertexLitGeneric_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Model_Vars_t &info)
{
	// Always needed...
	pShader->LoadTexture(info.m_nFlashlightTexture, TEXTUREFLAGS_SRGB);
	SET_FLAGS(MATERIAL_VAR_MODEL);								// We always use this on models.
	SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_HW_SKINNING);             // Required for skinning
	SET_FLAGS2(MATERIAL_VAR2_LIGHTING_VERTEX_LIT);              // Required for dynamic lighting
//	SET_FLAGS2(MATERIAL_VAR2_USES_ENV_CUBEMAP);
	SET_FLAGS2(MATERIAL_VAR2_NEEDS_BAKED_LIGHTING_SNAPSHOTS);   // Required for ambient cube
	SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_FLASHLIGHT);				// Yes, Yes we support
	SET_FLAGS2(MATERIAL_VAR2_USE_FLASHLIGHT);					// Yes, Yes we use it, what did you think? "Yeah we support it but don't use it, please"(?)

	if (params[info.m_nNormalTexture]->IsDefined())
	{
		SET_FLAGS2(MATERIAL_VAR2_NEEDS_TANGENT_SPACES);             // Required for dynamic lighting
		SET_FLAGS2(MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL);         // Required for dynamic lighting
	}

	// We don't check for Transparency we did so earlier and do so later...
	LoadTextureWithCheck(info.m_nBaseTexture, TEXTUREFLAGS_SRGB)
	// LoadNormalTextureLightmapFlag(info.m_nNormalTexture)
	LoadNormalTexture(info.m_nNormalTexture)
	LoadNormalTexture(info.m_nBumpMap)

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

#ifdef MODEL_LIGHTMAPPING
	LoadTextureWithCheck(info.m_nLightMap, 0)
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
	if(!params[info.m_nEnvMap]->GetTextureValue()->IsError())
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
				if (params[info.m_nNormalTexture]->GetTextureValue()->IsError())
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
		if (!GetBoolParamValue(info.m_nEnvMapMaskFlip) && IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK))
		{
			SetIntParamValue(info.m_nEnvMapMaskFlip, 1);
		}
	}
	else
#endif // #ifdef CUBEMAPS #endif
	{ // No EnvMap == No Masking.
		CLEAR_FLAGS(MATERIAL_VAR_BASEALPHAENVMAPMASK);
		CLEAR_FLAGS(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK);
	}

#ifdef PHONG_REFLECTIONS
	if (params[info.m_nPhong]->GetIntValue() == 1)
	{
		// Consistent with og Skinshader, no sRGB, despite PhongExponentTexture... 
		LoadTextureWithCheck(info.m_nPhongWarpTexture, 0)
		LoadTextureWithCheck(info.m_nPhongExponentTexture, 0)
	}
#endif

	// No Alphatesting/blendtint when we use the Alpha for other things
	// Valves Shaders ignore the fact you can use $envmapmask's alpha for selfillum...
	if (
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
void LuxVertexLitGeneric_Shader_Draw(
	CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow *pShaderShadow, Model_Vars_t &info, VertexCompressionType_t vertexCompression,
	CBasePerMaterialContextData **pContextDataPtr)
{
	// This is used for the function array that declares and sets static/dynamic shaders
	// 0 is the default. lux_lightmappedgeneric_simple_ps30
	// 1 is the bumped shader. lux_lmg_bump_ps30
	// 2 will probably be the phonged shader? I haven't looked into it yet...
	int iShaderCombo = 0;
	// Flagstuff
	// NOTE: We already made sure we don't have conflicting flags on Shader Init ( see above )
	bool bHasFlashlight				=						pShader->UsingFlashlight(params);
	bool bSelfIllum					=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_SELFILLUM); // No SelfIllum under the flashlight
	bool bAlphatest					=						IS_FLAG_SET(MATERIAL_VAR_ALPHATEST);
	bool bNormalMapAlphaEnvMapMask	=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK); // No Envmapping under the flashlight
	bool bBaseAlphaEnvMapMask		=	!bHasFlashlight	&&	IS_FLAG_SET(MATERIAL_VAR_BASEALPHAENVMAPMASK); // No Envmapping under the flashlight
	bool bIsDecal					=						IS_FLAG_SET(MATERIAL_VAR_DECAL);
	bool bHalfLambert				=						IS_FLAG_SET(MATERIAL_VAR_HALFLAMBERT);
	bool bBlendTintByBaseAlpha		=						GetIntParamValue(info.m_nBlendTintByBaseAlpha) != 0;

	// Texture related Boolean. We check for existing bools first because its faster
	bool	bHasBaseTexture			=						IsTextureLoaded(info.m_nBaseTexture);
	bool	bHasNormalTexture		=						IsTextureLoaded(info.m_nNormalTexture);
	bool	bHasSelfIllumMask		=	bSelfIllum		&&	IsTextureLoaded(info.m_nSelfIllumMask);

#ifdef LIGHTWARP
	bool	bHasLightWarpTexture	=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nLightWarpTexture); // No Lightwarp under the flashlight
	bool	bHasLightWarpNoBump		=	bHasLightWarpTexture && (GetIntParamValue(info.m_nLightWarpNoBump) != 0);
#else
	bool	bHasLightWarpTexture	=	false;
	bool	bHasLightWarpNoBump		=	false;
#endif
	//==================================================================================================
#ifdef DETAILTEXTURING
	bool	bHasDetailTexture		=						IsTextureLoaded(info.m_nDetailTexture);
#else
	bool	bHasDetailTexture		=	false;
#endif
	//==================================================================================================
#ifdef CUBEMAPS
	bool	bHasEnvMap				=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nEnvMap); // No Envmapping under the flashlight
	bool	bHasEnvMapMask			=	bHasEnvMap		&&	IsTextureLoaded(info.m_nEnvMapMask); // No Envmapping under the flashlight
	bool	bHasEnvMapFresnel		=	bHasEnvMap		&&	GetIntParamValue(info.m_nEnvMapFresnel) != 0;
#else
	bool	bHasEnvMap				=	false;
	bool	bHasEnvMapMask			=	false;
	bool	bHasEnvMapFresnel		=	false;
#endif
	//==================================================================================================
#ifdef SELFILLUMTEXTURING
	bool	bHasSelfIllumTexture	=	bSelfIllum		&&	IsTextureLoaded(info.m_nSelfIllumTexture);
#endif
	//==================================================================================================
#ifdef MODEL_LIGHTMAPPING
	bool	bHasLightMapUVs			=	GetIntParamValue(info.m_nLightMapUVs) == 1;
	bool	bHasLightMapTexture		=	!bHasFlashlight	&&	IsTextureLoaded(info.m_nLightMap);
#else
	bool	bHasLightMapUVs			= false;
	bool	bHasLightMapTexture		= false;
#endif
	//==================================================================================================
#ifdef PHONG_REFLECTIONS
	bool	bHasPhong					=	GetIntParamValue(info.m_nPhong) != 0; // Can use Phong if you don't have a NormalTexture
	bool	bHasPhongExponentTexture	=	bHasPhong			&&	IsTextureLoaded(info.m_nPhongExponentTexture);
	bool	bHasPhongWarpTexture		=	bHasPhong			&&	IsTextureLoaded(info.m_nPhongWarpTexture);
	bool	bHasPhongNewBehaviour		=	bHasPhong			&&	GetIntParamValue(info.m_nPhongNewBehaviour) != 0;
	bool	bHasRimLight				=	bHasPhong			&&	GetIntParamValue(info.m_nRimLight) != 0;
	bool	bHasBaseMapAlphaPhongMask	=	bHasPhong			&&	GetIntParamValue(info.m_nBaseMapAlphaPhongMask) != 0;
	bool	bHasBaseMapLuminancePhongMask = bHasPhong			&&	GetIntParamValue(info.m_nBaseMapLuminancePhongMask) != 0;

	// The RimMask is part of the PhongExponentTexture, therefore having one is a requirement...
	bool	bHasRimMask					=	bHasRimLight		&&	bHasPhongExponentTexture && GetIntParamValue(info.m_nRimMask) == 1;
#else
	// These need to be declared because we use them on the function array.
	bool	bHasPhongExponentTexture	=	false;
	bool	bHasRimLight				=	false;
#endif
	//==================================================================================================
	bool bHasSelfIllumFresnel			=	bSelfIllum			&&	GetIntParamValue(info.m_nSelfIllumFresnel) != 0;
	//==================================================================================================
	//	We check these only when we are sure we even have an EnvMapMask, Detail/NormalTexture
	//	NOTE: We don't check for BaseTextureTransform. Its the default fallback and if not changed, won't change anything.
	bool bDetailTextureTransform	=	false;
	bool bEnvMapMaskTransform		=	false;
	bool bNormalTextureTransform	=	false;
	// Seamless Data
	bool bHasSeamlessBase = GetIntParamValue(info.m_nSeamless_Base) != 0;
	bool bHasSeamlessDetail = GetIntParamValue(info.m_nSeamless_Detail) != 0;
	bool bHasSeamlessBump = GetIntParamValue(info.m_nSeamless_Bump) != 0;
	bool bHasSeamlessEnvMapMask = GetIntParamValue(info.m_nSeamless_EnvMapMask) != 0;
	// we need to tell the VertexShader if we want any Seamless Data at all
	bool bUsesSeamlessData = bHasSeamlessBase || bHasSeamlessDetail || bHasSeamlessBump || bHasSeamlessEnvMapMask;
	int	 iModeDetail = 0;
	int	 iModeBump = bHasPhong ? 1 : 0;
	int	 iModeEnvMapMask = 0;

	// When we don't have these, we don't compute them.
	if (bHasDetailTexture)
	{
		bDetailTextureTransform = !params[info.m_nDetailTextureTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nDetailTextureTransform);
		iModeDetail = CheckSeamless(bHasSeamlessDetail);
	}

#ifdef CUBEMAPS
	if (bHasEnvMapMask)
	{
		bEnvMapMaskTransform = !params[info.m_nEnvMapMaskTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nEnvMapMaskTransform);
		iModeEnvMapMask = CheckSeamless(bHasSeamlessEnvMapMask);
	}
#endif

	if (bHasNormalTexture || (bHasLightWarpTexture && !bHasLightWarpNoBump))
	{
		iShaderCombo = 1;
		bNormalTextureTransform = !params[info.m_nBumpTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nBumpTransform); // NOTE: Name is BumpTransform and not NormalTextureTransform
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

	int iSelfIllumMode		= bSelfIllum; // 0 if no selfillum, 1 if selfillum is on. 2 is $SelfIllum_EnvMapMask_Alpha. 3 is $SelfIllumTexture
	int iDetailBlendMode	= GetIntParamValue(info.m_nDetailBlendMode);
#ifdef TREESWAYING
	int iTreeSway			= 0;
#else
	int iTreeSway			= 0;
#endif

	// All of these conditions MUST be true.
	if (bHasEnvMapMask && bSelfIllum && (GetIntParamValue(info.m_nSelfIllum_EnvMapMask_Alpha) == 1))
		iSelfIllumMode = 2;

#ifdef SELFILLUMTEXTURING
	if (bHasSelfIllumTexture) iSelfIllumMode = 3;
#endif

	bool IsOpaque(bIsFullyOpaque, info.m_nBaseTexture, bAlphatest);

#ifdef PHONG_REFLECTIONS
	//////////////////////////////////////////////////////////////////////////
	//				THIS ENTIRE BLOCK IS JUST FOR PHONG						//
	//////////////////////////////////////////////////////////////////////////
	if (bHasPhong)
	{
		iShaderCombo = 2; // We want the Phong Shader...

		if (GetIntParamValue(info.m_nPhongDisableHalfLambert) == 1)
		{
			bHalfLambert = false; // Override...
		}
		else
		{
			bHalfLambert = true;
		}

		// Do we have to reproduce the old behaviour to not break existing materials?
		if (!bHasPhongNewBehaviour && bNormalMapAlphaEnvMapMask)
		{
				if (!bHasFlashlight) // bHasEnvmap = !bhasFlashlight. So iEnvMapMode cannot be not 0 during flashlight
				{
					bHasEnvMap			= true; // Stock Phong has EnvMapping.
					bHasEnvMapFresnel	= true; // Stock Phong has EnvMapFresnel from the PhongFresnelRanges.

					if (bHasSelfIllumFresnel)
					{
						// In this scenario Phong used to do -- fEnvMapMask = lerp(BT.a, $InvertPhongMask, $NormalMapAlphaEnvMapMask) --
						// And then -- lerp(EnvMapMask, 1-fEnvMapMask, $InvertPhongMask) --
						// TLDR : EnvMapMask = 0.0f, aka its as good as disabled.
						bHasEnvMap = false; // Nope
						bHasEnvMapFresnel = false; // Nope 
						iEnvMapMode = 0; // Disabled.
					}
					else
					{
						// Supposed to use whatever Phong uses
						if (bHasBaseMapAlphaPhongMask)
						{
							iEnvMapMode = 1;
							bBaseAlphaEnvMapMask = true;
						}
						else
						{
							// Supposed to use whatever Phong uses
							// TODO: Somehow implement this.
							// IDEA: Disable Envmap instead or override Tint to low values. The mask would be extremely weak even at high basetexture values
							// NOTE: Desaturation generally results in weak masks because of the luminance weights being very low values.
							//if (bHasBaseMapLuminancePhongMask)
							//{
							//	
							//}
							//else
							{
								iEnvMapMode = 1;
								bNormalMapAlphaEnvMapMask = true;
							}
						}
					}
				}
			}
		}
#endif

	//===========================================================================//
	// Snapshotting State
	// This gets setup basically once, or again when using the flashlight
	// Once this is done we use dynamic state.
	//===========================================================================//
	if (pShader->IsSnapshotting())
	{
		//===========================================================================//
		// Alphatesting, VertexShaderFormat, sRGBWrite...
		//===========================================================================//
		const bool bFastVertexTextures = g_pHardwareConfig->HasFastVertexTextures();
		
		pShaderShadow->EnableAlphaTest(bAlphatest);
		if (params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f) // 0 is default.
		{
			pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue());
		}
		
		pShaderShadow->EnableAlphaWrites(bIsFullyOpaque);

		if (bFastVertexTextures)
		{
			// Required for Morphing
				SET_FLAGS2(MATERIAL_VAR2_USES_VERTEXID); // Use the Vertex ID Stream
		}

		//===========================================================================//
		// VertexFormat
		//===========================================================================//
		unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) || IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA))
			flags |= VERTEX_COLOR;

		int iTexCoords = 1;
		if (g_pHardwareConfig->HasFastVertexTextures() && IS_FLAG_SET(MATERIAL_VAR_DECAL))
			iTexCoords = 3;
		int pTexCoordDim[3] = { 2, 0, 3 };

		// This enables morphing data for materials with bumpmaps. Lightmapped models don't morph... sooo
		int iUserDataSize = (bHasFlashlight || bHasNormalTexture && !bHasLightMapTexture ) ? 4 : 0;

		pShaderShadow->VertexShaderVertexFormat(flags, iTexCoords, pTexCoordDim, iUserDataSize);
		
		pShaderShadow->EnableSRGBWrite(true); // We always do sRGB Writes. We use ps30.

		//===========================================================================//
		// Enable Samplers
		//===========================================================================//
		EnableSampler(SAMPLER_BASETEXTURE, true) // We always have a basetexture, and yes they should always be sRGB
#ifdef LIGHTWARP
		EnableSamplerWithCheck(bHasLightWarpTexture, SAMPLER_LIGHTWARP, true)
		EnableSamplerWithCheck((bHasPhong || bHasNormalTexture || bHasLightWarpTexture), SAMPLER_NORMALTEXTURE, false)	// No sRGB
#else
		EnableSamplerWithCheck((bHasNormalTexture), SAMPLER_NORMALTEXTURE, false)	// No sRGB
#endif

		if (bSelfIllum)
		{
#ifdef SELFILLUMTEXTURING
		EnableSamplerWithCheck(bHasSelfIllumTexture, SAMPLER_SELFILLUM, true) else
#endif
		EnableSamplerWithCheck(bHasSelfIllumMask, SAMPLER_SELFILLUM, false) // We don't have to bind a standard texture. Sampler will return 0 0 0 0. Too bad!
		}
		
#ifdef MODEL_LIGHTMAPPING
		EnableSamplerWithCheck(bHasLightMapTexture, SAMPLER_LIGHTMAP, false)
#endif

#ifdef PHONG_REFLECTIONS
		EnableSamplerWithCheck(bHasPhongExponentTexture, SAMPLER_PHONGEXPONENT, false)
		EnableSamplerWithCheck(bHasPhongWarpTexture, SAMPLER_PHONGWARP, false)
#endif

#ifdef DETAILTEXTURING
		// ShiroDkxtro2: Stock Shaders do some cursed Ternary stuff with the detailblendmode
		// Which... If I interpret it correctly makes it read non-sRGB maps as sRGB and vice versa...
		if (bHasDetailTexture)
		{
			ITexture *pDetailTexture = params[info.m_nDetailTexture]->GetTextureValue();
			EnableSampler(SAMPLER_DETAILTEXTURE, (pDetailTexture->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
		}
#endif

#ifdef CUBEMAPS
		bool bHasEnvMapAnisotropy = false;
		if (bHasEnvMap)
		{
			EnableSampler(SAMPLER_ENVMAPTEXTURE, g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE)
			EnableSamplerWithCheck(bHasEnvMapMask, SAMPLER_ENVMAPMASK, false) // Stock Consistency, no sRGB read
			bHasEnvMapAnisotropy = bHasNormalTexture && GetBoolParamValue(info.m_nEnvMapAnisotropy);
		}
#endif

		EnableFlashlightSamplers(bHasFlashlight, SAMPLER_SHADOWDEPTH, SAMPLER_RANDOMROTATION, SAMPLER_FLASHLIGHTCOOKIE, info.m_nBaseTexture)

		//===========================================================================//
		// Declare Static Shaders using the function array
		//===========================================================================//
		auto DeclareStaticShaders = DeclareStatics[iShaderCombo];
		DeclareStaticShaders(pShaderShadow, bHasDetailTexture, bHasEnvMapMask,
											bHasFlashlight, iSelfIllumMode, iEnvMapMode, bBlendTintByBaseAlpha,
											bHasEnvMapFresnel, bHasLightMapUVs, bIsDecal, bHalfLambert, iTreeSway, bHasPhongExponentTexture,
											bHasSeamlessBase, iModeDetail, iModeBump, iModeEnvMapMask, bHasEnvMapAnisotropy);
	}
	else // End of Snapshotting ------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{
		// Getting the light state
		LightState_t LightState;
		pShaderAPI->GetDX9LightState(&LightState);

		//===========================================================================//
		// Bind Textures
		//===========================================================================//
		BindTextureWithCheckAndFallback(bHasBaseTexture, SAMPLER_BASETEXTURE, info.m_nBaseTexture, info.m_nBaseTextureFrame, TEXTURE_WHITE)

		// if mat_fullbright 2. Bind a standard white texture...
#ifdef DEBUG_FULLBRIGHT2 
		if (mat_fullbright.GetInt() == 2 && !IS_FLAG_SET(MATERIAL_VAR_NO_DEBUG_OVERRIDE))
			BindTextureStandard(SAMPLER_BASETEXTURE, TEXTURE_GREY)
#endif

#ifdef DETAILTEXTURING
		BindTextureWithCheck(bHasDetailTexture, SAMPLER_DETAILTEXTURE, info.m_nDetailTexture, info.m_nDetailFrame)
#endif

		BindTextureWithCheck(bHasSelfIllumMask, SAMPLER_SELFILLUM, info.m_nSelfIllumMask, info.m_nSelfIllumMaskFrame)
#ifdef SELFILLUMTEXTURING
		// SelfIllumTexture has priority over $selfillum via base alpha and SelfillumMask
		BindTextureWithCheck(bHasSelfIllumTexture, SAMPLER_SELFILLUM, info.m_nSelfIllumTexture, info.m_nSelfIllumTextureFrame)
#endif

#ifdef CUBEMAPS
		if (bHasEnvMap)
		{
			BindTextureWithCheckAndFallback(mat_specular.GetBool(), SAMPLER_ENVMAPTEXTURE, info.m_nEnvMap, info.m_nEnvMapFrame, TEXTURE_BLACK)
			BindTextureWithCheck(bHasEnvMapMask, SAMPLER_ENVMAPMASK, info.m_nEnvMapMask, info.m_nEnvMapMaskFrame)
		}
#endif

#ifdef LIGHTWARP
		BindTextureWithCheck(bHasLightWarpTexture, SAMPLER_LIGHTWARP, info.m_nLightWarpTexture, 0) // $lightwarptextureframe? :/? Should that exist...?
#endif
		if (bHasNormalTexture)
		{
			BindTextureWithoutCheck(SAMPLER_NORMALTEXTURE, info.m_nNormalTexture, info.m_nBumpFrame)	
		}
#ifdef LIGHTWARP
		else
		{
			// Default NormalMap unless $LightWarpNoBump
			if ((bHasLightWarpTexture && !bHasLightWarpNoBump))
			{
				BindTextureStandard(SAMPLER_NORMALTEXTURE, TEXTURE_NORMALMAP_FLAT)
			}
		}
#endif
		if (bHasPhong)
		{
			BindTextureWithCheck(bHasPhongExponentTexture, SAMPLER_PHONGEXPONENT, info.m_nPhongExponentTexture, 0)
			BindTextureWithCheck(bHasPhongWarpTexture, SAMPLER_PHONGWARP, info.m_nPhongWarpTexture, 0)
			if (!bHasNormalTexture)
				BindTextureStandard(SAMPLER_NORMALTEXTURE, TEXTURE_NORMALMAP_FLAT)
		}

#ifdef MODEL_LIGHTMAPPING
		BindTextureWithCheck(bHasLightMapTexture, SAMPLER_LIGHTMAP, info.m_nLightMap, 0)
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

		// $color/$color2 and $BlendTintColorOverBase
		GetVec3ParamValue(info.m_nColor, f4BaseTextureTint_Factor);
		f4BaseTextureTint_Factor[3] = GetFloatParamValue(info.m_nBlendTintColorOverBase);

#ifdef DETAILTEXTURING
		if (bHasDetailTexture)
		{
			// $DetailTint and $DetailBlendFactor
			GetVec3ParamValue(info.m_nDetailTint, f4DetailTint_BlendFactor);
			f4DetailTint_BlendFactor[3] = GetFloatParamValue(info.m_nDetailBlendFactor);
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
	
			f4EnvMapTint_LightScale[0] = powf(f4EnvMapTint_LightScale[0], 2.2f);
			f4EnvMapTint_LightScale[1] = powf(f4EnvMapTint_LightScale[1], 2.2f);
			f4EnvMapTint_LightScale[2] = powf(f4EnvMapTint_LightScale[2], 2.2f);
	
			f4EnvMapTint_LightScale[3] = GetFloatParamValue(info.m_nEnvMapLightScale); // We always need the LightScale.
	
			GetVec3ParamValue(info.m_nEnvMapSaturation, f4EnvMapSaturation_Contrast); // Yes. Yes this is a vec3 parameter.
			f4EnvMapSaturation_Contrast[3] = GetFloatParamValue(info.m_nEnvMapContrast);
	
			if (bHasEnvMapFresnel)
			{
				GetVec3ParamValue(info.m_nEnvMapFresnelMinMaxExp, f4EnvMapFresnelRanges_);
			}
		}
#endif
		BOOL BBools[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };
		BBools[0] = bHalfLambert; // Always used.
		BBools[1] = bHasLightWarpTexture; // Always used.
		BBools[2] = LightState.m_bAmbientLight; // We do this instead of dynamic combo.
	
#ifdef PHONG_REFLECTIONS
		f4Empty(f4PhongFresnelRanges_Exponent)
		f4Empty(f4PhongTint_Boost)
		f4Empty(f4RimLightControls)
	
		// Only do all of this, if we have Phong
		if (bHasPhong)
		{
			// $PhongAlbedoTint?
			bool bHasPhongAlbedoTint = GetIntParamValue(info.m_nPhongAlbedoTint) != 0;
			bool bHasInvertedPhongMask = GetIntParamValue(info.m_nInvertPhongMask) != 0;
			bool bHasPhongExponentTextureMask = GetIntParamValue(info.m_nPhongExponentTextureMask) != 0;
	
			// Get Fresnel Ranges for Phong
			GetVec3ParamValue(info.m_nPhongFresnelRanges, f4PhongFresnelRanges_Exponent);
			// Get $PhongExponent or $PhongExponentFactor if we have a $PhongExponentTexture.
			f4PhongFresnelRanges_Exponent[3] = bHasPhongExponentTexture ? GetFloatParamValue(info.m_nPhongExponentFactor) : GetFloatParamValue(info.m_nPhongExponent);
	
			GetVec3ParamValue(info.m_nPhongTint, f4PhongTint_Boost);
			f4PhongTint_Boost[3] = GetFloatParamValue(info.m_nPhongBoost);
	
			f4RimLightControls[0] = GetFloatParamValue(info.m_nPhongAlbedoBoost);
			if (bHasRimLight)
			{
				f4RimLightControls[1] = GetFloatParamValue(info.m_nRimLightExponent);
				f4RimLightControls[2] = GetFloatParamValue(info.m_nRimLightBoost);
				//	f4RimLightControls[3] = Empty...
			}
	
			// Use new Behaviour if desired
			if (bHasPhongNewBehaviour)
			{
				BBools[3]	=	bHasBaseMapAlphaPhongMask && !bHasBaseMapLuminancePhongMask; // bHasBaseMapLuminancePhongMask overrides the mask used.
				BBools[4]	=	bHasPhongAlbedoTint;
				BBools[5]	=	bHasInvertedPhongMask;
				BBools[6]	=	bHasRimMask;
				BBools[7]	=	bHasBaseMapLuminancePhongMask;
				BBools[8]	=	bHasPhongExponentTextureMask;
				BBools[9]	=	bHasRimLight;
				BBools[10]	=	bHasPhongWarpTexture;
				BBools[11]	=	bHasEnvMapFresnel;
				BBools[12]	=	bHasSelfIllumFresnel;
			}
			else // Old Behaviour.
			{
				// TODO: Make most of this execute on VMT read. ( Init )
				bool bHasPhongTint = (f4PhongTint_Boost[0] != 1.000000f) || (f4PhongTint_Boost[1] != 1.000000f) || (f4PhongTint_Boost[3] != 1.000000f);
	
				BBools[3] = bHasBaseMapAlphaPhongMask;
				BBools[4] = bHasPhongAlbedoTint && bHasPhongExponentTexture && !bHasPhongTint; // Using PhongTint? RIP AlbedoTint... Can only be used with ExponentTexture too.
				f4RimLightControls[0] = bHasDetailTexture ? 1.0f : f4RimLightControls[0]; // Using $Detail disables $PhongAlbedoBoost
				BBools[5] = bHasInvertedPhongMask;
				BBools[6] = bHasRimMask; // Can only be used with PhongExponentTexture regardless...
				BBools[7] = bHasBaseMapLuminancePhongMask;
				BBools[8] = false; // Stock Behaviour never had $PhongExponentTextureMask
				BBools[9] = bHasRimLight;
				BBools[10] = bHasPhongWarpTexture;
				BBools[11] = true; // EnvMapFresnel? Yep we got it on stock behaviour but it uses Phong Ranges...
				BBools[12] = bHasSelfIllumFresnel;
	
				// TODO: Is this correct or look at least similar...?
				f4EnvMapFresnelRanges_[0] = f4PhongFresnelRanges_Exponent[2]; // Scale = Max of PhongFresnel
				f4EnvMapFresnelRanges_[1] = 0.0f; // Bias = None
				f4EnvMapFresnelRanges_[2] = 1.0f; // Exponent = We don't want to change anything. Whatever^1 is still Whatever.
			}
		}
#endif // PhongReflections
	
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
	
		// This does a lot of things for us!!
		// Just don't touch constants <32 and you should be fine c:
		bool bFlashlightShadows = false;
		SetupStockConstantRegisters(bHasFlashlight, SAMPLER_FLASHLIGHTCOOKIE, SAMPLER_RANDOMROTATION, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame, bFlashlightShadows)
	
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
						//				   38, PCC EnvMap	Origin
						//				   39, PCC Bounding	Box
		if (bHasPhong)	//				   40, PCC Bounding	Box
		{				//				   41, PCC Bounding	Box
			pShaderAPI->SetPixelShaderConstant(42, f4PhongTint_Boost, 1);
			pShaderAPI->SetPixelShaderConstant(43, f4PhongFresnelRanges_Exponent, 1);
			pShaderAPI->SetPixelShaderConstant(44, f4RimLightControls, 1);
		}
		pShaderAPI->SetPixelShaderConstant(45, f4SelfIllumFresnelMinMaxExp, 1);
		pShaderAPI->SetPixelShaderConstant(46, f4EnvMapSaturation_Contrast, 1); 
	
		pShaderAPI->SetPixelShaderConstant(51, f4EnvMapControls, 1);
	
		BBools[13] = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
		BBools[14] = IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA);
		BBools[15] = bWriteDepthToAlpha;
	
		// ALWAYS! Required for HalfLambert, LightWarpTexture, and more.
		pShaderAPI->SetBooleanPixelShaderConstant(0, BBools, 16, true);
	
		pShaderAPI->CommitPixelShaderLighting(PSREG_LIGHT_INFO_ARRAY);
		pShaderAPI->SetPixelShaderStateAmbientLightCube(PSREG_AMBIENT_CUBE, !LightState.m_bAmbientLight); // Force To Black
	
		//==================================================================================================
		// Set Vertexshader Constant Registers (VSREG's...?) Also some other stuff
		//==================================================================================================
		// Always having this
		pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_13, info.m_nBaseTextureTransform);
	
		if (bNormalTextureTransform)
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_15, info.m_nBumpTransform);
		else
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_15, info.m_nBaseTextureTransform);
	
		if (bDetailTextureTransform)
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nDetailTextureTransform, info.m_nDetailScale);
		else
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nBaseTextureTransform, info.m_nDetailScale);
	
#ifdef CUBEMAPS
		if (bEnvMapMaskTransform)
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_19, info.m_nEnvMapMaskTransform);
		else
			pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_19, info.m_nBaseTextureTransform);
#endif

		if (g_pHardwareConfig->HasFastVertexTextures())
		{
			// This is a function that was previously... locked behind an #ifndef and shouldn't have been able to be used...
			SetHWMorphVertexShaderState(pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0);

			bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() || !bIsDecal };
			pShaderAPI->MarkUnusedVertexFields(0, 3, bUnusedTexCoords);
		}
		//==================================================================================================
		// Declare Dynamic Shaders ( Using FunctionArray Lookup Table )
		//==================================================================================================
		bool bHasStaticPropLighting = bStaticLightVertex(LightState); // If you are wondering what this function does, check its description!
		bool bHasDynamicPropLighting = LightState.m_bAmbientLight || (LightState.m_nNumLights > 0) ? 1 : 0;

		auto DeclareDynamic = DeclareDynamics[iShaderCombo]; 
		DeclareDynamic(pShaderAPI, iFogIndex, 0, bFlashlightShadows, (pShaderAPI->GetPixelFogCombo()), bWriteDepthToAlpha,
			bWriteWaterFogToAlpha, bHasLightMapTexture, LightState.m_nNumLights, (pShaderAPI->GetCurrentNumBones() > 0 ? true : false),
			(int)vertexCompression, bHasStaticPropLighting, bHasDynamicPropLighting, LightState.m_bAmbientLight, bHasFlashlight);

		// Used for debugging materials
		// Doesn't have to be pretty, and it won't be pretty to debug anyways.
		if (mat_printvalues.GetBool())
		{
			Warning("--- VertexLitGeneric Material : %s ---", params[info.m_nMultipurpose7]->GetStringValue());
			Warning("Static Vertex Shader Combo \n");
			Warning("%i %i %i %i %i %i %i \n", bHalfLambert, bHasLightMapUVs, iTreeSway, bHasSeamlessBase, iModeDetail, iModeBump, iModeEnvMapMask);
			Warning("Static Pixel Shader Combo \n");
			Warning("%s", bHasFlashlight ? "Flashlight Pass \n" : "Regular Pass \n");
			if (bHasFlashlight)
				Warning("%i %i %i %i %i %i %i \n", g_pHardwareConfig->GetShadowFilterMode(), bBlendTintByBaseAlpha, bHasDetailTexture, 0, bHasNormalTexture, bHasPhong, bHasPhongExponentTexture);
			else
			{
				if (bHasPhong)
					Warning("Phong %i %i %i %i %i %i %i \n", bHasDetailTexture, iSelfIllumMode, iEnvMapMode, GetIntParamValue(info.m_nEnvMapAnisotropy), bBlendTintByBaseAlpha, bHasPhongExponentTexture, 0);
				else if (bHasNormalTexture)
					Warning("Bump %i %i %i %i %i %i \n", bHasDetailTexture, iSelfIllumMode, iEnvMapMode, bBlendTintByBaseAlpha, bHasEnvMapFresnel, 0);
				else // simple
					Warning("Simple %i %i %i %i %i %i \n", bHasDetailTexture, iSelfIllumMode, iEnvMapMode, bBlendTintByBaseAlpha, bHasEnvMapFresnel, 0);
			}			
			Warning("Dynamic Vertex Shader Combo \n");
			Warning("%i %i %i %i %i %i \n", iFogIndex, g_pHardwareConfig->HasFastVertexTextures() && pShaderAPI->IsHWMorphingEnabled(), LightState.m_nNumLights, (pShaderAPI->GetCurrentNumBones() > 0 ? true : false), bHasStaticPropLighting, bHasDynamicPropLighting);
			Warning("Dynamic Pixel Shader Combo \n");
			if (bHasFlashlight)
				Warning("%i %i \n", bFlashlightShadows, pShaderAPI->GetPixelFogCombo());
			else
			{
				if (bHasPhong)
					Warning("Phong %i %i %i %i \n", pShaderAPI->GetPixelFogCombo(), bWriteWaterFogToAlpha, LightState.m_nNumLights, !bHasStaticPropLighting && LightState.m_bAmbientLight);
				else if (bHasNormalTexture)
					Warning("Bump %i %i %i \n", pShaderAPI->GetPixelFogCombo(), bWriteWaterFogToAlpha, bHasLightMapTexture);
				else
					Warning("Simple %i %i %i \n", pShaderAPI->GetPixelFogCombo(), bWriteWaterFogToAlpha, bHasLightMapTexture);
			}
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

} // End of lux*_Shader_Draw

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_VertexLitGeneric,		LUX_VertexLitGeneric)
DEFINE_FALLBACK_SHADER(SDK_Skin_DX9,				LUX_VertexLitGeneric)
DEFINE_FALLBACK_SHADER(SDK_VertexLitGeneric_DX9,	LUX_VertexLitGeneric)
DEFINE_FALLBACK_SHADER(SDK_VertexLitGeneric_DX8,	LUX_VertexLitGeneric)
DEFINE_FALLBACK_SHADER(SDK_VertexLitGeneric_DX7,	LUX_VertexLitGeneric)
DEFINE_FALLBACK_SHADER(SDK_VertexLitGeneric_DX6,	LUX_VertexLitGeneric)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_VertexLitGeneric, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
Declare_MiscParameters()
Declare_NormalTextureParameters()
Declare_SelfIlluminationParameters()
SHADER_PARAM(MATERIALNAME, SHADER_PARAM_TYPE_STRING, "", "") // Can't use pMaterialName on Draw so we put it on this parameter instead...
SHADER_PARAM(DISTANCEALPHA, SHADER_PARAM_TYPE_BOOL, "", "")

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

#ifdef MODEL_LIGHTMAPPING
Declare_LightmappingParameters()
#endif

#ifdef PHONG_REFLECTIONS
Declare_PhongParameters()
Declare_RimLightParameters()
#endif

// Additional Renderpasses
Declare_EmissiveBlendParameters()
Declare_FleshInteriorParameters()
Declare_CloakParameters()

Declare_SeamlessParameters()

END_SHADER_PARAMS

// Setup Params
LuxVertexLitGeneric_Params()
/*
LuxCloak_Params()
LuxEmissiveBlend_Params()
LuxFleshInterior_Params()
*/

SHADER_INIT_PARAMS()
{
	Model_Vars_t vars;
	LuxVertexLitGeneric_Link_Params(vars);
	LuxVertexLitGeneric_Init_Params(this, params, pMaterialName, vars);

	/*
	if (IS_FLAG2_SET(MATERIAL_VAR2_CLOAK))
	{
		Cloak_Vars_t CloakVars;
		LuxCloak_Link_Params(CloakVars);
		LuxCloak_Init_Params(this, params, pMaterialName, CloakVars);
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_EMISSIVEBLEND))
	{
		EmissiveBlend_Vars_t EmissiveBlendVars;
		LuxEmissiveBlend_Link_Params(EmissiveBlendVars);
		LuxEmissiveBlend_Init_Params(this, params, pMaterialName, EmissiveBlendVars);
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_FLESHINTERIOR))
	{
		FleshInterior_Vars_t FleshInteriorVars;
		LuxFleshInterior_Link_Params(FleshInteriorVars);
		LuxFleshInterior_Init_Params(this, params, pMaterialName, FleshInteriorVars);
	}
	*/
}

SHADER_FALLBACK
{
	if (mat_oldshaders.GetBool())
	{
		return "VertexLitGeneric";
	}

	if(IS_FLAG2_SET(MATERIAL_VAR2_DISTANCEALPHA))
	{
		return "LUX_DistanceAlpha";
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
	Model_Vars_t vars;
	LuxVertexLitGeneric_Link_Params(vars);
	LuxVertexLitGeneric_Shader_Init(this, params, vars);

	/*
	if (IS_FLAG2_SET(MATERIAL_VAR2_CLOAK))
	{
		Cloak_Vars_t CloakVars;
		LuxCloak_Link_Params(CloakVars);
		LuxCloak_Shader_Init(this, params, CloakVars);
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_EMISSIVEBLEND))
	{
		EmissiveBlend_Vars_t EmissiveBlendVars;
		LuxEmissiveBlend_Link_Params(EmissiveBlendVars);
		LuxEmissiveBlend_Shader_Init(this, params, EmissiveBlendVars);
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_FLESHINTERIOR))
	{
		FleshInterior_Vars_t FleshInteriorVars;
		LuxFleshInterior_Link_Params(FleshInteriorVars);
		LuxFleshInterior_Shader_Init(this, params, FleshInteriorVars);
	}
	*/
}

SHADER_DRAW
{
	// We only want a StandardPass if we could actually see it.
	bool bStandardPass = true;
	/*
	if (IS_FLAG2_SET(MATERIAL_VAR2_CLOAK) && (pShaderShadow == NULL)) // NOT Snapshotting.
	{
		Cloak_Vars_t CloakVars;
		LuxCloak_Link_Params(CloakVars);
		LuxCloak_Shader_Init(this, params, CloakVars);

		// Replicating Valve Code, This should  give the same result...
		float	CloakFactor = GetFloatParamValue(CloakVars.m_nCloakFactor);
		float	LerpCloakFactor = 1.0f + (clamp(CloakFactor, 0.0f, 1.0f) * (-0.35 - 1.0f));
				LerpCloakFactor = clamp(LerpCloakFactor, 0.0f, 1.0f); // Could potentially be negative, so clamp it... TODO: Math this out and optimise.

		if (LerpCloakFactor <= 0.4f)
			bStandardPass = false;
	}
	*/
	if (bStandardPass)
	{
		Model_Vars_t vars;
		LuxVertexLitGeneric_Link_Params(vars);
		LuxVertexLitGeneric_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);
	}
	else
	{
		Draw(false); // Skip the pass...
	}
	/*
	if (IS_FLAG2_SET(MATERIAL_VAR2_CLOAK))
	{
		Cloak_Vars_t CloakVars;
		LuxCloak_Link_Params(CloakVars);
		LuxCloak_Shader_Init(this, params, CloakVars);

		// Replicating Valve Code, This should  give the same result...
		float	CloakFactor = GetFloatParamValue(CloakVars.m_nCloakFactor);
		// Must Snapshot, when in Snapshot state otherwise the game will crash because there is no shader set up for use.
		// We also only want to render if CloakFactor is between 0.0f and 1.0f
		if ((pShaderShadow == NULL) || ((CloakFactor > 0.0f) && (CloakFactor < 1.0f)))
		{
			LuxCloak_Shader_Draw(this, params, pShaderAPI, pShaderShadow, CloakVars, vertexCompression, pContextDataPtr);
		}
		else
		{
			Draw(false);
		}
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_EMISSIVEBLEND))
	{
		EmissiveBlend_Vars_t EmissiveBlendVars;
		LuxEmissiveBlend_Link_Params(EmissiveBlendVars);

		if ((pShaderShadow != NULL) || params[EMISSIVEBLENDSTRENGTH]->GetFloatValue() > 0.0f)
		{
			LuxEmissiveBlend_Shader_Draw(this, params, pShaderAPI, pShaderShadow, EmissiveBlendVars, vertexCompression, pContextDataPtr);
		}
		else
		{
			// We're not snapshotting and we don't need to draw this frame.
			// This is important because we still have another renderpass set up that expects to do something for us.
			Draw(false);
		}
	}

	if (IS_FLAG2_SET(MATERIAL_VAR2_FLESHINTERIOR))
	{
		// Valve : If Snapshotting or "( true )"
		// Aka it always renders... What a waste of lines
		FleshInterior_Vars_t FleshInteriorVars;
		LuxFleshInterior_Link_Params(FleshInteriorVars);
		LuxFleshInterior_Shader_Draw(this, params, pShaderAPI, pShaderShadow, FleshInteriorVars, vertexCompression, pContextDataPtr);
	}
	*/

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER
