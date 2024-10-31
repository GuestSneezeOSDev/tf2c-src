// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ( $PIXELFOGTYPE == 0 )		&& ( $WRITEWATERFOGTODESTALPHA != 0 )

#include "shaderlib/cshader.h"
class lux_sky_hdri_ps30_Static_Index
{
	unsigned int m_nMODE : 2;
	unsigned int m_nHALF : 2;
	unsigned int m_nSTRETCH : 2;
	unsigned int m_nTOSRGB : 2;
#ifdef _DEBUG
	bool m_bMODE : 1;
	bool m_bHALF : 1;
	bool m_bSTRETCH : 1;
	bool m_bTOSRGB : 1;
#endif	// _DEBUG
public:
	void SetMODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nMODE = i;
#ifdef _DEBUG
		m_bMODE = true;
#endif	// _DEBUG
	}

	void SetHALF( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nHALF = i;
#ifdef _DEBUG
		m_bHALF = true;
#endif	// _DEBUG
	}

	void SetSTRETCH( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSTRETCH = i;
#ifdef _DEBUG
		m_bSTRETCH = true;
#endif	// _DEBUG
	}

	void SetTOSRGB( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nTOSRGB = i;
#ifdef _DEBUG
		m_bTOSRGB = true;
#endif	// _DEBUG
	}

	lux_sky_hdri_ps30_Static_Index(  )
	{
		m_nMODE = 0;
		m_nHALF = 0;
		m_nSTRETCH = 0;
		m_nTOSRGB = 0;
#ifdef _DEBUG
		m_bMODE = false;
		m_bHALF = false;
		m_bSTRETCH = false;
		m_bTOSRGB = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bMODE && m_bHALF && m_bSTRETCH && m_bTOSRGB );
		return ( 8 * m_nMODE ) + ( 24 * m_nHALF ) + ( 48 * m_nSTRETCH ) + ( 96 * m_nTOSRGB ) + 0;
	}
};

#define shaderStaticTest_lux_sky_hdri_ps30 psh_forgot_to_set_static_MODE + psh_forgot_to_set_static_HALF + psh_forgot_to_set_static_STRETCH + psh_forgot_to_set_static_TOSRGB


class lux_sky_hdri_ps30_Dynamic_Index
{
	unsigned int m_nPIXELFOGTYPE : 2;
	unsigned int m_nWRITE_DEPTH_TO_DESTALPHA : 2;
	unsigned int m_nWRITEWATERFOGTODESTALPHA : 2;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE : 1;
	bool m_bWRITE_DEPTH_TO_DESTALPHA : 1;
	bool m_bWRITEWATERFOGTODESTALPHA : 1;
#endif	// _DEBUG
public:
	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif	// _DEBUG
	}

	void SetWRITE_DEPTH_TO_DESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITE_DEPTH_TO_DESTALPHA = i;
#ifdef _DEBUG
		m_bWRITE_DEPTH_TO_DESTALPHA = true;
#endif	// _DEBUG
	}

	void SetWRITEWATERFOGTODESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITEWATERFOGTODESTALPHA = i;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = true;
#endif	// _DEBUG
	}

	lux_sky_hdri_ps30_Dynamic_Index(  )
	{
		m_nPIXELFOGTYPE = 0;
		m_nWRITE_DEPTH_TO_DESTALPHA = 0;
		m_nWRITEWATERFOGTODESTALPHA = 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
		m_bWRITE_DEPTH_TO_DESTALPHA = false;
		m_bWRITEWATERFOGTODESTALPHA = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPIXELFOGTYPE && m_bWRITE_DEPTH_TO_DESTALPHA && m_bWRITEWATERFOGTODESTALPHA );
		return ( 1 * m_nPIXELFOGTYPE ) + ( 2 * m_nWRITE_DEPTH_TO_DESTALPHA ) + ( 4 * m_nWRITEWATERFOGTODESTALPHA ) + 0;
	}
};

#define shaderDynamicTest_lux_sky_hdri_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_WRITE_DEPTH_TO_DESTALPHA + psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA

