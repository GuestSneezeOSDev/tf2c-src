#include "shaderlib/cshader.h"
class lux_displacement_vs30_Static_Index
{
private:
	int m_nSEAMLESS_BASE;
#ifdef _DEBUG
	bool m_bSEAMLESS_BASE;
#endif
public:
	void SetSEAMLESS_BASE( int i )
	{
		Assert( i >= 0 && i <= 1 );
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
		Assert( i >= 0 && i <= 2 );
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
	int m_nNORMALTEXTURE_UV;
#ifdef _DEBUG
	bool m_bNORMALTEXTURE_UV;
#endif
public:
	void SetNORMALTEXTURE_UV( int i )
	{
		Assert( i >= 0 && i <= 2 );
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
	int m_nENVMAPMASK_UV;
#ifdef _DEBUG
	bool m_bENVMAPMASK_UV;
#endif
public:
	void SetENVMAPMASK_UV( int i )
	{
		Assert( i >= 0 && i <= 2 );
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
	int m_nBLENDMODULATE_UV;
#ifdef _DEBUG
	bool m_bBLENDMODULATE_UV;
#endif
public:
	void SetBLENDMODULATE_UV( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nBLENDMODULATE_UV = i;
#ifdef _DEBUG
		m_bBLENDMODULATE_UV = true;
#endif
	}
	void SetBLENDMODULATE_UV( bool i )
	{
		m_nBLENDMODULATE_UV = i ? 1 : 0;
#ifdef _DEBUG
		m_bBLENDMODULATE_UV = true;
#endif
	}
private:
	int m_nSEAMLESS_SECONDARY;
#ifdef _DEBUG
	bool m_bSEAMLESS_SECONDARY;
#endif
public:
	void SetSEAMLESS_SECONDARY( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSEAMLESS_SECONDARY = i;
#ifdef _DEBUG
		m_bSEAMLESS_SECONDARY = true;
#endif
	}
	void SetSEAMLESS_SECONDARY( bool i )
	{
		m_nSEAMLESS_SECONDARY = i ? 1 : 0;
#ifdef _DEBUG
		m_bSEAMLESS_SECONDARY = true;
#endif
	}
public:
	lux_displacement_vs30_Static_Index( )
	{
#ifdef _DEBUG
		m_bSEAMLESS_BASE = false;
#endif // _DEBUG
		m_nSEAMLESS_BASE = 0;
#ifdef _DEBUG
		m_bDETAILTEXTURE_UV = false;
#endif // _DEBUG
		m_nDETAILTEXTURE_UV = 0;
#ifdef _DEBUG
		m_bNORMALTEXTURE_UV = false;
#endif // _DEBUG
		m_nNORMALTEXTURE_UV = 0;
#ifdef _DEBUG
		m_bENVMAPMASK_UV = false;
#endif // _DEBUG
		m_nENVMAPMASK_UV = 0;
#ifdef _DEBUG
		m_bBLENDMODULATE_UV = false;
#endif // _DEBUG
		m_nBLENDMODULATE_UV = 0;
#ifdef _DEBUG
		m_bSEAMLESS_SECONDARY = false;
#endif // _DEBUG
		m_nSEAMLESS_SECONDARY = 0;
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
		bool bAllStaticVarsDefined = m_bSEAMLESS_BASE && m_bDETAILTEXTURE_UV && m_bNORMALTEXTURE_UV && m_bENVMAPMASK_UV && m_bBLENDMODULATE_UV && m_bSEAMLESS_SECONDARY;
		Assert( bAllStaticVarsDefined );
#endif // _DEBUG
		return ( 1 * m_nSEAMLESS_BASE ) + ( 2 * m_nDETAILTEXTURE_UV ) + ( 6 * m_nNORMALTEXTURE_UV ) + ( 18 * m_nENVMAPMASK_UV ) + ( 54 * m_nBLENDMODULATE_UV ) + ( 162 * m_nSEAMLESS_SECONDARY ) + 0;
	}
};
#define shaderStaticTest_lux_displacement_vs30 vsh_forgot_to_set_static_SEAMLESS_BASE + vsh_forgot_to_set_static_DETAILTEXTURE_UV + vsh_forgot_to_set_static_NORMALTEXTURE_UV + vsh_forgot_to_set_static_ENVMAPMASK_UV + vsh_forgot_to_set_static_BLENDMODULATE_UV + vsh_forgot_to_set_static_SEAMLESS_SECONDARY + 0
class lux_displacement_vs30_Dynamic_Index
{
public:
	lux_displacement_vs30_Dynamic_Index()
	{
	}
	int GetIndex()
	{
		// Asserts to make sure that we aren't using any skipped combinations.
		// Asserts to make sure that we are setting all of the combination vars.
#ifdef _DEBUG
#endif // _DEBUG
		return 0;
	}
};
#define shaderDynamicTest_lux_displacement_vs30 0
