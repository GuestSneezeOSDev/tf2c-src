//---------------------------------------------------------------------------------------------
// Credits to Momentum Mod for this code and specifically xen-000
//---------------------------------------------------------------------------------------------

// ---> HEY HEY LOOK AT ME <----
// READ THIS SHIT OK
//      if you enable dbging, YOU ARE EXPOSING POSSIBLE FUNCTIONS UNOBSFUCATED TO CLIENTS
//      this is very obviously ONLY FOR DEBUGGING
//      -----> DO NOT SHIP ANY BINS TO ANYONE WITH THIS DEFINED EVER UNDER ANY CIRCUMSTANCES PLEASE <-----
//      -----> NOT EVEN OVER DISCORD, NOT EVER *EVER* IN SVN <-----
//      -----> COMPILE IT YOURSELF IF YOU ABSOLUTELY NEED TO <-----
//
//      Please.
// -sappho
////////// #define dbging yep //////////


#ifdef _WIN32
    #pragma once
#endif
#include "tier0/valve_minmax_off.h"
    //#include <cbase.h>
    #include <memy/memytools.h>
    #include <bin_patch.h>
    // #include <util/obfuscate.h>
    //#define AY_OBFUSCATE
#include "tier0/valve_minmax_on.h"

#ifdef dbging
    #define goodcolor   Color(90, 240, 90, 255) // green
    #define okcolor     Color(246, 190, 0, 255) // yellow
#endif

CBinary g_CBinary;


// This is needed so that CAutoGameSystem knows that we're using it, apparently
CBinary::CBinary() : CAutoGameSystem("")
{
}

