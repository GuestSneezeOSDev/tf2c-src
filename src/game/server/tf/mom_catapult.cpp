// https://github.com/momentum-mod/game/blob/09f0c8a65181daa454e559ef0cef9324e9b30420/mp/src/game/server/momentum/mom_triggers.cpp#L2252-L2586

#include "cbase.h"
#include <movevars_shared.h>
#include <mom_catapult.h>
#include "dt_utlvector_send.h"
#include <predictable_entity.h>
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------------------------
//--------- CTriggerMomentumCatapult -------------------------------------------------------------------
LINK_ENTITY_TO_CLASS(trigger_momentum_catapult, CTriggerMomentumCatapult);

// Alias tf2 trigger_catapult for backwards compat
LINK_ENTITY_TO_CLASS(trigger_catapult, CTriggerMomentumCatapult);

BEGIN_DATADESC(CTriggerMomentumCatapult)
    DEFINE_KEYFIELD(m_flPlayerSpeed, FIELD_FLOAT, "playerSpeed"),
    DEFINE_KEYFIELD(m_bUseThresholdCheck, FIELD_INTEGER, "useThresholdCheck"),
    DEFINE_KEYFIELD(m_flEntryAngleTolerance, FIELD_FLOAT, "entryAngleTolerance"),
    DEFINE_KEYFIELD(m_iUseExactVelocity, FIELD_INTEGER, "useExactVelocity"),
    DEFINE_KEYFIELD(m_iExactVelocityChoiceType, FIELD_INTEGER, "exactVelocityChoiceType"),
    DEFINE_KEYFIELD(m_flLowerThreshold, FIELD_FLOAT, "lowerThreshold"),
    DEFINE_KEYFIELD(m_flUpperThreshold, FIELD_FLOAT, "upperThreshold"),
    DEFINE_KEYFIELD(m_vLaunchDirection, FIELD_VECTOR, "launchDirection"),
    DEFINE_KEYFIELD(m_target, FIELD_STRING, "launchTarget"),
    DEFINE_KEYFIELD(m_bOnlyCheckVelocity, FIELD_INTEGER, "onlyCheckVelocity"),
    DEFINE_OUTPUT(m_OnCatapulted, "OnCatapulted"),
    DEFINE_KEYFIELD(m_flInterval, FIELD_FLOAT, "Interval"),
    DEFINE_KEYFIELD(m_bOnThink, FIELD_BOOLEAN, "OnThink"),
    DEFINE_KEYFIELD(m_bEveryTick, FIELD_BOOLEAN, "EveryTick"),
    DEFINE_KEYFIELD(m_flHeightOffset, FIELD_FLOAT, "heightOffset"),
END_DATADESC()


CTriggerMomentumCatapult::CTriggerMomentumCatapult()
{
    m_flPlayerSpeed = 450.0f;
    m_bUseThresholdCheck = 0;
    m_flEntryAngleTolerance = 0.0f;
    m_iUseExactVelocity = 0;
    m_iExactVelocityChoiceType = BEST;
    m_flLowerThreshold = 0.15f;
    m_flUpperThreshold = 0.30f;
    m_vLaunchDirection = vec3_angle;
    m_hLaunchTarget = nullptr;
    m_bOnlyCheckVelocity = false;
    m_flInterval = 1.0;
    m_bOnThink = false;
    m_bEveryTick = false;
    m_flHeightOffset = 32.0f;
}

void CTriggerMomentumCatapult::Spawn()
{
    BaseClass::Spawn();
    InitTrigger();

    m_flLowerThreshold = clamp(m_flLowerThreshold, 0.0f, 1.0f);
    m_flUpperThreshold = clamp(m_flUpperThreshold, 0.0f, 1.0f);

    m_flEntryAngleTolerance = clamp(m_flEntryAngleTolerance, -1.0f, 1.0f);

    if (!m_hLaunchTarget.Get())
    {
        if (m_target != NULL_STRING)
        {
            m_hLaunchTarget = gEntList.FindEntityByName(nullptr, m_target);
            m_bUseLaunchTarget = true;
        }
        else
        {
            m_bUseLaunchTarget = false;
        }
    }
}

