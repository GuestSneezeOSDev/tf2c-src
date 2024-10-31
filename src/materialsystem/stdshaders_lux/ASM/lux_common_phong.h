//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	21.02.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	-Phong Constant Register Declarations
//							-Phong Functions
//
//===========================================================================//

#ifndef LUX_COMMON_PHONG
#define LUX_COMMON_PHONG

//===========================================================================//
//	Declaring FLOAT PixelShader Constant Registers ( PSREG's )
//	There can be a maximum of 224 according to Microsoft Documentation
//	Ranging from 0-223. After this follows b0-b15 and i0 - i15
//===========================================================================//
#if !FLASHLIGHT // Flashlight has a special version of this.
#if defined(PHONG)
// 38-41 are used by PCC. SO we avoid those for Brush Phong by using 42+
const float4	g_PhongTint_Boost				: register(c42);
const float4	g_PhongFresnelRanges_Exponent	: register(c43);
const float4	g_PhongControls					: register(c44);
#define			f3PhongTint						(g_PhongTint_Boost.xyz)
#define			f1PhongBoost					(g_PhongTint_Boost.w)
#define			f3PhongFresnelRanges			(g_PhongFresnelRanges_Exponent.xyz)
#define			f1PhongExponentParam			(g_PhongFresnelRanges_Exponent.w)
#define			f1AlbedoTintBoost				(g_PhongControls.x)
#define			f1RimLightExponent				(g_PhongControls.y)
#define			f1RimLightBoost					(g_PhongControls.z)

//===========================================================================//
//	Declaring BOOLEAN PixelShader Constant Registers ( PSREG's )
//	Apparently these don't quite work...?
//===========================================================================//

#define			bHasBaseAlphaPhongMask			Bools[3]
#define			bHasPhongAlbedoTint				Bools[4]
#define			bHasInvertedPhongMask			Bools[5]
#define			bHasRimLightMask				Bools[6]
#define			bHasBasemapLuminancePhongMask	Bools[7]
#define			bHasPhongExponentTextureMask	Bools[8]
#define			bHasRimLight					Bools[9]
#define			bHasPhongWarpTexture			Bools[10]

//===========================================================================//
//	Declaring Samplers. We only have 16 on SM3.0. Ranging from 0-15
//	So we have to reuse them depending on what shader we are on and what features we require...
//	Note : Local definitions might be different. We don't always need to have clear names.
//===========================================================================//

// Always!
sampler Sampler_PhongWarpTexture: register(s7);
sampler Sampler_PhongExpTexture : register(s8);

//===========================================================================//
//	ShiroDkxtro2 : This is not the same function as in lux_common_lighting.h!!!
//===========================================================================//
float3 LUX_ComputeLight(float3 f3LightPosition, float3 f3WorldPos, float3 f3WorldNormal, float3 f3LightColor, float f1Attenuation)
{
	float3 f3Result = float3(0.0f, 0.0f, 0.0f);
	float f1dot_ = dot(f3WorldNormal, normalize(f3LightPosition - f3WorldPos)); // Potentially negative dot

	// bHalfLambert is Bools[0], defined in lux_common_ps_fxc.h
	if (bHalfLambert)
	{
		f1dot_ = saturate(f1dot_ * 0.5 + 0.5); // Scale and bias so we get positive range

		if (!bLightWarpTexture)
		{
			f1dot_ *= f1dot_; // Square
		}
	}
	else
	{
		f1dot_ = saturate(f1dot_);
	}

	f3Result = float3(f1dot_, f1dot_, f1dot_);

	if (bLightWarpTexture)
	{
		f3Result = 2.0f * tex1D(Sampler_LightWarpTexture, f1dot_).xyz;
	}

	// Order doesn't matter thanks to multiplication...
		f3Result *= f3LightColor * f1Attenuation;
	
	return f3Result;
}

//===========================================================================//
//	New Specular Light Function
//===========================================================================//
float3 LUX_DoSpecularLight(float f1NdL, float3 f3Reflect, float3 f3LightDir, float3 f3LightColor, float f1SpecularExponent, float f1Fresnel, out float3 f3RimLight)
{
	// L.R
	float	f1RdL = saturate(dot(f3Reflect, f3LightDir));
	float	f1Specular = pow(f1RdL, f1SpecularExponent); // Raise to the Power of the Exponent

	// Copy it around a bunch of times so we get a float3
		float3	f3Specular = float3(f1Specular, f1Specular, f1Specular);
		f3RimLight = float3(0.0, 0.0, 0.0); // By default no Rimlighting...

	// Warp as a function of *Specular and Fresnel
	if (bHasPhongWarpTexture)
	{
		f3Specular *= tex2D(Sampler_PhongWarpTexture, float2(f3Specular.x, f1Fresnel));
	}
	else
	{	// If we didn't apply Fresnel through Warping, apply it manually.
		f3Specular *= f1Fresnel;
	}

	if (bHasRimLight)
	{
		f3RimLight = pow(f1RdL, f1RimLightExponent);	// Raise to rim exponent
		f3RimLight *= f1NdL;							// Mask with N.L
		f3RimLight *= f3LightColor;
	}

	return (f3Specular * f1NdL * f3LightColor) ; // Mask with N.L and Modulation ( attenuation and color )
}

