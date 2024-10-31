#include "shaderlib/cshader.h"
class lux_vertexlitgeneric_phong_ps30_Static_Index
{
private:
	int m_nENVMAPMODE;
#ifdef _DEBUG
	bool m_bENVMAPMODE;
#endif
public:
	void SetENVMAPMODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nENVMAPMODE = i;
#ifdef _DEBUG
		m_bENVMAPMODE = true;
#endif
	}
	void SetENVMAPMODE( bool i )
	{
		m_nENVMAPMODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bENVMAPMODE = true;
#endif
	}
private:
	int m_nENVMAPANISOTROPY;
#ifdef _DEBUG
	bool m_bENVMAPANISOTROPY;
#endif
public:
	void SetENVMAPANISOTROPY( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENVMAPANISOTROPY = i;
#ifdef _DEBUG
		m_bENVMAPANISOTROPY = true;
#endif
	}
	void SetENVMAPANISOTROPY( bool i )
	{
		m_nENVMAPANISOTROPY = i ? 1 : 0;
#ifdef _DEBUG
		m_bENVMAPANISOTROPY = true;
#endif
	}
private:
	int m_nSELFILLUMMODE;
#ifdef _DEBUG
	bool m_bSELFILLUMMODE;
#endif
public:
	void SetSELFILLUMMODE( int i )
	{
		Assert( i >= 0 && i <= 3 );
		m_nSELFILLUMMODE = i;
#ifdef _DEBUG
		m_bSELFILLUMMODE = true;
#endif
	}
	void SetSELFILLUMMODE( bool i )
	{
		m_nSELFILLUMMODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bSELFILLUMMODE = true;
#endif
	}
private:
	int m_nBLENDTINTBYBASEALPHA;
#ifdef _DEBUG
	bool m_bBLENDTINTBYBASEALPHA;
#endif
public:
	void SetBLENDTINTBYBASEALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBLENDTINTBYBASEALPHA = i;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = true;
#endif
	}
	void SetBLENDTINTBYBASEALPHA( bool i )
	{
		m_nBLENDTINTBYBASEALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = true;
#endif
	}
private:
	int m_nDETAILTEXTURE;
#ifdef _DEBUG
	bool m_bDETAILTEXTURE;
#endif
public:
	void SetDETAILTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDETAILTEXTURE = i;
#ifdef _DEBUG
		m_bDETAILTEXTURE = true;
#endif
	}
	void SetDETAILTEXTURE( bool i )
	{
		m_nDETAILTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE = true;
#endif
	}
private:
	int m_nPHONGEXPONENTTEXTURE;
#ifdef _DEBUG
	bool m_bPHONGEXPONENTTEXTURE;
#endif
public:
	void SetPHONGEXPONENTTEXTURE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPHONGEXPONENTTEXTURE = i;
#ifdef _DEBUG
		m_bPHONGEXPONENTTEXTURE = true;
#endif
	}
	void SetPHONGEXPONENTTEXTURE( bool i )
	{
		m_nPHONGEXPONENTTEXTURE = i ? 1 : 0;
#ifdef _DEBUG
		m_bPHONGEXPONENTTEXTURE = true;
#endif
	}
private:
	int m_nDESATURATEBYBASEALPHA;
#ifdef _DEBUG
	bool m_bDESATURATEBYBASEALPHA;
#endif
public:
	void SetDESATURATEBYBASEALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDESATURATEBYBASEALPHA = i;
#ifdef _DEBUG
		m_bDESATURATEBYBASEALPHA = true;
#endif
	}
	void SetDESATURATEBYBASEALPHA( bool i )
	{
		m_nDESATURATEBYBASEALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bDESATURATEBYBASEALPHA = true;
#endif
	}
