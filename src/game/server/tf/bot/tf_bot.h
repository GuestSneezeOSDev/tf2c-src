/* TFBot
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#ifndef TF_BOT_H
#define TF_BOT_H
#ifdef _WIN32
#pragma once
#endif


#include <initializer_list>
#include <utility>
#include "NextBotPlayer.h"
#include "NextBotBehavior.h"
#include "NextBotPath.h"
#include "tf_bot_locomotion.h"
#include "tf_bot_body.h"
#include "tf_bot_vision.h"
#include "tf_bot_squad.h"
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#include "tf_team.h"
#include "tf_gamerules.h"
#include "GameEventListener.h"
#include "utlstring.h"
#include "utlstack.h"
#include "tf_shareddefs.h"


class CCaptureZone;
class CVIPSafetyZone;
class CFlagDetectionZone;
class CTeamControlPoint;
class CObjectSentrygun;
class CTFNavArea;
class CTFBotProxy;
class CTFBotGenerator;


#define BIT(x) (1ULL << (x))


// TODO: do a full search of live TF2 for references to this stuff:
// - FL_FAKECLIENT
// - vcall IsFakeClient
// - vcall IsBot
// - vcall GetBotType
// - vcall IsBotOfType
// - ToTFBot()
// - dynamic_cast with `typeinfo for'CTFBot


class CTFBot final : public NextBotPlayer<CTFPlayer>, public CGameEventListener
{
public:
	enum
	{
		BOT_TYPE = 1337,
	};
	
	enum DifficultyType
	{
		UNDEFINED_SKILL = -1,
		EASY            =  0,
		NORMAL          =  1,
		HARD            =  2,
		EXPERT          =  3,
		NUM_DIFFICULTY_LEVELS
	};
	
	enum AttributeType
	{
		NO_ATTRIBUTES               = 0,
		
		REMOVE_ON_DEATH             = BIT( 0),
		AGGRESSIVE                  = BIT( 1),
		PROXY_MANAGED               = BIT( 2),
		SUPPRESS_FIRE               = BIT( 3),
		DISABLE_DODGE               = BIT( 4),
		BECOME_SPECTATOR_ON_DEATH   = BIT( 5),
		QUOTA_MANAGED               = BIT( 6),
		RETAIN_BUILDINGS            = BIT( 7),
		SPAWN_WITH_FULL_CHARGE      = BIT( 8),
		ALWAYS_CRIT                 = BIT( 9),
		IGNORE_ENEMIES              = BIT(10),
		HOLD_FIRE_UNTIL_FULL_RELOAD = BIT(11),
		DEFEND_CLOSEST_POINT        = BIT(12),
		ALWAYS_FIRE_WEAPON          = BIT(13),
		TELEPORT_TO_HINT            = BIT(14),
		MINI_BOSS                   = BIT(15),
		USE_BOSS_HEALTH_BAR         = BIT(16),
		IGNORE_FLAG                 = BIT(17),
		AUTO_JUMP                   = BIT(18),
		AIR_CHARGE_ONLY             = BIT(19),
		VACCINATOR_BULLETS          = BIT(20),
		VACCINATOR_BLAST            = BIT(21),
		VACCINATOR_FIRE             = BIT(22),
		BULLET_IMMUNE               = BIT(23),
		BLAST_IMMUNE                = BIT(24),
		FIRE_IMMUNE                 = BIT(25),
		PARACHUTE                   = BIT(26),
		PROJECTILE_SHIELD           = BIT(27),
	};
	
	enum MissionType
	{
		NO_MISSION               = 0,
		
		MISSION_UNKNOWN1         = 1,
		MISSION_DESTROY_SENTRIES = 2,
		MISSION_SNIPER           = 3,
		MISSION_SPY              = 4,
		MISSION_ENGINEER         = 5,
		MISSION_REPROGRAMMED     = 6,
	};
	
	// TODO: rework this so it functions as a true multi bitmask
	// TODO: also probably expand it to work with *any* weapon slot (not just these 3)
	enum WeaponRestriction
	{
		ANY_WEAPON     = 0,
		
		MELEE_ONLY     = BIT(0),
		PRIMARY_ONLY   = BIT(1),
		SECONDARY_ONLY = BIT(2),
	};
	
	enum IgnoreMask
	{
		IGNORE_SCOUTS      = BIT( 0),
		IGNORE_SOLDIERS    = BIT( 1),
		IGNORE_PYROS       = BIT( 2),
		IGNORE_DEMOS       = BIT( 3),
		IGNORE_HEAVIES     = BIT( 4),
		IGNORE_MEDICS      = BIT( 5),
		IGNORE_ENGIES      = BIT( 6),
		IGNORE_SNIPERS     = BIT( 7),
		IGNORE_SPIES       = BIT( 8),
		IGNORE_CIVILIANS   = BIT( 9),
		IGNORE_SENTRIES    = BIT(11),
		IGNORE_GOALS       = BIT(12),
	};
	
	struct SniperSpotInfo
	{
		CTFNavArea *from_area;
		Vector from_vec;
		
		CTFNavArea *to_area;
		Vector to_vec;
		
		float dist;
		float delta_incdist;
	};

	DECLARE_ENT_SCRIPTDESC( CTFBot );
	
	CTFBot();
	virtual ~CTFBot();
	
	/* CTFPlayer overrides */
