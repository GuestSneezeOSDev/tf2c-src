/* NextBotManager
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBotManager.h"
#include "NextBotUtil.h"
#include "SharedFunctorUtils.h"


static ConVar nb_update_frequency ("nb_update_frequency",  ".1", FCVAR_CHEAT);
static ConVar nb_update_framelimit("nb_update_framelimit", "15", FCVAR_CHEAT);
static ConVar nb_update_maxslide  ("nb_update_maxslide",    "2", FCVAR_CHEAT);
static ConVar nb_update_debug     ("nb_update_debug",       "0", FCVAR_CHEAT);


// nb_update_frequency: how often individual NextBots should be updated;
// if there are 21 bots and nb_update_frequency is 0.1 (every 7 ticks) then
// each tick, the NextBotManager will update about 3 bots in round-robin fashion

// nb_update_framelimit: an upper limit on how much time the bot updates are
// allowed to take per frame (in milliseconds) before the NextBotManager starts
// skipping bot updates

// nb_update_maxslide: bots whose updates have been skipped (due to exceeding
// the frame time limit) are called "sliders"; the NextBotManager will attempt
// to prevent sliders from being skipped for more frames than this variable
// mandates, even if it means exceeding the frame limit by as much as double
// (when update-skipping is avoided, it's called a "blocked slide")


static const char *const debugTypeName[] = {
	"BEHAVIOR",
	"LOOK_AT",
	"PATH",
	"ANIMATION",
	"LOCOMOTION",
	"VISION",
	"HEARING",
	"EVENTS",
	"ERRORS",
	nullptr,
};


static int g_nBlockedSlides = 0;
static int g_nSlid = 0;
static int g_nRun = 0;


NextBotManager *NextBotManager::sInstance = nullptr;


void NextBotManager::Update()
{
	static int iCurFrame = -1;
	
	for (auto nextbot : this->m_NextBots) {
		nextbot->Upkeep();
	}
	
	if (this->m_NextBots.Count() != 0 && gpGlobals->framecount != iCurFrame) {
		this->m_flUpdateFrame = 0.0;
		
		int new_tickrate = Max(0, (int)((nb_update_frequency.GetFloat() / gpGlobals->interval_per_tick) + 0.5f));
		if (this->m_iTickRate != new_tickrate) {
			Msg("NextBot tickrate changed from %d (%.3fms) to %d (%.3fms)\n",
				this->m_iTickRate, this->m_iTickRate * gpGlobals->interval_per_tick,
				new_tickrate, new_tickrate * gpGlobals->interval_per_tick);
			this->m_iTickRate = new_tickrate;
		}
		
		int n_dead                = 0;
		int n_nonresponsive       = 0;
		int n_sched_next_tick     = 0;
		int n_intentional_sliders = 0;
		
		int i;
		
		if (this->m_iTickRate <= 0) {
			i = 0;
			n_sched_next_tick = this->m_NextBots.Count();
		} else {
			for (i = this->m_NextBots.Head();
				i != this->m_NextBots.InvalidIndex();
				i = this->m_NextBots.Next(i)) {
				if (this->IsBotDead(this->m_NextBots[i]->GetEntity())) {
					++n_dead;
				}
			}
			
			int frame_quota = ceil((float)(this->m_NextBots.Count() - n_dead) / (float)this->m_iTickRate);
			for (i = this->m_NextBots.Head();
				i != this->m_NextBots.InvalidIndex();
				i = this->m_NextBots.Next(i)) {
				if (frame_quota == 0) break;
				
				INextBot *nextbot = this->m_NextBots[i];
				
				/* we scheduled this bot for an update earlier, but it never
				 * got around to calling NextBotManager::ShouldUpdate at all */
				if (nextbot->IsFlaggedForUpdate()) {
					++n_nonresponsive;
					continue;
				}
				
				/* don't update bots more frequently than we need to */
				if (nextbot->GetTicksSinceUpdate() < this->m_iTickRate) {
					break;
				}
				
				if (!this->IsBotDead(nextbot->GetEntity())) {
					nextbot->FlagForUpdate(true);
					
					--frame_quota;
					++n_sched_next_tick;
				}
			}
		}
		
		if (nb_update_debug.GetBool()) {
			if (this->m_iTickRate > 0) {
				for ( ; i != this->m_NextBots.InvalidIndex();
					i = this->m_NextBots.Next(i)) {
					if (this->m_iTickRate <= this->m_NextBots[i]->GetTicksSinceUpdate()) {
						++n_intentional_sliders;
					}
				}
			}
			
			Msg("Frame %8d/tick%8d: %3d run of %3d, %3d sliders, %3d blocked slides, "
				"scheduled %3d for next tick, %3d intentional sliders, %d nonresponsive, %d dead\n",
				gpGlobals->framecount - 1, gpGlobals->tickcount - 1,
				g_nRun, this->m_NextBots.Count() - n_dead, g_nSlid, g_nBlockedSlides,
				n_sched_next_tick, n_intentional_sliders, n_nonresponsive, n_dead);
			
			g_nBlockedSlides = 0;
			g_nSlid          = 0;
			g_nRun           = 0;
		}
	}
}


