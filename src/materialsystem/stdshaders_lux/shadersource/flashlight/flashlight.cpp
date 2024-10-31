//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	26.05.2023 DMY
//	Last Change :	26.05.2023 DMY
//
//	Purpose of this File :	Multipass Flashlight Functions
//
//===========================================================================//

// File in which you can determine what to use.
// We do not use preprocessor definitions for this because this is honestly faster and easier to set up.
#include "../../ACROHS_Defines.h"

// We require the shared header for some things...
#include "../../ACROHS_Shared.h"

// We need all of these
#include "../CommonFunctions.h"
#include "../../cpp_shader_constant_register_map.h"

// this is required for our Function Array's
#include <functional>

// Includes for Shaderfiles...
#include "lux_model_vs30.inc"
#include "lux_brush_vs30.inc"
#include "lux_displacement_vs30.inc"
#include "lux_flashlight_simple_ps30.inc"

extern ConVar mat_printvalues;
extern ConVar mat_specular;

void LuxFlashlight_Simple_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI, IShaderShadow *pShaderShadow, VertexCompressionType_t vertexCompression, CBasePerMaterialContextData **pContextDataPtr, Flashlight_Vars_t &info, bool bIsModel)
{
	bool bAlphatest					=						IS_FLAG_SET(MATERIAL_VAR_ALPHATEST);
//	bool bBlendTintByBaseAlpha		=						GetIntParamValue(info.m_nBlendTintByBaseAlpha) != 0;
	bool bHasBaseTexture			=						IsTextureLoaded(info.m_nBaseTexture);

#ifdef DETAILTEXTURING
	bool	bHasDetailTexture		=						IsTextureLoaded(info.m_nDetailTexture);
#else
	bool	bHasDetailTexture		=	false;
#endif

	// We need this... I guess?
	bool bHasVertexRGBA = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR) || IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA);

	bool bDetailTextureTransform = false;
	int	 iModeDetail = 0;

	// When we don't have these, we don't compute them.
	if (bHasDetailTexture)
	{
		bDetailTextureTransform = !params[info.m_nDetailTextureTransform]->MatrixIsIdentity();//  IsParamDefined(info.m_nDetailTextureTransform);
		iModeDetail = CheckSeamless(false);
	}

	int iDetailBlendMode	=	GetIntParamValue(info.m_nDetailBlendMode);
#ifdef TREESWAYING
	int iTreeSway = 0;
#else
	int iTreeSway = 0;
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
		pShaderShadow->EnableAlphaTest(bAlphatest);
		if (params[info.m_nAlphaTestReference]->GetFloatValue() > 0.0f) // 0 is default.
		{
			pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GEQUAL, params[info.m_nAlphaTestReference]->GetFloatValue());
		}

		pShaderShadow->EnableSRGBWrite(true); // We always do sRGB Writes. We use ps30.

		//===========================================================================//
		// VertexFormat
		//===========================================================================//
		unsigned int flags = VERTEX_POSITION;
		if (IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR))
		{
			// Enables Vertex Color
			flags |= VERTEX_COLOR;
		}

		// bHasNormalTexture ? 3 : 2
		pShaderShadow->VertexShaderVertexFormat(flags, 2, 0, 0);

		//===========================================================================//
		// Enable Samplers
		//===========================================================================//
		EnableSampler(SAMPLER_BASETEXTURE, true) // We always have a basetexture, and yes they should always be sRGB

#ifdef DETAILTEXTURING
		// ShiroDkxtro2: Stock Shaders do some cursed Ternary stuff with the detailblendmode
		// Which... If I interpret it correctly makes it read non-sRGB maps as sRGB and vice versa...
		if (bHasDetailTexture)
		{
			ITexture *pDetailTexture = params[info.m_nDetailTexture]->GetTextureValue();
			EnableSampler(SAMPLER_DETAILTEXTURE, (pDetailTexture->GetFlags() & TEXTUREFLAGS_SRGB) ? true : false)
		}
