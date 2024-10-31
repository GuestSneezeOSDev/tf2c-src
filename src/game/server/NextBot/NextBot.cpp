/* NextBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "NextBot.h"
#include "NextBotManager.h"
#include "NextBotUtil.h"
#include "CRagdollMagnet.h"
#include "EntityFlame.h"
#include "vprof.h"
#include "datacache/imdlcache.h"
#include "team.h"


// TODO: ref: NextBotCombatCharacter::DoThink
// TODO: ref: NextBotGroundLocomotion::UpdatePosition
ConVar NextBotStop("nb_stop", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Stop all NextBots");


CON_COMMAND_F(nb_command, "Sends a command string to all bots", FCVAR_CHEAT)
{
	if (args.ArgC() < 2) {
		Msg("Missing command string\n");
		return;
	}
	
	TheNextBots().ForEachBot([=](INextBot *nextbot){
		nextbot->OnCommandString(args.ArgS());
		return true;
	});
}


CON_COMMAND_F(nb_delete_all, "Delete all non-player NextBot entities.", FCVAR_CHEAT)
{
	if (!UTIL_IsCommandIssuedByServerAdmin()) return;
	
	int teamnum = TEAM_ANY;
	if (args.ArgC() == 2) {
		CTeam *team_match = nullptr;
		for (auto team : g_Teams) {
			if (FStrEq(args[1], team->GetName())) {
				team_match = team;
				break;
			}
		}
		
		if (team_match == nullptr) {
			Msg("Invalid team '%s'\n", args[1]);
			return;
		}
		
		teamnum = team_match->GetTeamNumber();
	}
	
	// NOTE: live TF2 implements this as a separate NextBotDestroyer functor
	TheNextBots().ForEachBot([=](INextBot *nextbot){
		if (teamnum == TEAM_ANY || teamnum == nextbot->GetEntity()->GetTeamNumber()) {
			nextbot->RemoveEntity();
		}
		
		return true;
	});
}


CON_COMMAND_F(nb_move_to_cursor, "Tell all NextBots to move to the cursor position", FCVAR_CHEAT)
{
	if (!UTIL_IsCommandIssuedByServerAdmin()) return;
	
	CBasePlayer *host = UTIL_GetListenServerHost();
	if (host != nullptr) {
		Vector begin = host->EyePosition();
		Vector end   = host->EyePosition() + (EyeVectorsFwd(host) * 1.0e6f);
		
		trace_t tr;
		UTIL_TraceLine(begin, end, MASK_BLOCKLOS_AND_NPCS | CONTENTS_IGNORE_NODRAW_OPAQUE | CONTENTS_GRATE | CONTENTS_AUX, host, COLLISION_GROUP_NONE, &tr);
		
		if (tr.DidHit()) {
			NDebugOverlay::Cross3D(tr.endpos, 5.0f, NB_RGB_GREEN, true, 10.0f);
			
			// NOTE: live TF2 implements this as a separate functor
			// NOTE: live TF2 has a bug where if UTIL_GetListenServerHost() returns nullptr,
			// or if the raytrace doesn't hit, it will still issue OnCommandApproach with garbage data
			Vector vecWhere = tr.endpos;
			TheNextBots().ForEachBot([&](INextBot *nextbot){
				if (TheNextBots().IsDebugFilterMatch(nextbot)) {
					nextbot->OnCommandApproach(vecWhere, 0.0f);
				}
				
				return true;
			});
		}
	}
}


BEGIN_DATADESC(NextBotCombatCharacter)
	DEFINE_THINKFUNC(DoThink),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(NextBotCombatCharacter, DT_NextBot)
	// no props
END_SEND_TABLE()

BEGIN_ENT_SCRIPTDESC( NextBotCombatCharacter, CBaseCombatCharacter, "NextBot combat character" )
	DEFINE_SCRIPTFUNC( GetBotId, "Get this bot's id" )
	DEFINE_SCRIPTFUNC( FlagForUpdate, "Flag this bot for update" )
	DEFINE_SCRIPTFUNC( IsFlaggedForUpdate, "Is this bot flagged for update" )
	DEFINE_SCRIPTFUNC( GetTickLastUpdate, "Get last update tick" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetLocomotionInterface, "GetLocomotionInterface", "Get this bot's locomotion interface" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetBodyInterface, "GetBodyInterface", "Get this bot's body interface" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetIntentionInterface, "GetIntentionInterface", "Get this bot's intention interface" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptGetVisionInterface, "GetVisionInterface", "Get this bot's vision interface" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsEnemy, "IsEnemy", "Returns true if given entity is our enemy" )
	DEFINE_SCRIPTFUNC_NAMED( ScriptIsFriend, "IsFriend", "Returns true if given entity is our friend" )
	DEFINE_SCRIPTFUNC( IsImmobile, "Returns true if we haven't move in a while" )
	DEFINE_SCRIPTFUNC( GetImmobileDuration, "How long have we been immobile" )
	DEFINE_SCRIPTFUNC( ClearImmobileStatus, "Clear immobile status" )
	DEFINE_SCRIPTFUNC( GetImmobileSpeedThreshold, "Returns units/s below which this actor is considered immobile")
END_SCRIPTDESC()


#pragma message("TODO: verify that NextBotCombatCharacter has all the expected DT- and netprop- related members in the obj file!")


NextBotCombatCharacter::NextBotCombatCharacter()
{
}

void NextBotCombatCharacter::Spawn()
{
	CBaseCombatCharacter::Spawn();
	INextBot::Reset();
	
	this->CollisionProp()->SetSolid(SOLID_BBOX);
	this->CollisionProp()->AddSolidFlags(FSOLID_NOT_STANDABLE);
	
	this->SetMoveType(MOVETYPE_CUSTOM);
	this->SetCollisionGroup(COLLISION_GROUP_PLAYER);
	
	this->m_iMaxHealth = this->m_iHealth;
	this->m_takedamage = DAMAGE_YES;
	
	{
		MDLCACHE_CRITICAL_SECTION();
		CBaseCombatCharacter::InitBoneControllers();
	}
	
	UpdateLastKnownArea();
	
	SetThink(&NextBotCombatCharacter::DoThink);
	this->SetNextThink(gpGlobals->curtime);
	
	this->m_hLastAttacker = nullptr;
}

void NextBotCombatCharacter::SetModel(const char *szModelName)
{
	CBaseCombatCharacter::SetModel(szModelName);
	
	this->m_bModelChanged = true;
}

void NextBotCombatCharacter::Event_Killed(const CTakeDamageInfo& info)
{
	if (ToBaseCombatCharacter(info.GetAttacker()) != nullptr) {
		this->m_hLastAttacker = ToBaseCombatCharacter(info.GetAttacker());
	}
	
	INextBot::OnKilled(info);
	
	this->m_lifeState = LIFE_DYING;
	
	CBaseEntity *owner = this->GetOwnerEntity();
	if (owner != nullptr) {
		owner->DeathNotice(this);
	}
	
	TheNextBots().OnKilled(this, info);
}

void NextBotCombatCharacter::Touch(CBaseEntity *pOther)
{
	if (INextBot::ShouldTouch(pOther)) {
		/* implicit operator assignment */
		trace_t tr;
		tr = CBaseEntity::GetTouchTrace();
		
		if (tr.DidHit() || pOther->IsCombatCharacter()) {
			INextBot::OnContact(pOther, &tr);
		}
	}
	
	CBaseEntity::Touch(pOther);
}