//	virtual int ShouldTransmit(const CCheckTransmitInfo *pInfo) override;
	virtual void Spawn() override;
	virtual int DrawDebugTextOverlays() override;
	virtual void Event_Killed(const CTakeDamageInfo& info) override;
	virtual void Touch(CBaseEntity *pOther) override;
	virtual void PhysicsSimulate() override;
//	virtual void UpdateOnRemove() override;
//	virtual bool ShouldGib(const CTakeDamageInfo& info) override;
//	virtual void InitialSpawn() override;
//	virtual void ChangeTeam(int iTeamNum, bool bAutoTeam, bool bSilent) override;
	virtual int GetBotType() const override { return BOT_TYPE; }
//	virtual bool IsAllowedToPickUpFlag() const override;
//	virtual int GetAllowedTauntPartnerTeam() const override;
	
	/* NextBotPlayer overrides */
	virtual bool IsDormantWhenDead() const override { return false; }
	virtual void AvoidPlayers(CUserCmd *usercmd) override;
	
	/* CGameEventListener overrides */
	virtual void FireGameEvent(IGameEvent *event) override;
	
	/* INextBotEventResponder overrides */
	virtual void OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon) override;
	
	/* INextBot overrides */
	virtual ILocomotion *GetLocomotionInterface() const override { return this->m_pLocomotion; }
	virtual IBody *GetBodyInterface() const override             { return this->m_pBody; }
	virtual IIntention *GetIntentionInterface() const override   { return this->m_pIntention; }
	virtual IVision *GetVisionInterface() const override         { return this->m_pVision; }
	virtual bool IsEnemy(const CBaseEntity *ent) const override;  // <-- TODO: need to add "IsApparentEnemy" and "IsApparentFriend" for spy disguise/cloak stuff
	virtual bool IsFriend(const CBaseEntity *ent) const override; // <-- TODO: and then have *all* code in TFBot and TFBot actions call the appropriate thing (also consider calls to CollectPlayers etc)
	virtual bool IsDebugFilterMatch(const char *filter) const override;
	virtual float GetDesiredPathLookAheadRange() const override;
	
	Behavior<CTFBot> *GetBehavior() const { return static_cast<Behavior<CTFBot> *>(this->GetIntentionInterface()->FirstContainedResponder()); }
	
	bool IsTeamEnemy(int team) const;
	bool IsTeamFriend(int team) const;
	CTFTeam *GetOpposingTFTeam( void ) const;
	
	CTFNavArea *GetHomeArea() const { return this->m_HomeArea; }
	
	DifficultyType GetSkill() const { return this->m_iSkill; }
	void SetSkill(int skill)        { this->m_iSkill = Clamp(static_cast<DifficultyType>(skill), EASY, EXPERT); }
	// SetSkill TODO: also need to set CTFPlayer::m_nBotSkill!
	
	CTFNavArea *GetEngieBuildAreaOverride() const { return this->m_EngieBuildArea; }
	
	CBaseEntity *GetMovementGoal() const   { return this->m_hMovementGoal; }
	void SetMovementGoal(CBaseEntity *ent) { this->m_hMovementGoal = ent; }
	
	void SetGenerator(CTFBotGenerator *generator) { this->m_hGenerator = generator; }
	
	bool IsLineOfFireClear(CBaseEntity *ent) const  { return this->IsLineOfFireClear(const_cast<CTFBot *>(this)->EyePosition(), ent); }
	bool IsLineOfFireClear(const Vector& vec) const { return this->IsLineOfFireClear(const_cast<CTFBot *>(this)->EyePosition(), vec); }
	bool IsLineOfFireClear(const Vector& from, CBaseEntity *to) const;
	bool IsLineOfFireClear(const Vector& from, const Vector& to) const;
	
	bool HasTag(const char *str) const;
	void AddTag(const char *str);
	void RemoveTag(const char *str);
	void ClearTags();
	
	/* weapon queries: these functions dispatch virtual calls to the weapon code */
	bool IsCombatWeapon                 (CTFWeaponBase *weapon = nullptr) const;
