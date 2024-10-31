//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	20.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Modified DecalModulate Shader from ASW
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
#include "lux_model_vs30.inc"
#include "lux_brush_vs30.inc"
#include "lux_decalmodulate_ps30.inc"

static ConVar lux_writelut("lux_writelut", "");

#define LuxDecalModulate_Params() void LuxDecalModulate_Link_Params(Unlit_Vars_t &info)\
{																						\
	Link_MiscParameters()																 \
	Link_GlobalParameters()																  \
	Link_DetailTextureParameters()														   \
	info.m_nMultipurpose7	=	MATERIALNAME;												\
	info.m_nMultipurpose6	=	FOGEXPONENT;												 \
	info.m_nMultipurpose5	=	FOGSCALE;													  \
	info.m_nMultipurpose4	=	PARALLAXMAP;												   \
	info.m_nMultipurpose3	=	PARALLAXMAPSCALE;												\
	info.m_nMultipurpose2	=	PARALLAXMAPMAXOFFSET;											 \
	info.m_nMultipurpose1	=	MODULATEBIAS;													  \
}//------------------------------------------------------------------------------------------------|

void LuxDecalModulate_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Unlit_Vars_t &info)
{
	// I wanted to debug print some values and needed to know which material they belong to. However pMaterialName is not available to the draw!
	// Well, now it is...
	params[info.m_nMultipurpose7]->SetStringValue(pMaterialName);

	SetFloatParamValue(info.m_nMultipurpose6, 0.4f) // FogExponent
	SetFloatParamValue(info.m_nMultipurpose5, 1.0f) // FogScale

	// Default Value is supposed to be 1.0f
	FloatParameterDefault(info.m_nDetailBlendFactor, 1.0f)

	// Default Value is supposed to be 4.0f
	FloatParameterDefault(info.m_nDetailScale, 4.0f)
	
	FloatParameterDefault(info.m_nMultipurpose3, 1.0f) // ParallaxMapScale
	FloatParameterDefault(info.m_nMultipurpose2, 1.0f) // ParallaxMapMaxOffset

	// ASW Code does this so we do it too.
	SET_FLAGS(MATERIAL_VAR_NO_DEBUG_OVERRIDE);

	if (g_pHardwareConfig->HasFastVertexTextures())
	{
		// The vertex shader uses the vertex id stream
		SET_FLAGS2(MATERIAL_VAR2_USES_VERTEXID);
		SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_HW_SKINNING);
	}
}

void LuxDecalModulate_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Unlit_Vars_t &info)
{
	LoadTextureWithCheck(info.m_nBaseTexture, TEXTUREFLAGS_SRGB)
	LoadTextureWithCheck(info.m_nDetailTexture, 0)
	LoadTextureWithCheck(info.m_nMultipurpose4,0 ) // Parallax Map
}

