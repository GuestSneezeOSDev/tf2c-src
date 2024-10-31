/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_sentrygun.h"


LINK_ENTITY_TO_CLASS(bot_hint_sentrygun, CTFBotHintSentrygun);


BEGIN_DATADESC(CTFBotHintSentrygun)
	
	DEFINE_KEYFIELD(m_isSticky, FIELD_BOOLEAN, "sticky"),
	
	DEFINE_OUTPUT(m_outputOnSentryGunDestroyed, "OnSentryGunDestroyed"),
	
END_DATADESC()


bool CTFBotHintSentrygun::IsAvailableForSelection(CTFPlayer *player) const
{
	if ( GetOwnerPlayer() && GetOwnerPlayer() != player )
		return false;
	
	return true;
}


void CTFBotHintSentrygun::OnSentryGunDestroyed(CBaseEntity *ent)
{
	this->m_outputOnSentryGunDestroyed.FireOutput(ent, ent);
}


/* from tf.fgd:

@PointClass base(Targetname,Parentname,BaseObject,Angles,EnableDisable) studio("models/buildables/sentry3.mdl") = bot_hint_sentrygun : 
	"TF2 Sentry Gun Placement Hint for Bots" 
[
	sequence(integer) : "Sequence" : 5 : "Default animation sequence for the model to be playing after spawning."

	sticky(choices) : "Sticky" : 0 : "If set, Engineer bots using this hint will stay here instead of destroying their equipment and moving up as the scenario changes." =
	[
		0 : "No"
		1 : "Yes"
	]
	output OnSentryGunDestroyed(void) : "Fired when a sentry gun built on this hint is destroyed."
]

*/