// Bin Patch format:
//==============================
// m_pSignature:    Memory signature
// m_iSize:         Size of mem sig
// 
// m_iOffset:       Patch offset
// m_bImmediate:    Immediate or referenced variable
// m_pPatch:        Patch bytes (int/float/char*)
//==============================
// Signature mask is now autogenerated. Wildcard bytes = \x2A
// FOR THE RECORD THIS IS SDK2013 BINS NOT TF2 ONES
//
// -sappho
CBinPatch g_EnginePatches[] =
{
    #ifdef _WIN32
        #ifdef TF_CLASSIC
        // Server only!
        /*
            Patch:
            Prevent clients from lagging out the server by spamming a bunch of invalid *netchan->RequestFile requests
            This lags out the server by spamming messages to console (CALL ConMsg), which is resource intensive, because of course it is
        */
        // --- PART ONE ---
        //
        // Signature for sub_101C9090:
        // 55 8B EC A1 ? ? ? ? 81 EC 04 01 00 00 A8 01
        // Unique string: "Download file '%s' %s"
        //
        // CNetChan::HandleUpload(char *, int)
        {
            AY_OBFUSCATE("\x55\x8B\xEC\xA1\x2A\x2A\x2A\x2A\x81\xEC\x04\x01\x00\x00\xA8\x01"),
            16,
            0x167,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP NOP
        },
        // --- PART TWO THREE FOUR AND FIVE---
        //
        // offsets for the ConMsgs in this func are
        // 0x40
        // 0x82
        // 0xA8
        // 0x121
        //
        // Signature for sub_101C8430:
        // 55 8B EC 51 56 8B 75 08 8B C1
        // Unique string: "CreateFragmentsFromFile: '%s' doesn't"
        //
        // CNetChan::CreateFragmentsFromFile(char const*, int, unsigned int)
        {
            AY_OBFUSCATE("\x55\x8B\xEC\x51\x56\x8B\x75\x08\x8B\xC1"),
            10,
            0x40,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP NOP
        },
        {
            AY_OBFUSCATE("\x55\x8B\xEC\x51\x56\x8B\x75\x08\x8B\xC1"),
            10,
            0x82,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP NOP
        },
        {
            AY_OBFUSCATE("\x55\x8B\xEC\x51\x56\x8B\x75\x08\x8B\xC1"),
            10,
            0xA8,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP NOP
        },
        {
            AY_OBFUSCATE("\x55\x8B\xEC\x51\x56\x8B\x75\x08\x8B\xC1"),
            10,
            0x121,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP NOP
        },
        #else
        // Client only!
        /*
            Patch:
            Prevent the culling of skyboxes at high FOVs
        */
        // Signature for sub_10106EA0:
        // 55 8B EC 81 EC 54 02 00 00 8B 0D ? ? ? ?
        // Uniqueish string: R_DrawSkybox
        // 
        //
        // if ( (((v6 * *&dword_103C0274) + (v7 * *&dword_103C0278)) + (v8 * *&dword_103C027C)) >= -0.29289001 )
        // ->
        // if ( (((v6 * *&dword_103C0274) + (v7 * *&dword_103C0278)) + (v8 * *&dword_103C027C)) >= -1.0 )
        //
        // R_DrawSkyBox
        {
            AY_OBFUSCATE("\x55\x8B\xEC\x81\xEC\x54\x02\x00\x00\x8B\x0D\x2A\x2A\x2A\x2A"),
            15,
            0x133,
            PATCH_REFERENCE, // we are changing the value of a float**
            -1.0f
        },
        #endif
    #else 
    // LINUX
        #ifdef TF_CLASSIC
        // Server only!
        /*
            Patch:
            Prevent clients from lagging out the server by spamming a bunch of invalid *netchan->RequestFile requests
            This lags out the server by spamming messages to console (CALL ConMsg), which is resource intensive, because of course it is
        */
        // --- PART ONE ---
        //
        // Signature for _ZN8CNetChan12HandleUploadEPNS_15dataFragments_sEP18INetChannelHandler:
        // 55 89 E5 81 EC 48 01 00 00 80 3D ? ? ? ? 00
        // Unique string: "Download file '%s' %s"
        //
        // CNetChan::HandleUpload(char *, int)
        {
            AY_OBFUSCATE("\x55\x89\xE5\x81\xEC\x48\x01\x00\x00\x80\x3D\x2A\x2A\x2A\x2A\x00"),
            16,
            0x60,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP
        },
        // --- PART TWO THREE FOUR AND FIVE---
        //
        // offsets for the ConMsgs in this func are
        // 1DE
        // 1B5
        // 201
        // 21D
        //
        // Signature for _ZN8CNetChan23CreateFragmentsFromFileEPKcij:
        // 55 89 E5 83 EC 48 89 5D F4 8B 5D 0C 89 7D FC
        // \x55\x89\xE5\x83\xEC\x48\x89\x5D\xF4\x8B\x5D\x0C\x89\x7D\xFC
        // Unique string: "CreateFragmentsFromFile: '%s' doesn't"
        //
        // CNetChan::CreateFragmentsFromFile(char const*, int, unsigned int)
        {
            AY_OBFUSCATE("\x55\x89\xE5\x83\xEC\x48\x89\x5D\xF4\x8B\x5D\x0C\x89\x7D\xFC"),
            15,
            0x1DE,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP
        },
        {
            AY_OBFUSCATE("\x55\x89\xE5\x83\xEC\x48\x89\x5D\xF4\x8B\x5D\x0C\x89\x7D\xFC"),
            15,
            0x1B5,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP
        },
        {
            AY_OBFUSCATE("\x55\x89\xE5\x83\xEC\x48\x89\x5D\xF4\x8B\x5D\x0C\x89\x7D\xFC"),
            15,
            0x201,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP
        },
        {
            AY_OBFUSCATE("\x55\x89\xE5\x83\xEC\x48\x89\x5D\xF4\x8B\x5D\x0C\x89\x7D\xFC"),
            15,
            0x21D,
            PATCH_IMMEDIATE,
            AY_OBFUSCATE("\x90\x90\x90\x90\x90") // CALL -> NOP NOP NOP NOP NOP
        },
        #else
        // Client only!
        /*
            Patch:
            Prevent the culling of skyboxes at high FOVs
        */
        //
        // Signature for sub_464880:
        // 55 89 E5 57 56 53 81 EC CC 02 00 00 C7 45 C8 00 00 00 00
        //
        // Uniqueish string: R_DrawSkybox
        //
        // if ( (((v6 * *&dword_B71424) + (v5 * *&dword_B71420)) + (v7 * *&dword_B71428)) < -0.29289001 )
        // ->
        // if ( (((v6 * *&dword_B71424) + (v5 * *&dword_B71420)) + (v7 * *&dword_B71428)) < -1.0 )
        //
        // R_DrawSkyBox
        {
            AY_OBFUSCATE("\x55\x89\xE5\x57\x56\x53\x81\xEC\xCC\x02\x00\x00\xC7\x45\xC8\x00\x00\x00\x00"),
            19,
            0x424,
            PATCH_REFERENCE, // we are changing the value of a float**
            -1.0f
        },
        #endif
    #endif
};



void CBinary::PostInit()
{
    // Only run this on dedicated servers, not on clients
    #ifdef TF_CLASSIC
        if (engine->IsDedicatedServer())
        {
            bool didpatches = ApplyAllPatches();

            if (!didpatches)
            {
                #ifdef dbging
                    Warning("CEngineBinary::EPostInit -> Failed to apply server patches!\n");
                #else
                    Error("Failed to apply server patches!\n");
                    return;
                #endif
            }
            #ifdef dbging
            else
            {
                ConColorMsg(goodcolor, "CEngineBinary::EPostInit -> Successfully applied server patches!\n");
            }
            #endif
        }
    #else
        bool didpatches = ApplyAllPatches();

        if (!didpatches)
        {
            #ifdef dbging
                Warning("CEngineBinary::EPostInit -> Failed to apply client patches!\n");
            #else
                Error("Failed to apply client patches!\n");
                return;
            #endif
        }
        #ifdef dbging
        else
        {
            ConColorMsg(goodcolor, "CEngineBinary::EPostInit -> Successfully applied client patches!\n");
        }
        #endif

    #endif
    return;
}

