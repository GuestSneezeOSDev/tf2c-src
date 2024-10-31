#include "shaderlib/cshader.h"
class lux_worldvertextransition_simple_ps30_Static_Index
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
	int m_nDETAILTEXTURE2;
#ifdef _DEBUG
	bool m_bDETAILTEXTURE2;
#endif
public:
	void SetDETAILTEXTURE2( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDETAILTEXTURE2 = i;
#ifdef _DEBUG
		m_bDETAILTEXTURE2 = true;
#endif
	}
	void SetDETAILTEXTURE2( bool i )
	{
		m_nDETAILTEXTURE2 = i ? 1 : 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE2 = true;
#endif
	}
public:
	lux_worldvertextransition_simple_ps30_Static_Index( )
	{
#ifdef _DEBUG
		m_bENVMAPMODE = false;
#endif // _DEBUG
		m_nENVMAPMODE = 0;
#ifdef _DEBUG
		m_bENVMAPFRESNEL = false;
#endif // _DEBUG
		m_nENVMAPFRESNEL = 0;
#ifdef _DEBUG
		m_bPCC = false;
#endif // _DEBUG
		m_nPCC = 0;
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
		m_bDETAILTEXTURE2 = false;
#endif // _DEBUG
		m_nDETAILTEXTURE2 = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bENVMAPMODE && m_bENVMAPFRESNEL && m_bPCC && m_bSELFILLUMMODE && m_bBLENDTINTBYBASEALPHA && m_bDETAILTEXTURE && m_bDETAILTEXTURE2;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nENVMAPMODE ) + ( 12 * m_nENVMAPFRESNEL ) + ( 24 * m_nPCC ) + ( 48 * m_nSELFILLUMMODE ) + ( 192 * m_nBLENDTINTBYBASEALPHA ) + ( 384 * m_nDETAILTEXTURE ) + ( 768 * m_nDETAILTEXTURE2 ) + 0;
	}
};
#define shaderStaticTest_lux_worldvertextransition_simple_ps30 psh_forgot_to_set_static_ENVMAPMODE + psh_forgot_to_set_static_ENVMAPFRESNEL + psh_forgot_to_set_static_PCC + psh_forgot_to_set_static_SELFILLUMMODE + psh_forgot_to_set_static_BLENDTINTBYBASEALPHA + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_DETAILTEXTURE2 + 0
class lux_worldvertextransition_simple_ps30_Dynamic_Index
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
	lux_worldvertextransition_simple_ps30_Dynamic_Index()
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
#define shaderDynamicTest_lux_worldvertextransition_simple_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + 0