Vector CTriggerMomentumCatapult::CalculateLaunchVelocity(CBaseEntity* pOther)
{
    // Calculated from time ignoring grav, then compensating for gravity later
    // From https://www.gamasutra.com/blogs/KainShin/20090515/83954/Predictive_Aim_Mathematics_for_AI_Targeting.php
    // and setting the target's velocity vector to zero

    Vector vecPlayerOrigin = pOther->GetAbsOrigin();

    vecPlayerOrigin.z += m_flHeightOffset;

    Vector vecAbsDifference = m_hLaunchTarget->GetAbsOrigin() - vecPlayerOrigin;
    float flSpeedSquared = m_flPlayerSpeed * m_flPlayerSpeed;
    float flGravity = GetCurrentGravity();

    float flDiscriminant = 4.0f * flSpeedSquared * vecAbsDifference.Length() * vecAbsDifference.Length();

    flDiscriminant = sqrtf(flDiscriminant);
    float fTime = 0.5f * (flDiscriminant / flSpeedSquared);

    Vector vecLaunchVelocity = (vecAbsDifference / fTime);

    Vector vecGravityComp(0, 0, 0.5f * -flGravity * fTime);
    vecLaunchVelocity -= vecGravityComp;

    return vecLaunchVelocity;
}

Vector CTriggerMomentumCatapult::CalculateLaunchVelocityExact(CBaseEntity* pOther)
{
    // Uses exact trig and gravity

    Vector vecPlayerOrigin = pOther->GetAbsOrigin();

    vecPlayerOrigin.z += m_flHeightOffset;

    Vector vecAbsDifference = m_hLaunchTarget->GetAbsOrigin() - vecPlayerOrigin;
    Vector vecAbsDifferenceXY = Vector(vecAbsDifference.x, vecAbsDifference.y, 0.0f);

    float flSpeedSquared = m_flPlayerSpeed * m_flPlayerSpeed;
    float flSpeedQuad = m_flPlayerSpeed * m_flPlayerSpeed * m_flPlayerSpeed * m_flPlayerSpeed;
    float flAbsX = vecAbsDifferenceXY.Length();
    float flAbsZ = vecAbsDifference.z;
    float flGravity = GetCurrentGravity();

    float flDiscriminant = flSpeedQuad - flGravity * (flGravity * flAbsX * flAbsX + 2.0f * flAbsZ * flSpeedSquared);

    // Maybe not this but some sanity check ofc, then default to non exact case which should always have a solution
    if (m_flPlayerSpeed < sqrtf(flGravity * (flAbsZ + vecAbsDifference.Length())))
    {
        DevWarning("Not enough speed to reach target.\n");
        return CalculateLaunchVelocity(pOther);
    }
    if (flDiscriminant < 0.0f)
    {
        DevWarning("Not enough speed to reach target.\n");
        return CalculateLaunchVelocity(pOther);
    }
    if (CloseEnough(flAbsX, 0.0f))
    {
        DevWarning("Target position cannot be the same as catapult position?\n");
        return CalculateLaunchVelocity(pOther);
    }

    flDiscriminant = sqrtf(flDiscriminant);

    float flLowAng = atanf((flSpeedSquared - flDiscriminant) / (flGravity * flAbsX));
    float flHighAng = atanf((flSpeedSquared + flDiscriminant) / (flGravity * flAbsX));

    Vector fGroundDir = vecAbsDifferenceXY.Normalized();
    Vector vecLowAngVelocity = m_flPlayerSpeed * (fGroundDir * cosf(flLowAng) + Vector(0, 0, sinf(flLowAng)));
    Vector vecHighAngVelocity = m_flPlayerSpeed * (fGroundDir * cosf(flHighAng) + Vector(0, 0, sinf(flHighAng)));
    Vector vecLaunchVelocity = vec3_origin;
    Vector vecPlayerEntryVel = pOther->GetAbsVelocity();

    switch (m_iExactVelocityChoiceType)
    {
    case BEST:
        // "Best" solution seems to minimize angle of entry with respect to launch vector
        vecLaunchVelocity = vecPlayerEntryVel.Dot(vecLowAngVelocity) < vecPlayerEntryVel.Dot(vecHighAngVelocity)
            ? vecLowAngVelocity
            : vecHighAngVelocity;
        break;

    case SOLUTION_ONE:
        vecLaunchVelocity = vecLowAngVelocity;
        break;

    case SOLUTION_TWO:
        vecLaunchVelocity = vecHighAngVelocity;
        break;

    default:
        break;
    }

    return vecLaunchVelocity;
}
void CTriggerMomentumCatapult::LaunchAtDirection(CBaseEntity* pOther)
{
    pOther->SetGroundEntity(nullptr);
    Vector vecLaunchDir = vec3_origin;
    AngleVectors(m_vLaunchDirection, &vecLaunchDir);
    pOther->SetAbsVelocity(m_flPlayerSpeed * vecLaunchDir);
    m_OnCatapulted.FireOutput(pOther, this);
}

