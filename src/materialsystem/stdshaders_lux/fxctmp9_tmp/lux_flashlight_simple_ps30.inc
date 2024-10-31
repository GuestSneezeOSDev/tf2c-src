#include "shaderlib/cshader.h"
class lux_flashlight_simple_ps30_Static_Index
{
private:
	int m_nFLASHLIGHTDEPTHFILTERMODE;
#ifdef _DEBUG
	bool m_bFLASHLIGHTDEPTHFILTERMODE;
#endif
public:
	void SetFLASHLIGHTDEPTHFILTERMODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nFLASHLIGHTDEPTHFILTERMODE = i;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = true;
#endif
	}
	void SetFLASHLIGHTDEPTHFILTERMODE( bool i )
	{
		m_nFLASHLIGHTDEPTHFILTERMODE = i ? 1 : 0;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = true;
#endif
	}
private:
	int m_nWORLDVERTEXTRANSITION;
#ifdef _DEBUG
	bool m_bWORLDVERTEXTRANSITION;
#endif
public:
	void SetWORLDVERTEXTRANSITION( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWORLDVERTEXTRANSITION = i;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = true;
#endif
	}
	void SetWORLDVERTEXTRANSITION( bool i )
	{
		m_nWORLDVERTEXTRANSITION = i ? 1 : 0;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = true;
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
		Assert( i >= 0 && i <= 0 );
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
	int m_nBUMPMAPPED;
#ifdef _DEBUG
	bool m_bBUMPMAPPED;
#endif
public:
	void SetBUMPMAPPED( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nBUMPMAPPED = i;
#ifdef _DEBUG
		m_bBUMPMAPPED = true;
#endif
	}
	void SetBUMPMAPPED( bool i )
	{
		m_nBUMPMAPPED = i ? 1 : 0;
#ifdef _DEBUG
		m_bBUMPMAPPED = true;
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
	int m_nPHONG;
#ifdef _DEBUG
	bool m_bPHONG;
#endif
public:
	void SetPHONG( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPHONG = i;
#ifdef _DEBUG
		m_bPHONG = true;
#endif
	}
	void SetPHONG( bool i )
	{
		m_nPHONG = i ? 1 : 0;
#ifdef _DEBUG
		m_bPHONG = true;
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
public:
	lux_flashlight_simple_ps30_Static_Index( )
	{
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = false;
#endif // _DEBUG
		m_nFLASHLIGHTDEPTHFILTERMODE = 0;
#ifdef _DEBUG
		m_bWORLDVERTEXTRANSITION = false;
#endif // _DEBUG
		m_nWORLDVERTEXTRANSITION = 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE2 = false;
#endif // _DEBUG
		m_nDETAILTEXTURE2 = 0;
#ifdef _DEBUG
		m_bBLENDTINTBYBASEALPHA = false;
#endif // _DEBUG
		m_nBLENDTINTBYBASEALPHA = 0;
#ifdef _DEBUG
		m_bDESATURATEBYBASEALPHA = false;
#endif // _DEBUG
		m_nDESATURATEBYBASEALPHA = 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE = false;
#endif // _DEBUG
		m_nDETAILTEXTURE = 0;
#ifdef _DEBUG
		m_bBUMPMAPPED = false;
#endif // _DEBUG
		m_nBUMPMAPPED = 0;
#ifdef _DEBUG
		m_bSSBUMP = false;
#endif // _DEBUG
		m_nSSBUMP = 0;
#ifdef _DEBUG
		m_bPHONG = false;
#endif // _DEBUG
		m_nPHONG = 0;
#ifdef _DEBUG
		m_bPHONGEXPONENTTEXTURE = false;
#endif // _DEBUG
		m_nPHONGEXPONENTTEXTURE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bFLASHLIGHTDEPTHFILTERMODE && m_bWORLDVERTEXTRANSITION && m_bDETAILTEXTURE2 && m_bBLENDTINTBYBASEALPHA && m_bDESATURATEBYBASEALPHA && m_bDETAILTEXTURE && m_bBUMPMAPPED && m_bSSBUMP && m_bPHONG && m_bPHONGEXPONENTTEXTURE;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 4 * m_nFLASHLIGHTDEPTHFILTERMODE ) + ( 12 * m_nWORLDVERTEXTRANSITION ) + ( 24 * m_nDETAILTEXTURE2 ) + ( 24 * m_nBLENDTINTBYBASEALPHA ) + ( 48 * m_nDESATURATEBYBASEALPHA ) + ( 96 * m_nDETAILTEXTURE ) + ( 192 * m_nBUMPMAPPED ) + ( 384 * m_nSSBUMP ) + ( 768 * m_nPHONG ) + ( 1536 * m_nPHONGEXPONENTTEXTURE ) + 0;
	}
};
#define shaderStaticTest_lux_flashlight_simple_ps30 psh_forgot_to_set_static_FLASHLIGHTDEPTHFILTERMODE + psh_forgot_to_set_static_WORLDVERTEXTRANSITION + psh_forgot_to_set_static_DETAILTEXTURE2 + psh_forgot_to_set_static_BLENDTINTBYBASEALPHA + psh_forgot_to_set_static_DESATURATEBYBASEALPHA + psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_BUMPMAPPED + psh_forgot_to_set_static_SSBUMP + psh_forgot_to_set_static_PHONG + psh_forgot_to_set_static_PHONGEXPONENTTEXTURE + 0
class lux_flashlight_simple_ps30_Dynamic_Index
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
	int m_nFLASHLIGHTSHADOWS;
#ifdef _DEBUG
	bool m_bFLASHLIGHTSHADOWS;
#endif
public:
	void SetFLASHLIGHTSHADOWS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHTSHADOWS = i;
#ifdef _DEBUG
		m_bFLASHLIGHTSHADOWS = true;
#endif
	}
	void SetFLASHLIGHTSHADOWS( bool i )
	{
		m_nFLASHLIGHTSHADOWS = i ? 1 : 0;
#ifdef _DEBUG
		m_bFLASHLIGHTSHADOWS = true;
#endif
	}
public:
	lux_flashlight_simple_ps30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
#ifdef _DEBUG
		m_bFLASHLIGHTSHADOWS = false;
#endif // _DEBUG
		m_nFLASHLIGHTSHADOWS = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bPIXELFOGTYPE && m_bFLASHLIGHTSHADOWS;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPIXELFOGTYPE ) + ( 2 * m_nFLASHLIGHTSHADOWS ) + 0;
	}
};
#define shaderDynamicTest_lux_flashlight_simple_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_FLASHLIGHTSHADOWS + 0
