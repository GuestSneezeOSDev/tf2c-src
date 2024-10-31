//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_player_resource.h"
#include "tf_player.h"
#include "tf_gamestats.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// This ride's going to be a little bumpy, so fasten your seatbelts when you hop on!
struct PlayerRank_t
{
	uint64 ulMaskedSteamID;
	const uint64 *pulMask;
	int iRank;
};

static const uint64 ulMaskContributingPlaytestingDeveloper = 0xFAB2423BFFA352AF;
static const uint64 ulMaskContributingPlaytester           = 0x53926FD728C825BF;
static const uint64 ulMaskContributor                      = 0x1D8BB6671CAE36CF;
static const uint64 ulMaskPlaytester                       = 0x1274D2380F3BC3DF;

#define _PLAYERRANK(steamid64, mask, rank) \
	{ (steamid64) ^ (mask), &(mask), (rank) },

#define PLAYERRANK_CONTRIB_TEST_DEV(steamid64) _PLAYERRANK(steamid64, ulMaskContributingPlaytestingDeveloper, TF_RANK_DEVELOPER | TF_RANK_CONTRIBUTOR | TF_RANK_PLAYTESTER)
#define PLAYERRANK_CONTRIB_TEST(steamid64)     _PLAYERRANK(steamid64, ulMaskContributingPlaytester,                               TF_RANK_CONTRIBUTOR | TF_RANK_PLAYTESTER)
#define PLAYERRANK_CONTRIB(steamid64)          _PLAYERRANK(steamid64, ulMaskContributor,                                          TF_RANK_CONTRIBUTOR                     )
#define PLAYERRANK_TEST(steamid64)             _PLAYERRANK(steamid64, ulMaskPlaytester,                                                                 TF_RANK_PLAYTESTER)

#define PLAYERRANK_CONTRIB_EX_DEV(steamid64)   PLAYERRANK_CONTRIB(steamid64)