public:
	lux_vertexlitgeneric_phong_ps30_Static_Index( )
	{
#ifdef _DEBUG
		m_bENVMAPMODE = false;
#endif // _DEBUG
		m_nENVMAPMODE = 0;
#ifdef _DEBUG
		m_bENVMAPANISOTROPY = false;
#endif // _DEBUG
		m_nENVMAPANISOTROPY = 0;
#ifdef _DEBUG
		m_bSELFILLUMMODE = false;
#endif // _DEBUG
		m_nSELFILLUMMODE = 0;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = false;
#endif // _DEBUG
		m_nBLENDTINTBYBASEALPHA = 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE = false;
#endif // _DEBUG
		m_nDETAILTEXTURE = 0;
#ifdef _DEBUG
		m_bPHONGEXPONENTTEXTURE = false;
#endif // _DEBUG
		m_nPHONGEXPONENTTEXTURE = 0;
#ifdef _DEBUG
		m_bDESATURATEBYBASEALPHA = false;
#endif // _DEBUG
		m_nDESATURATEBYBASEALPHA = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bENVMAPMODE && m_bENVMAPANISOTROPY && m_bSELFILLUMMODE && m_bBLENDTINTBYBASEALPHA && m_bDETAILTEXTURE && m_bPHONGEXPONENTTEXTURE && m_bDESATURATEBYBASEALPHA;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 40 * m_nENVMAPMODE ) + ( 120 * m_nENVMAPANISOTROPY ) + ( 240 * m_nSELFILLUMMODE ) + ( 960 * m_nBLENDTINTBYBASEALPHA ) + ( 1920 * m_nDETAILTEXTURE ) + ( 3840 * m_nPHONGEXPONENTTEXTURE ) + ( 7680 * m_nDESATURATEBYBASEALPHA ) + 0;
	}
};
#define shaderStaticTest_lux_vertexlitgeneric_phong_ps30 psh_forgot_to_set_static_ENVMAPMODE + psh_forgot_to_set_static_ENVMAPANISOTROPY + psh_forgot_to_set_static_SELFILLUMMODE + psh_forgot_to_set_static_BLENDTINTBYBASEALPHA + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_PHONGEXPONENTTEXTURE + psh_forgot_to_set_static_DESATURATEBYBASEALPHA + 0
class lux_vertexlitgeneric_phong_ps30_Dynamic_Index
{
private:
	int m_nPIXELFOGTYPE;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE;
#endif
public:
	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif
	}
	void SetPIXELFOGTYPE( bool i )
	{
		m_nPIXELFOGTYPE = i ? 1 : 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif
	}
private:
	int m_nWRITEWATERFOGTODESTALPHA;
#ifdef _DEBUG
	bool m_bWRITEWATERFOGTODESTALPHA;
#endif
public:
	void SetWRITEWATERFOGTODESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITEWATERFOGTODESTALPHA = i;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = true;
#endif
	}
	void SetWRITEWATERFOGTODESTALPHA( bool i )
	{
		m_nWRITEWATERFOGTODESTALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = true;
#endif
	}
private:
	int m_nNUM_LIGHTS;
#ifdef _DEBUG
	bool m_bNUM_LIGHTS;
#endif
public:
	void SetNUM_LIGHTS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nNUM_LIGHTS = i;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif
	}
	void SetNUM_LIGHTS( bool i )
	{
		m_nNUM_LIGHTS = i ? 1 : 0;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif
	}
private:
	int m_nAMBIENT_LIGHT;
#ifdef _DEBUG
	bool m_bAMBIENT_LIGHT;
#endif
public:
	void SetAMBIENT_LIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nAMBIENT_LIGHT = i;
#ifdef _DEBUG
		m_bAMBIENT_LIGHT = true;
#endif
	}
	void SetAMBIENT_LIGHT( bool i )
	{
		m_nAMBIENT_LIGHT = i ? 1 : 0;
#ifdef _DEBUG
		m_bAMBIENT_LIGHT = true;
#endif
	}
public:
	lux_vertexlitgeneric_phong_ps30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = false;
#endif // _DEBUG
		m_nWRITEWATERFOGTODESTALPHA = 0;
#ifdef _DEBUG
		m_bNUM_LIGHTS = false;
#endif // _DEBUG
		m_nNUM_LIGHTS = 0;
#ifdef _DEBUG
		m_bAMBIENT_LIGHT = false;
#endif // _DEBUG
		m_nAMBIENT_LIGHT = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bPIXELFOGTYPE && m_bWRITEWATERFOGTODESTALPHA && m_bNUM_LIGHTS && m_bAMBIENT_LIGHT;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPIXELFOGTYPE ) + ( 2 * m_nWRITEWATERFOGTODESTALPHA ) + ( 4 * m_nNUM_LIGHTS ) + ( 20 * m_nAMBIENT_LIGHT ) + 0;
	}
};
#define shaderDynamicTest_lux_vertexlitgeneric_phong_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_NUM_LIGHTS + psh_forgot_to_set_dynamic_AMBIENT_LIGHT + 0
