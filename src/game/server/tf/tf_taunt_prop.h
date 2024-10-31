//=============================================================================
//
//  Purpose: Used for taunts that use an extra model.
//
//=============================================================================
#ifndef TF_TAUNT_PROP_H
#define TF_TAUNT_PROP_H

#ifdef _WIN32
#pragma once
#endif

class CTFTauntProp : public CBaseCombatCharacter
{
public:
	DECLARE_CLASS( CTFTauntProp, CBaseCombatCharacter );
	DECLARE_SERVERCLASS();

	CTFTauntProp();

	virtual float PlayScene( const char *pszScene, float flDelay = 0.0f, AI_Response *response = NULL, IRecipientFilter *filter = NULL );
	virtual bool ProcessSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event );
	virtual	bool StartSceneEvent( CSceneEventInfo *info, CChoreoScene *scene, CChoreoEvent *event, CChoreoActor *actor, CBaseEntity *pTarget );
	virtual void UpdateOnRemove( void );

private:
	EHANDLE m_hTauntScene;
};

#endif // TF_TAUNT_PROP_H