static const PlayerRank_t g_PlayerRanks[] =
{
	// Developers
	PLAYERRANK_CONTRIB_TEST_DEV(76561198033171144) // Agrimar
	PLAYERRANK_CONTRIB_TEST_DEV(76561198014717105) // Benjamoose
	PLAYERRANK_CONTRIB_TEST_DEV(76561198070672899) // hunterbunney
	PLAYERRANK_CONTRIB_TEST_DEV(76561198128576895) // KaydemonLP
	PLAYERRANK_CONTRIB_TEST_DEV(76561198159282409) // MaartenS11
	PLAYERRANK_CONTRIB_TEST_DEV(76561197970945736) // MacD11
	PLAYERRANK_CONTRIB_TEST_DEV(76561198029733418) // Magnus
	PLAYERRANK_CONTRIB_TEST_DEV(76561198126285176) // MajinBro
	PLAYERRANK_CONTRIB_TEST_DEV(76561198192597054) // Mazern
	PLAYERRANK_CONTRIB_TEST_DEV(76561198004108258) // NassimO
	PLAYERRANK_CONTRIB_TEST_DEV(76561198018706932) // Old Hermit
	PLAYERRANK_CONTRIB_TEST_DEV(76561198082329692) // Omniary
	PLAYERRANK_CONTRIB_TEST_DEV(76561198149631847) // SgerbwdGwyn
	PLAYERRANK_CONTRIB_TEST_DEV(76561198023936575) // sigsegv
	PLAYERRANK_CONTRIB_TEST_DEV(76561198034114941) // Sir Matrix
	PLAYERRANK_CONTRIB_TEST_DEV(76561198006395451) // Stachekip
	PLAYERRANK_CONTRIB_TEST_DEV(76561198052461198) // Suomimies55
	PLAYERRANK_CONTRIB_TEST_DEV(76561197993638233) // Trotim
	PLAYERRANK_CONTRIB_TEST_DEV(76561198307785783) // Wheat
	// 2.0.4
	PLAYERRANK_CONTRIB_TEST_DEV(76561197983217546) // abp
	PLAYERRANK_CONTRIB_TEST_DEV(76561198825918211) // azzy
	PLAYERRANK_CONTRIB_TEST_DEV(76561198865269690) // boba
	PLAYERRANK_CONTRIB_TEST_DEV(76561198076264543) // Deathreus
	PLAYERRANK_CONTRIB_TEST_DEV(76561198088832581) // Lurondor
	PLAYERRANK_CONTRIB_TEST_DEV(76561198041080341) // newgreenshoot
	PLAYERRANK_CONTRIB_TEST_DEV(76561198412064447) // not Dave or Daniel
	PLAYERRANK_CONTRIB_TEST_DEV(76561198061895450) // PieceOfPork
	PLAYERRANK_CONTRIB_TEST_DEV(76561198000213270) // Pyrew
	PLAYERRANK_CONTRIB_TEST_DEV(76561198208622111) // sappho.io
	PLAYERRANK_CONTRIB_TEST_DEV(76561198132162105) // Technochips
	PLAYERRANK_CONTRIB_TEST_DEV(76561198121924308) // Waugh101
	// 2.1.0
	PLAYERRANK_CONTRIB_TEST_DEV(76561198286547540) // EveriK
	PLAYERRANK_CONTRIB_TEST_DEV(76561198068123507) // Exelaratore
	PLAYERRANK_CONTRIB_TEST_DEV(76561199228907126) // SamTheBitch
	PLAYERRANK_CONTRIB_TEST_DEV(76561198067179043) // Fadian
	PLAYERRANK_CONTRIB_TEST_DEV(76561198025615709) // Wonderland_War
	// (kolloom)
	PLAYERRANK_CONTRIB_TEST_DEV(76561198059601101) // Mark Unread
	PLAYERRANK_CONTRIB_TEST_DEV(76561198145902393) // NitoTheFunky
	PLAYERRANK_CONTRIB_TEST_DEV(76561199228907126) // SamDum
	PLAYERRANK_CONTRIB_TEST_DEV(76561198025319188) // Stuffy360
	PLAYERRANK_CONTRIB_TEST_DEV(76561198225676280) // ultr4nima
	PLAYERRANK_CONTRIB_TEST_DEV(76561198168842369) // Ventrici
	// 2.1.3
	PLAYERRANK_CONTRIB_TEST_DEV(76561198041975311) // 14bit
	PLAYERRANK_CONTRIB_TEST_DEV(76561198101710968) // fizzyphysics

	// Ex-developers
	PLAYERRANK_CONTRIB_EX_DEV(76561198019173266) // 2P
	PLAYERRANK_CONTRIB_EX_DEV(76561198179600693) // Alaxe
	PLAYERRANK_CONTRIB_EX_DEV(76561198030362593) // Berry
	PLAYERRANK_CONTRIB_EX_DEV(76561197988141888) // Blaholtzen
	PLAYERRANK_CONTRIB_EX_DEV(76561198219035087) // Cazsu
	PLAYERRANK_CONTRIB_EX_DEV(76561198066916585) // cheesypuff
	PLAYERRANK_CONTRIB_EX_DEV(76561198018055115) // Crazyhalo
	PLAYERRANK_CONTRIB_EX_DEV(76561198027337019) // Cufflux
	PLAYERRANK_CONTRIB_EX_DEV(76561198138908840) // Digivee
	PLAYERRANK_CONTRIB_EX_DEV(76561198011507712) // drew
	PLAYERRANK_CONTRIB_EX_DEV(76561198025334020) // DrPyspy
	PLAYERRANK_CONTRIB_EX_DEV(76561198046666510) // Drudlyclean
	PLAYERRANK_CONTRIB_EX_DEV(76561198038157852) // EonDynamo
	PLAYERRANK_CONTRIB_EX_DEV(76561198124881366) // Farlander
	PLAYERRANK_CONTRIB_EX_DEV(76561198006774758) // FissionMetroid101
	PLAYERRANK_CONTRIB_EX_DEV(76561198043782587) // Foxysen
	PLAYERRANK_CONTRIB_EX_DEV(76561197988875517) // Gadget
	PLAYERRANK_CONTRIB_EX_DEV(76561198001171456) // Game Zombie
	PLAYERRANK_CONTRIB_EX_DEV(76561198010246458) // H.Gaspar
	PLAYERRANK_CONTRIB_EX_DEV(76561197998364880) // Hawf
	PLAYERRANK_CONTRIB_EX_DEV(76561198007621815) // hotpockette
	PLAYERRANK_CONTRIB_EX_DEV(76561198062721414) // Hutty
	PLAYERRANK_CONTRIB_EX_DEV(76561197966759649) // iiboharz
	PLAYERRANK_CONTRIB_EX_DEV(76561198005557902) // Insanicide
	PLAYERRANK_CONTRIB_EX_DEV(76561198040553497) // Kikkini
	PLAYERRANK_CONTRIB_EX_DEV(76561198004373176) // Lord Blundernaut
	PLAYERRANK_CONTRIB_EX_DEV(76561198033547232) // Maxxy
	PLAYERRANK_CONTRIB_EX_DEV(76561198101094232) // Momo
	PLAYERRANK_CONTRIB_EX_DEV(76561198029219422) // MrModez
	PLAYERRANK_CONTRIB_EX_DEV(76561198053356818) // Nicknine
	PLAYERRANK_CONTRIB_EX_DEV(76561198112766514) // PistonMiner
	PLAYERRANK_CONTRIB_EX_DEV(76561198057735456) // Pretz
	PLAYERRANK_CONTRIB_EX_DEV(76561198048600700) // Py-Bun
	PLAYERRANK_CONTRIB_EX_DEV(76561198047437575) // SediSocks
	PLAYERRANK_CONTRIB_EX_DEV(76561197988525039) // Sparkwire
	PLAYERRANK_CONTRIB_EX_DEV(76561198024991627) // Square
	PLAYERRANK_CONTRIB_EX_DEV(76561197999442625) // Stev
	PLAYERRANK_CONTRIB_EX_DEV(76561198134226028) // theatreTECHIE
	PLAYERRANK_CONTRIB_EX_DEV(76561197995805528) // Thirteen
	PLAYERRANK_CONTRIB_EX_DEV(76561198399916290) // Tzlil
	PLAYERRANK_CONTRIB_EX_DEV(76561197982676963) // Zoey

	// Contributing Playtesters
	PLAYERRANK_CONTRIB_TEST(76561198080213691) // Alex Episode
	PLAYERRANK_CONTRIB_TEST(76561197994898729) // Allen Scott
	PLAYERRANK_CONTRIB_TEST(76561198025043579) // AlphaBlaster
	PLAYERRANK_CONTRIB_TEST(76561198095397492) // Anreol
	PLAYERRANK_CONTRIB_TEST(76561197992046533) // Avast AntiPony 9445
	PLAYERRANK_CONTRIB_TEST(76561198122246898) // Basysta
	PLAYERRANK_CONTRIB_TEST(76561198254418755) // Delta
	PLAYERRANK_CONTRIB_TEST(76561198127902928) // LuizFilipeRN
	PLAYERRANK_CONTRIB_TEST(76561198084242369) // Rad
	PLAYERRANK_CONTRIB_TEST(76561198828415839) // Suspect
	// 2.1.0
	PLAYERRANK_CONTRIB_TEST(76561199092517415) // HypnOS 1999
	PLAYERRANK_CONTRIB_TEST(76561198889336794) // Japonezul 75
	PLAYERRANK_CONTRIB_TEST(76561198988799866) // JJay
	PLAYERRANK_CONTRIB_TEST(76561198023100544) // Kiri
	PLAYERRANK_CONTRIB_TEST(76561198219112932) // Lev1679
	PLAYERRANK_CONTRIB_TEST(76561198451949065) // Mr. Skullium
	PLAYERRANK_CONTRIB_TEST(76561198034892765) // savva
	PLAYERRANK_CONTRIB_TEST(76561198425205873) // Wendy
	// 2.1.2
	PLAYERRANK_CONTRIB_TEST(76561198333569494) // Bavi Ratto
	PLAYERRANK_CONTRIB_TEST(76561198045208572) // Raptor Dan
	// 2.1.3
	PLAYERRANK_CONTRIB_TEST(76561198196406966) // panfractal

	// Contributors
	PLAYERRANK_CONTRIB(76561198000823482) // Bakscratch
	PLAYERRANK_CONTRIB(76561197999842942) // Boomsta
	PLAYERRANK_CONTRIB(76561198353390294) // CaptSparkle
	PLAYERRANK_CONTRIB(76561197964795247) // Cytosolic
	PLAYERRANK_CONTRIB(76561197997892136) // DaBeatzProject
	PLAYERRANK_CONTRIB(76561197973859098) // Dr. Spud
	PLAYERRANK_CONTRIB(76561198025680450) // FGD5
	PLAYERRANK_CONTRIB(76561198008861029) // Freyja
	PLAYERRANK_CONTRIB(76561197970803850) // Heyo
	PLAYERRANK_CONTRIB(76561198020560509) // Izotope
	PLAYERRANK_CONTRIB(76561198044541359) // Jukebox
	PLAYERRANK_CONTRIB(76561198022170748) // lokkdokk
	PLAYERRANK_CONTRIB(76561198205643167) // Maggots
	PLAYERRANK_CONTRIB(76561198015063202) // Mattie
	PLAYERRANK_CONTRIB(76561198000823482) // bakscratch
	PLAYERRANK_CONTRIB(76561198353390294) // Morshu ( Italian Translation ) bombe? corda? olio per lampade?
	PLAYERRANK_CONTRIB(76561198988799866) // JJay ( Spanish Translation )
	PLAYERRANK_CONTRIB(76561198205643167) // maggots ( hungarian translation )
	PLAYERRANK_CONTRIB(76561198000161739) // teownik ( polish translation )
	PLAYERRANK_CONTRIB(76561198210669477) // patrxgt ( polish translation )
	// 2.1.0
	PLAYERRANK_CONTRIB(76561198425205873) // Wendy
	PLAYERRANK_CONTRIB(76561198001122686) // murphy
	PLAYERRANK_CONTRIB(76561197972481083) // NeoDement
	PLAYERRANK_CONTRIB(76561197997491987) // Nineaxis
	PLAYERRANK_CONTRIB(76561198210669477) // Patrxgt
	PLAYERRANK_CONTRIB(76561197970127395) // Rozzy
	PLAYERRANK_CONTRIB(76561197993359730) // Svdl
	PLAYERRANK_CONTRIB(76561197960854627) // Tamari
	PLAYERRANK_CONTRIB(76561198000161739) // Teownik
	PLAYERRANK_CONTRIB(76561198003981752) // TheoF114
	PLAYERRANK_CONTRIB(76561197994150794) // void
	PLAYERRANK_CONTRIB(76561198009093943) // Yacan1
	PLAYERRANK_CONTRIB(76561197988052879) // YM
	// 2.1.1
	PLAYERRANK_CONTRIB(76561198382505273) // Emil
	PLAYERRANK_CONTRIB(76561198250471340) // H20Gamez
	PLAYERRANK_CONTRIB(76561198841559088) // Mountain Man
	// 2.1.2
	PLAYERRANK_CONTRIB(76561198039117675) // phi ( ctf_pelican_peak )
	PLAYERRANK_CONTRIB(76561198029982207) // chin ( ctf_pelican_peak )
	PLAYERRANK_CONTRIB(76561198196406966) // Fish Bait (ukrainian translation)
	PLAYERRANK_CONTRIB(76561198065978168) // Coffee Bean (ukrainian translation)
	// 2.1.3
	PLAYERRANK_CONTRIB(76561198329954436) // Dantube ( german translation )
	PLAYERRANK_CONTRIB(76561198029982207) // chin
	PLAYERRANK_CONTRIB(76561198065978168) // Coffee Bean
	PLAYERRANK_CONTRIB(76561198068032763) // Gianni Matragrano
	PLAYERRANK_CONTRIB(76561198039117675) // phi
	PLAYERRANK_CONTRIB(76561198072146551) // Diva Dan
	PLAYERRANK_CONTRIB(76561198101099843) // erk

	// Playtesters
	PLAYERRANK_TEST(76561197993622014) // ARMaster
	PLAYERRANK_TEST(76561198091919938) // Azure
	PLAYERRANK_TEST(76561198146096076) // Barple Bapkins
	PLAYERRANK_TEST(76561198078144144) // Bikos77
	PLAYERRANK_TEST(76561198057156382) // Bine
	PLAYERRANK_TEST(76561198119577732) // borkiee
	PLAYERRANK_TEST(76561198261851104) // Bumpmap
	PLAYERRANK_TEST(76561198065324447) // B.A.S.E
	PLAYERRANK_TEST(76561198115876488) // Cyruktez
	PLAYERRANK_TEST(76561198167997186) // dead_thing
	PLAYERRANK_TEST(76561198082283950) // Dr. Seal
	PLAYERRANK_TEST(76561198007462668) // ENDove
	PLAYERRANK_TEST(76561198062722856) // EverMatt
	PLAYERRANK_TEST(76561198199830546) // Eyedol9000
	PLAYERRANK_TEST(76561197972236154) // Goodies
	PLAYERRANK_TEST(76561198142494252) // GuffKid
	PLAYERRANK_TEST(76561198193780653) // HDMineFace
	PLAYERRANK_TEST(76561198085098321) // Hex
	PLAYERRANK_TEST(76561198174031850) // Igarni
	PLAYERRANK_TEST(76561198018687683) // JimmDallas
	PLAYERRANK_TEST(76561198263004448) // Kristofer
	PLAYERRANK_TEST(76561198083491218) // laura
	PLAYERRANK_TEST(76561198068094602) // lexile
	PLAYERRANK_TEST(76561198063905055) // MoiraPrime
	PLAYERRANK_TEST(76561198139797454) // Moldy
	PLAYERRANK_TEST(76561198005690007) // onefourth
	PLAYERRANK_TEST(76561198084239115) // PacMania67
	PLAYERRANK_TEST(76561198958482440) // ProfessorPootis
	PLAYERRANK_TEST(76561198253692266) // ralph
	PLAYERRANK_TEST(76561198049254607) // Reddking
	PLAYERRANK_TEST(76561198046846304) // Retronary
	PLAYERRANK_TEST(76561198136740758) // Rhyan
	PLAYERRANK_TEST(76561198009192838) // Saint
	PLAYERRANK_TEST(76561198851124770) // Sandvich Thief
	PLAYERRANK_TEST(76561197991605918) // Sgt. Stacker
	PLAYERRANK_TEST(76561198253768763) // Snowy Snowtime
	PLAYERRANK_TEST(76561198043546441) // SolarFlare10000
	PLAYERRANK_TEST(76561198305988335) // sploink
	PLAYERRANK_TEST(76561198116553704) // swix
	PLAYERRANK_TEST(76561198202094036) // TR4SHQU33N
	PLAYERRANK_TEST(76561198128547733) // Trittyburd
	PLAYERRANK_TEST(76561198023268899) // Xyk
	PLAYERRANK_TEST(76561197968120420) // ZombieGuildford
	PLAYERRANK_TEST(76561198091919938) // Azure
	PLAYERRANK_TEST(76561198065324447) // B.A.S.E
	PLAYERRANK_TEST(76561198078144144) // Bikos77
	PLAYERRANK_TEST(76561198115876488) // Cyruktez
	PLAYERRANK_TEST(76561198199830546) // Eyedol9000
	PLAYERRANK_TEST(76561198083491218) // laura
	PLAYERRANK_TEST(76561198305988335) // GibusCat
	PLAYERRANK_TEST(76561198084239115) // PacMania67
	PLAYERRANK_TEST(76561198263004448) // Private Polygon
	PLAYERRANK_TEST(76561198009192838) // Saint
	PLAYERRANK_TEST(76561198253692266) // FarmZombie_
	PLAYERRANK_TEST(76561198253768763) // Snowy
	PLAYERRANK_TEST(76561198043546441) // SolarFlare10000
	PLAYERRANK_TEST(76561198128547733) // Trittyburd
	PLAYERRANK_TEST(76561198018687683) // JimmDallas
	PLAYERRANK_TEST(76561198046846304) // Androsynth
	PLAYERRANK_TEST(76561198142494252) // GuffKid
	PLAYERRANK_TEST(76561198063905055) // Moira Prime
	PLAYERRANK_TEST(76561198261851104) // Bumpmap
	PLAYERRANK_TEST(76561198139797454) // Moldy
	PLAYERRANK_TEST(76561198167997186) // Dead_thing
	PLAYERRANK_TEST(76561198068094602) // Lexile
	PLAYERRANK_TEST(76561198082283950) // Dr. Seal
	PLAYERRANK_TEST(76561198958482440) // ProfessorPootis/Heavy
	PLAYERRANK_TEST(76561198851124770) // SandvichThief

	// 2.1.0
	PLAYERRANK_TEST(76561199028624786) // beegscarf
	PLAYERRANK_TEST(76561198095722739) // CarlmanZ
	PLAYERRANK_TEST(76561198164315915) // Clanker707
	PLAYERRANK_TEST(76561198434522602) // Endermage77
	PLAYERRANK_TEST(76561198094637461) // Eris
	PLAYERRANK_TEST(76561197989848197) // julian juliansson
	PLAYERRANK_TEST(76561198312300565) // Juno
	PLAYERRANK_TEST(76561198117216644) // Kai
	PLAYERRANK_TEST(76561198061173364) // KingSluggies
	PLAYERRANK_TEST(76561198040878923) // MrCrossa
	PLAYERRANK_TEST(76561198449966087) // mufina
	PLAYERRANK_TEST(76561197999225040) // Olmus
	PLAYERRANK_TEST(76561197973074856) // Reagy
	PLAYERRANK_TEST(76561198105594962) // steff
	PLAYERRANK_TEST(76561198156042296) // swa
	PLAYERRANK_TEST(76561198403209507) // TheNameNoOneKnew
	PLAYERRANK_TEST(76561198124781832) // Vror
	PLAYERRANK_TEST(76561198297337023) // your
	// 2.1.2
	PLAYERRANK_TEST(76561198121399739) // Aceknow
	PLAYERRANK_TEST(76561198031239041) // Ciccioimberlicchio
	PLAYERRANK_TEST(76561198137283895) // enasona
	PLAYERRANK_TEST(76561198043872052) // Future10s
	PLAYERRANK_TEST(76561198025434795) // Glömmerska
	PLAYERRANK_TEST(76561198042311946) // Kube
	PLAYERRANK_TEST(76561198009832041) // PeeGurts
	PLAYERRANK_TEST(76561198079645112) // Tarluk
	PLAYERRANK_TEST(76561198121399739) // Aceknow
	// 2.1.3
	PLAYERRANK_TEST(76561198069525597) // Noclue
	PLAYERRANK_TEST(76561198068383694) // dopadream
	PLAYERRANK_TEST(76561198296928593) // fiiv_e
	PLAYERRANK_TEST(76561198206157715) // J1may
	PLAYERRANK_TEST(76561198975527343) // vivian
};


