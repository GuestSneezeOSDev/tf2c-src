#ifndef Cloak_H
#define Cloak_H
#ifdef _WIN32
#pragma once
#endif

void LuxCloak_Link_Params(Cloak_Vars_t &info);
void LuxCloak_Init_Params(CBaseVSShader *pShader, IMaterialVar **params, const char *pMaterialName, Cloak_Vars_t &info);
void LuxCloak_Shader_Init(CBaseVSShader *pShader, IMaterialVar **params, Cloak_Vars_t &info);
void LuxCloak_Shader_Draw(	CBaseVSShader *pShader,			IMaterialVar **params,	IShaderDynamicAPI *pShaderAPI,
							IShaderShadow *pShaderShadow,	Cloak_Vars_t &info,		VertexCompressionType_t vertexCompression,
							CBasePerMaterialContextData **pContextDataPtr);

#define LuxCloak_Params() void LuxCloak_Link_Params(Cloak_Vars_t &info)\
{																		\
info.m_nCloakFactor = CLOAKFACTOR;										 \
info.m_nCloakColorTint = CLOAKCOLORTINT;								  \
info.m_nRefractAmount = REFRACTAMOUNT;									   \
info.m_nBumpMap = BUMPMAP;													\
info.m_nBumpFrame = BUMPFRAME;												 \
info.m_nBumpTransform = BUMPTRANSFORM;										  \
}//----------------------------------------------------------------------------\

#endif