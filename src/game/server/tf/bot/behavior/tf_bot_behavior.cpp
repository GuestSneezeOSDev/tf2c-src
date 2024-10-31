/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_bot_behavior.h"
#include "tf_bot_tactical_monitor.h"
#include "tf_bot_taunt.h"
#include "tf_bot_dead.h"
#include "tf_bot_manager.h"
#include "tf_obj_sentrygun.h"
#include "tf_weapon_flamethrower.h"
#include "tf_weapon_sniperrifle.h"
#include "tf_weapon_compound_bow.h"
#include "tf_weapon_coilgun.h"
#include "tf_gamerules.h"
#include "team.h"
#include "NextBotPathFollow.h"


       ConVar tf_bot_path_lookahead_range     ("tf_bot_path_lookahead_range",         "300", FCVAR_NONE);
static ConVar tf_bot_sniper_aim_error         ("tf_bot_sniper_aim_error",            "0.01", FCVAR_CHEAT);
static ConVar tf_bot_sniper_aim_steady_rate   ("tf_bot_sniper_aim_steady_rate",        "10", FCVAR_CHEAT);
       ConVar tf_bot_debug_sniper             ("tf_bot_debug_sniper",                   "0", FCVAR_CHEAT);
static ConVar tf_bot_fire_weapon_min_time     ("tf_bot_fire_weapon_min_time",         "0.2", FCVAR_CHEAT);
static ConVar tf_bot_taunt_victim_chance      ("tf_bot_taunt_victim_chance",           "20", FCVAR_NONE);
static ConVar tf_bot_notice_backstab_chance   ("tf_bot_notice_backstab_chance",        "25", FCVAR_CHEAT);
static ConVar tf_bot_notice_backstab_min_range("tf_bot_notice_backstab_min_range",    "100", FCVAR_CHEAT);
static ConVar tf_bot_notice_backstab_max_range("tf_bot_notice_backstab_max_range",    "750", FCVAR_CHEAT);
static ConVar tf_bot_arrow_elevation_rate     ("tf_bot_arrow_elevation_rate",      "0.0001", FCVAR_CHEAT, "When firing arrows at far away targets, this is the degree/range slope to raise our aim");
static ConVar tf_bot_ballistic_elevation_rate ("tf_bot_ballistic_elevation_rate",    "0.01", FCVAR_CHEAT, "When lobbing grenades at far away targets, this is the degree/range slope to raise our aim");
static ConVar tf_bot_hitscan_range_limit      ("tf_bot_hitscan_range_limit",         "1800", FCVAR_CHEAT);
static ConVar tf_bot_always_full_reload       ("tf_bot_always_full_reload",             "0", FCVAR_CHEAT);
static ConVar tf_bot_fire_weapon_allowed      ("tf_bot_fire_weapon_allowed",            "1", FCVAR_CHEAT, "If zero, TFBots will not pull the trigger of their weapons (but will act like they did)");


class CCompareFriendFoeInfluence : public IVision::IForEachKnownEntity
{
public:
	// NOTE: live TF2 doesn't account for the actor's own influence in this functor;
	// we do account for it, by starting m_flFriend with the actor's influence on himself, rather than at zero
	CCompareFriendFoeInfluence(CTFBot *actor) : m_Actor(actor), m_flFriend(actor->GetThreatDanger(actor)) {}
	virtual ~CCompareFriendFoeInfluence() {}
	
	virtual bool Inspect(const CKnownEntity& known) override
	{
		CBaseEntity *ent = known.GetEntity();
		
		if (ent->IsAlive() && this->m_Actor->IsRangeLessThan(ent, 1250.0f)) {
			if (this->m_Actor->IsFriend(ent)) {
				this->m_flFriend += this->m_Actor->GetThreatDanger(ToBaseCombatCharacter(ent));
			} else if (this->m_Actor->IsEnemy(ent) && known.WasEverVisible() && known.GetTimeSinceLastSeen() < 3.0f) {
				if (!this->m_Actor->GetVisionInterface()->IsIgnored(ent) && UTIL_IsFacingWithinTolerance(ent, this->m_Actor->EyePosition(), 0.5f)) {
					this->m_flFoe += this->m_Actor->GetThreatDanger(ToBaseCombatCharacter(ent));
				}
			}
		}
		
		return true;
	}
	
	bool IsFoeGreater() const { return (this->m_flFoe > this->m_flFriend); }
	
private:
	CTFBot *m_Actor;
	float m_flFriend;
	float m_flFoe = 0.0f;
};


static const CKnownEntity *SelectClosestSpyToMe(CTFBot *actor, const CKnownEntity *known1, const CKnownEntity *known2)
{
	CTFPlayer *spy1 = ToTFPlayer(known1->GetEntity());
	CTFPlayer *spy2 = ToTFPlayer(known2->GetEntity());
	
	bool spy1_valid = (spy1 != nullptr && spy1->IsPlayerClass(TF_CLASS_SPY, true));
	bool spy2_valid = (spy2 != nullptr && spy2->IsPlayerClass(TF_CLASS_SPY, true));
	
	if (spy1_valid && spy2_valid) {
		float range1 = actor->GetRangeSquaredTo(spy1);
		float range2 = actor->GetRangeSquaredTo(spy2);
		
		if (range1 > range2) {
			return known2;
		} else {
			return known1;
		}
	}
	
	if (spy1_valid) return known1;
	if (spy2_valid) return known2;
	
	return nullptr;
}


ActionResult<CTFBot> CTFBotMainAction::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_hContactEntity = nullptr;
	this->m_flContactTime  = 0.0f;
	
	this->m_flSniperAim1 = 0.0f;
	this->m_flSniperAim2 = 0.0f;
	
	this->m_iDisguiseClass = TF_CLASS_UNDEFINED;
	this->m_iDisguiseTeam  = TEAM_INVALID;
	
	this->m_flSniperYawRate = 0.0f;
	this->m_flSniperLastYaw = 0.0f;
	
	this->m_bReloadingBarrage = false;
	
	Continue();
}