void NextBotManager::OnKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info)
{
	CTakeDamageInfo copy(info);
	
	for (auto nextbot : this->m_NextBots) {
		if (nextbot->GetEntity()->IsAlive() && !nextbot->IsSelf(who)) {
			nextbot->OnOtherKilled(who, copy);
		}
	}
}

void NextBotManager::OnSound(CBaseEntity *ent, const Vector& where, KeyValues *kv)
{
	for (auto nextbot : this->m_NextBots) {
		if (nextbot->GetEntity()->IsAlive() && !nextbot->IsSelf(ent)) {
			nextbot->OnSound(ent, where, kv);
		}
	}
	
	if (ent != nullptr && this->IsDebugging(INextBot::DEBUG_HEARING)) {
		int red   = 0xff;
		int green = 0xff;
		
		/* these colors are holdovers from L4D */
		switch (ent->GetTeamNumber()) {
		case TF_TEAM_RED:
			red   = 0x00;
			green = 0xff;
			break;
		case TF_TEAM_BLUE:
			red   = 0xff;
			green = 0x00;
			break;
		}
		
		NDebugOverlay::Circle(where, Vector(1.0f, 0.0f, 0.0f), Vector(0.0f, 1.0f, 0.0f), 5.0f, red, green, 0x00, 0xff, true, 3.0f);
	}
}

void NextBotManager::OnSpokeConcept(CBaseCombatCharacter *who, const char *aiconcept, AI_Response *response)
{
	for (auto nextbot : this->m_NextBots) {
		if (nextbot->GetEntity()->IsAlive()) {
			nextbot->OnSpokeConcept(who, aiconcept, response);
		}
	}
	
	if (this->IsDebugging(INextBot::DEBUG_EVENTS)) {
		DevMsg("%3.2f: OnSpokeConcept( %s, %s )\n", gpGlobals->curtime, who->GetDebugName(), "concept.GetStringConcept()");
	}
}

void NextBotManager::OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)
{
	for (auto nextbot : this->m_NextBots) {
		if (nextbot->GetEntity()->IsAlive()) {
			nextbot->OnWeaponFired(who, weapon);
		}
	}
	
	if (this->IsDebugging(INextBot::DEBUG_EVENTS)) {
		DevMsg("%3.2f: OnWeaponFired( %s, %s )\n", gpGlobals->curtime, who->GetDebugName(), weapon->GetName());
	}
}


void NextBotManager::CollectAllBots(CUtlVector<INextBot *> *nextbots)
{
	if (nextbots != nullptr) {
		nextbots->RemoveAll();
		
		for (auto nextbot : this->m_NextBots) {
			nextbots->AddToTail(nextbot);
		}
	}
}