//	bool IsHitScanWeapon                (CTFWeaponBase *weapon = nullptr) const;
	bool IsExplosiveProjectileWeapon    (CTFWeaponBase *weapon = nullptr) const;
	bool IsContinuousFireWeapon         (CTFWeaponBase *weapon = nullptr) const;
	bool IsBarrageAndReloadWeapon       (CTFWeaponBase *weapon = nullptr) const;
	bool IsQuietWeapon                  (CTFWeaponBase *weapon = nullptr) const;
	bool ShouldFireAtInvulnerableEnemies(CTFWeaponBase *weapon = nullptr) const;
	bool ShouldAimForHeadshots          (CTFWeaponBase *weapon = nullptr) const;
	bool ShouldCompensateAimForGravity  (CTFWeaponBase *weapon = nullptr) const;
	bool IsSniperRifle                  (CTFWeaponBase *weapon = nullptr) const;
	bool IsRocketLauncher               (CTFWeaponBase *weapon = nullptr) const;
	
	float TransientlyConsistentRandomValue(float duration, int seed = 0) const;
	float TransientlyConsistentRandomValue(CNavArea *area, float duration, int seed = 0) const;
	
	bool EquipRequiredWeapon();
	bool EquipLongRangeWeapon();
	void EquipBestWeaponForThreat(const CKnownEntity *threat);
	
	bool LostControlPointRecently() const;
	
	bool IsWeaponRestricted(CTFWeaponBase *weapon) const;
	bool HasWeaponRestriction(int restriction) const;
	void SetWeaponRestriction(int restriction);
	void RemoveWeaponRestriction(int restriction);
	void ClearWeaponRestrictions();
	
	// TODO: look for all cases where actions are naughty and call AddAttribute/RemoveAttribute on a temporary basis
	// (without using stack-based mechanisms for ensuring that the attr modification is done safely/coherently)
	bool HasAttribute(int attr) const;
	void SetAttribute(int attr);
	void ClearAttribute(int attr);
	void ClearAllAttributes();
	
	void PushAttribute(AttributeType attr, bool set);
	void PopAttribute(AttributeType attr);
	
	CTFBotSquad *GetSquad() const { return this->m_Squad; }
	bool IsInASquad() const        { return (this->m_Squad != nullptr); }
	bool IsSquadLeader() const    { return (this->m_Squad != nullptr && this == this->m_Squad->GetLeader()); }
	void JoinSquad(CTFBotSquad *squad);
	void LeaveSquad();
	bool IsSquadmate(CTFPlayer *player) const;
	void DeleteSquad();
	float GetSquadFormationError() const { return this->m_flFormationError; }
	void SetSquadFormationError( float error ) { this->m_flFormationError = error; }
	bool IsOutOfSquadFormation() const { return (!this->m_bIsInFormation && !this->GetLocomotionInterface()->IsStuck() && !this->IsPlayerClass(TF_CLASS_MEDIC)); }
	
	// probably delete the junk here:
//	CObjectSentrygun *GetTargetSentry() const      { return this->m_hTargetSentry; }
//	void SetTargetSentry(CObjectSentrygun *sentry) { this->m_hTargetSentry = sentry; }	
//	const Vector& GetTargetSentryPos() const   { return this->m_vecTargetSentry; }
//	void SetTargetSentryPos(const Vector& pos) { this->m_vecTargetSentry = pos; }
	
	bool HasTargetSentry() const                   { return (this->m_hTargetSentry != nullptr); }
	CObjectSentrygun *GetTargetSentry() const      { return this->m_hTargetSentry; }
	void SetTargetSentry(CObjectSentrygun *sentry) { this->m_hTargetSentry = sentry; this->m_vecTargetSentry = sentry->GetAbsOrigin(); }
	
	bool IsKnownSpy(CTFPlayer *spy) const;
	void RealizeSpy(CTFPlayer *spy);
	void ForgetSpy(CTFPlayer *spy);
	
	bool IsSuspectedSpy(CTFPlayer *spy) const;
	void SuspectSpy(CTFPlayer *spy);
	void StopSuspectingSpy(CTFPlayer *spy);
	
	void SetupSniperSpotAccumulation();
	void AccumulateSniperSpots();
	void ClearSniperSpots();
	const SniperSpotInfo *GetRandomSniperSpot() const;
	
	bool IsLookingAroundForEnemies() const   { return this->m_bLookAroundForEnemies; }
