//=============================================================================
//
//  Purpose: Used for taunts that use an extra model.
//
//=============================================================================
#include "cbase.h"
#include "c_tf_taunt_prop.h"
#include "choreoscene.h"

IMPLEMENT_CLIENTCLASS_DT( C_TFTauntProp, DT_TFTauntProp, CTFTauntProp )
END_RECV_TABLE()


bool C_TFTauntProp::StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget )
{
	if ( event->GetType() == CChoreoEvent::SEQUENCE )
	{
		info->m_nSequence = LookupSequence( event->GetParameters() );

		if ( info->m_nSequence < 0 )
			return false;

		SetSequence( info->m_nSequence );
		m_flPlaybackRate = 1.0f;

		// Fix up for looping taunts.
		SetCycle( scene->GetTime() / scene->GetDuration() );

		return true;
	}

	return BaseClass::StartSceneEvent( info, scene, event, actor, pTarget );
}