INextBot *NextBotManager::GetBotUnderCrosshair(CBasePlayer *player)
{
	TargetScan<CBaseCombatCharacter> scan(player, TEAM_ANY, 0.3f, 4000.0f, 50.0f);
	
	for (auto nextbot : this->m_NextBots) {
		if (!scan(nextbot->GetEntity())) break;
	}
	
	return ToNextBot(scan.GetTarget());
}


bool NextBotManager::IsDebugFilterMatch(const INextBot *nextbot) const
{
	if (this->m_DebugFilters.IsEmpty()) return true;
	
	for (const auto& filter : this->m_DebugFilters) {
		if (filter.entindex == nextbot->GetEntity()->entindex()) {
			return true;
		}
		
		if (filter.str[0] != '\0' && nextbot->IsDebugFilterMatch(filter.str)) {
			return true;
		}
		
		if (V_strnicmp(filter.str, "lookat", strlen(filter.str)) == 0) {
			CBasePlayer *host = UTIL_GetListenServerHost();
			if (host != nullptr) {
				CBaseEntity *target = host->GetObserverTarget();
				if (target != nullptr && nextbot->IsSelf(target)) {
					return true;
				}
			}
		}
		
		if (V_strnicmp(filter.str, "selected", strlen(filter.str)) == 0 &&
			this->m_SelectedBot != nullptr && nextbot->IsSelf(this->m_SelectedBot->GetEntity())) {
			return true;
		}
	}
	
	return false;
}


void NextBotManager::DebugFilterAdd(const char *str)
{
	DebugFilter filter;
	filter.entindex = -1;
	V_strcpy_safe(filter.str, str);
	
	this->m_DebugFilters.AddToTail(filter);
}

void NextBotManager::DebugFilterAdd(int entindex)
{
	DebugFilter filter;
	filter.entindex = entindex;
	filter.str[0] = '\0';
	
	this->m_DebugFilters.AddToTail(filter);
}

void NextBotManager::DebugFilterClear()
{
	this->m_DebugFilters.RemoveAll();
}

void NextBotManager::DebugFilterRemove(const char *str)
{
	FOR_EACH_VEC(this->m_DebugFilters, i) {
		if (this->m_DebugFilters[i].str[0] != '\0' && V_strnicmp(str, this->m_DebugFilters[i].str, Min(strlen(str), sizeof(this->m_DebugFilters[i].str))) == 0) {
			this->m_DebugFilters.Remove(i);
			return;
		}
	}
}

void NextBotManager::DebugFilterRemove(int entindex)
{
	FOR_EACH_VEC(this->m_DebugFilters, i) {
		if (this->m_DebugFilters[i].entindex == entindex) {
			this->m_DebugFilters.Remove(i);
			return;
		}
	}
}


bool NextBotManager::ShouldUpdate(INextBot *nextbot)
{
	if (this->m_iTickRate > 0) {
		float frame_ms = 0.0f;
		
		float frame_limit = nb_update_framelimit.GetFloat();
		
		if (nextbot->IsFlaggedForUpdate()) {
			nextbot->FlagForUpdate(false);
			
			frame_ms = 1000.0 * this->m_flUpdateFrame;
			
			if (frame_limit > 0.0f) {
				if (frame_ms < frame_limit) return true;
				
				if (nb_update_debug.GetBool()) {
					Msg("Frame %8d/tick %8d: frame out of budget (%.2fms > %.2fms)\n",
						gpGlobals->framecount, gpGlobals->tickcount, frame_ms, frame_limit);
				}
			}
		}
		
		int slide_ticks = nextbot->GetTicksSinceUpdate() - this->m_iTickRate;
		if (slide_ticks < nb_update_maxslide.GetInt() || (frame_limit != 0.0f && frame_ms >= nb_update_framelimit.GetFloat() * 2.0)) {
			if (nb_update_debug.GetBool() && slide_ticks > 0) {
				++g_nSlid;
			}
			
			return false;
		} else {
			++g_nBlockedSlides;
		}
	}
	
	return true;
}