void LuxDecalModulate_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, Unlit_Vars_t &info, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr)
{
	bool bHasBaseTexture	=	IsTextureLoaded(info.m_nBaseTexture);
	bool bHasDetailTexture	=	IsTextureLoaded(info.m_nDetailTexture);
	bool bHasParallaxMap	=	IsTextureLoaded(info.m_nMultipurpose4);
	bool bIsModel			=	IS_FLAG_SET(MATERIAL_VAR_MODEL);

	//===========================================================================//
	// Snapshotting State
	// This gets setup basically once.
	// Once this is done we use dynamic state.
	//===========================================================================//
	if (pShader->IsSnapshotting())
	{

		pShaderShadow->EnableAlphaTest(true);
		pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GREATER, 0.0f);
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnablePolyOffset(SHADER_POLYOFFSET_DECAL);

		// We don't want to write to destination alpha.
		pShaderShadow->EnableAlphaWrites(false);
		pShaderShadow->EnableSRGBWrite(false);

		pShaderShadow->EnableBlending(true);
		pShaderShadow->BlendFunc(SHADER_BLEND_DST_COLOR, SHADER_BLEND_SRC_COLOR);
		pShaderShadow->DisableFogGammaCorrection(true); //fog should stay exactly middle grey
		pShader->FogToGrey();

		//===========================================================================//
		// Enable Samplers
		//===========================================================================//

		//"SRGB conversions hose the blend on some hardware, so keep everything in gamma space."
		// -ASW Code
		EnableSampler(SAMPLER_BASETEXTURE, true)

		// We only support Blendmode 0, however we will still check for the sRGB flag here.
		if (bHasDetailTexture)
		{
			ITexture *pDetailTexture = params[info.m_nDetailTexture]->GetTextureValue();
			EnableSampler(SAMPLER_DETAILTEXTURE, (pDetailTexture->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
		}
		EnableSamplerWithCheck(bHasParallaxMap, SAMPLER_NORMALTEXTURE, false)

		//===========================================================================//
		// Declare Static Shaders, and Vertex Shader Format.
		//===========================================================================//
		bool bHasVertexAlpha = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) && IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA);
		if (!bIsModel )
		{
			unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
			if (bHasVertexAlpha)
			{
				// Enables Vertex Color
				flags |= VERTEX_COLOR;
			}

			pShaderShadow->VertexShaderVertexFormat(flags, 2, 0, 0);

			// We don't need the displacement Shader as we can just get 
			DECLARE_STATIC_VERTEX_SHADER(lux_brush_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(VERTEX_RGBA, bHasVertexAlpha);
			SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, false);
			SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, bHasDetailTexture); // Won't use the second mode.
			SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, 0); // No EnvMap
			SET_STATIC_VERTEX_SHADER(lux_brush_vs30);
			
		}
		else
		{
			// Always.
			GetVertexShaderFormat_ModelMorphed()

			DECLARE_STATIC_VERTEX_SHADER(lux_model_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(DECAL, 1); // Literally the shader is called **Decal**Modulate
			SET_STATIC_VERTEX_SHADER_COMBO(HALFLAMBERT, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(TREESWAY, 0); //... ? This might result in glitches later when the underlying surface gets transformed but the decal ain't.
			SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, bHasDetailTexture); // Won't use the second mode.
			SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, 0); // No EnvMap
			SET_STATIC_VERTEX_SHADER(lux_model_vs30);
		}

		DECLARE_STATIC_PIXEL_SHADER(lux_decalmodulate_ps30);
		SET_STATIC_PIXEL_SHADER_COMBO(DETAILTEXTURE, bHasDetailTexture);
		SET_STATIC_PIXEL_SHADER_COMBO(VERTEXALPHA, bHasVertexAlpha);
		SET_STATIC_PIXEL_SHADER_COMBO(PARALLAX, bHasParallaxMap);
		SET_STATIC_PIXEL_SHADER(lux_decalmodulate_ps30);
	}
	else //
	{
		// TODO: Is this intentional? Wouldn't that cause decals to... disappear?
		/*
		if (pShaderAPI->InFlashlightMode())
		{
			// "Don't draw anything for the flashlight pass" -ASW Code
			pShader->Draw(false);
			return;
		}
		*/

		BindTextureWithCheckAndFallback(bHasBaseTexture, SAMPLER_BASETEXTURE, info.m_nBaseTexture, info.m_nBaseTextureFrame, TEXTURE_GREY)
		BindTextureWithCheck(bHasDetailTexture, SAMPLER_DETAILTEXTURE, info.m_nDetailTexture, info.m_nDetailFrame)
		BindTextureWithCheck(bHasParallaxMap, SAMPLER_NORMALTEXTURE, info.m_nMultipurpose4, 0)

		if (g_pHardwareConfig->HasFastVertexTextures())
		{
			// This is a function that was previously... locked behind an #ifndef and shouldn't have been able to be used...
			SetHWMorphVertexShaderState(pShaderAPI, VERTEX_SHADER_SHADER_SPECIFIC_CONST_10, VERTEX_SHADER_SHADER_SPECIFIC_CONST_11, SHADER_VERTEXTEXTURE_SAMPLER0);

			bool bUnusedTexCoords[3] = { false, false, !pShaderAPI->IsHWMorphingEnabled() };
			pShaderAPI->MarkUnusedVertexFields(0, 3, bUnusedTexCoords);
		}

		// Eye Position required for fog.
		float vEyePos_SpecExponent[4] = { 0, 0, 0, 4 };
		pShaderAPI->GetWorldSpaceCameraPosition(vEyePos_SpecExponent);
		pShaderAPI->SetPixelShaderConstant(PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1);

		// Need this for fog.
		pShaderAPI->SetPixelShaderFogParams(PSREG_FOG_PARAMS);

		f4Empty(f4DetailTint_BlendFactor)
		f4Empty(f4FogControls)
		f4Empty(f4ParallaxControls)
		f4Empty(f4BaseTextureTint)
		// Tint has modulate
		f4BaseTextureTint[0]			= GetFloatParamValue(info.m_nMultipurpose1);
		f4DetailTint_BlendFactor[3]		= GetFloatParamValue(info.m_nDetailBlendFactor);	// We only do Factor
		f4FogControls[1]				= GetFloatParamValue(info.m_nMultipurpose5);		// FogScale
		f4FogControls[3]				= GetFloatParamValue(info.m_nMultipurpose6);		// FogExponent

		f4ParallaxControls[0] = GetFloatParamValue(info.m_nMultipurpose3);
		f4ParallaxControls[1] = GetFloatParamValue(info.m_nMultipurpose2);

		pShaderAPI->SetPixelShaderConstant(32, f4BaseTextureTint, 1);
		pShaderAPI->SetPixelShaderConstant(33, f4DetailTint_BlendFactor, 1);
		pShaderAPI->SetPixelShaderConstant(37, f4FogControls, 1);
		pShaderAPI->SetPixelShaderConstant(50, f4ParallaxControls, 1);

		// Always having this
		pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_13, info.m_nBaseTextureTransform);

		// If DetailTextureTransform exists...
		if (!params[info.m_nDetailTextureTransform]->MatrixIsIdentity())
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nDetailTextureTransform, info.m_nDetailScale);
		else
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nBaseTextureTransform, info.m_nDetailScale);

		//===========================================================================//
		// Declare Dynamic Shaders.
		//===========================================================================//
		if (bIsModel)
		{
			DECLARE_DYNAMIC_VERTEX_SHADER(lux_model_vs30);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(MORPHING, g_pHardwareConfig->HasFastVertexTextures() && pShaderAPI->IsHWMorphingEnabled());
			SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, pShaderAPI->GetCurrentNumBones() > 0 ? true : false);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(STATICPROPLIGHTING, 0);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(DYNAMICPROPLIGHTING, 0);
			SET_DYNAMIC_VERTEX_SHADER(lux_model_vs30);
		}
		else // Gotta be a brush...
		{
			DECLARE_DYNAMIC_VERTEX_SHADER(lux_brush_vs30);
			SET_DYNAMIC_VERTEX_SHADER(lux_brush_vs30);
		}

		MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
		bool bHasFog = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z) ? 1 : 0;

		DECLARE_DYNAMIC_PIXEL_SHADER(lux_decalmodulate_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, bHasFog);
		SET_DYNAMIC_PIXEL_SHADER(lux_decalmodulate_ps30);

	}

	// ShiroDkxtro2:	I had it happen a bunch of times...
	//		pShader->Draw(); MUST BE above the final }
	//		It is very easy to mess this up ( done so several times )
	//		Game will just crash and debug leads you to a bogus function.
	pShader->Draw();
}

