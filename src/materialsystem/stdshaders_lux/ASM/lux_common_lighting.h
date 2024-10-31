//
//========= ShiroDkxtro2's --ACROHS Ultimate Shaders Project-- ============//
//
//	Initial D.	:	21.02.2023 DMY
//	Last Change :	24.05.2023 DMY
//
//	Purpose of this File :	-Lighting Constant Register Declarations
//							-Lighting functions
//
//===========================================================================//

#ifndef LUX_COMMON_LIGHTING_H_
#define LUX_COMMON_LIGHTING_H_

//  Source gives us four light colors and positions in an
//  array of three of these structures like so:
//
//       x		y		z      w
//    +------+------+------+------+
//    |       L0.rgb       |      |
//    +------+------+------+      |
//    |       L0.pos       |  L3  |
//    +------+------+------+  rgb |
//    |       L1.rgb       |      |
//    +------+------+------+------+
//    |       L1.pos       |      |
//    +------+------+------+      |
//    |       L2.rgb       |  L3  |
//    +------+------+------+  pos |
//    |       L2.pos       |      |
//    +------+------+------+------+
//

struct PixelShaderLightInfo
{
	float4 color;
	float4 pos;
};

#define cOverbright 2.0f
#define cOOOverbright 0.5f

#define LIGHTTYPE_NONE				0
#define LIGHTTYPE_SPOT				1
#define LIGHTTYPE_POINT				2
#define LIGHTTYPE_DIRECTIONAL		3

//===========================================================================//
//	Declaring FLOAT PixelShader Constant Registers ( PSREG's )
//	There can be a maximum of 224 according to Microsoft Documentation
//	Ranging from 0-223. After this follows b0-b15 and i0 - i15
//===========================================================================//
const float3 cAmbientCube[6]				: register(c4);
// 5-used by cAmbientCube[1]
// 6-used by cAmbientCube[2]
// 7-used by cAmbientCube[3]
// 8-used by cAmbientCube[4]
// 9-used by cAmbientCube[5]
PixelShaderLightInfo cLightInfo[3]			: register(c20); // 4th light spread across w's
// 21--------used by cLightInfo[1]
// 22--------used by cLightInfo[2]
// 23--------used by cLightInfo[2]
// 24--------used by cLightInfo[3]
// 25--------used by cLightInfo[3]
// const float4 cLightScale					: register(c30); // common_fxc.h

// Always used with common_lighting!
sampler Sampler_LightWarpTexture : register(s6);

// Ripped from vertexlitgeneric header. And less function-routing
float3 AmbientLight(const float3 worldNormal, const float3 cAmbientCube[6])
{
	float3 linearColor, nSquared = worldNormal * worldNormal;
	float3 isNegative = (worldNormal >= 0.0) ? 0 : nSquared;
	float3 isPositive = (worldNormal >= 0.0) ? nSquared : 0;
	linearColor = isPositive.x * cAmbientCube[0] + isNegative.x * cAmbientCube[1] +
		isPositive.y * cAmbientCube[2] + isNegative.y * cAmbientCube[3] +
		isPositive.z * cAmbientCube[4] + isNegative.z * cAmbientCube[5];
	return linearColor;
}

//===========================================================================//
// Function for sampling Diffuselighting from all NUM_LIGHTS's
//===========================================================================//
#if !defined(PHONG) // We use the Phong Header for a special Phong Lighting function that is more efficient

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

#if defined(NORMALTEXTURE)
//===========================================================================//
// Function for sampling Bumped Diffuse Lighting from all NUM_LIGHTS's
//===========================================================================//  
float3 LUX_BumpedLighting(float3 f3Normal, float3 f3WorldPos, float4 f4LightAtten)
{
	float3 f3Result = float3(0,0,0);
	if (bAmbientLight)
	{
		f3Result += AmbientLight(f3Normal, cAmbientCube);
	}

#if (NUM_LIGHTS > 0)
	f3Result += LUX_ComputeLight(cLightInfo[0].pos.xyz, f3WorldPos, f3Normal, cLightInfo[0].color.xyz, f4LightAtten.x);
#if (NUM_LIGHTS > 1)
	f3Result += LUX_ComputeLight(cLightInfo[1].pos.xyz, f3WorldPos, f3Normal, cLightInfo[1].color.xyz, f4LightAtten.y);
#if (NUM_LIGHTS > 2)
	f3Result += LUX_ComputeLight(cLightInfo[2].pos.xyz, f3WorldPos, f3Normal, cLightInfo[2].color.xyz, f4LightAtten.z);
#if (NUM_LIGHTS > 3)
	// Unpack the 4th light's data from tight constant packing
	float3 vLight3Color = float3(cLightInfo[0].color.w, cLightInfo[0].pos.w, cLightInfo[1].color.w);
	float3 vLight3Pos = float3(cLightInfo[1].pos.w, cLightInfo[2].color.w, cLightInfo[2].pos.w);
	f3Result += LUX_ComputeLight(vLight3Pos, f3WorldPos, f3Normal, vLight3Color, f4LightAtten.w);
#endif
#endif
#endif
#endif

	return f3Result;
}

#endif // NormalTextures
#endif // Phong

#endif // End of LUX_COMMON_LIGHTING_H_