void NextBotManager::NotifyBeginUpdate(INextBot *nextbot)
{
	if (nb_update_debug.GetBool()) {
		++g_nRun;
	}
	
	this->m_NextBots.Unlink(nextbot->GetBotId());
	this->m_NextBots.LinkToTail(nextbot->GetBotId());
	
	nextbot->ResetLastUpdateTick();
	
	this->m_flUpdateBegin = Plat_FloatTime();
}

void NextBotManager::NotifyEndUpdate(INextBot *nextbot)
{
	this->m_flUpdateFrame += (Plat_FloatTime() - this->m_flUpdateBegin);
}


int NextBotManager::Register(INextBot *nextbot)
{
	return this->m_NextBots.AddToHead(nextbot);
}

void NextBotManager::UnRegister(INextBot *nextbot)
{
	this->m_NextBots.Free(nextbot->GetBotId());
	
	if (this->m_SelectedBot == nextbot) {
		this->m_SelectedBot = nullptr;
	}
}


void NextBotManager::NotifyPathDestruction(const PathFollower *follower)
{
	for (auto nextbot : this->m_NextBots) {
		nextbot->NotifyPathDestruction(follower);
	}
}


void NextBotManager::Reset()
{
	for (auto nextbot : this->m_NextBots) {
		if (nextbot->IsRemovedOnReset()) {
			nextbot->RemoveEntity();
		}
	}
	
	this->m_SelectedBot = nullptr;
}


bool NextBotManager::IsBotDead(CBaseCombatCharacter *bot) const
{
	if (bot != nullptr) {
		if (bot->IsPlayer() && bot->m_lifeState == LIFE_DEAD) return true;
		if (bot->IsMarkedForDeletion())                       return true;
		if (bot->m_pfnThink == &CBaseEntity::SUB_Remove)      return true;
	}
	
	return false;
}


NextBotManager& TheNextBots()
{
#ifdef TF_CLASSIC
	Assert(NextBotManager::sInstance != nullptr);
#endif
	
	if (NextBotManager::sInstance != nullptr) {
		return *NextBotManager::sInstance;
	} else {
		static NextBotManager manager;
		return manager;
	}
}


static void CC_SetDebug(const CCommand& args)
{
	if (args.ArgC() <= 1) {
		Msg("Debugging stopped\n");
		TheNextBots().SetDebugMask(INextBot::DEBUG_NONE);
		return;
	}
	
	unsigned int mask = 0;
	for (int i = 1; i < args.ArgC(); ++i) {
		int j;
		for (j = 0; debugTypeName[j] != nullptr; ++j) {
			if (args[i][0] == '*') {
				mask = INextBot::DEBUG_ANY;
				break;
			}
			
			if (V_strnicmp(args[i], debugTypeName[j], strlen(args[i])) == 0) {
				mask |= (1 << j);
				break;
			}
		}
		
		if (debugTypeName[j] == nullptr) {
			Msg("Invalid debug type '%s'\n", args[i]);
		}
	}
	
	TheNextBots().SetDebugMask(mask);
}

static void CC_SetDebugFilter(const CCommand& args)
{
	if (args.ArgC() <= 1) {
		Msg("Debug filter cleared.\n");
		TheNextBots().DebugFilterClear();
		return;
	}
	
	for (int i = 1; i < args.ArgC(); ++i) {
		int entindex = V_atoi(args[i]);
		if (entindex > 0) {
			TheNextBots().DebugFilterAdd(entindex);
		} else {
			TheNextBots().DebugFilterAdd(args[i]);
		}
	}
}

