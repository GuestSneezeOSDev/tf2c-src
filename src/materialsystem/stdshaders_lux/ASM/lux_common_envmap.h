//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	24.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Shader Constant Register Declaration for EnvMapping
//
//===========================================================================//

#ifndef LUX_COMMON_ENVMAP_H_
#define LUX_COMMON_ENVMAP_H_

// Cubemaps are not rendered under the flashlight.
#if !FLASHLIGHT
//===========================================================================//
//	Declaring FLOAT PixelShader Constant Registers ( PSREG's )
//	There can be a maximum of 224 according to Microsoft Documentation
//	Ranging from 0-223. After this follows b0-b15 and i0 - i15
//===========================================================================//
// const float4 g_EyePos					: register(c11); // lux_common_ps_fxc.h
// const float4 cLightScale					: register(c30); // common_fxc.h

#define ENVMAPLOD							(g_EyePos.a)
#define ENV_MAP_SCALE						(cLightScale.z)

//===========================================================================//
//	Constants found on all shaders with Environment Mapping.
//===========================================================================//
#if (ENVMAPMODE != 0)
const float4	g_EnvMapTint_LightScale			: register(c35); 
#define			f3EnvMapTint					(g_EnvMapTint_LightScale.xyz)
#define			f1EnvMapLightScale				(g_EnvMapTint_LightScale.w)
const float4	g_EnvMapFresnel_				: register(c36);
#define			f1EnvMapFresnelScale			(g_EnvMapFresnel_.x)
#define			f1EnvMapFresnelBias				(g_EnvMapFresnel_.y)
#define			f1EnvMapFresnelExponent			(g_EnvMapFresnel_.z)
const float4	g_EnvMapSaturation_Contrast		: register(c46);
#define			f3EnvMapSaturation				(g_EnvMapSaturation_Contrast.xyz)
#define			f1EnvMapContrast				(g_EnvMapSaturation_Contrast.w)


#if PCC
const float4	CubeMapPos						: register(c38);
const float4x3	f4x3CorrectionMatrix			: register(c39); // c40...c41
#define			f3CubeMapPos					(CubeMapPos.xyz)
#endif // PCC

const float4	EnvMapMaskControls				: register(c51);
#define			f1LerpBaseAlpha					(EnvMapMaskControls.x)
#define			f1LerpNormalAlpha				(EnvMapMaskControls.y)
#define			f1EnvMapMaskFlip				(EnvMapMaskControls.z) // 1 when flipped and 0 when not flipped.
#define			f1EnvMapAnisotropyScale			(EnvMapMaskControls.w)

// Only used on Phong, otherwise its Static Combo.
#if defined(PHONG)
#define			bHasEnvMapFresnel			Bools[11]
#endif

#endif // ENVMAPMODE != 0

#if (ENVMAPMODE == 2)
sampler Sampler_EnvMapMask		: register(s5);
#endif
sampler Sampler_EnvironmentMap	: register(s14);

#endif // !FLASHLIGHT
#endif // End of LUX_COMMON_ENVMAP_H_