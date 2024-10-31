/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_squad.h"
#include "tf_bot.h"
#include "tf_gamerules.h"


INextBotEventResponder *CTFBotSquad::FirstContainedResponder() const
{
	if (!this->m_Members.IsEmpty()) {
		return this->m_Members.Head();
	} else {
		return nullptr;
	}
}

INextBotEventResponder *CTFBotSquad::NextContainedResponder(INextBotEventResponder *prev) const
{
	int idx = this->m_Members.Find(assert_cast<CTFBot *>(prev));
	if (idx == -1) return nullptr;
	
	return this->m_Members[idx + 1];
}


int CTFBotSquad::GetMemberCount() const
{
	int count = 0;
	
	for (CTFBot *bot : this->m_Members) {
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		++count;
	}
	
	return count;
}

void CTFBotSquad::CollectMembers(CUtlVector<CTFBot *> *members) const
{
	for (CTFBot *bot : this->m_Members) {
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		members->AddToTail(bot);
	}
}


CTFBotSquad::Iterator CTFBotSquad::GetFirstMember() const
{
	for (int i = 0; i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i];
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		return Iterator(bot, i);
	}
	
	return Iterator();
}

CTFBotSquad::Iterator CTFBotSquad::GetNextMember(const CTFBotSquad::Iterator& prev) const
{
	// NOTE: live TF2 doesn't check for invalid iterators
	if (prev.m_iIndex < 0) return Iterator();
	
	for (int i = prev.m_iIndex + 1; i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i];
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		return Iterator(bot, i);
	}
	
	return Iterator();
}


bool CTFBotSquad::IsInFormation() const
{
	for (int i = 1; i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i];
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		if (bot->IsOutOfSquadFormation() && bot->GetSquadFormationError() > 0.75f) {
			return false;
		}
	}
	
	return true;
}

float CTFBotSquad::GetMaxSquadFormationError() const
{
	float err = 0.0f;
	
	for (int i = 1; i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i];
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		err = Max(err, bot->GetSquadFormationError());
	}
	
	return err;
}

bool CTFBotSquad::ShouldSquadLeaderWaitForFormation() const
{
	for (int i = 1; i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i];
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		if (bot->GetSquadFormationError() >= 1.0f && bot->IsOutOfSquadFormation()) {
			return true;
		}
	}
	
	return false;
}


float CTFBotSquad::GetSlowestMemberIdealSpeed(bool include_leader) const
{
	float speed = FLT_MAX;
	
	for (int i = (include_leader ? 0 : 1); i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i];
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		speed = Min(speed, bot->GetPlayerClass()->GetMaxSpeed());
	}
	
	return speed;
}

float CTFBotSquad::GetSlowestMemberSpeed(bool include_leader) const
{
	float speed = FLT_MAX;
	
	for (int i = (include_leader ? 0 : 1); i < this->m_Members.Count(); ++i) {
		CTFBot *bot = this->m_Members[i];
		if (bot == nullptr)  continue;
		if (!bot->IsAlive()) continue;
		
		speed = Min(speed, bot->MaxSpeed());
	}
	
	return speed;
}


void CTFBotSquad::Join(CTFBot *bot)
{
	if (this->m_Members.IsEmpty()) {
		this->m_hLeader = bot;
	} else {
		if (TFGameRules()->IsMannVsMachineMode()) {
			bot->SetFlagTarget(nullptr);
		}
	}
	
	this->m_Members.AddToTail(bot);
}

void CTFBotSquad::Leave(CTFBot *bot)
{
	this->m_Members.FindAndRemove(bot);
	
	if (bot == this->m_hLeader) {
		this->m_hLeader = nullptr;
		
		if (this->m_bShouldPreserveSquad) {
			// NOTE: live TF2 uses CollectMembers here, which is wasteful
			this->m_hLeader = *this->GetFirstMember();
		}
	} else {
		if (TFGameRules()->IsMannVsMachineMode()) {
			CCaptureFlag *flag = bot->GetFlagToFetch();
			if (flag != nullptr) {
				bot->SetFlagTarget(flag);
			}
		}
	}
	
	if (this->GetMemberCount() == 0) {
		this->DisbandAndDeleteSquad();
	}
}


void CTFBotSquad::DisbandAndDeleteSquad()
{
	for (CTFBot *bot : this->m_Members) {
		if (bot == nullptr) continue;
		
		bot->DeleteSquad();
	}
	
	delete this;
}