static void CC_SelectBot(const CCommand& args)
{
	class SelectBotFunctor
	{
	public:
		SelectBotFunctor(CBasePlayer *player) :
			m_pPlayer(player), m_vecEyes(EyeVectorsFwd(player)) {}
		
		bool operator()(INextBot *nextbot)
		{
			if (nextbot->GetEntity()->IsAlive()) {
				Vector los = (nextbot->GetEntity()->WorldSpaceCenter() - this->m_pPlayer->EyePosition());
				float dist = VectorNormalize(los);
				
				if (DotProduct(los, this->m_vecEyes) > 0.98f && dist < this->m_flDist &&
					(!this->m_bCheckLOS || this->m_pPlayer->IsAbleToSee(nextbot->GetEntity(), CBaseCombatCharacter::DISREGARD_FOV))) {
					this->m_pNextBot = nextbot;
					this->m_flDist = dist;
				}
			}
			
			return true;
		}
		
		INextBot *GetNextBot() const { return this->m_pNextBot; }
		
	private:
		CBasePlayer *m_pPlayer;
		Vector m_vecEyes;
		INextBot *m_pNextBot = nullptr;
		float m_flDist = 1.0e14f;
		bool m_bCheckLOS = false;
	};
	
	
	CBasePlayer *host = UTIL_GetListenServerHost();
	
	if (host != nullptr) {
		SelectBotFunctor func(host);
		TheNextBots().ForEachBot(func);
		
		INextBot *nextbot = func.GetNextBot();
		
		TheNextBots().SetSelectedBot(nextbot);
		
		if (nextbot != nullptr) {
			NDebugOverlay::Circle(VecPlusZ(nextbot->GetLocomotionInterface()->GetFeet(), 5.0f),
				Vector(1.0f, 0.0f, 0.0f), Vector(0.0f, -1.0f, 0.0f), 25.0f, NB_RGBA_GREEN, false, 1.0f);
		}
	}
}

static void CC_ForceLookAt(const CCommand& args)
{
	CBasePlayer *host = UTIL_GetListenServerHost();
	INextBot *nextbot = TheNextBots().GetSelectedBot();
	
	if (nextbot != nullptr && host != nullptr) {
		nextbot->GetBodyInterface()->AimHeadTowards(host, IBody::PRI_CRITICAL, 1.0e7f, nullptr, "Aim forced");
	}
}

static void CC_WarpSelectedHere(const CCommand& args)
{
	CBasePlayer *client = UTIL_GetCommandClient();
	INextBot *nextbot = TheNextBots().GetSelectedBot();
	
	if (client != nullptr && nextbot != nullptr) {
		Vector begin = client->EyePosition();
		Vector end   = client->EyePosition() + (EyeVectorsFwd(client) * 1.0e6f);
		
		trace_t tr;
		UTIL_TraceLine(begin, end, MASK_BLOCKLOS_AND_NPCS | CONTENTS_IGNORE_NODRAW_OPAQUE, client, COLLISION_GROUP_NONE, &tr);
		
		if (tr.DidHit()) {
			Vector dest = VecPlusZ(tr.endpos, 10.0f);
			nextbot->GetEntity()->Teleport(&dest, &vec3_angle, &vec3_origin);
		}
	}
}


static ConCommand SetDebug        ("nb_debug",              &CC_SetDebug,         "Debug NextBots.  Categories are: BEHAVIOR, LOOK_AT, PATH, ANIMATION, LOCOMOTION, VISION, HEARING, EVENTS, ERRORS.", FCVAR_CHEAT);
static ConCommand SetDebugFilter  ("nb_debug_filter",       &CC_SetDebugFilter,   "Add items to the NextBot debug filter. Items can be entindexes or part of the indentifier of one or more bots.",    FCVAR_CHEAT);
static ConCommand SelectBot       ("nb_select",             &CC_SelectBot,        "Select the bot you are aiming at for further debug operations.",                                                    FCVAR_CHEAT);
static ConCommand ForceLookAt     ("nb_force_look_at",      &CC_ForceLookAt,      "Force selected bot to look at the local player's position",                                                         FCVAR_CHEAT);
static ConCommand WarpSelectedHere("nb_warp_selected_here", &CC_WarpSelectedHere, "Teleport the selected bot to your cursor position",                                                                 FCVAR_CHEAT);