void CTriggerMomentumCatapult::LaunchAtTarget(CBaseEntity* pOther)
{
    pOther->SetGroundEntity(nullptr);
    Vector vecLaunchVelocity = vec3_origin;

    if (m_iUseExactVelocity)
    {
        vecLaunchVelocity = CalculateLaunchVelocityExact(pOther);
    }
    else
    {
        vecLaunchVelocity = CalculateLaunchVelocity(pOther);
    }

    pOther->SetAbsVelocity(vecLaunchVelocity);
    m_OnCatapulted.FireOutput(pOther, this);
}

void CTriggerMomentumCatapult::Launch(CBaseEntity* pOther)
{
    bool bLaunch = true;

    // Check threshold
    if (m_bUseThresholdCheck)
    {
        Vector vecPlayerVelocity = pOther->GetAbsVelocity();
        float flPlayerSpeed = vecPlayerVelocity.Length();
        bLaunch = false;

        // From VDC
        if (flPlayerSpeed > m_flPlayerSpeed - (m_flPlayerSpeed * m_flLowerThreshold) &&
            flPlayerSpeed < m_flPlayerSpeed + (m_flPlayerSpeed * m_flUpperThreshold))
        {
            float flPlayerEntryAng = 0.0f;

            if (m_bUseLaunchTarget)
            {
                Vector vecAbsDifference = m_hLaunchTarget->GetAbsOrigin() - pOther->GetAbsOrigin();
                flPlayerEntryAng = DotProduct(vecAbsDifference.Normalized(), vecPlayerVelocity.Normalized());

            }
            else
            {
                Vector vecLaunchDir = vec3_origin;
                AngleVectors(m_vLaunchDirection, &vecLaunchDir);
                flPlayerEntryAng = DotProduct(vecLaunchDir.Normalized(), vecPlayerVelocity.Normalized());
            }

            // VDC uses brackets so inclusive??
            if (flPlayerEntryAng >= m_flEntryAngleTolerance)
            {
                if (m_bOnlyCheckVelocity)
                {
                    m_OnCatapulted.FireOutput(pOther, this);
                    return;
                }
                bLaunch = true;
            }
        }
    }

    if (!bLaunch)
    {
        return;
    }

    if (m_bUseLaunchTarget)
    {
        LaunchAtTarget(pOther);
    }
    else
    {
        LaunchAtDirection(pOther);
    }
}

void CTriggerMomentumCatapult::StartTouch(CBaseEntity* pOther)
{
    BaseClass::StartTouch(pOther);

    // Ignore vphys only allow players
    if (pOther && pOther->IsPlayer())
    {
        Launch(pOther);

        if (m_bOnThink)
            SetNextThink(gpGlobals->curtime + m_flInterval);
    }
}

void CTriggerMomentumCatapult::Touch(CBaseEntity* pOther)
{
    BaseClass::Touch(pOther);

    if (m_bEveryTick)
    {
        if (!PassesTriggerFilters(pOther))
            return;

        if (pOther && pOther->IsPlayer())
        {
            Launch(pOther);
        }
    }
}

void CTriggerMomentumCatapult::Think()
{
    if (!m_bOnThink)
    {
        SetNextThink(TICK_NEVER_THINK);
        return;
    }

    FOR_EACH_VEC(m_hTouchingEntities, i)
    {
        const auto pEnt = m_hTouchingEntities[i].Get();
        if (pEnt && pEnt->IsPlayer())
        {
            Launch(pEnt);
            SetNextThink(gpGlobals->curtime + m_flInterval);
        }
    }
}

int CTriggerMomentumCatapult::DrawDebugTextOverlays()
{
    int text_offset = BaseClass::DrawDebugTextOverlays();

    char tempstr[255];

    if (m_target != NULL_STRING)
    {
        Q_snprintf(tempstr, sizeof(tempstr), "Launch target: %s", m_target.ToCStr());
        EntityText(text_offset, tempstr, 0);
        text_offset++;
    }

    Q_snprintf(tempstr, sizeof(tempstr), "Player velocity: %f", m_flPlayerSpeed);
    EntityText(text_offset, tempstr, 0);
    text_offset++;

    Vector vecLaunchVelocity = vec3_origin;
    Vector vecLaunchVelocityExact = vec3_origin;
    if (m_target != NULL_STRING)
    {
        vecLaunchVelocity = CalculateLaunchVelocity(this);
        vecLaunchVelocityExact = CalculateLaunchVelocityExact(this);

        Q_snprintf(tempstr, sizeof(tempstr), "Adjusted player velocity: %f",
            m_iUseExactVelocity ? (float)vecLaunchVelocity.Length() : (float)vecLaunchVelocityExact.Length());

        EntityText(text_offset, tempstr, 0);
        text_offset++;
    }


    return text_offset;
}
//-----------------------------------------------------------------------------------------------