// Lux shaders will replace whatever already exists.
DEFINE_FALLBACK_SHADER(SDK_DecalModulate, LUX_DecalModulate)

// ShiroDkxtro2 : Brainchild of Totterynine.
// Declare param declarations separately then call that from within shader declaration.
// This makes it possible to easily run multiple shaders in one file
BEGIN_VS_SHADER(LUX_DECALMODULATE, "ShiroDkxtro2's ACROHS Shader-Rewrite Shader")

BEGIN_SHADER_PARAMS
Declare_MiscParameters()
Declare_DetailTextureParameters()
SHADER_PARAM(MATERIALNAME			, SHADER_PARAM_TYPE_STRING	,"", "") // Can't use pMaterialName on Draw so we put it on this parameter instead...
SHADER_PARAM(FOGEXPONENT			, SHADER_PARAM_TYPE_FLOAT	,"", "")
SHADER_PARAM(FOGSCALE				, SHADER_PARAM_TYPE_FLOAT	,"", "")
SHADER_PARAM(PARALLAXMAP			, SHADER_PARAM_TYPE_TEXTURE	,"", "")
SHADER_PARAM(PARALLAXMAPSCALE		, SHADER_PARAM_TYPE_FLOAT	,"", "")
SHADER_PARAM(PARALLAXMAPMAXOFFSET	, SHADER_PARAM_TYPE_FLOAT	,"", "")
SHADER_PARAM(MODULATEBIAS			, SHADER_PARAM_TYPE_FLOAT	,"", "")
END_SHADER_PARAMS

LuxDecalModulate_Params()

SHADER_INIT_PARAMS()
{
	Unlit_Vars_t vars;
	LuxDecalModulate_Link_Params(vars);
	LuxDecalModulate_Init_Params(this, params, pMaterialName, vars);
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
	Unlit_Vars_t vars;
	LuxDecalModulate_Link_Params(vars);
	LuxDecalModulate_Shader_Init(this, params, vars);
}

SHADER_DRAW
{
	Unlit_Vars_t vars;
	LuxDecalModulate_Link_Params(vars);
	LuxDecalModulate_Shader_Draw(this, params, pShaderAPI, pShaderShadow, vars, vertexCompression, pContextDataPtr);

#ifdef ACROHS_CSM
	// Function Here
#endif

}
END_SHADER