bool CBinary::ApplyAllPatches()
{
    // loop thru our engine patches
    for (int i = 0; i < (sizeof(g_EnginePatches) / sizeof(*g_EnginePatches)); i++)
    {
        // if something goes wrong, bail
        if (!g_EnginePatches[i].ApplyPatch(engine_bin))
        {
            #ifdef dbging
                Warning("CEngineBinary::ApplyAllPatches -> Couldn't apply patch %i\n", i);
                continue;
            #else
                return false;
            #endif
            
        }
        else
        {
            #ifdef dbging
                ConColorMsg(goodcolor, "CEngineBinary::ApplyAllPatches -> Applied patch %i\n", i);
            #endif
        }
    }
    return true;
}

bool CBinPatch::ApplyPatch(modbin* mbin)
{
    if (!m_pPatch)
    {
        // No value provided to patch
        #ifdef dbging
            Warning("CBinPatch::ApplyPatch -> No value provided for patch\n");
        #endif

        return false;
    }


    void* addr = (void*)memy::FindPattern(
        mbin->addr,
        mbin->size,
        m_pSignature,
        m_iSize,
        m_iOffset
    );

    if (addr)
    {
        auto pMemory = m_bImmediate ? (uintptr_t*)addr : *reinterpret_cast<uintptr_t**>(addr);

        // Memory is write-protected so it needs to be lifted before the patch is applied
        if (memy::SetMemoryProtection(pMemory, m_iPatchLength, MEM_READ | MEM_WRITE | MEM_EXEC))
        {
            V_memcpy(pMemory, m_pPatch, m_iPatchLength);

            memy::SetMemoryProtection(pMemory, m_iPatchLength, MEM_READ | MEM_EXEC);

            // Success!
            return true;
        }
        else
        {
            #ifdef dbging
                Warning("CBinPatch::ApplyPatch -> Couldn't override mem protection for pattern %s, size %i, offs %i\n", m_pSignature, m_iSize, m_iOffset);
            #endif
            return false;
        }
    }
    else
    {
        // Couldn't find sig
        #ifdef dbging
            Warning("CBinPatch::ApplyPatch -> Couldn't find sig for pattern %s, size %i, offs %i\n", m_pSignature, m_iSize, m_iOffset);
        #endif
        return false;
    }
}

CBinPatch::CBinPatch(char* signature, size_t sigsize, size_t offset, bool immediate)
{
    m_pSignature    = signature;
    m_iSize         = sigsize,
    m_iOffset       = offset;
    m_bImmediate    = immediate;
    m_pPatch        = nullptr;
    m_iPatchLength  = 0;
}

// Converting numeric types into bytes
// Wait, isn't this new a memory leak?
// -sappho
CBinPatch:: CBinPatch(char* signature, size_t sigsize, size_t offset, bool immediate, int value)
    :       CBinPatch(signature, sigsize, offset, immediate)
{
    m_iPatchLength = sizeof(value);
    m_pPatch = new char[m_iPatchLength];
    Q_memcpy(m_pPatch, &value, m_iPatchLength);
}

CBinPatch:: CBinPatch(char* signature, size_t sigsize, size_t offset, bool immediate, float value)
    :       CBinPatch(signature, sigsize, offset, immediate)
{
    m_iPatchLength = sizeof(value);
    m_pPatch = new char[m_iPatchLength];
    Q_memcpy(m_pPatch, &value, m_iPatchLength);
}

// BUGBUG!!!!
// SIZEOF ISN'T CORRECT IF YOU HAVE NULL BYTES IN THE PATCH??
CBinPatch:: CBinPatch(char* signature, size_t sigsize, size_t offset, bool immediate, char* bytes)
    :       CBinPatch(signature, sigsize, offset, immediate)
{
    m_iPatchLength = strlen(bytes);

    #ifdef dbging
        char hexstr[128] = {};
        V_binarytohex
        (
            reinterpret_cast<const byte*>(signature),
            (sigsize * 2),
            hexstr,
            (sigsize * 2)
        );

        Warning("patchlen %s = %i / %i \n", hexstr, sizeof(bytes), strlen(bytes));
    #endif

    m_pPatch = bytes;
}