Vector NextBotCombatCharacter::EyePosition()
{
	IBody *body = this->GetBodyInterface();
	if (body != nullptr) {
		return body->GetEyePosition();
	}
	
	return CBaseCombatCharacter::EyePosition();
}

void NextBotCombatCharacter::PerformCustomPhysics(Vector *pNewPosition, Vector *pNewVelocity, QAngle *pNewAngles, QAngle *pNewAngVelocity)
{
	ILocomotion *loco = this->GetLocomotionInterface();
	if (loco != nullptr) {
		this->SetGroundEntity(loco->GetGround());
	}
}


void NextBotCombatCharacter::HandleAnimEvent(animevent_t *pEvent)
{
	// don't call CBaseCombatCharacter::HandleAnimEvent
	INextBot::OnAnimationEvent(pEvent);
}

void NextBotCombatCharacter::Ignite(float flFlameLifetime, bool bNPCOnly, float flSize, bool bCalledByLevelDesigner)
{
	CBaseCombatCharacter::Ignite(flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner);
	INextBot::OnIgnite();
}


int NextBotCombatCharacter::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	if (ToBaseCombatCharacter(info.GetAttacker()) != nullptr) {
		this->m_hLastAttacker = ToBaseCombatCharacter(info.GetAttacker());
	}
	
	INextBot::OnInjured(info);
	
	return CBaseCombatCharacter::OnTakeDamage_Alive(info);
}

