#include "discord_register.h"
#include "discord-rpc.h"

class CTFDiscordRPC : public CAutoGameSystemPerFrame, public CGameEventListener
{
public:
	CTFDiscordRPC(const char* pszName);
	~CTFDiscordRPC();

	// Methods of IGameSystem
	virtual bool Init(void) override;
	virtual void Shutdown(void) override;
	virtual void LevelInitPostEntity(void) override;
	virtual void LevelShutdownPostEntity(void) override;
	// Gets called each frame
	virtual void Update(float frametime);

	virtual void FireGameEvent(IGameEvent* event);

	void UpdatePresence(void);
	bool CountPlayers(void);
	const char* GetGameTypeName(void);


	DiscordUser* localdiscorduser = nullptr;

	bool m_bDisabledViaCommandLine = false;
private:

	time_t m_iConnectionTime = {};
	char m_szConnectionString[128] = {};
	char m_szSteamID[128] = {};
	int m_iNumPlayers = 0;
	bool m_bInGame = false;

	bool m_bIsSaneServerToConnectTo = false;
	char m_szHostName[128] = {};
};

extern CTFDiscordRPC g_Discord;