ActionResult<CTFBot> CTFBotMainAction::Update(CTFBot *actor, float dt)
{
	VPROF_BUDGET("CTFBotMainAction::Update", "NextBot");
	
	if (!actor->IsAlive()) {
		ChangeTo(new CTFBotDead(), "I died!");
	}
	
	if (actor->GetTeamNumber() < FIRST_GAME_TEAM) {
		Done("Not on a playing team");
	}
	
	actor->GetVisionInterface()->SetFieldOfView(actor->GetFOV());
	
	if (TFGameRules()->IsInTraining() && actor->GetTeamNumber() == TF_TEAM_BLUE) {
		actor->GiveAmmo(1000, TF_AMMO_METAL, true);
	}
	
	this->m_flSniperYawRate = abs((actor->EyeAngles().y - this->m_flSniperLastYaw) / (dt + 0.0001f));
	this->m_flSniperLastYaw = actor->EyeAngles().y;
	
	if (this->m_flSniperYawRate < tf_bot_sniper_aim_steady_rate.GetFloat()) {
		if (!this->m_itSniperRifleTrace.HasStarted()) {
			this->m_itSniperRifleTrace.Start();
		}
	} else {
		this->m_itSniperRifleTrace.Invalidate();
	}
	
	if (TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() == TF_TEAM_BLUE) {
		// TODO: MvM stuff
		Assert(false);
	}
	
	if (!actor->IsFiringWeapon() && !actor->m_Shared.InCond(TF_COND_DISGUISED) && !actor->m_Shared.InCond(TF_COND_DISGUISING) && actor->CanDisguise()) {
		if (this->m_iDisguiseClass != TF_CLASS_UNDEFINED) {
			Assert(this->m_iDisguiseTeam >= FIRST_GAME_TEAM);
			Assert(this->m_iDisguiseTeam <  GetNumberOfTeams());
			actor->m_Shared.Disguise(this->m_iDisguiseTeam, this->m_iDisguiseClass);
		} else {
			if (actor->GetSkill() >= CTFBot::HARD) {
				actor->DisguiseAsMemberOfEnemyTeam();
			} else {
				actor->DisguiseAsRandomClass();
			}
		}
	}
	
	actor->EquipRequiredWeapon();
	actor->UpdateLookingAroundForEnemies();
	
	this->FireWeaponAtEnemy(actor);
	this->Dodge(actor);

	// Try to get back onto the navmesh if we somehow ended up on a prop
	CNavArea *area = actor->GetLastKnownArea();
	if ( area == nullptr ) {
		this->FindNavMesh( actor );
	}
	
	// TODO: this needs to be weapon-specific, not class-specific
	actor->SetAutoReload(!actor->IsPlayerClass(TF_CLASS_DEMOMAN, true));

	actor->FindFrontlineIncDist();
	
	Continue();
}


// TODO: REMOVE ME
static ConVar tf_bot_puppet("tf_bot_puppet", "0", FCVAR_CHEAT);


Action<CTFBot> *CTFBotMainAction::InitialContainedAction(CTFBot *actor)
{
	if (tf_bot_puppet.GetBool()) {
		return new CTFBotPuppet();
	}
	
	return new CTFBotTacticalMonitor();
}


