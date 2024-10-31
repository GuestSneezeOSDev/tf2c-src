//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	24.05.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Function Declarations for the Infected Shader
//
//===========================================================================//

#ifndef LUX_INFECTED_FXC_H_
#define LUX_INFECTED_FXC_H_

#if FLASHLIGHT

sampler Sampler_LightWarpTexture : register(s6);
#include "lux_common_phong_flashlight.h"
//				Required for the flashlight.
//				In lux_common_phong_flashlight.h
//				g_ShadowTweaks					: register(c2); // Flashlight ShadowTweaks
//				g_FlashlightAttenuationFactors	: register(c13);
//				g_FlashlightPos					: register(c14);
//				g_FlashlightWorldToTexture		: register(c15);
//				g_FlashlightWorldToTexture[1]
//				g_FlashlightWorldToTexture[2]
//				g_FlashlightWorldToTexture[3]
//				cFlashlightColor				: register(c28); // common_fxc.h
//				cFlashlightScreenScale			: register(c31); // common_fxc.h - .zw are currently unused
//#define		f1FlashlightNoLambertValue		(cFlashlightColor.w) // This is either 0.0 or 2.0

//				Sampler_ShadowDepth				: register(s13);
//				Sampler_RandomRotation			: register(s14);
//				Sampler_FlashlightCookie		: register(s15);
#endif

// Since we don't use this here, we can reuse the constant register declaration for randomisation offsets! :)
#define			g_RandomisationA				(g_DetailMode.x)
#define			g_RandomisationB				(g_DetailMode.y)
#define			g_UVRandomisation				(g_DetailMode.zw)

// Can use that for Blood now!
#define			f3BloodTint						(g_BaseTextureTint_Factor.xyz)
#define			f1Debug							(g_BaseTextureTint_Factor.w)

const float4	g_BloodMaskSpecPhong			: register(c34);
#define			f2BloodMaskRange				(g_BloodMaskSpecPhong.xy)
#define			f1BloodPhongBoost				(g_BloodMaskSpecPhong.z)
#define			f1BloodExponent					(g_BloodMaskSpecPhong.w)

const float4	g_PhongExponents				: register(c35);
#define			f1DefaultExponent				(g_PhongExponents.x)
#define			f1ClothExponent					(g_PhongExponents.y)
#define			f1SkinExponent					(g_PhongExponents.z)
#define			f1DetailExponent				(g_PhongExponents.w)

//				In lux_common_phong_flashlight.h
#if !FLASHLIGHT
const float4	g_PhongTint_Boost				: register(c42);
#define			f3PhongTint						(g_PhongTint_Boost.xyz)
#define			f1PhongBoost					(g_PhongTint_Boost.w)
#endif
//				g_PhongFresnelRanges_Exponent	: register(c43);
//#define		f3PhongFresnelRanges			(g_PhongFresnelRanges_Exponent.xyz)
//#define		f1PhongExponentParam			(g_PhongFresnelRanges_Exponent.w)
//				g_PhongControls					: register(c44);

//===========================================================================//
//	ShiroDkxtro2 : This is not the same function as in lux_common_lighting.h!!!
//===========================================================================//
#if !FLASHLIGHT
float3 LUX_InfectedLight(float3 f3LightPosition, float3 f3WorldPos, float3 f3WorldNormal, float3 f3LightColor, float f1Attenuation)
{
	float3 f3Result = float3(0.0f, 0.0f, 0.0f);
	float f1dot_ = dot(f3WorldNormal, normalize(f3LightPosition - f3WorldPos)); // Potentially negative dot

	// bHalfLambert is Bools[0], defined in lux_common_ps_fxc.h
	if (bHalfLambert)
	{
		f1dot_ = saturate(f1dot_ * 0.5 + 0.5); // Scale and bias so we get positive range
		f1dot_ *= f1dot_; // Square
	}
	else
	{
		f1dot_ = saturate(f1dot_);
	}

	f3Result = float3(f1dot_, f1dot_, f1dot_);

	// Order doesn't matter thanks to multiplication...
	f3Result *= f3LightColor * f1Attenuation;

	return f3Result;
}


//===========================================================================//
//	New Specular Light Function
//===========================================================================//
float3 LUX_InfectedSpecularLight(float f1NdL, float3 f3Reflect, float3 f3LightDir, float3 f3LightColor, float f1SpecularExponent, float f1Fresnel)
{
	// L.R
	float	f1RdL = saturate(dot(f3Reflect, f3LightDir));
	float	f1Specular = pow(f1RdL, f1SpecularExponent); // Raise to the Power of the Exponent

	// Copy it around a bunch of times so we get a float3
	float3	f3Specular = float3(f1Specular, f1Specular, f1Specular);

	// Apply Fresnel, no warping.
	f3Specular *= f1Fresnel;

	return (f3Specular * f1NdL * f3LightColor); // Mask with N.L and Modulation ( attenuation and color )
}


