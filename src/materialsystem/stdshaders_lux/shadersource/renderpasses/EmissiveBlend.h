#ifndef EmissiveBlend_H
#define EmissiveBlend_H
#ifdef _WIN32
#pragma once
#endif

void LuxEmissiveBlend_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, EmissiveBlend_Vars_t &info);
void LuxEmissiveBlend_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, EmissiveBlend_Vars_t &info);
void LuxEmissiveBlend_Shader_Draw(CBaseVSShader *pShader, IMaterialVar **params, IShaderDynamicAPI *pShaderAPI,
	IShaderShadow *pShaderShadow, EmissiveBlend_Vars_t &info, VertexCompressionType_t vertexCompression,
	CBasePerMaterialContextData **pContextDataPtr);

#define LuxEmissiveBlend_Params() void LuxEmissiveBlend_Link_Params(EmissiveBlend_Vars_t &info)\
{																								\
	info.m_nBlendStrength			= EMISSIVEBLENDSTRENGTH;									 \
	info.m_nBaseTexture				= EMISSIVEBLENDBASETEXTURE;									  \
	info.m_nFlowTexture				= EMISSIVEBLENDFLOWTEXTURE;									   \
	info.m_nEmissiveTexture			= EMISSIVEBLENDTEXTURE;											\
	info.m_nEmissiveTint			= EMISSIVEBLENDTINT;											 \
	info.m_nEmissiveScrollVector	= EMISSIVEBLENDSCROLLVECTOR;									  \
	info.m_nTime					= TIME;															   \
}//-----------------------------------------------------------------------------------------------------\

#endif