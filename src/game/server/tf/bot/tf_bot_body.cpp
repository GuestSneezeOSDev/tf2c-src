/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_body.h"
#include "tf_bot.h"
#include "tf_gamerules.h"


CTFBotBody::CTFBotBody(INextBot *nextbot) :
	PlayerBody(nextbot), m_pTFBot(assert_cast<CTFBot *>(nextbot)) {}


float CTFBotBody::GetHeadAimTrackingInterval() const
{
	float trackInterval = 0.00f;

	if (TFGameRules()->IsMannVsMachineMode() && this->GetTFBot()->IsPlayerClass(TF_CLASS_SPY, true)) {
		return 0.25f;
	}
	
	switch ( GetTFBot()->GetSkill() ) {
	case CTFBot::EASY:   
		trackInterval = 1.00f; 
		break;
	case CTFBot::NORMAL: 
		trackInterval = 0.25f;
		break;
	case CTFBot::HARD:   
		trackInterval = 0.10f;
		break;
	case CTFBot::EXPERT: 
		trackInterval = 0.05f;
		break;
	default:             
		trackInterval = 0.00f;
		break;
	}

	if ( GetTFBot()->m_Shared.InCond( TF_COND_TRANQUILIZED ) )
	{
		trackInterval += 0.5f;
	}

	return trackInterval;
}

float CTFBotBody::GetMaxHeadAngularVelocity() const
{
	if ( GetTFBot()->IsTaunting() )
	{
		return 0.0f;
	}

	return 1000.0f;
}