//===========================================================================//
//	Easy to use function to compute Phong and Lighting for all 4 Lights
//===========================================================================//
float3 LUX_InfectedSpecular
(float3 f3Normal, float3 f3WorldPos, float4 f4LightAtten,
float f1NdotV, float3 f3EyeDir, float f1SpecularExponent, float f1Fresnel,
float3 f3PhongModulation, float3 f3BaseTexture, float f1PhongBoostFinal)
{
	float3 f3BumpedLight = float3(0, 0, 0);
	float3	f3Specular = float3(0, 0, 0);
	float3	f3RimLight = float3(0, 0, 0);
	float f1PhongFresnel = f1Fresnel;

	// This code was commented from common_vertexlitgeneric_dx9.h but it actually works by comparison... Using whats t here will cause weird purple artifacts and look nothing like the original ( Hunter model )
	/*
	if (f1Fresnel > 0.5f)
	{
		f1PhongFresnel = lerp(f3PhongFresnelRanges.y, f3PhongFresnelRanges.z, (2.0f * f1Fresnel) - 1.0f);
	}
	else
	{
		f1PhongFresnel = lerp(f3PhongFresnelRanges.x, f3PhongFresnelRanges.y, 2.0f * f1Fresnel);
	}
	*/

	if (bAmbientLight)
	{
		f3BumpedLight = AmbientLight(f3Normal, cAmbientCube);
	}

	// Eye -> Normal -> Reflected
	float3	f3Reflect = 2 * f3Normal * f1NdotV - f3EyeDir;

#if (NUM_LIGHTS > 0)
	float3	f3L1Ray = normalize(cLightInfo[0].pos.xyz - f3WorldPos);
	float	f1L1NdL = saturate(dot(f3Normal, f3L1Ray));
	f3Specular += LUX_InfectedSpecularLight(f1L1NdL, f3Reflect, f3L1Ray, cLightInfo[0].color.xyz * f4LightAtten.x, f1SpecularExponent, f1PhongFresnel);
	f3BumpedLight += LUX_InfectedLight(cLightInfo[0].pos.xyz, f3WorldPos, f3Normal, cLightInfo[0].color.xyz, f4LightAtten.x);

#if (NUM_LIGHTS > 1)
	float3	f3L2Ray = normalize(cLightInfo[1].pos.xyz - f3WorldPos);
	float	f1L2NdL = saturate(dot(f3Normal, f3L2Ray));
	f3Specular += LUX_InfectedSpecularLight(f1L2NdL, f3Reflect, f3L2Ray, cLightInfo[1].color.xyz * f4LightAtten.y, f1SpecularExponent, f1PhongFresnel);
	f3BumpedLight += LUX_InfectedLight(cLightInfo[1].pos.xyz, f3WorldPos, f3Normal, cLightInfo[1].color.xyz, f4LightAtten.y);

#if (NUM_LIGHTS > 2)
	float3	f3L3Ray = normalize(cLightInfo[2].pos.xyz - f3WorldPos);
	float	f1L3NdL = saturate(dot(f3Normal, f3L3Ray));
	f3Specular += LUX_InfectedSpecularLight(f1L3NdL, f3Reflect, f3L3Ray, cLightInfo[2].color.xyz * f4LightAtten.z, f1SpecularExponent, f1PhongFresnel);
	f3BumpedLight += LUX_InfectedLight(cLightInfo[2].pos.xyz, f3WorldPos, f3Normal, cLightInfo[2].color.xyz, f4LightAtten.z);

#if (NUM_LIGHTS > 3)
	float3	vLight4Color = float3(cLightInfo[0].color.w, cLightInfo[0].pos.w, cLightInfo[1].color.w);
	float3	vLight4Pos = float3(cLightInfo[1].pos.w, cLightInfo[2].color.w, cLightInfo[2].pos.w);
	float3	f3L4Ray = normalize(vLight4Pos - f3WorldPos);
	float	f1L4NdL = saturate(dot(f3Normal, f3L4Ray));
	f3Specular += LUX_InfectedSpecularLight(f1L4NdL, f3Reflect, f3L4Ray, vLight4Color * f4LightAtten.w, f1SpecularExponent, f1PhongFresnel);
	f3BumpedLight += LUX_InfectedLight(vLight4Pos, f3WorldPos, f3Normal, vLight4Color, f4LightAtten.w);
#endif
#endif
#endif
#endif

	// f3PhongModulation is basically the Masks used for Phong.
	f3Specular *= f3PhongModulation * f1PhongBoostFinal;
	f3Specular *= f3PhongTint; // $PhongTint
	f3Specular += f3BaseTexture * f3BumpedLight;
	return f3Specular;
}

#endif

#if 0

