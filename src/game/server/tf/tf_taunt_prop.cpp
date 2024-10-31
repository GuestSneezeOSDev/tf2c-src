//=============================================================================
//
//  Purpose: Used for taunts that use an extra model.
//
//=============================================================================
#include "cbase.h"
#include "tf_taunt_prop.h"
#include "sceneentity.h"
#include "choreoscene.h"

IMPLEMENT_SERVERCLASS_ST( CTFTauntProp, DT_TFTauntProp )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_taunt_prop, CTFTauntProp );

CTFTauntProp::CTFTauntProp()
{
	UseClientSideAnimation();
}


float CTFTauntProp::PlayScene( const char *pszScene, float flDelay, AI_Response *response, IRecipientFilter *filter )
{
	// Kill the previous scene.
	if ( m_hTauntScene.Get() )
	{
		StopScriptedScene( this, m_hTauntScene );
		m_hTauntScene = NULL;
	}

	return InstancedScriptedScene( this, pszScene, &m_hTauntScene, flDelay, false, response, true, filter );
}


bool CTFTauntProp::ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event )
{
	// Only accept sequence events.
	if ( event->GetType() != CChoreoEvent::SEQUENCE )
		return false;

	return BaseClass::ProcessSceneEvent( info, scene, event );
}


bool CTFTauntProp::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	if ( event->GetType() == CChoreoEvent::SEQUENCE )
	{
		info->m_nSequence = LookupSequence( event->GetParameters() );

		if ( info->m_nSequence < 0 )
			return false;

		SetSequence( info->m_nSequence );
		m_flPlaybackRate = 1.0f;
		SetCycle( 0.0f );
		ResetSequenceInfo();

		if ( IsUsingClientSideAnimation() )
		{
			ResetClientsideFrame();
		}

		return true;
	}

	return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
}


void CTFTauntProp::UpdateOnRemove( void )
{
	if ( m_hTauntScene.Get() )
	{
		StopScriptedScene( this, m_hTauntScene );
		m_hTauntScene = NULL;
	}

	BaseClass::UpdateOnRemove();
}
