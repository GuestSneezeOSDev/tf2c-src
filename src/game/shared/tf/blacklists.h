#include <steam/isteamhttp.h>
#include "clientsteamcontext.h"

class CBlacklists : public CClientSteamContext
{
public:

    CBlacklists();

    static void         InitInit();
    static bool         CompareServerBlacklist(const char* ipaddr);
    
    CCallResult<CBlacklists, HTTPRequestCompleted_t> BlacklistsCallResult;

private:
    void                GetBlacklist();
    void                BlacklistDownloadCallback(HTTPRequestCompleted_t* arg, bool bFailed);
};