// Special version with no rimlight or phongwarp
float3 LUX_InfectedSpecularLight(float f1NdL, float3 f3Reflect, float3 f3LightDir, float3 f3LightColor, float f1SpecularExponent, float f1Fresnel)
{
	// L.R
	float	f1RdL = saturate(dot(f3Reflect, f3LightDir));
	float	f1Specular = pow(f1RdL, f1SpecularExponent); // Raise to the Power of the Exponent

	// Copy it around a bunch of times so we get a float3
	float3	f3Specular = float3(f1Specular, f1Specular, f1Specular);
			f3Specular *= f1Fresnel;

	return (f3Specular * f1NdL * f3LightColor); // Mask with N.L and Modulation ( attenuation and color )
}


//===========================================================================//
//	New Flashlight Shadow Function
//===========================================================================//
float3 LUX_InfectedFlashlightSpecular(float3 f3WorldPosition, float3 f3FaceNormal, bool bDoShadows, float3 f3Reflect, float f1SpecularExponent, float f1Fresnel, out float3 f3Specular)
{
		float4 f4FlashlightSpacePosition = mul(float4(f3WorldPosition, 1.0), g_FlashlightWorldToTexture);
		clip(f4FlashlightSpacePosition.w);

		float3 f3ProjCoords = f4FlashlightSpacePosition.xyz / f4FlashlightSpacePosition.w;

		float3 f3FlashlightColor = tex2D(Sampler_FlashlightCookie, f3ProjCoords.xy).xyz;

		// Removed some ifdef for Shadermodels here.
		f3FlashlightColor *= cFlashlightColor.xyz;

		float3	f3Delta = g_FlashlightPos.xyz - f3WorldPosition;
		float3	f3L = normalize(f3Delta);
		float	f1DistSquared = dot(f3Delta, f3Delta);
		float	f1Dist = sqrt(f1DistSquared);

		float	f1FarZ = g_FlashlightAttenuationFactors.w;
		float	f1EndFalloffFactor = RemapValClamped(f1Dist, f1FarZ, 0.6f * f1FarZ, 0.0f, 1.0f);

		// Attenuation for light and to fade out shadow over distance
		float	f1Atten = saturate(dot(g_FlashlightAttenuationFactors.xyz, float3(1.0f, 1.0f / f1Dist, 1.0f / f1DistSquared)));

		// Shadowing and coloring terms
		if (bDoShadows)
		{
			// TODO: Figure out if this was set to 2048 because of forced Shadow Resolution or because its good values...
			float f1Shadow = LUX_DoShadowNvidiaPCF5x5Gaussian(f3ProjCoords, float2(1.0 / 2048.0, 1.0 / 2048.0));
			float f1Attenuated = lerp(saturate(f1Shadow), 1.0f, g_ShadowTweaks.y);	// Blend between fully attenuated and not attenuated

			f1Shadow = saturate(lerp(f1Attenuated, f1Shadow, f1Atten));	// Blend between shadow and above, according to light attenuation
			f3FlashlightColor *= f1Shadow;									// Shadow term
		}

		float3 f3DiffuseLighting = f1Atten;
		f3DiffuseLighting *= saturate(dot(f3L.xyz, f3FaceNormal) + f1FlashlightNoLambertValue); // NoLambertValue is either 0 or 2

		f3DiffuseLighting *= f3FlashlightColor;
		f3DiffuseLighting *= f1EndFalloffFactor;

		float	f1L1NdL = saturate(dot(f3FaceNormal, f3L));
		f3Specular = LUX_InfectedSpecularLight(f1L1NdL, f3Reflect, f3L, f3DiffuseLighting, f1SpecularExponent, f1Fresnel);
		return f3DiffuseLighting;
}


float3 LUX_InfectedSpecular
(float3 f3Normal, float3 f3WorldPos,
float f1NdotV, float3 f3EyeDir, float f1SpecularExponent, float f1Fresnel,
float3 f3PhongModulation, float3 f3BaseTexture, float f1SpecularBoost)
{
	float3	f3BumpedLight = float3(0, 0, 0);
	float3	f3Specular = float3(0, 0, 0);

	// Eye -> Normal -> Reflected
	float3	f3Reflect = 2 * f3Normal * f1NdotV - f3EyeDir;

	// This also does Specular by outputting to f3Specular
	f3BumpedLight = LUX_InfectedFlashlightSpecular(f3WorldPos, f3Normal, bFlashlightShadows, f3Reflect, f1SpecularExponent, f1Fresnel, f3Specular);

	// f3PhongModulation is basically the Masks used for Phong.
	f3Specular *= f3PhongModulation * f1SpecularBoost;

	// f3PhongModulation is basically the Masks used for Phong.
	f3Specular *= f3PhongModulation * f1SpecularBoost;

	f3Specular *= f3PhongTint; // $PhongTint

	f3Specular += f3BaseTexture * f3BumpedLight;
	return f3Specular;
}

#endif

#endif //#ifndef LUX_INFECTED_FXC_H_