int NextBotCombatCharacter::OnTakeDamage_Dying(const CTakeDamageInfo& info)
{
	if (ToBaseCombatCharacter(info.GetAttacker()) != nullptr) {
		this->m_hLastAttacker = ToBaseCombatCharacter(info.GetAttacker());
	}
	
	INextBot::OnInjured(info);
	
	return CBaseCombatCharacter::OnTakeDamage_Dying(info);
}

bool NextBotCombatCharacter::BecomeRagdoll(const CTakeDamageInfo& info, const Vector& forceVector)
{
	Vector modVector = forceVector;
	
	CRagdollMagnet *magnet = CRagdollMagnet::FindBestMagnet(this);
	if (magnet != nullptr) {
		modVector += magnet->GetForceVector(this);
	}
	
	this->EmitSound("BaseCombatCharacter.StopWeaponSounds");
	
	return CBaseCombatCharacter::BecomeRagdoll(info, modVector);
}

bool NextBotCombatCharacter::IsAreaTraversable(const CNavArea *area) const
{
	if (area == nullptr) return false;
	
	ILocomotion *loco = this->GetLocomotionInterface();
	if (loco != nullptr && !loco->IsAreaTraversable(area)) {
		return false;
	}
	
	return CBaseCombatCharacter::IsAreaTraversable(area);
}


void NextBotCombatCharacter::OnNavAreaChanged(CNavArea *area1, CNavArea *area2)
{
	FOR_EACH_RESPONDER(OnNavAreaChanged, area1, area2);
}


void NextBotCombatCharacter::Ignite(float flFlameLifetime, CBaseEntity *pEntity)
{
	/* check for FL_ONFIRE */
	if (this->IsOnFire()) return;
	
	CEntityFlame *flame = CEntityFlame::Create(this);
	if (flame != nullptr) {
		flame->SetLifetime(flFlameLifetime);
		
		this->AddFlag(FL_ONFIRE);
		this->SetEffectEntity(flame);
	}
	
	this->m_OnIgnite.FireOutput(this, this);
	INextBot::OnIgnite();
}

bool NextBotCombatCharacter::IsUseableEntity(CBaseEntity *pEntity, unsigned int requiredCaps)
{
	if (pEntity == nullptr) return false;
	
	unsigned int caps = pEntity->ObjectCaps() & (FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE);
	if (caps == 0) return false;
	
	return (requiredCaps == (requiredCaps & caps));
}

CBaseCombatCharacter *NextBotCombatCharacter::GetLastAttacker() const
{
	return this->m_hLastAttacker;
}


void NextBotCombatCharacter::DoThink()
{
	VPROF_BUDGET("NextBotCombatCharacter::DoThink", "NextBot");
	
	this->SetNextThink(gpGlobals->curtime);
	
	if (INextBot::BeginUpdate()) {
		if (this->m_bModelChanged) {
			this->m_bModelChanged = false;
			
			INextBot::OnModelChanged();
			FOR_EACH_RESPONDER(OnModelChanged);
		}
		
		CBaseCombatCharacter::UpdateLastKnownArea();
		
		if (!NextBotStop.GetBool() && (this->GetFlags() & FL_FROZEN) == 0) {
			INextBot::Update();
		}
		
		INextBot::EndUpdate();
	}
}

void NextBotCombatCharacter::UseEntity(CBaseEntity *pEntity, USE_TYPE useType)
{
	if (this->IsUseableEntity(pEntity, 0)) {
		variant_t emptyVariant;
		pEntity->AcceptInput("Use", this, this, emptyVariant, useType);
	}
}
