//---------------------------------------------------------------------------------------------
// Credits to Momentum Mod for this code and specifically xen-000
//---------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// Functions used to find patterns of bytes in the engine's memory to hook or patch
//-----------------------------------------------------------------------------------
#ifdef _WIN32
#pragma once
#endif

class CBinary : public CAutoGameSystem
{
public:
                        CBinary();

    void                PostInit() override;

private:
    static bool         ApplyAllPatches();

};

enum PatchType
{
    PATCH_IMMEDIATE     = true,
    PATCH_REFERENCE     = false
};

class CBinPatch
{
public:
    CBinPatch(char*, size_t, size_t, bool);
    CBinPatch(char*, size_t, size_t, bool, int);
    CBinPatch(char*, size_t, size_t, bool, float);
    CBinPatch(char*, size_t, size_t, bool, char*);

    bool ApplyPatch(modbin* mbin);

private:
    char *m_pSignature;
    char *m_pPatch;
    size_t m_iSize;

    size_t m_iOffset;
    size_t m_iPatchLength;

    bool m_bImmediate;
};
