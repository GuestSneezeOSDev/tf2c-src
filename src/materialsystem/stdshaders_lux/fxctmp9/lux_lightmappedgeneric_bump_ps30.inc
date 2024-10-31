#include "shaderlib/cshader.h"
class lux_lightmappedgeneric_bump_ps30_Static_Index
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
	int m_nPCC;
#ifdef _DEBUG
	bool m_bPCC;
#endif
public:
	void SetPCC( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPCC = i;
#ifdef _DEBUG
		m_bPCC = true;
#endif
	}
	void SetPCC( bool i )
	{
		m_nPCC = i ? 1 : 0;
#ifdef _DEBUG
		m_bPCC = true;
#endif
	}
private:
	int m_nENVMAPFRESNEL;
#ifdef _DEBUG
	bool m_bENVMAPFRESNEL;
#endif
public:
	void SetENVMAPFRESNEL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nENVMAPFRESNEL = i;
#ifdef _DEBUG
		m_bENVMAPFRESNEL = true;
#endif
	}
	void SetENVMAPFRESNEL( bool i )
	{
		m_nENVMAPFRESNEL = i ? 1 : 0;
#ifdef _DEBUG
		m_bENVMAPFRESNEL = true;
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
	int m_nSSBUMP;
#ifdef _DEBUG
	bool m_bSSBUMP;
#endif
public:
	void SetSSBUMP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSSBUMP = i;
#ifdef _DEBUG
		m_bSSBUMP = true;
#endif
	}
	void SetSSBUMP( bool i )
	{
		m_nSSBUMP = i ? 1 : 0;
#ifdef _DEBUG
		m_bSSBUMP = true;
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
private:
	int m_nPARALLAX;
#ifdef _DEBUG
	bool m_bPARALLAX;
#endif
public:
	void SetPARALLAX( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPARALLAX = i;
#ifdef _DEBUG
		m_bPARALLAX = true;
#endif
	}
	void SetPARALLAX( bool i )
	{
		m_nPARALLAX = i ? 1 : 0;
#ifdef _DEBUG
		m_bPARALLAX = true;
#endif
	}
public:
	lux_lightmappedgeneric_bump_ps30_Static_Index( )
	{
#ifdef _DEBUG
		m_bENVMAPMODE = false;
#endif // _DEBUG
		m_nENVMAPMODE = 0;
#ifdef _DEBUG
		m_bPCC = false;
#endif // _DEBUG
		m_nPCC = 0;
#ifdef _DEBUG
		m_bENVMAPFRESNEL = false;
#endif // _DEBUG
		m_nENVMAPFRESNEL = 0;
#ifdef _DEBUG
		m_bENVMAPANISOTROPY = false;
#endif // _DEBUG
		m_nENVMAPANISOTROPY = 0;
#ifdef _DEBUG
		m_bSSBUMP = false;
#endif // _DEBUG
		m_nSSBUMP = 0;
#ifdef _DEBUG
		m_bSELFILLUMMODE = false;
#endif // _DEBUG
		m_nSELFILLUMMODE = 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE = false;
#endif // _DEBUG
		m_nDETAILTEXTURE = 0;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = false;
#endif // _DEBUG
		m_nBLENDTINTBYBASEALPHA = 0;
#ifdef _DEBUG
		m_bDESATURATEBYBASEALPHA = false;
#endif // _DEBUG
		m_nDESATURATEBYBASEALPHA = 0;
#ifdef _DEBUG
		m_bPARALLAX = false;
#endif // _DEBUG
		m_nPARALLAX = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bENVMAPMODE && m_bPCC && m_bENVMAPFRESNEL && m_bENVMAPANISOTROPY && m_bSSBUMP && m_bSELFILLUMMODE && m_bDETAILTEXTURE && m_bBLENDTINTBYBASEALPHA && m_bDESATURATEBYBASEALPHA && m_bPARALLAX;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nENVMAPMODE ) + ( 12 * m_nPCC ) + ( 24 * m_nENVMAPFRESNEL ) + ( 48 * m_nENVMAPANISOTROPY ) + ( 96 * m_nSSBUMP ) + ( 192 * m_nSELFILLUMMODE ) + ( 768 * m_nDETAILTEXTURE ) + ( 1536 * m_nBLENDTINTBYBASEALPHA ) + ( 3072 * m_nDESATURATEBYBASEALPHA ) + ( 6144 * m_nPARALLAX ) + 0;
	}
};
#define shaderStaticTest_lux_lightmappedgeneric_bump_ps30 psh_forgot_to_set_static_ENVMAPMODE + psh_forgot_to_set_static_PCC + psh_forgot_to_set_static_ENVMAPFRESNEL + psh_forgot_to_set_static_ENVMAPANISOTROPY + psh_forgot_to_set_static_SSBUMP + psh_forgot_to_set_static_SELFILLUMMODE + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_BLENDTINTBYBASEALPHA + psh_forgot_to_set_static_DESATURATEBYBASEALPHA + psh_forgot_to_set_static_PARALLAX + 0
class lux_lightmappedgeneric_bump_ps30_Dynamic_Index
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
public:
	lux_lightmappedgeneric_bump_ps30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = false;
#endif // _DEBUG
		m_nWRITEWATERFOGTODESTALPHA = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bPIXELFOGTYPE && m_bWRITEWATERFOGTODESTALPHA;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPIXELFOGTYPE ) + ( 2 * m_nWRITEWATERFOGTODESTALPHA ) + 0;
	}
};
#define shaderDynamicTest_lux_lightmappedgeneric_bump_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + 0
