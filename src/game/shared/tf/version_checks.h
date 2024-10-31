// basic file operations
#include <chksum_file.h>
#include <cbase.h>
#include "igameevents.h"
#include "GameEventListener.h"

#pragma once

class CVersioning : public CAutoGameSystem, CGameEventListener
{
public:
                    CVersioning();

    virtual void    FireGameEvent(IGameEvent* event);

    bool            Init()     override;                // CAutoGameSystem :: Init
    void            PostInit() override;                // CAutoGameSystem :: PostInit
    static void     RecalcHashes();
};