// IF THIS DATATABLE IS  DT_TFPlayerResource  THAN SOURCEMOD DETECTS THIS MOD AS TF2
// THIS IS NO GOOD
// -sappho.io
IMPLEMENT_SERVERCLASS_ST( CTFPlayerResource, DT_TF2CPlayerResource )
	SendPropArray3( SENDINFO_ARRAY3( m_iTotalScore ), SendPropInt( SENDINFO_ARRAY( m_iTotalScore ), 12, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iMaxHealth ), SendPropInt( SENDINFO_ARRAY( m_iMaxHealth ), 10, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerClass ), SendPropInt( SENDINFO_ARRAY( m_iPlayerClass ), 5, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iActiveDominations ), SendPropInt( SENDINFO_ARRAY( m_iActiveDominations ), Q_log2( MAX_PLAYERS ) + 1, SPROP_UNSIGNED ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bArenaSpectator ), SendPropBool( SENDINFO_ARRAY( m_bArenaSpectator ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageAssist ), SendPropInt( SENDINFO_ARRAY( m_iDamageAssist ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iDamageBlocked ), SendPropInt( SENDINFO_ARRAY( m_iDamageBlocked ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iBonusPoints ), SendPropInt( SENDINFO_ARRAY( m_iBonusPoints ), -1, SPROP_UNSIGNED | SPROP_VARINT ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_iPlayerRank ), SendPropInt( SENDINFO_ARRAY( m_iPlayerRank ), -1, SPROP_UNSIGNED ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( tf_player_manager, CTFPlayerResource );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPlayerResource::CTFPlayerResource( void )
{

}


void CTFPlayerResource::UpdatePlayerData( void )
{
	int i, j, x, iPing, iPacketloss;
	const CSteamID *psteamID;
	CTFPlayer *pPlayer;
	PlayerStats_t *pPlayerStats;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pPlayer = static_cast<CTFPlayer *>( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsConnected() )
		{
			// Skipped the base code to cut down on a for loop.
			{
				m_iScore.Set( i, pPlayer->FragCount() );
				m_iDeaths.Set( i, pPlayer->DeathCount() );
				m_bConnected.Set( i, 1 );
				m_iTeam.Set( i, pPlayer->GetTeamNumber() );
				m_bAlive.Set( i, pPlayer->IsAlive() ? 1 : 0 );
				m_iHealth.Set( i, Max( 0, pPlayer->GetHealth() ) );

				// Don't update ping or packetloss every time.
				if ( !( m_nUpdateCounter % 20 ) )
				{
					// Update ping all 20 think ticks (20 * 0.1 = 2 seconds).
					UTIL_GetPlayerConnectionInfo( i, iPing, iPacketloss );
				
					// Calc average for scoreboard so it's not so jittery.
					m_iPing.Set( i, 0.8f * m_iPing.Get( i ) + 0.2f * iPing );
					//m_iPacketloss.Set( i, iPacketloss );
				}
			}

			m_iMaxHealth.Set( i, pPlayer->GetMaxHealth() );
			m_iPlayerClass.Set( i, pPlayer->GetPlayerClass()->GetClassIndex() );

			m_iActiveDominations.Set( i, pPlayer->GetNumberOfDominations() );
			m_bArenaSpectator.Set( i, pPlayer->IsArenaSpectator() );

			pPlayerStats = CTF_GameStats.FindPlayerStats( pPlayer );
			if ( pPlayerStats )
			{
				m_iTotalScore.Set( i, CTFGameRules::CalcPlayerScore( &pPlayerStats->statsAccumulated ) );
				m_iDamageAssist.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_ASSIST] );
				m_iDamageBlocked.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_DAMAGE_BLOCKED] );
				m_iBonusPoints.Set( i, pPlayerStats->statsCurrentRound.m_iStat[TFSTAT_BONUS_POINTS] );
			}

			// Set the player's current rank (Developer, Contributor, Playtester).
			// This will be shown next to all instances of this player's name via a medal.
			if ( m_iPlayerRank[i] == TF_RANK_INVALID )
			{
				psteamID = engine->GetClientSteamID( pPlayer->edict() );
				if ( psteamID )
				{
					m_iPlayerRank.Set( i, TF_RANK_NONE );

					for ( j = 0, x = ARRAYSIZE( g_PlayerRanks ); j < x; j++ )
					{
						if ( psteamID->ConvertToUint64() == ( g_PlayerRanks[j].ulMaskedSteamID ^ *g_PlayerRanks[j].pulMask ) )
						{
							// We've found this player's embedded rank, set it and we're done here.
							m_iPlayerRank.Set( i, g_PlayerRanks[j].iRank );
							break;
						}
					}
				}
			}
		}
		else
		{
			m_bConnected.Set( i, 0 );
			m_iPlayerRank.Set( i, TF_RANK_INVALID );
		}
	}
}