//===========================================================================//
//	Easy to use function to compute Phong and Lighting for all 4 Lights
//===========================================================================//
float3 LUX_DoSpecular
(float3 f3Normal, float3 f3WorldPos, float4 f4LightAtten,
float f1NdotV, float3 f3EyeDir, float f1SpecularExponent, float f1Fresnel, float f1RimMask,
float3 f3PhongModulation, float3 f3BaseTexture, out float3 f3BumpedLight)
{
	f3BumpedLight = float3(0, 0, 0);
	float3	f3Specular = float3(0, 0, 0);
	float3	f3RimLight = float3(0, 0, 0);
	float3	f3RimLightAmbient = float3(0, 0, 0);
	float f1PhongFresnel;

	// This code was commented from common_vertexlitgeneric_dx9.h but it actually works by comparison... Using whats t here will cause weird purple artifacts and look nothing like the original ( Hunter model )
	if(f1Fresnel > 0.5f)
	{
		f1PhongFresnel = lerp(f3PhongFresnelRanges.y, f3PhongFresnelRanges.z, (2.0f * f1Fresnel)-1.0f);
	}
	else
	{
		f1PhongFresnel = lerp(f3PhongFresnelRanges.x, f3PhongFresnelRanges.y, 2.0f * f1Fresnel);
	}

	if (bAmbientLight)
	{
		f3BumpedLight = AmbientLight(f3Normal, cAmbientCube);
		
		// This is pretty wasteful to do when we don't have a rimlight...
			f3RimLightAmbient = (AmbientLight(f3EyeDir, cAmbientCube) * f1RimLightBoost) * saturate(f1RimMask * f3Normal.z);
	}
	
	// Eye -> Normal -> Reflected
		float3	f3Reflect = 2 * f3Normal * f1NdotV - f3EyeDir;

#if (NUM_LIGHTS > 0)
	float3	f3L1Ray = normalize(cLightInfo[0].pos.xyz - f3WorldPos);
	float	f1L1NdL = saturate(dot(f3Normal, f3L1Ray));
			f3Specular += LUX_DoSpecularLight(f1L1NdL, f3Reflect, f3L1Ray, cLightInfo[0].color.xyz * f4LightAtten.x, f1SpecularExponent, f1PhongFresnel, f3RimLight);
			f3BumpedLight += LUX_ComputeLight(cLightInfo[0].pos.xyz, f3WorldPos, f3Normal, cLightInfo[0].color.xyz, f4LightAtten.x);

#if (NUM_LIGHTS > 1)
	float3	f3L2Ray = normalize(cLightInfo[1].pos.xyz - f3WorldPos);
	float	f1L2NdL = saturate(dot(f3Normal, f3L2Ray));
			f3Specular += LUX_DoSpecularLight(f1L2NdL, f3Reflect, f3L2Ray, cLightInfo[1].color.xyz * f4LightAtten.y, f1SpecularExponent, f1PhongFresnel, f3RimLight);
			f3BumpedLight += LUX_ComputeLight(cLightInfo[1].pos.xyz, f3WorldPos, f3Normal, cLightInfo[1].color.xyz, f4LightAtten.y);

#if (NUM_LIGHTS > 2)
	float3	f3L3Ray = normalize(cLightInfo[2].pos.xyz - f3WorldPos);
	float	f1L3NdL = saturate(dot(f3Normal, f3L3Ray));
			f3Specular += LUX_DoSpecularLight(f1L3NdL, f3Reflect, f3L3Ray, cLightInfo[2].color.xyz * f4LightAtten.z, f1SpecularExponent, f1PhongFresnel, f3RimLight);
			f3BumpedLight += LUX_ComputeLight(cLightInfo[2].pos.xyz, f3WorldPos, f3Normal, cLightInfo[2].color.xyz, f4LightAtten.z);

#if (NUM_LIGHTS > 3)
	float3	vLight4Color = float3(cLightInfo[0].color.w, cLightInfo[0].pos.w, cLightInfo[1].color.w);
	float3	vLight4Pos = float3(cLightInfo[1].pos.w, cLightInfo[2].color.w, cLightInfo[2].pos.w);
	float3	f3L4Ray = normalize(vLight4Pos - f3WorldPos);
	float	f1L4NdL = saturate(dot(f3Normal, f3L4Ray));
			f3Specular += LUX_DoSpecularLight(f1L4NdL, f3Reflect, f3L4Ray, vLight4Color * f4LightAtten.w, f1SpecularExponent, f1PhongFresnel, f3RimLight);
			f3BumpedLight += LUX_ComputeLight(vLight4Pos, f3WorldPos, f3Normal, vLight4Color, f4LightAtten.w);
#endif
#endif
#endif
#endif

	// f3PhongModulation is basically the Masks used for Phong.
	f3Specular *= f3PhongModulation * f1PhongBoost;
	
	if (bHasRimLight)
	{
		// Now that we added all those lights together, mask them.
		// NOTE: Double Squared Fresnel with no ranges for RimLight. Double-square for 'more subtle look'
		f3RimLight		*=	f1RimMask * (f1Fresnel * f1Fresnel);
	
		// This is what Valve did, so we have to do it... I could think of a better way, that is.
		// This avoids oversaturation. We have two seperate instances of light information here and we want to avoid having it twice.
		f3Specular = max(f3Specular, f3RimLight);
	
		// This is probably the reason why on the VDC it says something about $rimlightexponent not working in HLMV
		// They probably meant $rimlightboost. And it probably means there is no ambientcubes in HLMV.
		// This will add 0,0,0 if no ambient cubes exist. So its only minor performance decrease.
		f3Specular += f3RimLightAmbient;
	}

	f3Specular *= f3PhongTint; // $PhongTint

	f3Specular += f3BaseTexture * f3BumpedLight;
	return f3Specular;
}

#endif // #if defined(PHONG)
#endif // #if !FLASHLIGHT
#endif // End of LUX_COMMON_PHONG_H_