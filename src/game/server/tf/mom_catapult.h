// https://github.com/momentum-mod/game/blob/09f0c8a65181daa454e559ef0cef9324e9b30420/mp/src/game/server/momentum/mom_triggers.h#L895-L943

#include "cbase.h"
#include "triggers.h"

class CTriggerMomentumCatapult : public CBaseTrigger
{
public:
    DECLARE_CLASS(CTriggerMomentumCatapult, CBaseTrigger);
    DECLARE_DATADESC();

    CTriggerMomentumCatapult();

public:
    void StartTouch(CBaseEntity*) override;
    void Touch(CBaseEntity*) override;
    void Spawn() override;
    void Think() override;

    int DrawDebugTextOverlays() override;

private:
    Vector CalculateLaunchVelocity(CBaseEntity*);
    Vector CalculateLaunchVelocityExact(CBaseEntity*);
    void Launch(CBaseEntity*);
    void LaunchAtDirection(CBaseEntity*);
    void LaunchAtTarget(CBaseEntity*);

private:

    enum ExactVelocityChoice_t
    {
        BEST = 0,
        SOLUTION_ONE,
        SOLUTION_TWO,
    };

    float m_flPlayerSpeed;
    int m_iUseExactVelocity;
    int m_iExactVelocityChoiceType;
    QAngle m_vLaunchDirection;
    EHANDLE m_hLaunchTarget;
    bool m_bUseLaunchTarget;
    bool m_bUseThresholdCheck;
    float m_flLowerThreshold;
    float m_flUpperThreshold;
    bool m_bOnlyCheckVelocity;
    float m_flEntryAngleTolerance;
    COutputEvent m_OnCatapulted;
    float m_flInterval;
    bool m_bOnThink;
    bool m_bEveryTick;
    float m_flHeightOffset;
};