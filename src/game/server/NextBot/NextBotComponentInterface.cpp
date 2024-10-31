/* NextBotComponentInterface
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotComponentInterface.h"
#include "NextBotInterface.h"
#include "vscript/ivscript.h"

static class INextBotComponentScriptInstanceHelper : public IScriptInstanceHelper
{
public:
	virtual bool ToString( void *p, char *pBuf, int bufSize )
	{
		INextBotComponent *pComponent = reinterpret_cast<INextBotComponent *>( p );
		if ( pComponent && pComponent->GetBot() )
		{
			V_snprintf( pBuf, bufSize, "([%d]) NextBotComponent", pComponent->GetBot()->GetBotId() );
			return true;
		}
		
		V_snprintf( pBuf, bufSize, "(Invalid NextBotComponent)" );
		return true;
	}
} g_NextBotComponentScriptInstanceHelper;
DEFINE_SCRIPT_INSTANCE_HELPER( INextBotComponent, &g_NextBotComponentScriptInstanceHelper )

// This is not correct usage but it handles templating GetScriptDesc
BEGIN_ENT_SCRIPTDESC_ROOT( INextBotComponent, "Next bot component" )
	DEFINE_SCRIPTFUNC( Reset, "Resets the internal update state" )
	DEFINE_SCRIPTFUNC( ComputeUpdateInterval, "Recomputes the component update interval" )
	DEFINE_SCRIPTFUNC( GetUpdateInterval, "Returns the component update interval" )
END_SCRIPTDESC()


INextBotComponent::INextBotComponent(INextBot *nextbot)
	: m_NextBot(nextbot), m_hScriptInstance(NULL)
{
	this->Reset();
	
	nextbot->RegisterComponent(this);
}


INextBotComponent::~INextBotComponent()
{
	if ( m_hScriptInstance )
	{
		g_pScriptVM->RemoveInstance( m_hScriptInstance );
		m_hScriptInstance = NULL;
	}
}

void INextBotComponent::Reset()
{
	this->m_flLastUpdate   = 0.0f;
	this->m_flTickInterval = gpGlobals->interval_per_tick;
}

FORCEINLINE bool INextBotComponent::ComputeUpdateInterval()
{
	if ( m_flLastUpdate == 0.0f )
	{
		m_flTickInterval = 0.033f;
		m_flLastUpdate = gpGlobals->curtime - 0.033f;
		return true;
	}

	float flTimeSinceUpdate = gpGlobals->curtime - m_flLastUpdate;
	if ( flTimeSinceUpdate > 0.0001f )
	{
		m_flTickInterval = flTimeSinceUpdate;
		m_flLastUpdate = gpGlobals->curtime;
		return true;
	}

	return false;
}

HSCRIPT INextBotComponent::GetScriptInstance()
{
	if( !m_hScriptInstance )
		m_hScriptInstance = g_pScriptVM->RegisterInstance( GetScriptDesc(), this );

	return m_hScriptInstance;
}