//	void LookAroundForEnemies(bool val)      { this->m_bLookAroundForEnemies = val; }
	void LookAroundForEnemies_Push(bool val) { this->m_LookAroundForEnemiesStack.Push(this->m_bLookAroundForEnemies); this->m_bLookAroundForEnemies = val; }
	void LookAroundForEnemies_Pop()          { this->m_LookAroundForEnemiesStack.Pop(this->m_bLookAroundForEnemies); }
	
	IgnoreMask GetIgnoreMask() const    { return this->m_nIgnoreMask; }
	void SetIgnoreMask(IgnoreMask mask) { this->m_nIgnoreMask = mask; }
	bool CheckIgnoreMask(IgnoreMask mask) const;
	
	void SetAttentionFocus(CBaseEntity *ent) { this->m_hAttentionFocus = ent; }
	void ClearAttentionFocus()               { this->m_hAttentionFocus = nullptr; }
	bool IsAttentionFocused() const          { return (this->m_hAttentionFocus != nullptr); }
	bool IsAttentionFocusedOn(CBaseEntity *ent) const;
	
	CTeamControlPoint *GetMyControlPoint() const;
	void ClearMyControlPoint();
	
	bool HasMission(MissionType mission) const { return (this->m_iMission == mission); }
	bool IsOnAnyMission() const                { return (this->m_iMission != NO_MISSION); }
	void ClearMission()                        { this->SetMission(NO_MISSION, false); }
	void SetMission(MissionType mission, bool reset_intention);
	
	void PushRequiredWeapon(CTFWeaponBase *weapon) { this->m_RequiredWeapons.Push(weapon); }
	void PopRequiredWeapon()                       { this->m_RequiredWeapons.Pop(); }
	
	void DelayedThreatNotice(CHandle<CBaseEntity> ent, float delay);
	void UpdateDelayedThreatNotices();
	
	const char *GetNextSpawnClassname() const;
	
	float GetDesiredAttackRange() const;
	float GetMaxAttackRange() const;
	
	bool IsAmmoFull() const;
	bool IsAmmoLow() const;
	
	int GetUberHealthThreshold() const;
	float GetUberDeployDelayDuration() const;
	
	float GetVisionRange() const { return this->m_flVisionRange; }
	
	void UpdateLookingAroundForEnemies();
	void UpdateLookingAroundForIncomingPlayers(bool friendly_areas);
	
	void DisguiseAsMemberOfEnemyTeam();
	void DisguiseAsRandomClass();
	
	// AreAllPointsUncontestedSoFar // NO XREFS!
	float GetTimeLeftToCapture() const;
	bool IsAnyPointBeingCaptured() const;
	bool IsNearPoint(CTeamControlPoint *point) const;
	bool IsPointBeingCaptured(CTeamControlPoint *point) const;
	CTeamControlPoint *SelectPointToCapture(const CUtlVector<CTeamControlPoint *> *points) const;
	CTeamControlPoint *SelectPointToDefend(const CUtlVector<CTeamControlPoint *> *points) const;
	CTeamControlPoint *SelectClosestControlPointByTravelDistance(const CUtlVector<CTeamControlPoint *> *points) const;
	
	CCaptureZone *GetFlagCaptureZone() const;
	CCaptureFlag *GetFlagToFetch() const;
	bool IsAllowedToPickUpFlag() const;
	void SetFlagTarget(CCaptureFlag *flag);
	CCaptureFlag *GetFlagTarget() const { return this->m_hFlagTarget; }

	CVIPSafetyZone *GetVIPEscapeZone() const;
	CFlagDetectionZone *GetFlagDetectionZone() const;

	void FindFrontlineIncDist();
	float GetFrontlineIncDistAvg() const { return this->m_flFrontlineIncDistAvg; }
	float GetFrontlineIncDistFar() const { return this->m_flFrontlineIncDistFar; }
	
	float GetThreatDanger(CBaseCombatCharacter *threat) const;
	
	CTFWeaponBase *GetTFWeapon_Primary() const   { return this->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_PRIMARY  ); }
	CTFWeaponBase *GetTFWeapon_Secondary() const { return this->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_SECONDARY); }
	CTFWeaponBase *GetTFWeapon_Melee() const     { return this->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_MELEE    ); }
	CTFWeaponBase *GetTFWeapon_Building() const  { return this->GetWeaponForLoadoutSlot(TF_LOADOUT_SLOT_BUILDING ); }
	
	CTFWeaponBase *SwitchWeapon(ETFLoadoutSlot slot);
	CTFWeaponBase *SwitchToPrimary()   { return this->SwitchWeapon(TF_LOADOUT_SLOT_PRIMARY);   }
	CTFWeaponBase *SwitchToSecondary() { return this->SwitchWeapon(TF_LOADOUT_SLOT_SECONDARY); }
	CTFWeaponBase *SwitchToMelee()     { return this->SwitchWeapon(TF_LOADOUT_SLOT_MELEE);     }
	CTFWeaponBase *SwitchToBuilding()  { return this->SwitchWeapon(TF_LOADOUT_SLOT_BUILDING);  }
	
	bool IsSpyInCloakRevealingCondition(const CTFPlayer *player) const;
	bool AmInCloakRevealingCondition() const { return this->IsSpyInCloakRevealingCondition(this); }
	
	// FindSplashTarget // NO XREFS!
	// FindVantagePoint // NO ACTUAL XREFS!
	CTFPlayer *GetClosestHumanLookingAtMe(int team) const;
	CBaseObject *GetNearestKnownSappableTarget() const;
	bool IsEntityBetweenTargetAndSelf(CBaseEntity *ent, CBaseEntity *target) const;
	Action<CTFBot> *OpportunisticallyUseWeaponAbilities();
	void SelectReachableObjects(const CUtlVector<CHandle<CBaseEntity>>& ents_in, CUtlVector<CHandle<CBaseEntity>> *ents_out, const INextBotFilter& filter, CNavArea *area, float range) const;
	bool ShouldFireCompressionBlast();
	
	CTFPlayer *SelectRandomReachableEnemy() const;
	bool IsRandomReachableEnemyStillValid(CTFPlayer *enemy) const;
	
	// TODO: item-giving stuff
	// AddItem
	// GiveRandomItem
	
	// TODO: REMOVE ME!
	void DrawProjectileImpactEstimation(bool good) const;
	
	// in live TF2, these are in CTFPlayer; but they really should be here in CTFBot instead
