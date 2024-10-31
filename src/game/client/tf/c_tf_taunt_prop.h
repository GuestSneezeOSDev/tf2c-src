//=============================================================================
//
//  Purpose: Used for taunts that use an extra model.
//
//=============================================================================
#ifndef C_TF_TAUNT_PROP_H
#define C_TF_TAUNT_PROP_H

#ifdef _WIN32
#pragma once
#endif

class C_TFTauntProp : public C_BaseCombatCharacter
{
public:
	DECLARE_CLASS( C_TFTauntProp, C_BaseCombatCharacter );
	DECLARE_CLIENTCLASS();

	virtual bool StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, C_BaseEntity *pTarget );
};

#endif // C_TF_TAUNT_PROP_H
