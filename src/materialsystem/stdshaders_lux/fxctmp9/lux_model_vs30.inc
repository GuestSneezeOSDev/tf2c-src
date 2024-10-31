#include "shaderlib/cshader.h"
class lux_model_vs30_Static_Index
{
private:
	int m_nNORMALTEXTURE_UV;
#ifdef _DEBUG
	bool m_bNORMALTEXTURE_UV;
#endif
public:
	void SetNORMALTEXTURE_UV( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nNORMALTEXTURE_UV = i;
#ifdef _DEBUG
		m_bNORMALTEXTURE_UV = true;
#endif
	}
	void SetNORMALTEXTURE_UV( bool i )
	{
		m_nNORMALTEXTURE_UV = i ? 1 : 0;
#ifdef _DEBUG
		m_bNORMALTEXTURE_UV = true;
#endif
	}
private:
	int m_nHALFLAMBERT;
#ifdef _DEBUG
	bool m_bHALFLAMBERT;
#endif
public:
	void SetHALFLAMBERT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHALFLAMBERT = i;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
#endif
	}
	void SetHALFLAMBERT( bool i )
	{
		m_nHALFLAMBERT = i ? 1 : 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = true;
#endif
	}
private:
	int m_nTREESWAY;
#ifdef _DEBUG
	bool m_bTREESWAY;
#endif
public:
	void SetTREESWAY( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nTREESWAY = i;
#ifdef _DEBUG
		m_bTREESWAY = true;
#endif
	}
	void SetTREESWAY( bool i )
	{
		m_nTREESWAY = i ? 1 : 0;
#ifdef _DEBUG
		m_bTREESWAY = true;
#endif
	}
private:
	int m_nLIGHTMAP_UV;
#ifdef _DEBUG
	bool m_bLIGHTMAP_UV;
#endif
public:
	void SetLIGHTMAP_UV( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nLIGHTMAP_UV = i;
#ifdef _DEBUG
		m_bLIGHTMAP_UV = true;
#endif
	}
	void SetLIGHTMAP_UV( bool i )
	{
		m_nLIGHTMAP_UV = i ? 1 : 0;
#ifdef _DEBUG
		m_bLIGHTMAP_UV = true;
#endif
	}
private:
	int m_nSEAMLESS_BASE;
#ifdef _DEBUG
	bool m_bSEAMLESS_BASE;
#endif
public:
	void SetSEAMLESS_BASE( int i )
	{
		Assert( i >= 0 && i <= 0 );
		m_nSEAMLESS_BASE = i;
#ifdef _DEBUG
		m_bSEAMLESS_BASE = true;
#endif
	}
	void SetSEAMLESS_BASE( bool i )
	{
		m_nSEAMLESS_BASE = i ? 1 : 0;
#ifdef _DEBUG
		m_bSEAMLESS_BASE = true;
#endif
	}
private:
	int m_nDETAILTEXTURE_UV;
#ifdef _DEBUG
	bool m_bDETAILTEXTURE_UV;
#endif
public:
	void SetDETAILTEXTURE_UV( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDETAILTEXTURE_UV = i;
#ifdef _DEBUG
		m_bDETAILTEXTURE_UV = true;
#endif
	}
	void SetDETAILTEXTURE_UV( bool i )
	{
		m_nDETAILTEXTURE_UV = i ? 1 : 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE_UV = true;
#endif
	}
private:
	int m_nENVMAPMASK_UV;
#ifdef _DEBUG
	bool m_bENVMAPMASK_UV;
#endif
public:
	void SetENVMAPMASK_UV( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENVMAPMASK_UV = i;
#ifdef _DEBUG
		m_bENVMAPMASK_UV = true;
#endif
	}
	void SetENVMAPMASK_UV( bool i )
	{
		m_nENVMAPMASK_UV = i ? 1 : 0;
#ifdef _DEBUG
		m_bENVMAPMASK_UV = true;
#endif
	}
private:
	int m_nDECAL;
#ifdef _DEBUG
	bool m_bDECAL;
#endif
public:
	void SetDECAL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDECAL = i;
#ifdef _DEBUG
		m_bDECAL = true;
#endif
	}
	void SetDECAL( bool i )
	{
		m_nDECAL = i ? 1 : 0;
#ifdef _DEBUG
		m_bDECAL = true;
#endif
	}