#if 0
	Vector EstimateProjectileImpactPosition(CTFWeaponBaseGun *weapon) const;
	Vector EstimateProjectileImpactPosition(float pitch, float yaw, float speed) const;
	Vector EstimateStickybombProjectileImpactPosition(float pitch, float yaw, float charge) const;
#endif
	Vector EstimateGrenadeProjectileImpactPosition(CTFWeaponBase *launcher, float pitch, float yaw) const;
	Vector EstimateStickybombProjectileImpactPosition(CTFWeaponBase *launcher, float pitch, float yaw, float charge) const;
	bool IsAnyEnemySentryAbleToAttackMe() const;
	bool IsThreatAimingTowardMe(CBaseEntity *threat, float dot) const;
	bool IsThreatFiringAtMe(CBaseEntity *threat) const;
	
	int CollectEnemyPlayers(CUtlVector<CTFPlayer *> *playerVector, bool isAlive = false, bool shouldAppend = false) const;
	
	template<typename Functor> bool ForEachEnemyTeam(Functor&& func) const { return ForEachEnemyTFTeam(this->GetTeamNumber(), std::forward<Functor>(func)); }
	
	static CBasePlayer *AllocatePlayerEntity(edict_t *pEdict, const char *playername);

	void SetMaxVisionRangeOverride(float) {}
	float GetMaxVisionRangeOverride() const { return 0.0f; }
	void SetScaleOverride(float) {}
	void SetAutoJump(bool) {}
	bool ShouldAutoJump() const { return false; }
	bool ShouldQuickBuild() const { return false; }
	void SetShouldQuickBuild(bool) {}

	// ----------------------------------------------------------------------------
	// VScript accessors
	// ----------------------------------------------------------------------------
	bool ScriptIsWeaponRestricted( HSCRIPT weapon );
	int ScriptGetDifficulty();
	void ScriptSetDifficulty( int difficulty );
	bool ScriptIsDifficulty( int difficulty );
	HSCRIPT ScriptGetHomeArea();
	void ScriptSetHomeArea( HSCRIPT area );
	void ScriptDelayedThreatNotice( HSCRIPT threat, float delay );
	HSCRIPT ScriptGetNearestKnownSappableTarget();
	void ScriptGenerateAndWearItem( char const *itemName );
	HSCRIPT ScriptFindVantagePoint( float distance );
	void ScriptSetAttentionFocus( HSCRIPT entity );
	bool ScriptIsAttentionFocusedOn( HSCRIPT entity );
	HSCRIPT ScriptGetSpawnArea();
	void ScriptDisbandCurrentSquad();
	