EventDesiredResult<CTFBot> CTFBotMainAction::OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace)
{
	if (ent != nullptr && !ent->IsSolidFlagSet(FSOLID_NOT_SOLID) && !FNullEnt(ent->edict()) && !ent->IsPlayer()) {
		this->m_hContactEntity = ent;
		this->m_flContactTime = gpGlobals->curtime;
		
		if (TFGameRules()->IsMannVsMachineMode()) {
			// TODO: MvM stuff
			Assert(false);
		}
	}
	
	Continue();
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnStuck(CTFBot *actor)
{
	if (TFGameRules()->IsMannVsMachineMode()) {
		if (actor->m_Shared.InCond(TF_COND_MVM_BOT_STUN_RADIOWAVE)) {
			Continue();
		}
		
		// TODO: MvM stuff
		Assert(false);
	}
	
	UTIL_LogPrintf("\"%s<%i><%s><%s>\" stuck (position \"%3.2f %3.2f %3.2f\") (duration \"%3.2f\") ",
		actor->GetPlayerName(), actor->GetUserID(), actor->GetNetworkIDString(), actor->GetTeam()->GetName(),
		VectorExpand(actor->GetAbsOrigin()), actor->GetLocomotionInterface()->GetStuckDuration());
	
	const PathFollower *path = actor->GetCurrentPath();
	if (path != nullptr && path->GetCurrentGoal() != nullptr) {
		UTIL_LogPrintf("   path_goal ( \"%3.2f %3.2f %3.2f\" )\n", VectorExpand(path->GetCurrentGoal()->pos));
	} else {
		UTIL_LogPrintf("   path_goal ( \"NULL\" )\n");
	}
	
	actor->GetLocomotionInterface()->Jump();
	
	if (RandomInt(0, 1) == 0) {
		actor->PressLeftButton();
	} else {
		actor->PressRightButton();
	}
	
	Continue();
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnInjured(CTFBot *actor, const CTakeDamageInfo& info)
{
	if (dynamic_cast<CBaseObject *>(info.GetInflictor()) != nullptr) {
		actor->GetVisionInterface()->AddKnownEntity(info.GetInflictor());
	} else {
		actor->GetVisionInterface()->AddKnownEntity(info.GetAttacker());
	}
	
	if (info.GetInflictor() != nullptr && actor->IsEnemy(info.GetInflictor())) {
		auto sentry = dynamic_cast<CObjectSentrygun *>(info.GetInflictor());
		if (sentry != nullptr) {
			actor->SetTargetSentry(sentry);
		}
		
		if (info.GetDamageCustom() == TF_DMG_CUSTOM_BACKSTAB) {
			actor->DelayedThreatNotice(info.GetInflictor(), 0.5f);
			
			CUtlVector<CTFPlayer *> teammates;
			CollectPlayers(&teammates, actor->GetTeamNumber(), true);
			
			float min_range = tf_bot_notice_backstab_min_range.GetFloat();
			float max_range = tf_bot_notice_backstab_max_range.GetFloat();
			
			float min_range_chance = tf_bot_notice_backstab_chance.GetFloat();
			float max_range_chance = 0.0f;
			
			for (auto teammate : teammates) {
				CTFBot *bot = ToTFBot(teammate);
				if (bot == nullptr)     continue;
				if (actor->IsSelf(bot)) continue;
				
				float chance = RemapValClamped(actor->GetRangeTo(bot), min_range, max_range, min_range_chance, max_range_chance);
				Assert(chance >= 0.0f && chance <= 100.0f);
				
				if (chance != 0.0f && chance >= RandomFloat(0.0f, 100.0f)) {
					bot->DelayedThreatNotice(info.GetInflictor(), 0.5f);
				}
			}
		}
		
		// TODO: broken anti-backburner stuff
	}
	
	Continue();
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnKilled(CTFBot *actor, const CTakeDamageInfo& info)
{
	ChangeTo(new CTFBotDead(), "I died!", SEV_CRITICAL);
}

EventDesiredResult<CTFBot> CTFBotMainAction::OnOtherKilled(CTFBot *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info)
{
	actor->GetVisionInterface()->ForgetEntity(who);
	
	CTFPlayer *player = ToTFPlayer(who);
	if (player != nullptr) {
		actor->ForgetSpy(player);
		
		if (actor->IsSelf(info.GetAttacker())) {
			if (actor->IsPlayerClass(TF_CLASS_SPY, true)) {
				this->m_iDisguiseClass = player->GetPlayerClass()->GetClassIndex();
				this->m_iDisguiseTeam  = player->GetTeamNumber();
			}

			if (!player->IsBot() && actor->IsEnemy(player) && !actor->IsSelf(player) && !actor->HasTheFlag() && !(TFGameRules()->IsMannVsMachineMode() && actor->IsMiniBoss())) {
				if (tf_bot_taunt_victim_chance.GetFloat() >= RandomFloat(0.0f, 100.0f)) {
					SuspendFor(new CTFBotTaunt(), "Taunting our victim");
				}
			}
		}
	}
	
	if (who->IsPlayer() && actor->IsFriend(who) && actor->IsEnemy(info.GetInflictor()) && actor->IsLineOfSightClear(who->WorldSpaceCenter())) {
		auto sentry = dynamic_cast<CObjectSentrygun *>(info.GetInflictor());
		if (sentry != nullptr && !actor->HasTargetSentry()) {
			actor->SetTargetSentry(sentry);
		}
	}

	// Voice responses on special kills
	if ( player && actor->IsEnemy( player ) && ( RandomInt( 1, 100 ) <= 30 ) )
	{
		if ( !actor->IsSelf( info.GetAttacker() ) )
		{
			if ( info.GetDamageCustom() == TF_DMG_CUSTOM_HEADSHOT )
			{
				// Nice shot
				TFGameRules()->VoiceCommand( actor, 2, 6 );
			}
		}
	}
	
	Continue();
}


ThreeState_t CTFBotMainAction::ShouldRetreat(const INextBot *nextbot) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	if (TheTFBots().IsMeleeOnly())								return TRS_FALSE;
	if ( TFGameRules() && TFGameRules()->IsInMedievalMode() )	return TRS_FALSE;
	if (actor->m_Shared.IsInvulnerable())						return TRS_FALSE;
	if (actor->m_Shared.IsCritBoosted())						return TRS_FALSE;
	if (actor->HasAttribute(CTFBot::IGNORE_ENEMIES))			return TRS_FALSE;
	
	if (actor->m_Shared.IsControlStunned())    return TRS_TRUE;
	if (actor->m_Shared.IsLoserStateStunned()) return TRS_TRUE;

	// Civilian in VIP stays very careful
	if ( actor->IsVIP() ){
		if ( actor->GetTimeSinceLastInjuryByAnyEnemyTeam() < 1.0f && actor->HealthFraction() <= 0.8f ) {
			return TRS_TRUE;
		}

		const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat( true );
		if ( threat != nullptr ) {
			if ( actor->IsRangeLessThan( threat->GetEntity(), 750.0f ) && actor->IsLineOfFireClear( actor->WorldSpaceCenter(), threat->GetLastKnownPosition() ) ) {
				return TRS_TRUE;
			}
		}

		CTFNavArea *lkarea = actor->GetLastKnownTFArea();
		if ( lkarea && lkarea->GetIncursionDistance( actor->GetTeamNumber() ) > actor->GetFrontlineIncDistFar() )
		{
			return TRS_TRUE;
		}
	}

	// don't retreat if healer has uber
	for ( int i = 0; i < actor->m_Shared.GetNumHealers(); ++i ) {
		CTFPlayer *healer = ToTFPlayer( actor->m_Shared.GetHealerByIndex( i ) );
		if ( healer == nullptr ) continue;

		if ( healer->MedicGetChargeLevel() > 0.90f ) return TRS_FALSE;
	}
	
	if (TFGameRules()->InSetup()) return TRS_FALSE;
	
	if (actor->m_Shared.IsStealthed()) return TRS_FALSE;
	if (actor->IsPlayerClass(TF_CLASS_SPY, true) && (actor->m_Shared.InCond(TF_COND_DISGUISED) || actor->m_Shared.InCond(TF_COND_DISGUISING))) {
		return TRS_FALSE;
	}
	
	/* determine whether to retreat based on the relative strength of nearby teammates and enemies */
	CCompareFriendFoeInfluence functor( actor );
	actor->GetVisionInterface()->ForEachKnownEntity( functor );
	return ( functor.IsFoeGreater() ? TRS_TRUE : TRS_FALSE );
}

ThreeState_t CTFBotMainAction::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	// TODO: MvM stuff
	
	return TRS_TRUE;
}

// TODO: rework this whole function, probably
Vector CTFBotMainAction::SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	if (them != nullptr) {
		// BUG: bots shooting at sentry guns won't do any special aim stuff
		// (NOTABLE: pipe/sticky/arrow vertical compensation)
		//	if (them->IsBaseObject()) {
		//		auto sentry = dynamic_cast<const CObjectSentrygun *>(them);
		//		if (sentry != nullptr) {
		//			// TODO: why not WSC? check what the difference is between this and that
		//			return (sentry->GetAbsOrigin() + (0.5f * sentry->GetViewOffset()));
		//		}
		//	}
		
		CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
		if (weapon != nullptr) {
			if (actor->GetSkill() > CTFBot::EASY) {
				// BUG: bots will attempt to shoot at the "feet" of non-sentry baseobjects;
				// we should probably have a special case for all objects to shoot at the WSC (with gravity adjustments!)
				if (actor->IsRocketLauncher(weapon)) {
					// NOTE: the time value used for predicting enemy locations once the rocket arrives
					// is based on the distance between the bot's position and the *current* position of the enemy;
					// this might be fine, but for slower projectiles we'd need some kind of iterative algorithm
					// that takes into account that the enemy will have moved in that timespan, so the range will have changed,
					// so our target location has changed, so the timespan has changed a bit, and we have to asymptotically approach the result
					
					Vector target = them->WorldSpaceCenter();
					float range = actor->GetRangeTo( const_cast<CBaseCombatCharacter *>( them ) );
					
					if (actor->GetAbsOrigin().z >= them->GetAbsOrigin().z - 30.0f) {
						/* airborne enemy */
						if (them->GetGroundEntity() == nullptr) {
							trace_t tr;
							UTIL_TraceLine(them->GetAbsOrigin(), VecPlusZ(them->GetAbsOrigin(), -200.0f), MASK_SOLID, them, COLLISION_GROUP_NONE, &tr);
							
							if (tr.DidHit()) {
								/* shoot at the ground directly under the enemy */
								target = tr.endpos;
							}
						}
						
						if (range > 150.0f) {
							// potential way to get actual hypothetical projectile speed no matter what... ugly though!
						//	auto l_get_rocket_speed = [&]{
						//		CTFProjectile_Rocket *rocket = CTFProjectile_Rocket::Create(weapon, vec3_origin, vec3_angle, actor, actor);
						//		if (rocket == nullptr) return 0.0f;
						//		
						//		float speed = rocket->GetAbsVelocity().Length();
						//		
						//		UTIL_Remove(rocket);
						//		return speed;
						//	};
							
							// TODO: find some way to ensure that this will always stay in sync with actual weapon code
							float rocket_speed = 1100.0f;
							CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, rocket_speed, mult_projectile_speed);
							
							float travel_time = range / rocket_speed;
							
							Vector predicted_feet = them->GetAbsOrigin() + (travel_time * them->GetAbsVelocity());
							if (actor->GetVisionInterface()->IsAbleToSee(predicted_feet, IVision::DISREGARD_FOV)) {
								/* shoot at where the enemy's feet are predicted to be when the rocket arrives */
								target = predicted_feet;
							} else {
								Vector predicted_head = them->EyePosition() + (travel_time * them->GetAbsVelocity());
								
								/* shoot at where the enemy's head is predicted to be when the rocket arrives */
								target = predicted_head;
							}
						} else {
							/* shoot at the enemy's head */
							target = them->EyePosition();
						}
					} else {
						if (actor->GetVisionInterface()->IsAbleToSee(them->GetAbsOrigin(), IVision::DISREGARD_FOV)) {
							/* shoot at the enemy's feet */
							target = them->GetAbsOrigin();
						} else if (actor->GetVisionInterface()->IsAbleToSee(them->WorldSpaceCenter(), IVision::DISREGARD_FOV)) {
							/* shoot at the enemy's center */
							target = them->WorldSpaceCenter();
						} else {
							/* shoot at the enemy's head */
							target = them->EyePosition();
						}
					}

					float rocket_gravity = 0.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( weapon, rocket_gravity, mod_rocket_gravity );

					if ( rocket_gravity > 0.0f ) {

						// ballistic_elevation_rate is based on grenade gravity of 0.4
						float gravScale = rocket_gravity / 0.4f;

						float angle = Min( 45.0f, range * tf_bot_ballistic_elevation_rate.GetFloat() * gravScale );

						float f_sin; float f_cos;
						FastSinCos( DEG2RAD( angle ), &f_sin, &f_cos );

						if ( f_cos != 0.0f ) {
							target.z += range * ( f_sin / f_cos );
						}
					}

					return target;
				}
				
				// TODO: ensure we also include alternative compound bow weapon IDs (if any)
				// TODO: integrate this better with ShouldAimForHeadshots type logic
				if (weapon->IsWeapon(TF_WEAPON_COMPOUND_BOW)) {
					float range = actor->GetRangeTo(const_cast<CBaseCombatCharacter *>(them));
					if (range > 150.0f) {
						// TODO: find some way to ensure that this will always stay in sync with actual weapon code
						float arrow_speed = assert_cast<CTFWeaponBaseGun *>(weapon)->GetProjectileSpeed();
						CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, arrow_speed, mult_projectile_speed);
						
						float travel_time = range / arrow_speed;
						
						Vector target;
						if (actor->GetSkill() > CTFBot::NORMAL) {
							/* shoot at where the enemy's head is predicted to be when the arrow arrives */
							target = them->EyePosition();
						} else {
							/* shoot at where the enemy's center is predicted to be when the arrow arrives */
							target = them->WorldSpaceCenter();
						}
						target += (travel_time * them->GetAbsVelocity());
						
						float angle = Min(45.0f, range * tf_bot_arrow_elevation_rate.GetFloat());
						
						float f_sin; float f_cos;
						FastSinCos(DEG2RAD(angle), &f_sin, &f_cos);
						
						if (f_cos != 0.0f) {
							target.z += range * (f_sin / f_cos);
						}
						
						return target;
					} else {
						/* shoot at the enemy's head */
						return them->EyePosition();
					}
				}
			}
			
			if (actor->ShouldAimForHeadshots(weapon)) {
				if (this->m_ctSniperAim.IsElapsed()) {
					this->m_ctSniperAim.Start(RandomFloat(0.5f, 1.5f));
					
					this->m_flSniperAim1 = RandomFloat(0.0f, tf_bot_sniper_aim_error.GetFloat());
					this->m_flSniperAim2 = RandomFloat(-M_PI_F, M_PI_F);
				}
				
				Vector target;
				switch (actor->GetSkill()) {
				case CTFBot::EASY:
					/* shoot at the enemy's center */
					target = them->WorldSpaceCenter();
					break;
				case CTFBot::NORMAL:
					/* shoot at a point 1/3 of the way down from the enemy's head to their center */
					target = (them->WorldSpaceCenter() + (2.0f * them->EyePosition())) / 3.0f;
					break;
				case CTFBot::HARD:
				case CTFBot::EXPERT:
					/* shoot at the enemy's head */
					target = them->EyePosition();
					break;
				default:
					Assert(false);
					break;
				}
				
				Vector dir = (them->GetAbsOrigin() - actor->GetAbsOrigin());
				float dist = VectorNormalize(dir);
				
				float f_sin1; float f_cos1;
				FastSinCos(this->m_flSniperAim1, &f_sin1, &f_cos1);
				
				float f_sin2; float f_cos2;
				FastSinCos(this->m_flSniperAim2, &f_sin2, &f_cos2);
				
				target.x +=  dir.y * dist * f_sin1 * f_cos2;
				target.y += -dir.x * dist * f_sin1 * f_cos2;
				target.z +=          dist * f_sin1 * f_sin2;
				
				return target;
			}
			
			// TODO: take into account the enemy's motion and the travel time of the projectile
			// (if estimating the travel time is even feasible... since grenades use vphysics and have air drag etc)
			if (actor->ShouldCompensateAimForGravity(weapon)) {
				Vector target = them->WorldSpaceCenter();
				
				float dist = (them->GetAbsOrigin() - actor->GetAbsOrigin()).Length();
				
				float angle = Min(45.0f, dist * tf_bot_ballistic_elevation_rate.GetFloat());
				
				float f_sin; float f_cos;
				FastSinCos(DEG2RAD(angle), &f_sin, &f_cos);
				
				if (f_cos != 0.0f) {
					target.z += dist * (f_sin / f_cos);
				}
				
				return target;
			}
		}
	}
	
	/* by default: shoot at the enemy's center */
	return them->WorldSpaceCenter();
}

