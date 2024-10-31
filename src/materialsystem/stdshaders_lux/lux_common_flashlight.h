//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	24.01.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	Shader Constant Register Declaration for
//							flashlight behaviour
//
//===========================================================================//

#ifndef LUX_COMMON_FLASHLIGHT_H_
#define LUX_COMMON_FLASHLIGHT_H_

//===========================================================================//
//	Declaring FLOAT PixelShader Constant Registers ( PSREG's )
//	There can be a maximum of 224 according to Microsoft Documentation
//	Ranging from 0-223. After this follows b0-b15 and i0 - i15
//===========================================================================//
const float4 g_ShadowTweaks					: register(c2); // Flashlight ShadowTweaks
const float4 g_FlashlightAttenuationFactors : register(c13);
const float4 g_FlashlightPos				: register(c14);
const float4x4 g_FlashlightWorldToTexture	: register(c15);
// 16--used by g_FlashlightWorldToTexture
// 17--used by g_FlashlightWorldToTexture
// 18--used by g_FlashlightWorldToTexture
const float4 cFlashlightColor				: register(c28); // common_fxc.h
const float4 cFlashlightScreenScale			: register(c31); // common_fxc.h - .zw are currently unused
#define f1FlashlightNoLambertValue			(cFlashlightColor.w) // This is either 0.0 or 2.0

//===========================================================================//
//	Declaring Samplers. We only have 16 on SM3.0. Ranging from 0-15
//	So we have to reuse them depending on what shader we are on and what features we require...
//	Note : Local definitions might be different. We don't always need to have clear names.
//===========================================================================//
//#if FLASHLIGHT && !CASCADED_SHADOWS
sampler Sampler_ShadowDepth		: register(s13);
sampler Sampler_RandomRotation	: register(s14);
sampler Sampler_FlashlightCookie: register(s15);

// Stock function.
float RemapValClamped(float val, float A, float B, float C, float D)
{
	float cVal = (val - A) / (B - A);
	cVal = saturate(cVal);

	return C + (D - C) * cVal;
}

//===========================================================================//
// ShiroDkxtro2: This was taken from the SourcePlusPlus repository
//===========================================================================//
float LUX_DoShadowNvidiaPCF5x5Gaussian(const float3 shadowMapPos, const float2 vShadowTweaks)
{

	/*
	float fEpsilonX = vShadowTweaks.x;
	float fTwoEpsilonX = 2.0f * fEpsilonX;
	float fEpsilonY = vShadowTweaks.y;
	float fTwoEpsilonY = 2.0f * fEpsilonY;

	float3 shadowMapCenter_objDepth = shadowMapPos;

	float2 shadowMapCenter = shadowMapCenter_objDepth.xy;			// Center of shadow filter
	float objDepth = shadowMapCenter_objDepth.z;					// Object depth in shadow space

	float4 vOneTaps;
	vOneTaps.x = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fTwoEpsilonX, fTwoEpsilonY), objDepth, 1)).x;
	vOneTaps.y = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fTwoEpsilonX, fTwoEpsilonY), objDepth, 1)).x;
	vOneTaps.z = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fTwoEpsilonX, -fTwoEpsilonY), objDepth, 1)).x;
	vOneTaps.w = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fTwoEpsilonX, -fTwoEpsilonY), objDepth, 1)).x;
	float flOneTaps = dot(vOneTaps, float4(1.0f / 331.0f, 1.0f / 331.0f, 1.0f / 331.0f, 1.0f / 331.0f));

	float4 vSevenTaps;
	vSevenTaps.x = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fTwoEpsilonX, 0), objDepth, 1)).x;
	vSevenTaps.y = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fTwoEpsilonX, 0), objDepth, 1)).x;
	vSevenTaps.z = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(0, fTwoEpsilonY), objDepth, 1)).x;
	vSevenTaps.w = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(0, -fTwoEpsilonY), objDepth, 1)).x;
	float flSevenTaps = dot(vSevenTaps, float4(7.0f / 331.0f, 7.0f / 331.0f, 7.0f / 331.0f, 7.0f / 331.0f));

	float4 vFourTapsA, vFourTapsB;
	vFourTapsA.x = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fTwoEpsilonX, fEpsilonY), objDepth, 1)).x;
	vFourTapsA.y = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fEpsilonX, fTwoEpsilonY), objDepth, 1)).x;
	vFourTapsA.z = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fEpsilonX, fTwoEpsilonY), objDepth, 1)).x;
	vFourTapsA.w = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fTwoEpsilonX, fEpsilonY), objDepth, 1)).x;
	vFourTapsB.x = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fTwoEpsilonX, -fEpsilonY), objDepth, 1)).x;
	vFourTapsB.y = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fEpsilonX, -fTwoEpsilonY), objDepth, 1)).x;
	vFourTapsB.z = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fEpsilonX, -fTwoEpsilonY), objDepth, 1)).x;
	vFourTapsB.w = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fTwoEpsilonX, -fEpsilonY), objDepth, 1)).x;
	float flFourTapsA = dot(vFourTapsA, float4(4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f));
	float flFourTapsB = dot(vFourTapsB, float4(4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f, 4.0f / 331.0f));

	float4 v20Taps;
	v20Taps.x = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fEpsilonX, fEpsilonY), objDepth, 1)).x;
	v20Taps.y = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fEpsilonX, fEpsilonY), objDepth, 1)).x;
	v20Taps.z = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fEpsilonX, -fEpsilonY), objDepth, 1)).x;
	v20Taps.w = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fEpsilonX, -fEpsilonY), objDepth, 1)).x;
	float fl20Taps = dot(v20Taps, float4(20.0f / 331.0f, 20.0f / 331.0f, 20.0f / 331.0f, 20.0f / 331.0f));

	float4 v33Taps;
	v33Taps.x = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(fEpsilonX, 0), objDepth, 1)).x;
	v33Taps.y = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(-fEpsilonX, 0), objDepth, 1)).x;
	v33Taps.z = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(0, fEpsilonY), objDepth, 1)).x;
	v33Taps.w = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter + float2(0, -fEpsilonY), objDepth, 1)).x;
	float fl33Taps = dot(v33Taps, float4(33.0f / 331.0f, 33.0f / 331.0f, 33.0f / 331.0f, 33.0f / 331.0f));

	float flCenterTap = tex2Dproj(Sampler_ShadowDepth, float4(shadowMapCenter, objDepth, 1)).x * (55.0f / 331.0f);

	// Sum all 25 Taps
	return flOneTaps + flSevenTaps + flFourTapsA + flFourTapsB + fl20Taps + fl33Taps + flCenterTap;
	*/
}