void CTFPlayerResource::Spawn( void )
{
	for ( int i = 0; i < MAX_PLAYERS + 1; i++ )
	{
		m_iTotalScore.Set( i, 0 );
		m_iMaxHealth.Set( i, 1 );
		m_iPlayerClass.Set( i, TF_CLASS_UNDEFINED );
		m_iPlayerRank.Set( i, TF_RANK_INVALID );
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Gets a value from an array member
//-----------------------------------------------------------------------------
int CTFPlayerResource::GetTotalScore( int iIndex )
{
	CTFPlayer *pPlayer = static_cast<CTFPlayer *>( UTIL_PlayerByIndex( iIndex ) );
	if ( pPlayer && pPlayer->IsConnected() )
		return m_iTotalScore[iIndex];

	return 0;
}


int CTFPlayerResource::GetPlayerRank( int iIndex )
{
	CTFPlayer *pPlayer = static_cast<CTFPlayer *>( UTIL_PlayerByIndex( iIndex ) );
	if ( pPlayer && pPlayer->IsConnected() )
		return m_iPlayerRank[iIndex];

	return TF_RANK_NONE;
}


void CTFPlayerResource::SetPlayerRank( int iIndex, int iRank )
{
	m_iPlayerRank.Set( iIndex, iRank );
}


CSteamID CTFPlayerResource::GetPlayerSteamID( int iIndex )
{
	CSteamID steamID;

	CTFPlayer *pPlayer = static_cast<CTFPlayer *>( UTIL_PlayerByIndex( iIndex ) );
	if ( pPlayer && pPlayer->IsConnected() )
	{
		pPlayer->GetSteamID( &steamID );
	}

	return steamID;
}