#endif

		pShaderShadow->EnableAlphaWrites(false);
		pShaderShadow->EnableDepthWrites(false);
		pShaderShadow->EnableTexture(SAMPLER_SHADOWDEPTH, true);
		pShaderShadow->SetShadowDepthFiltering(SAMPLER_SHADOWDEPTH);
		pShaderShadow->EnableSRGBRead(SAMPLER_SHADOWDEPTH, false);
		pShaderShadow->EnableTexture(SAMPLER_RANDOMROTATION, true);
		pShaderShadow->EnableTexture(SAMPLER_FLASHLIGHTCOOKIE, true);
		pShaderShadow->EnableSRGBRead(SAMPLER_FLASHLIGHTCOOKIE, true);
		pShader->SetAdditiveBlendingShadowState(info.m_nBaseTexture, true);
		pShader->FogToBlack();

		//===========================================================================//
		// Declare Static Shaders, manually.
		//===========================================================================//
		if (bIsModel)
		{
			DECLARE_STATIC_VERTEX_SHADER(lux_model_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(DECAL, IS_FLAG_SET(MATERIAL_VAR_DECAL));
			SET_STATIC_VERTEX_SHADER_COMBO(HALFLAMBERT, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAP_UV, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(TREESWAY, iTreeSway);
			SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iModeDetail);
			SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, 0); // 
			SET_STATIC_VERTEX_SHADER(lux_model_vs30);
		}
		else // Gotta be a brush...
		{
			// This will most definitely be cheaper to render.
			DECLARE_STATIC_VERTEX_SHADER(lux_brush_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(VERTEX_RGBA, bHasVertexRGBA); // This does not work with the below.
			SET_STATIC_VERTEX_SHADER_COMBO(SEAMLESS_BASE, 0);
			SET_STATIC_VERTEX_SHADER_COMBO(DETAILTEXTURE_UV, iModeDetail);
			SET_STATIC_VERTEX_SHADER_COMBO(NORMALTEXTURE_UV, 0); // No Lighting.
			SET_STATIC_VERTEX_SHADER_COMBO(ENVMAPMASK_UV, 0);
			SET_STATIC_VERTEX_SHADER(lux_brush_vs30);
		}



	}
	else // End of Snapshotting ------------------------------------------------------------------------------------------------------------------------------------------------------------------
	{
		//===========================================================================//
		// Bind Textures
		//===========================================================================//
		BindTextureWithCheckAndFallback(bHasBaseTexture, SAMPLER_BASETEXTURE, info.m_nBaseTexture, info.m_nBaseTextureFrame, TEXTURE_WHITE)

#ifdef DETAILTEXTURING
		BindTextureWithCheck(bHasDetailTexture, SAMPLER_DETAILTEXTURE, info.m_nDetailTexture, info.m_nDetailFrame)
#endif

		// This does a lot of things for us!!
		// Just don't touch constants <32 and you should be fine c:
		bool bFlashlightShadows = false;
		SetupStockConstantRegisters(false, SAMPLER_FLASHLIGHTCOOKIE, SAMPLER_RANDOMROTATION, info.m_nFlashlightTexture, info.m_nFlashlightTextureFrame, bFlashlightShadows)

			/*
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
		*/

		int iFogIndex = 0;
		bool bWriteDepthToAlpha = false;
		bool bWriteWaterFogToAlpha = false;
		SetupDynamicComboVariables(iFogIndex, bWriteDepthToAlpha, bWriteWaterFogToAlpha, false)

		//===========================================================================//
		// Prepare floats for Constant Registers
		//===========================================================================//
		// f4Empty is just float4 Name = {1,1,1,1};
		// Yes I have some excessive problems with macro overusage...
		f4Empty(f4BaseTextureTint_Factor)
		f4Empty(f4DetailTint_BlendFactor)
		f4Empty(f4DetailBlendMode_) // yzw empty
		BOOL BBools[16] = { false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false };

		// $color/$color2 and $BlendTintColorOverBase
		GetVec3ParamValue(info.m_nColor, f4BaseTextureTint_Factor);
		f4BaseTextureTint_Factor[3] = GetFloatParamValue(info.m_nBlendTintColorOverBase);

#ifdef DETAILTEXTURING
		if (bHasDetailTexture)
		{
			// $DetailTint and $DetailBlendFactor
			GetVec3ParamValue(info.m_nDetailTint, f4DetailTint_BlendFactor);
			f4DetailTint_BlendFactor[3] = GetFloatParamValue(info.m_nDetailBlendFactor); // Consider writing a macro for this
			f4DetailBlendMode_[0] = iDetailBlendMode; // We trunc it later and do some dynamic branching. Saves a ton of combos but makes rendering slower. but oh well...
		}
#endif

		//==================================================================================================
		// Set Pixelshader Constant Registers (PSREG's)
		//==================================================================================================
		pShaderAPI->SetPixelShaderConstant(	32, f4BaseTextureTint_Factor, 1);
		pShaderAPI->SetPixelShaderConstant(	33, f4DetailTint_BlendFactor, 1);
		//									34, f4SelfIllumTint_Scale
		//									35, f4EnvMapTint_LightScale
		//									36, f4EnvMapFresnelRanges_
		pShaderAPI->SetPixelShaderConstant(	37, f4DetailBlendMode_, 1);
		//									38, PCC Bounding Box
		//									39, PCC Bounding Box
		//									40, PCC Bounding Box
		//									41, PCC Bounding Box
		//									42, f4PhongTint_Boost
		//									43, f4PhongFresnelRanges_Exponent
		//									44, f4RimLightControls		
		//									45, f4SelfIllumFresnelRanges
		//									46, f4EnvMapSaturation_Contrast, 1);

		//									51, f4EnvMapControls, 1);

		//==================================================================================================
		// Set Vertexshader Constant Registers (VSREG's...?) Also some other stuff
		//==================================================================================================
		// Always having this
		pShader->SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_13, info.m_nBaseTextureTransform);

		if (bDetailTextureTransform)
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nDetailTextureTransform, info.m_nDetailScale);
		else
			pShader->SetVertexShaderTextureScaledTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_17, info.m_nBaseTextureTransform, info.m_nDetailScale);

		BBools[13] = IS_FLAG_SET(MATERIAL_VAR_VERTEXCOLOR);
		BBools[14] = IS_FLAG_SET(MATERIAL_VAR_VERTEXALPHA);
//		BBools[15] = bWriteDepthToAlpha; // We don't care

		pShaderAPI->SetBooleanPixelShaderConstant(0, BBools, 16, true);

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

		// Setting up DYNAMIC pixel shader
		DECLARE_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
		SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
		SET_DYNAMIC_PIXEL_SHADER(lux_flashlight_simple_ps30);
	}

	if (mat_printvalues.GetBool())
	{
		mat_printvalues.SetValue(0);
	}

	// ShiroDkxtro2:	I had it happen a bunch of times...
	//		pShader->Draw(); MUST BE above the final }
	//		It is very easy to mess this up ( done so several times )
	//		Game will just crash and debug leads you to a bogus function.
	pShader->Draw();
}