ThreeState_t CTFBotMainAction::IsPositionAllowed(const INextBot *nextbot, const Vector& pos) const
{
	return TRS_TRUE;
}

const CKnownEntity *CTFBotMainAction::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	const CKnownEntity *threat = this->SelectMoreDangerousThreatInternal(nextbot, them, threat1, threat2);
	
	switch (actor->GetSkill()) {
	default:
	case CTFBot::EASY:
		return threat;
		
	case CTFBot::NORMAL:
		if (actor->TransientlyConsistentRandomValue(10.0f) < 0.5f) {
			return threat;
		} else {
			return this->GetHealerOfThreat(threat);
		}
		
	case CTFBot::HARD:
	case CTFBot::EXPERT:
		return this->GetHealerOfThreat(threat);
	}
}


void CTFBotMainAction::Dodge(CTFBot *actor)
{
	if (actor->GetSkill() == CTFBot::EASY)                              return;
	if (actor->HasAttribute(CTFBot::DISABLE_DODGE))                     return;
	if (actor->m_Shared.IsInvulnerable())                               return;
	if (actor->m_Shared.InCond(TF_COND_ZOOMED))                         return;
	if (actor->IsTaunting())                                            return;
	if (!actor->IsCombatWeapon())                                       return;
	if (actor->GetIntentionInterface()->ShouldHurry(actor) == TRS_TRUE) return;
	if (actor->IsPlayerClass(TF_CLASS_ENGINEER, true))                  return;
	if (actor->m_Shared.InCond(TF_COND_DISGUISED))                      return;
	if (actor->m_Shared.InCond(TF_COND_DISGUISING))                     return;
	if (actor->m_Shared.IsStealthed())                                  return;

	// don't dodge on areas we marked as PRECISE e.g. thin walkways
	CNavArea *area = actor->GetLastKnownArea();
	if ( area != nullptr && area->HasAttributes( NAV_MESH_PRECISE ) ) {
		return;
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat();
	if (threat == nullptr)            return;
	if (!threat->IsVisibleRecently()) return;
	
	// TODO: probably rework this a bit...
	CTFWeaponBase *primary = actor->GetTFWeapon_Primary();
	if (primary != nullptr && primary->IsWeapon(TF_WEAPON_COMPOUND_BOW)) {
		if (assert_cast<CTFCompoundBow *>(primary)->GetCurrentCharge() != 0.0f) {
			return;
		}
		
		// TODO: can we extend this to any "chargeable" weapon, potentially?
		// - are there cases where it would be beneficial?
		// - are there cases where it would break stuff?
		// - ITFChargeUpWeapon
	} else {
		if (!actor->IsLineOfFireClear(threat->GetLastKnownPosition())) {
			return;
		}
	}
	
	Vector eye_vec  = EyeVectorsFwd(actor);
	Vector side_dir = Vector(-eye_vec.y, eye_vec.x, 0.0f).Normalized();
	
	switch (RandomInt(0, 2)) {
	case 1: // 33% chance to go left
		if (actor->GetLocomotionInterface()->HasPotentialGap(actor->GetAbsOrigin(), actor->GetAbsOrigin() + (25.0f * side_dir))) {
			actor->PressLeftButton();
		}
		break;
	case 2: // 33% chance to go right
		if (actor->GetLocomotionInterface()->HasPotentialGap(actor->GetAbsOrigin(), actor->GetAbsOrigin() - (25.0f * side_dir))) {
			actor->PressRightButton();
		}
		break;
	}

	// jump occasionally (20% chance)
	if ( actor->TransientlyConsistentRandomValue( 1.0f ) < 0.2f )
	{
		actor->GetLocomotionInterface()->Jump();
	}
}

void CTFBotMainAction::FireWeaponAtEnemy(CTFBot *actor)
{
	if (!actor->IsAlive())                           return;
	if (actor->HasAttribute(CTFBot::IGNORE_ENEMIES)) return;
	if (actor->HasAttribute(CTFBot::SUPPRESS_FIRE))  return;
	if (actor->IsTaunting())                         return;
	if (!tf_bot_fire_weapon_allowed.GetBool())       return;
	
	CTFWeaponBase *weapon = actor->GetActiveTFWeapon();
	if (weapon == nullptr) return;
	
	if (actor->IsBarrageAndReloadWeapon() && (actor->HasAttribute(CTFBot::HOLD_FIRE_UNTIL_FULL_RELOAD) || tf_bot_always_full_reload.GetBool())) {
		if (weapon->Clip1() <= 0) {
			this->m_bReloadingBarrage = true;
		}
		
		if (this->m_bReloadingBarrage) {
			if (weapon->Clip1() < weapon->GetMaxClip1()) {
				return;
			}
			
			this->m_bReloadingBarrage = false;
		}
	}
	
	if (actor->HasAttribute(CTFBot::ALWAYS_FIRE_WEAPON)) {
		actor->PressFireButton();
		return;
	}
	
	if ( actor->IsPlayerClass( TF_CLASS_MEDIC ) && ( weapon->IsWeapon( TF_WEAPON_MEDIGUN )
#ifdef TF2C_BETA
	|| weapon->IsWeapon( TF_WEAPON_HEALLAUNCHER )
#endif
	|| weapon->IsWeapon(TF_WEAPON_PAINTBALLRIFLE)
	) ) {
		return;
	}
	
	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat( false );
	if ( threat == nullptr || threat->GetEntity() == nullptr || !threat->IsVisibleRecently() ) {
		return;
	}

	if (weapon->IsMinigun() && !actor->IsAmmoLow() && actor->GetIntentionInterface()->ShouldHurry(actor) != TRS_TRUE && actor->IsRangeLessThan(threat->GetEntity(), actor->GetMaxAttackRange())) {
		bool enemy_visible_recently = false;
		actor->ForEachEnemyTeam([&](int team){
			if (actor->GetVisionInterface()->GetTimeSinceVisible(team) < 3.0f) {
				enemy_visible_recently = true;
				return false;
			}
			
			return true;
		});
		
		if (enemy_visible_recently) {
			actor->PressAltFireButton(1.0f);
		}
	}
	
	if (!actor->IsLineOfFireClear(threat->GetEntity()->EyePosition()) &&
		!actor->IsLineOfFireClear(threat->GetEntity()->WorldSpaceCenter()) &&
		!actor->IsLineOfFireClear(threat->GetEntity()->GetAbsOrigin())) {
		return;
	}
	
	if (!TFGameRules()->IsMannVsMachineMode()) {
		CTFPlayer *player = ToTFPlayer(threat->GetEntity());
		if (player != nullptr && player->m_Shared.IsInvulnerable()) {
			if (!actor->ShouldFireAtInvulnerableEnemies(weapon)) {
				return;
			}
		}
	}
	
	if (!actor->GetIntentionInterface()->ShouldAttack(actor, threat) || TFGameRules()->InSetup()) {
		return;
	}
	
	if (weapon->IsMeleeWeapon()) {
		if (actor->IsRangeLessThan(threat->GetEntity(), 250.0f)) {
			actor->PressFireButton();
		}
		
		return;
	}
	
	// TODO: fix this ugly/flawed logic
	// (perhaps use actor->IsSniperRifle(weapon) instead of the class check, among other things...)
//	if (TFGameRules()->IsMannVsMachineMode() && !actor->IsPlayerClass(TF_CLASS_SNIPER, true) && actor->IsHitScanWeapon() && actor->IsRangeGreaterThan(threat->GetEntity(), tf_bot_hitscan_range_limit.GetFloat())) {
//		return;
//	}
	
	if (weapon->IsWeapon(TF_WEAPON_FLAMETHROWER)) {
		auto flamethrower = assert_cast<CTFFlameThrower *>(weapon);
		if (flamethrower->CanAirBlast() && actor->ShouldFireCompressionBlast()) {
			actor->PressAltFireButton();
		} else {
			if (threat->GetTimeSinceLastSeen() < 1.0f) {
				Vector threat_to_actor = (actor->GetAbsOrigin() - threat->GetEntity()->GetAbsOrigin());
				if (threat_to_actor.IsLengthLessThan(actor->GetMaxAttackRange())) {
					actor->PressFireButton(Max(1.0f, tf_bot_fire_weapon_min_time.GetFloat()));
				}
			}
		}
		
		return;
	}
	
	Vector actor_to_threat = (threat->GetEntity()->GetAbsOrigin() - actor->GetAbsOrigin());
	float dist_to_threat = actor_to_threat.Length();
	
	if (!actor->GetBodyInterface()->IsHeadAimingOnTarget()) {
		return;
	}
	
	if (dist_to_threat >= actor->GetMaxAttackRange()) {
		return;
	}

	if ( weapon->IsSniperRifle() ) {
		auto rifle = assert_cast<CTFSniperRifle *>( weapon );
		if ( !rifle->CanHeadshot() ) {
			if ( !actor->m_Shared.InCond( TF_COND_ZOOMED ) ) actor->PressAltFireButton();
			return;
		}
	}

	if ( weapon->IsWeapon( TF_WEAPON_PIPEBOMBLAUNCHER ) ) {
		auto pStickyLauncher = assert_cast<CTFPipebombLauncher *>( weapon );
		if ( pStickyLauncher )
		{
			float flChargeTime = RemapValClamped( dist_to_threat, 756.0f, 3072.0f, 0.0f, pStickyLauncher->GetChargeMaxTime() );
			if ( flChargeTime > pStickyLauncher->GetCurrentCharge() )
			{
				actor->PressFireButton( flChargeTime );
			}
			else
			{
				actor->ReleaseFireButton();
			}
		}

		return;
	}
	
	if (weapon->IsWeapon(TF_WEAPON_COMPOUND_BOW)) {
		auto huntsman = assert_cast<CTFCompoundBow *>(weapon);
		if (huntsman->GetCurrentCharge() < 0.95f || !actor->IsLineOfFireClear(threat->GetEntity())) {
			actor->PressFireButton();
		}
		
		return;
	}

	if ( weapon->IsWeapon( TF_WEAPON_COILGUN ) ) {
		auto coilgun = assert_cast<CTFCoilGun *>( weapon );

		if ( dist_to_threat < actor->GetDesiredAttackRange() ) {
			actor->PressFireButton( tf_bot_fire_weapon_min_time.GetFloat() );
			return;
		}

		if ( coilgun->HasPrimaryAmmoToFire() && coilgun->GetCurrentCharge() < 1.5f ) {
			actor->PressAltFireButton( 1.5f );
		} else {
			if ( coilgun->GetCurrentCharge() >= 1.5f && actor->IsLineOfFireClear( threat->GetEntity() ) ) {
				actor->ReleaseAltFireButton();
			}
		}

		return;
	}
	
	if (actor->IsCombatWeapon()) {
		if (actor->IsContinuousFireWeapon()) {
			actor->PressFireButton(tf_bot_fire_weapon_min_time.GetFloat());
		} else {
			if (actor->IsExplosiveProjectileWeapon()) {
				float trace_len = 1.1f * dist_to_threat;
				
				trace_t tr;
				UTIL_TraceLine(actor->EyePosition(), actor->EyePosition() + (trace_len * EyeVectorsFwd(actor)), MASK_SHOT, actor, COLLISION_GROUP_NONE, &tr);
				
				// BUG: constant 146 HU splash radius is assumed and may not be correct in all cases
				/* avoid self-splash damage */
				if (tr.fraction * trace_len < 146.0f && (tr.m_pEnt == nullptr || !tr.m_pEnt->IsCombatCharacter())) {
					return;
				}
			}
			
			actor->PressFireButton();
		}
	}
}

const CKnownEntity *CTFBotMainAction::GetHealerOfThreat(const CKnownEntity *threat) const
{
	if (threat == nullptr)              return nullptr;
	if (threat->GetEntity() == nullptr) return nullptr;
	
	CTFPlayer *player = ToTFPlayer(threat->GetEntity());
	if (player == nullptr) return threat;
	
	for (int i = 0; i < player->m_Shared.GetNumHealers(); ++i) {
		CTFPlayer *healer = ToTFPlayer(player->m_Shared.GetHealerByIndex(i));
		if (healer == nullptr) continue;
		
		const CKnownEntity *known = this->GetActor()->GetVisionInterface()->GetKnown(healer);
		if (known != nullptr && known->IsVisibleInFOVNow()) return known;
	}
	
	return threat;
}

bool CTFBotMainAction::IsImmediateThreat(const CBaseCombatCharacter *who, const CKnownEntity *threat) const
{
	CTFBot *actor = this->GetActor();
	if (actor == nullptr) return false;
	
	if (!actor->IsSelf(who))                            return false;
	if (!actor->IsEnemy(threat->GetEntity()))           return false;
	if (!threat->GetEntity()->IsAlive())                return false;
	if (!threat->IsVisibleRecently())                   return false;
	if (!actor->IsLineOfFireClear(threat->GetEntity())) return false;
	
	Vector threat_to_actor = actor->GetAbsOrigin() - threat->GetLastKnownPosition();
	float dist = threat_to_actor.Length();
	if (dist < 500.0f) return true;
	
	if (actor->IsThreatFiringAtMe(threat->GetEntity())) return true;
	
	CTFPlayer *player = ToTFPlayer(threat->GetEntity());
	if (player != nullptr) {
		if (player->IsPlayerClass(TF_CLASS_SNIPER, true)) {
			return (threat_to_actor.Normalized().Dot(EyeVectorsFwd(player)) > 0.0f);
		}
		
		if (actor->GetSkill() > CTFBot::NORMAL) {
			if (player->IsPlayerClass(TF_CLASS_MEDIC, true))    return true;
			if (player->IsPlayerClass(TF_CLASS_ENGINEER, true)) return true;
		}
	} else {
		auto sentry = dynamic_cast<CObjectSentrygun *>(threat->GetEntity());
		if (sentry != nullptr && !sentry->HasSapper() && !sentry->IsPlacing() && dist < 1650.0f) {
			return (threat_to_actor.Normalized().Dot(sentry->GetTurretVector()) > 0.8f);
		}
	}
	
	return false;
}

const CKnownEntity *CTFBotMainAction::SelectCloserThreat(CTFBot *actor, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	float range1 = actor->GetRangeSquaredTo(threat1->GetEntity());
	float range2 = actor->GetRangeSquaredTo(threat2->GetEntity());
	
	if (range1 < range2) {
		return threat1;
	} else {
		return threat2;
	}
}

const CKnownEntity *CTFBotMainAction::SelectMoreDangerousThreatInternal(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	CTFBot *actor = ToTFBot(nextbot->GetEntity());
	
	const CKnownEntity *closer = this->SelectCloserThreat(actor, threat1, threat2);
	
	// TODO: handle melee-only weapon restriction case
	
	CObjectSentrygun *sentry1 = nullptr;
	if (threat1->IsVisibleRecently() && !threat1->GetEntity()->IsPlayer()) {
		sentry1 = dynamic_cast<CObjectSentrygun *>(threat1->GetEntity());
	}
	CObjectSentrygun *sentry2 = nullptr;
	if (threat2->IsVisibleRecently() && !threat2->GetEntity()->IsPlayer()) {
		sentry2 = dynamic_cast<CObjectSentrygun *>(threat2->GetEntity());
	}
	
	bool sentry1_danger = (sentry1 != nullptr && actor->IsRangeLessThan(sentry1, sentry1->GetMaxRange()) && !sentry1->HasSapper() && !sentry1->IsPlacing());
	bool sentry2_danger = (sentry2 != nullptr && actor->IsRangeLessThan(sentry2, sentry2->GetMaxRange()) && !sentry2->HasSapper() && !sentry2->IsPlacing());
	
	if (sentry1_danger && sentry2_danger) {
		return closer;
	} else if (sentry1_danger) {
		return threat1;
	} else if (sentry2_danger) {
		return threat2;
	}
	
	bool imm1 = this->IsImmediateThreat(them, threat1);
	bool imm2 = this->IsImmediateThreat(them, threat2);
	
	if (imm1 && imm2) {
		const CKnownEntity *spy = SelectClosestSpyToMe(actor, threat1, threat2);
		if (spy != nullptr) {
			return spy;
		}
		
		bool firing1 = actor->IsThreatFiringAtMe(threat1->GetEntity());
		bool firing2 = actor->IsThreatFiringAtMe(threat2->GetEntity());
		
		if (firing1 && firing2) {
			return closer;
		} else if (firing1) {
			return threat1;
		} else if (firing2) {
			return threat2;
		} else {
			return closer;
		}
	} else if (imm1) {
		return threat1;
	} else if (imm2) {
		return threat2;
	} else {
		return closer;
	}
}


bool CTFBotMainAction::FindNavMesh( CTFBot *actor )
{
	// Make sure we're actually outside the nav mesh.
	actor->UpdateLastKnownArea();

	CNavArea *goalArea;
	if ( actor->GetLastKnownArea() || actor->IsAirborne( JumpCrouchHeight ) )
	{
		return true;
	}
	else
	{
		Vector vecStart = actor->EyePosition();
		Vector vecEnd = vecStart;
		vecEnd.z -= DeathDrop;
		trace_t trace;
		UTIL_TraceHull( vecStart, vecEnd, actor->GetPlayerMins(), actor->GetPlayerMaxs(), MASK_PLAYERSOLID, actor, COLLISION_GROUP_DEBRIS, &trace );

		// We'll be on a lift of some kind, don't panic and stay on (sd_doomsday).
		if ( trace.m_pEnt && trace.m_pEnt->GetMoveType() == MOVETYPE_PUSH )
		{
			if ( actor->IsDebugging( INextBot::DEBUG_LOCOMOTION ) ) {
				DevMsg( "%3.2f: %s On a moving platform.\n", gpGlobals->curtime, actor->GetDebugIdentifier() );
			}
			return true;
		}

		if ( actor->IsDebugging( INextBot::DEBUG_LOCOMOTION ) ) {
			DevMsg( "%3.2f: %s Trying to get back on nav mesh!\n", gpGlobals->curtime, actor->GetDebugIdentifier() );
			NDebugOverlay::Circle( VecPlusZ( actor->GetAbsOrigin(), 5.0f ), QAngle( -90.0f, 0.0f, 0.0f ), 5.0f, NB_RGBA_RED, true, 1.0f );
		}

		goalArea = TheNavMesh->GetNearestNavArea( actor->GetAbsOrigin() );
	}

	if ( goalArea )
	{
		Vector pos;
		goalArea->GetClosestPointOnArea( actor->GetAbsOrigin(), &pos );

		// Move deeper into area (in case we need to fall)
		Vector to = pos - actor->GetAbsOrigin();
		to.NormalizeInPlace();

		const float stepInDist = actor->GetBodyInterface()->GetHullWidth();
		pos = pos + ( stepInDist * to );

		if ( actor->GetLocomotionInterface()->IsPotentiallyTraversable( actor->GetAbsOrigin(), pos, ILocomotion::TRAVERSE_DEFAULT ) )
		{
			actor->GetLocomotionInterface()->Approach( pos );
		}
		else
		{
			// Can't reach nearest area. Try random movements.
			switch ( RandomInt( 0, 3 ) )
			{
				case 0:
					actor->PressLeftButton();
					break;
				case 1:
					actor->PressRightButton();
					break;
				case 2:
					actor->PressForwardButton();
					break;
				case 3:
					actor->PressBackwardButton();
					break;
			}
		}

		return false;
	}

	return false;
}


ActionResult<CTFBot> CTFBotPuppet::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	this->m_PathFollower.Initialize(actor);
	
	Continue();
}