private:
	class CTFBotIntention : public IIntention
	{
	public:
		CTFBotIntention(INextBot *nextbot);
		virtual ~CTFBotIntention();
		
		virtual INextBotEventResponder *FirstContainedResponder() const override                            { return this->m_pBehavior; }
		virtual INextBotEventResponder *NextContainedResponder(INextBotEventResponder *prev) const override { return nullptr; }
		
		virtual void Reset() override;
		virtual void Update() override;
		
	private:
		Behavior<CTFBot> *m_pBehavior = nullptr;
	};
	
	struct DelayedNoticeInfo
	{
		CHandle<CBaseEntity> what;
		float when;
	};
	
	
	Vector EstimateProjectileImpactPosition(float pitch, float yaw, float proj_speed) const;
	
	void StartIdleSound() { /* TODO? */ }
	void StopIdleSound()  { /* TODO? */ }
	
	
	
	// 0x2ae0: TODO? not sure whether anything is at this offset
	
	CTFBotLocomotion *m_pLocomotion;
	CTFBotBody       *m_pBody;
	CTFBotIntention  *m_pIntention;
	CTFBotVision     *m_pVision;
	
	CountdownTimer m_ctLookAroundForIncomingPlayers; // +0x2af4
	
	CTFNavArea *m_HomeArea = nullptr; // +0x2b00
	
	CountdownTimer m_ctRecentPointLost; // +0x2b04
	
	// TODO: init to 0
	WeaponRestriction m_nRestrict; // +0x2b10
	
	// TODO: init to 0
	AttributeType m_nAttributes; // +0x2b14
	CUtlStack<AttributeType> m_AttributeStack;
	
	// TODO: initialize m_iSkill?
	// TODO: do we need the corresponding CNetworkVar for bot skill? is that used for anything other than robot eye color?
	DifficultyType m_iSkill; // +0x2b18
	
	/* never assigned non-null values in live TF2; some vestigial or debug-only thing */
	CTFNavArea *m_EngieBuildArea = nullptr; // +0x2b1c
	
	CHandle<CBaseEntity> m_hMovementGoal; // +0x2b20
	
	CHandle<CTFBotProxy> m_hProxy;         // +0x2b24
	CHandle<CTFBotGenerator> m_hGenerator; // +0x2b28
	
	CTFBotSquad *m_Squad = nullptr; // +0x2b2c
	
	bool m_bKeepClassAfterDeath = false;       // +0x2b30
	CHandle<CObjectSentrygun> m_hTargetSentry; // +0x2b34
	Vector m_vecTargetSentry = vec3_origin;    // +0x2b38
	
	/* this stuff is from 20151007a, but we're not using that garbage */
	// CUtlVectorAutoPurge<SuspectedSpyInfo_t *> m_SuspectedSpies; // +0x2b44
	// CUtlVector<CHandle<CTFPlayer>>            m_KnownSpies;     // +0x2b58
	
	/* here's the corresponding stuff from 20120811b, which we're actually using */
	CUtlVector<CHandle<CTFPlayer>> m_SuspectedSpies; // +0x1de0 [20120811b]
	CUtlVector<CHandle<CTFPlayer>> m_KnownSpies;     // +0x1df4 [20120811b]
	
	CUtlVector<SniperSpotInfo> m_SniperSpots;   // +0x2b6c
	CUtlVector<CTFNavArea *> m_SniperAreasFrom; // +0x2b80 TODO: name
	CUtlVector<CTFNavArea *> m_SniperAreasTo;   // +0x2b94 TODO: name
	CHandle<CBaseEntity> m_hSniperGoalEntity;   // +0x2ba8
	Vector m_vecSniperGoalEntity;               // +0x2bac
	CountdownTimer m_ctSniperSpots;             // +0x2bb8 TODO: name
	
	bool m_bLookAroundForEnemies = true; // +0x2bc4
	CUtlStack<bool> m_LookAroundForEnemiesStack;
	
	// TODO: init to 0
	IgnoreMask m_nIgnoreMask; // +0x2bc8
	
	CUtlVector<CUtlString> m_Tags; // +0x2bcc [was: CUtlVector<CFmtStr>]
	
	CHandle<CBaseEntity> m_hAttentionFocus; // +0x2be0
	
	mutable CHandle<CTeamControlPoint> m_hMyControlPoint; // +0x2be4
	mutable CountdownTimer m_ctMyControlPoint; // +0x2be8
	
	// float m_flScale; // +0x2bf4 [init value: -1.0f]
	
	// TODO: initialize m_iMission and m_iLasMission?
	MissionType m_iMission;     // +0x2bf8
	MissionType m_iLastMission; // +0x2bfc
	
	// CHandle<CBaseEntity> m_hSBTarget; // +0x2c00
	// 0x2c04: CUtlString
	
	CUtlStack<CHandle<CTFWeaponBase>> m_RequiredWeapons; // +0x2c08
	
	CountdownTimer m_ctLoudWeaponHeard; // +0x2c1c
	
	CUtlVector<DelayedNoticeInfo> m_DelayedThreatNotices; // +0x2c28
	
	float m_flVisionRange = -1.0f; // +0x2c3c
	
	CountdownTimer m_ctUseWeaponAbilities; // +0x2c40
	
	// 0x2c4c: TODO
	
	float m_flFormationError = 0.0f; // +0x2c50
	bool m_bIsInFormation;           // +0x2c54
	
	// CUtlStringList m_TeleportWhere; // +0x2c58
	// bool m_bTeleQuickBuild;         // +0x2c6c [init value: false]
	
	// float m_flAutoJumpMin; // +0x2c70 [init value: 0.0f]
	// float m_flAutoJumpMax; // +0x2c74 [init value: 0.0f]
	
	// 0x2c78: CountdownTimer
	
	CHandle<CCaptureFlag> m_hFlagTarget; // +0x2c84
	
	// CUtlVector<const EventChangeAttributes_t *> m_ECAttrs; // +0x2c88 (maybe autopurge)
	
	// TODO: is sizeof(CTFBot) > 0x2c94?

	float m_flFrontlineIncDistAvg; // How far teammates have advanced along incursion distance on average
	float m_flFrontlineIncDistFar; // How far the farthest teammate has advanced along incursion distance

	CTFTeam *m_pOpposingTeam;
};


DEFINE_ENUM_BITWISE_OPERATORS(CTFBot::AttributeType);
inline bool CTFBot::HasAttribute(int attr) const { return ((this->m_nAttributes & attr) != 0); }
inline void CTFBot::SetAttribute(int attr)       { this->m_nAttributes |=  (CTFBot::AttributeType)attr; }
inline void CTFBot::ClearAttribute(int attr)     { this->m_nAttributes &= (CTFBot::AttributeType)~attr; }
inline void CTFBot::ClearAllAttributes()                           { this->m_nAttributes = NO_ATTRIBUTES; }

DEFINE_ENUM_BITWISE_OPERATORS(CTFBot::IgnoreMask);
inline bool CTFBot::CheckIgnoreMask(IgnoreMask mask) const { return ((this->m_nIgnoreMask & mask) != 0); }

DEFINE_ENUM_BITWISE_OPERATORS(CTFBot::WeaponRestriction);
inline bool CTFBot::HasWeaponRestriction(int restriction) const { return ((this->m_nRestrict & restriction) != 0); }
inline void CTFBot::SetWeaponRestriction(int restriction)       { this->m_nRestrict |=  (CTFBot::WeaponRestriction)restriction; }
inline void CTFBot::RemoveWeaponRestriction(int restriction)    { this->m_nRestrict &= (CTFBot::WeaponRestriction)~restriction; }
inline void CTFBot::ClearWeaponRestrictions()                   { this->m_nRestrict = ANY_WEAPON; }