public:
	lux_model_vs30_Static_Index( )
	{
#ifdef _DEBUG
		m_bNORMALTEXTURE_UV = false;
#endif // _DEBUG
		m_nNORMALTEXTURE_UV = 0;
#ifdef _DEBUG
		m_bHALFLAMBERT = false;
#endif // _DEBUG
		m_nHALFLAMBERT = 0;
#ifdef _DEBUG
		m_bTREESWAY = false;
#endif // _DEBUG
		m_nTREESWAY = 0;
#ifdef _DEBUG
		m_bLIGHTMAP_UV = false;
#endif // _DEBUG
		m_nLIGHTMAP_UV = 0;
#ifdef _DEBUG
		m_bSEAMLESS_BASE = false;
#endif // _DEBUG
		m_nSEAMLESS_BASE = 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE_UV = false;
#endif // _DEBUG
		m_nDETAILTEXTURE_UV = 0;
#ifdef _DEBUG
		m_bENVMAPMASK_UV = false;
#endif // _DEBUG
		m_nENVMAPMASK_UV = 0;
#ifdef _DEBUG
		m_bDECAL = false;
#endif // _DEBUG
		m_nDECAL = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bNORMALTEXTURE_UV && m_bHALFLAMBERT && m_bTREESWAY && m_bLIGHTMAP_UV && m_bSEAMLESS_BASE && m_bDETAILTEXTURE_UV && m_bENVMAPMASK_UV && m_bDECAL;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 16 * m_nNORMALTEXTURE_UV ) + ( 32 * m_nHALFLAMBERT ) + ( 64 * m_nTREESWAY ) + ( 64 * m_nLIGHTMAP_UV ) + ( 64 * m_nSEAMLESS_BASE ) + ( 64 * m_nDETAILTEXTURE_UV ) + ( 128 * m_nENVMAPMASK_UV ) + ( 256 * m_nDECAL ) + 0;
	}
};
#define shaderStaticTest_lux_model_vs30 vsh_forgot_to_set_static_NORMALTEXTURE_UV + vsh_forgot_to_set_static_HALFLAMBERT + vsh_forgot_to_set_static_TREESWAY + vsh_forgot_to_set_static_LIGHTMAP_UV + vsh_forgot_to_set_static_SEAMLESS_BASE + vsh_forgot_to_set_static_DETAILTEXTURE_UV + vsh_forgot_to_set_static_ENVMAPMASK_UV + vsh_forgot_to_set_static_DECAL + 0
class lux_model_vs30_Dynamic_Index
{
private:
	int m_nSTATICPROPLIGHTING;
#ifdef _DEBUG
	bool m_bSTATICPROPLIGHTING;
#endif
public:
	void SetSTATICPROPLIGHTING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTATICPROPLIGHTING = i;
#ifdef _DEBUG
		m_bSTATICPROPLIGHTING = true;
#endif
	}
	void SetSTATICPROPLIGHTING( bool i )
	{
		m_nSTATICPROPLIGHTING = i ? 1 : 0;
#ifdef _DEBUG
		m_bSTATICPROPLIGHTING = true;
#endif
	}
private:
	int m_nDYNAMICPROPLIGHTING;
#ifdef _DEBUG
	bool m_bDYNAMICPROPLIGHTING;
#endif
public:
	void SetDYNAMICPROPLIGHTING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDYNAMICPROPLIGHTING = i;
#ifdef _DEBUG
		m_bDYNAMICPROPLIGHTING = true;
#endif
	}
	void SetDYNAMICPROPLIGHTING( bool i )
	{
		m_nDYNAMICPROPLIGHTING = i ? 1 : 0;
#ifdef _DEBUG
		m_bDYNAMICPROPLIGHTING = true;
#endif
	}
private:
	int m_nMORPHING;
#ifdef _DEBUG
	bool m_bMORPHING;
#endif
public:
	void SetMORPHING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nMORPHING = i;
#ifdef _DEBUG
		m_bMORPHING = true;
#endif
	}
	void SetMORPHING( bool i )
	{
		m_nMORPHING = i ? 1 : 0;
#ifdef _DEBUG
		m_bMORPHING = true;
#endif
	}
private:
	int m_nSKINNING;
#ifdef _DEBUG
	bool m_bSKINNING;
#endif
public:
	void SetSKINNING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSKINNING = i;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
	void SetSKINNING( bool i )
	{
		m_nSKINNING = i ? 1 : 0;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif
	}
public:
	lux_model_vs30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bSTATICPROPLIGHTING = false;
#endif // _DEBUG
		m_nSTATICPROPLIGHTING = 0;
#ifdef _DEBUG
		m_bDYNAMICPROPLIGHTING = false;
#endif // _DEBUG
		m_nDYNAMICPROPLIGHTING = 0;
#ifdef _DEBUG
		m_bMORPHING = false;
#endif // _DEBUG
		m_nMORPHING = 0;
#ifdef _DEBUG
		m_bSKINNING = false;
#endif // _DEBUG
		m_nSKINNING = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bSTATICPROPLIGHTING && m_bDYNAMICPROPLIGHTING && m_bMORPHING && m_bSKINNING;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nSTATICPROPLIGHTING ) + ( 2 * m_nDYNAMICPROPLIGHTING ) + ( 4 * m_nMORPHING ) + ( 8 * m_nSKINNING ) + 0;
	}
};
#define shaderDynamicTest_lux_model_vs30 vsh_forgot_to_set_dynamic_STATICPROPLIGHTING + vsh_forgot_to_set_dynamic_DYNAMICPROPLIGHTING + vsh_forgot_to_set_dynamic_MORPHING + vsh_forgot_to_set_dynamic_SKINNING + 0