ActionResult<CTFBot> CTFBotPuppet::Update(CTFBot *actor, float dt)
{
	Vector vel = actor->GetAbsVelocity();
	actor->DisplayDebugText(CFmtStr("|v| = % 7.1f", vel.Length()));
	actor->DisplayDebugText(CFmtStr("v.x = % 7.1f", vel.x));
	actor->DisplayDebugText(CFmtStr("v.y = % 7.1f", vel.y));
	actor->DisplayDebugText(CFmtStr("v.z = % 7.1f", vel.z));
	
	this->m_PathFollower.Update(actor);
	
	Continue();
}


static ConVar tf_bot_puppet_debug("tf_bot_puppet_debug", "0", FCVAR_CHEAT);

#define PuppetMsg(...) do { if (tf_bot_puppet_debug.GetBool()) { \
	ConColorMsg(Color(0xff, 0x00, 0xff, 0xff), ##__VA_ARGS__); } } while (false)

EventDesiredResult<CTFBot> CTFBotPuppet::OnCommandString(CTFBot *actor, const char *cmd)
{
	PuppetMsg("[OnCommandString] actor #%d \"%s\", cmd \"%s\"\n", actor->entindex(), actor->GetPlayerName(), cmd);
	
	CSplitString split(cmd, " ");
	
	PuppetMsg("[OnCommandString] CSplitString has %d elements:\n", split.Count());
	FOR_EACH_VEC(split, i) {
		PuppetMsg("  - \"%s\"\n", split[i]);
	}
	
	if (split.Count() >= 1 && FStrEq(split[0], "goto")) {
		PuppetMsg("[OnCommandString] got 'goto' instruction\n");
		
		RouteType rtype = DEFAULT_ROUTE;
		if (split.Count() >= 2) {
			if (FStrEq(split[1], "fastest")) rtype = FASTEST_ROUTE;
			if (FStrEq(split[1], "safest"))  rtype =  SAFEST_ROUTE;
			if (FStrEq(split[1], "retreat")) rtype = RETREAT_ROUTE;
		}
		
		PuppetMsg("[OnCommandString] rtype is %d\n", (int)rtype);
		
		Assert(UTIL_GetListenServerHost() != nullptr);
		CBasePlayer *host = UTIL_GetListenServerHost();
		Vector fwd = EyeVectorsFwd(host);
		
		PuppetMsg("[OnCommandString] listen server host is #%d \"%s\"\n", host->entindex(), host->GetPlayerName());
		PuppetMsg("[OnCommandString] fwd is     [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(fwd));
		PuppetMsg("[OnCommandString] eye pos is [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(host->EyePosition()));
		
		Vector from = host->EyePosition();
		PuppetMsg("[OnCommandString] trace from [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(from));
		Vector to = host->EyePosition() + (fwd * MAX_TRACE_LENGTH);
		PuppetMsg("[OnCommandString] trace to   [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(to));
		
		trace_t tr;
		UTIL_TraceLine(host->EyePosition(), host->EyePosition() + (fwd * MAX_TRACE_LENGTH), MASK_SOLID, host, COLLISION_GROUP_NONE, &tr);
		
		PuppetMsg("[OnCommandString] tr.startpos:   [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(tr.startpos));
		PuppetMsg("[OnCommandString] tr.endpos:     [ % 7.1f % 7.1f % 7.1f ]\n", VectorExpand(tr.endpos));
		PuppetMsg("[OnCommandString] tr.fraction:   %.3f\n", tr.fraction);
		PuppetMsg("[OnCommandString] tr.allsolid:   %s\n", (tr.allsolid      ? "true" : "false"));
		PuppetMsg("[OnCommandString] tr.startsolid: %s\n", (tr.startsolid    ? "true" : "false"));
		PuppetMsg("[OnCommandString] hit world:     %s\n", (tr.DidHitWorld() ? "true" : "false"));
		PuppetMsg("[OnCommandString] hit entity:    #%d\n", tr.GetEntityIndex());
		
		if (tf_bot_puppet_debug.GetBool()) {
			NDebugOverlay::Box(tr.startpos, Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f), NB_RGBA_GREEN, 10.0f);
			NDebugOverlay::EntityTextAtPosition(tr.startpos, 1, "tr.startpos", 10.0f);
			
			NDebugOverlay::Line(tr.startpos, tr.endpos, NB_RGB_YELLOW, false, 10.0f);
			
			NDebugOverlay::Box(tr.endpos, Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f), NB_RGBA_RED, 10.0f);
			NDebugOverlay::EntityTextAtPosition(tr.endpos, 1, "tr.endpos", 10.0f);
		}
		
		if (tr.DidHit()) {
			PuppetMsg("[OnCommandString] trace did hit; computing path to tr.endpos\n");
			
			if (this->m_PathFollower.Compute(actor, tr.endpos, CTFBotPathCost(actor, rtype))) {
				PuppetMsg("[OnCommandString] path computation succeeded\n");
				NDebugOverlay::Cross3D(tr.endpos, 5.0f, NB_RGB_GREEN, false, 10.0f);
			} else {
				PuppetMsg("[OnCommandString] path computation failed\n");
				NDebugOverlay::Cross3D(tr.endpos, 5.0f, NB_RGB_RED,   false, 10.0f);
			}
		}
	}
	
	Continue();
}

CON_COMMAND(sig_overlay_test, "")
{
	NDebugOverlay::EntityTextAtPosition(UTIL_GetListenServerHost()->EyePosition(), 0, "line0", 10.0f);
	NDebugOverlay::EntityTextAtPosition(UTIL_GetListenServerHost()->EyePosition(), 1, "line1", 10.0f);
	NDebugOverlay::EntityTextAtPosition(UTIL_GetListenServerHost()->EyePosition(), 2, "[line2]", 10.0f);
}