inline CTFWeaponBase *CTFBot::SwitchWeapon(ETFLoadoutSlot slot)
{
	CTFWeaponBase *weapon = this->GetWeaponForLoadoutSlot(slot);
	if (weapon == nullptr) return nullptr;
	
	this->Weapon_Switch(weapon);
	return weapon;
}


inline bool CTFBot::IsSpyInCloakRevealingCondition(const CTFPlayer *player) const
{
	// TODO: wet players: use CTFPlayer::IsDripping or CTFPlayer::IsWet or something similar to that
	// TODO: new TF2C conds?
	
	// TODO: shouldn't this not apply at all if we're stealthed, or disguised, or some combination thereof...?
	
	// NOTE: TFBots in live TF2 don't check for TF_COND_MAD_MILK
	for (ETFCond cond : { TF_COND_STEALTHED_BLINK, TF_COND_BURNING, TF_COND_BLEEDING, TF_COND_URINE, TF_COND_MAD_MILK }) {
		if (player->m_Shared.InCond(cond)) return true;
	}
	
	return false;
}


class CTFBotPathCost : public IPathCost
{
public:
	CTFBotPathCost(const CTFBot *actor, RouteType rtype);
	
	virtual float operator()(CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder = nullptr, const CFuncElevator *elevator = nullptr, float length = -1.0f) const override;
	
private:
	const CTFBot *m_pBot;
	RouteType m_iRouteType;
	float m_flStepHeight;
	float m_flMaxJumpHeight;
	float m_flDeathDropHeight;
	CUtlVector<CBaseObject *> m_EnemyObjects;
};

class CTFPlayertPathCost : public IPathCost
{
public:
	CTFPlayertPathCost(const CTFPlayer *player) :
		m_pPlayer(player), m_flStepHeight(18.0f), m_flMaxJumpHeight(72.0f), m_flDeathDropHeight(200.0f) {}
	
	virtual float operator()(CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder = nullptr, const CFuncElevator *elevator = nullptr, float length = -1.0f) const override;
	
private:
	const CTFPlayer *m_pPlayer;
	float m_flStepHeight;
	float m_flMaxJumpHeight;
	float m_flDeathDropHeight;
};


CTFBot *CreateTFBot(const char *name = nullptr, CTFBot::DifficultyType skill = CTFBot::UNDEFINED_SKILL);

template<size_t LEN> void CreateBotName(char (&dst)[LEN], CTFBot::DifficultyType skill, const char *name = nullptr)
{
	Assert(skill >= CTFBot::EASY);
	Assert(skill <= CTFBot::EXPERT);
	
	if (TFGameRules()->IsInTraining()) {
		Assert(false);
	}
	
	if (name == nullptr) {
		name = GetRandomBotName();
	}
	
	const char *prefix = "";
	
	extern ConVar tf_bot_prefix_name_with_difficulty;
	if (tf_bot_prefix_name_with_difficulty.GetBool()) {
		switch (skill) {
		case CTFBot::EASY:   prefix =   "[Easy] "; break;
		case CTFBot::NORMAL: prefix = "[Normal] "; break;
		case CTFBot::HARD:   prefix =   "[Hard] "; break;
		case CTFBot::EXPERT: prefix = "[Expert] "; break;
		}
	}
	
	V_sprintf_safe(dst, "%s%s", prefix, name);
}


inline CTFBot *ToTFBot(CBaseEntity *pEntity)
{
	CTFPlayer *pTFPlayer = ToTFPlayer(pEntity);
	if (pTFPlayer == nullptr) return nullptr;
	
	if (pTFPlayer->IsBotOfType(CTFBot::BOT_TYPE)) {
		return assert_cast<CTFBot *>(pTFPlayer);
	} else {
		return nullptr;
	}
}
inline const CTFBot *ToTFBot(const CBaseEntity *pEntity)
{
	const CTFPlayer *pTFPlayer = ToTFPlayer(pEntity);
	if (pTFPlayer == nullptr) return nullptr;
	
	if (pTFPlayer->IsBotOfType(CTFBot::BOT_TYPE)) {
		return assert_cast<const CTFBot *>(pTFPlayer);
	} else {
		return nullptr;
	}
}


class CClosestTFPlayer
{
public:
	CClosestTFPlayer(const Vector& where, int team = TEAM_ANY) : m_vecWhere(where), m_iTeam(team) {}
	
	bool operator()(CBasePlayer *player)
	{
		[=]{
			if (!player->IsAlive()) return;
			
			if (player->GetTeamNumber() <  FIRST_GAME_TEAM) return;
			if (player->GetTeamNumber() >= TF_TEAM_COUNT)   return;
			
			if (this->m_iTeam != TEAM_ANY && player->GetTeamNumber() != this->m_iTeam) return;
			
			CTFBot *bot = ToTFBot(player);
			if (bot != nullptr && bot->HasAttribute(CTFBot::PROXY_MANAGED)) return;
			
			float range = player->GetAbsOrigin().DistTo(this->m_vecWhere);
			if (range < this->m_flMaxRange) {
				this->m_flMaxRange = range;
				this->m_Player = assert_cast<CTFPlayer *>(player);
			}
		}();
		
		return true;
	}
	