//===========================================================================//
//	New Flashlight Shadow Function
//===========================================================================//
float3 LUX_DoFlashlight(float3 f3WorldPosition, float3 f3FaceNormal, bool bDoShadows, float3 f3ProjPos = float3(0.0f, 0.0f, 0.0f))
{
	// Speeds up rendering by not doing anything if this is the case.
	// TODO: What the heck is in the .w
//	if (g_FlashlightPos.w < 0)
	{
//		return float3(0, 0, 0);
	}
// ShiroDkxtro2 :	For some reason commenting this fixed the issue where Decals outside the Flashlight area would start glowing in the dark.
//					Makes me wonder what's going on here. We removed the check above from the PBR shader because it would screw the flashlight entirely.
//					kinda odd...
//	else
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

		float f1Shadow = 0;
		// Shadowing and coloring terms
		if (bDoShadows)
		{
			// TODO: Figure out if this was set to 2048 because of forced Shadow Resolution or because its good values...
			//f1Shadow = LUX_DoShadowNvidiaPCF5x5Gaussian(f3ProjCoords, float2(0.5f / 512.f, 0.5f / 512.f));

			// stock shadows
			
			float2 vPoissonOffset[8] = { float2(0.3475f, 0.0042f),
				float2(0.8806f, 0.3430f),
				float2(-0.0041f, -0.6197f),
				float2(0.0472f, 0.4964f),
				float2(-0.3730f, 0.0874f),
				float2(-0.9217f, -0.3177f),
				float2(-0.6289f, 0.7388f),
				float2(0.5744f, -0.7741f) };

			float flScaleOverMapSize = g_ShadowTweaks.x * 2;		// Tweak parameters to shader
			float2 vNoiseOffset = g_ShadowTweaks.zw;
			float4 vLightDepths = 0, accum = 0.0f;
			float2 rotOffset = 0;

			float2 shadowMapCenter = f3ProjCoords.xy;			// Center of shadow filter
			float objDepth = min(f3ProjCoords.z, 0.99999);		// Object depth in shadow space

			// 2D Rotation Matrix setup
			float3 RMatTop = 0, RMatBottom = 0;
			RMatTop.xy = tex2D(Sampler_RandomRotation, cFlashlightScreenScale.xy * ((f3ProjPos.xy / f3ProjPos.z) * 0.5 + 0.5) + vNoiseOffset) * 2.0 - 1.0;
			RMatBottom.xy = float2(-1.0, 1.0) * RMatTop.yx;	// 2x2 rotation matrix in 4-tuple

			RMatTop *= flScaleOverMapSize;				// Scale up kernel while accounting for texture resolution
			RMatBottom *= flScaleOverMapSize;

			RMatTop.z = shadowMapCenter.x;				// To be added in d2adds generated below
			RMatBottom.z = shadowMapCenter.y;

//			float fResult = 0.0f;

			// NvidiaHardwarePCF
			/*
			rotOffset.x = dot(RMatTop.xy, vPoissonOffset[0].xy) + RMatTop.z;
			rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[0].xy) + RMatBottom.z;
			vLightDepths.x += tex2Dproj(Sampler_ShadowDepth, float4(rotOffset, objDepth, 1)).x;

			rotOffset.x = dot(RMatTop.xy, vPoissonOffset[1].xy) + RMatTop.z;
			rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[1].xy) + RMatBottom.z;
			vLightDepths.y += tex2Dproj(Sampler_ShadowDepth, float4(rotOffset, objDepth, 1)).x;

			rotOffset.x = dot(RMatTop.xy, vPoissonOffset[2].xy) + RMatTop.z;
			rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[2].xy) + RMatBottom.z;
			vLightDepths.z += tex2Dproj(Sampler_ShadowDepth, float4(rotOffset, objDepth, 1)).x;

			rotOffset.x = dot(RMatTop.xy, vPoissonOffset[3].xy) + RMatTop.z;
			rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[3].xy) + RMatBottom.z;
			vLightDepths.w += tex2Dproj(Sampler_ShadowDepth, float4(rotOffset, objDepth, 1)).x;

			rotOffset.x = dot(RMatTop.xy, vPoissonOffset[4].xy) + RMatTop.z;
			rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[4].xy) + RMatBottom.z;
			vLightDepths.x += tex2Dproj(Sampler_ShadowDepth, float4(rotOffset, objDepth, 1)).x;

			rotOffset.x = dot(RMatTop.xy, vPoissonOffset[5].xy) + RMatTop.z;
			rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[5].xy) + RMatBottom.z;
			vLightDepths.y += tex2Dproj(Sampler_ShadowDepth, float4(rotOffset, objDepth, 1)).x;

			rotOffset.x = dot(RMatTop.xy, vPoissonOffset[6].xy) + RMatTop.z;
			rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[6].xy) + RMatBottom.z;
			vLightDepths.z += tex2Dproj(Sampler_ShadowDepth, float4(rotOffset, objDepth, 1)).x;

			rotOffset.x = dot(RMatTop.xy, vPoissonOffset[7].xy) + RMatTop.z;
			rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[7].xy) + RMatBottom.z;
			vLightDepths.w += tex2Dproj(Sampler_ShadowDepth, float4(rotOffset, objDepth, 1)).x;
			f1Shadow = dot(vLightDepths, float4(0.25, 0.25, 0.25, 0.25));
			*/
			
			for (int i = 0; i<2; i++)
			{
				rotOffset.x = dot(RMatTop.xy, vPoissonOffset[4 * i + 0].xy) + RMatTop.z;
				rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[4 * i + 0].xy) + RMatBottom.z;
				vLightDepths.x = tex2D(Sampler_ShadowDepth, rotOffset.xy).x;

				rotOffset.x = dot(RMatTop.xy, vPoissonOffset[4 * i + 1].xy) + RMatTop.z;
				rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[4 * i + 1].xy) + RMatBottom.z;
				vLightDepths.y = tex2D(Sampler_ShadowDepth, rotOffset.xy).x;

				rotOffset.x = dot(RMatTop.xy, vPoissonOffset[4 * i + 2].xy) + RMatTop.z;
				rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[4 * i + 2].xy) + RMatBottom.z;
				vLightDepths.z = tex2D(Sampler_ShadowDepth, rotOffset.xy).x;

				rotOffset.x = dot(RMatTop.xy, vPoissonOffset[4 * i + 3].xy) + RMatTop.z;
				rotOffset.y = dot(RMatBottom.xy, vPoissonOffset[4 * i + 3].xy) + RMatBottom.z;
				vLightDepths.w = tex2D(Sampler_ShadowDepth, rotOffset.xy).x;

				accum += (vLightDepths > objDepth.xxxx);
			}

			f1Shadow = dot(accum, float4(0.125, 0.125, 0.125, 0.125));


			float f1Attenuated = lerp(saturate(f1Shadow), 1.0f, g_ShadowTweaks.y);	// Blend between fully attenuated and not attenuated

			f1Shadow = saturate(lerp(f1Attenuated, f1Shadow, f1Atten));	// Blend between shadow and above, according to light attenuation
			f3FlashlightColor *= f1Shadow;									// Shadow term
		}

		float3 f3DiffuseLighting = f1Atten;
#if defined(NORMALTEXTURE)
		f3DiffuseLighting *= saturate(dot(f3L.xyz, f3FaceNormal) + f1FlashlightNoLambertValue); // NoLambertValue is either 0 or 2
#else
		// This code makes no sense. What the heck? We cap (1.0f + 0 or 2) at 0-1 range. You might as well just remove all of this
		// EXCEPT we were LIED to and the NoLambertValue is in fact like, idk. ( 1.0f - 0.318310 ) or something. aka 1.0f - (1/pi)
		// In that case if pi was precalculated in the lighting intensity or whatever, we could savely not have it.
		// In that case HOWEVER, whats the _______ point of the saturate instruction???
		f3DiffuseLighting *= saturate(1.0f + f1FlashlightNoLambertValue);
#endif
		
		f3DiffuseLighting *= f3FlashlightColor;
		f3DiffuseLighting *= f1EndFalloffFactor;

//		return f3DiffuseLighting;
		return float3(f1Shadow, f1Shadow, f1Shadow);
	}
}
//#endif // #if FLASHLIGHT


#endif // End of LUX_COMMON_PS_FXC_H_