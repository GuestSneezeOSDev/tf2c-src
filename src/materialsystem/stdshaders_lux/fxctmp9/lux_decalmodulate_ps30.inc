#include "shaderlib/cshader.h"
class lux_decalmodulate_ps30_Static_Index
{
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
	int m_nVERTEXALPHA;
#ifdef _DEBUG
	bool m_bVERTEXALPHA;
#endif
public:
	void SetVERTEXALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nVERTEXALPHA = i;
#ifdef _DEBUG
		m_bVERTEXALPHA = true;
#endif
	}
	void SetVERTEXALPHA( bool i )
	{
		m_nVERTEXALPHA = i ? 1 : 0;
#ifdef _DEBUG
		m_bVERTEXALPHA = true;
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
	lux_decalmodulate_ps30_Static_Index( )
	{
#ifdef _DEBUG
		m_bDETAILTEXTURE = false;
#endif // _DEBUG
		m_nDETAILTEXTURE = 0;
#ifdef _DEBUG
		m_bVERTEXALPHA = false;
#endif // _DEBUG
		m_nVERTEXALPHA = 0;
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
		bool bAllStaticVarsDefined = m_bDETAILTEXTURE && m_bVERTEXALPHA && m_bPARALLAX;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 2 * m_nDETAILTEXTURE ) + ( 4 * m_nVERTEXALPHA ) + ( 8 * m_nPARALLAX ) + 0;
	}
};
#define shaderStaticTest_lux_decalmodulate_ps30 psh_forgot_to_set_static_DETAILTEXTURE + psh_forgot_to_set_static_VERTEXALPHA + psh_forgot_to_set_static_PARALLAX + 0
class lux_decalmodulate_ps30_Dynamic_Index
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
public:
	lux_decalmodulate_ps30_Dynamic_Index()
	{
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
#endif // _DEBUG
		m_nPIXELFOGTYPE = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllDynamicVarsDefined = m_bPIXELFOGTYPE;
		Assert( bAllDynamicVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nPIXELFOGTYPE ) + 0;
	}
};
#define shaderDynamicTest_lux_decalmodulate_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + 0