	CTFPlayer *GetPlayer() const { return this->m_Player; }
	
private:
	Vector m_vecWhere;
	int m_iTeam;
	float m_flMaxRange  = FLT_MAX;
	CTFPlayer *m_Player = nullptr;
};


// These helper classes exist mostly to help TFBot actions ensure:
// (a) that they remember to pop before destruction
// (b) that they *don't* pop if they didn't push (this can happen in cases where OnSuspend pops to restore the original
//     value, and then OnResume pushes to override it again; if the action is terminated while suspended, then its
//     OnEnd function will be called without OnResume ever being called, which could result in a pop-without-push)
class CTFBotActionHelper_LookAroundForEnemies
{
protected:
	CTFBotActionHelper_LookAroundForEnemies() = default;
	~CTFBotActionHelper_LookAroundForEnemies()
	{
		Assert(!this->m_bPushed);
	}
	
	void LookAroundForEnemies_Set(CTFBot *actor, bool val)
	{
		Assert(!this->m_bPushed);
		if (!this->m_bPushed) {
			actor->LookAroundForEnemies_Push(val);
			this->m_bPushed = true;
		}
	}
	void LookAroundForEnemies_Reset(CTFBot *actor)
	{
		if (this->m_bPushed) {
			actor->LookAroundForEnemies_Pop();
			this->m_bPushed = false;
		}
	}
	
private:
	bool m_bPushed = false;
};

class CTFBotActionHelper_IgnoreEnemies
{
protected:
	CTFBotActionHelper_IgnoreEnemies() = default;
	~CTFBotActionHelper_IgnoreEnemies()
	{
		Assert(!this->m_bPushed);
	}
	
	void IgnoreEnemies_Set(CTFBot *actor, bool set)
	{
		Assert(!this->m_bPushed);
		if (!this->m_bPushed) {
			actor->PushAttribute(CTFBot::IGNORE_ENEMIES, set);
			this->m_bPushed = true;
		}
	}
	void IgnoreEnemies_Reset(CTFBot *actor)
	{
		if (this->m_bPushed) {
			actor->PopAttribute(CTFBot::IGNORE_ENEMIES);
			this->m_bPushed = false;
		}
	}
	
private:
	bool m_bPushed = false;
};


// TODO: REMOVE ME (when no longer needed)
#define ACTION_STUB(name) \
	class name final : public Action<CTFBot> \
	{ \
	public: \
		template<typename... ARGS> name(ARGS...) {} \
		template<typename... ARGS> static bool IsPossible(ARGS...) { return false; } \
		virtual const char *GetName() const override { return "STUB[" #name "]"; } \
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override \
		{ \
			DevMsg("STUB: " #name "\n"); \
			Done("STUB!"); \
		} \
	};

//ACTION_STUB(CTFBotGetAmmo);
//ACTION_STUB(CTFBotGetHealth);
//ACTION_STUB(CTFBotUseTeleporter);
//ACTION_STUB(CTFBotDestroyEnemySentry);
//ACTION_STUB(CTFBotUberAttackEnemySentry);

//ACTION_STUB(CTFBotCapturePoint);
//ACTION_STUB(CTFBotDefendPoint);
//ACTION_STUB(CTFBotDefendPointBlockCapture);

//ACTION_STUB(CTFBotFetchFlag);
//ACTION_STUB(CTFBotDeliverFlag);
//ACTION_STUB(CTFBotPushToCapturePoint);
//ACTION_STUB(CTFBotEscortFlagCarrier);
//ACTION_STUB(CTFBotAttackFlagDefenders);

//ACTION_STUB(CTFBotPayloadPush);
//ACTION_STUB(CTFBotPayloadGuard);
//ACTION_STUB(CTFBotPayloadBlock);

//ACTION_STUB(CTFBotPrepareStickybombTrap);
//ACTION_STUB(CTFBotStickybombSentrygun);

//ACTION_STUB(CTFBotEngineerBuild);
//ACTION_STUB(CTFBotEngineerBuilding);
//ACTION_STUB(CTFBotEngineerMoveToBuild);
//ACTION_STUB(CTFBotEngineerBuildSentryGun);
//ACTION_STUB(CTFBotEngineerBuildDispenser);
//ACTION_STUB(CTFBotEngineerBuildTeleportEntrance);
//ACTION_STUB(CTFBotEngineerBuildTeleportExit);

//ACTION_STUB(CTFBotMedicHeal);
//ACTION_STUB(CTFBotMedicRetreat);

//ACTION_STUB(CTFBotSniperLurk);

//ACTION_STUB(CTFBotSpyInfiltrate);
//ACTION_STUB(CTFBotSpyAttack);
//ACTION_STUB(CTFBotSpySap);

// TODO: more action stubs needed probably


#undef BIT


#endif
