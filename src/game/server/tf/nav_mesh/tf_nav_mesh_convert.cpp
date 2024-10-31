/* TF Nav Mesh
 * based on code in modern TF2, reverse engineered by sigsegv
 */


#include "cbase.h"
#include "tf_nav_mesh.h"
#include "fmtstr.h"


template<class T> static inline void BufGet(CUtlBuffer& buf, T& dst);
template<> inline void BufGet(CUtlBuffer& buf, Vector& dst) { buf.Get(&dst, 3 * sizeof(float)); }
template<> inline void BufGet(CUtlBuffer& buf, uint8&  dst) { dst = buf.GetUnsignedChar ();     }
template<> inline void BufGet(CUtlBuffer& buf, uint16& dst) { dst = buf.GetUnsignedShort();     }
template<> inline void BufGet(CUtlBuffer& buf, uint32& dst) { dst = buf.GetUnsignedInt  ();     }
template<> inline void BufGet(CUtlBuffer& buf, int32&  dst) { dst = buf.GetInt          ();     }
template<> inline void BufGet(CUtlBuffer& buf, int64&  dst) { dst = buf.GetInt64        ();     }
template<> inline void BufGet(CUtlBuffer& buf, float&  dst) { dst = buf.GetFloat        ();     }

template<class T> static inline void BufPut(CUtlBuffer& buf, const T& src);
template<> inline void BufPut(CUtlBuffer& buf, const Vector& src) { buf.Put(&src, 3 * sizeof(float)); }
template<> inline void BufPut(CUtlBuffer& buf, const uint8&  src) { buf.PutUnsignedChar (src);        }
template<> inline void BufPut(CUtlBuffer& buf, const uint16& src) { buf.PutUnsignedShort(src);        }
template<> inline void BufPut(CUtlBuffer& buf, const uint32& src) { buf.PutUnsignedInt  (src);        }
template<> inline void BufPut(CUtlBuffer& buf, const uint64& src) { buf.PutUint64       (src);        }
template<> inline void BufPut(CUtlBuffer& buf, const int32&  src) { buf.PutInt          (src);        }
template<> inline void BufPut(CUtlBuffer& buf, const int64&  src) { buf.PutInt64        (src);        }
template<> inline void BufPut(CUtlBuffer& buf, const float&  src) { buf.PutFloat        (src);        }


static_assert(MAX_NAV_TEAMS   == 4, "Team number assumptions in nav file conversion code violated!");
static_assert(FIRST_GAME_TEAM == 2, "Team number assumptions in nav file conversion code violated!");
static_assert(TF_TEAM_COUNT   == 6, "Team number assumptions in nav file conversion code violated!");

static_assert(CTFNavMesh::SUB_VERSION_CURRENT == 0x100, "The live TF2 --> TF2C nav file conversion code needs to be updated!");


// NAV FILE CONVERSION LIMITATIONS:
// - all new-to-TF2C nav area attributes will be unset by default
//   - GREEN_SETUP_GATE
//   - YELLOW_SETUP_GATE
//   - GREEN_ONE_WAY_DOOR
//   - YELLOW_ONE_WAY_DOOR
// - anything dependent on MAX_NAV_TEAMS won't have any info for green and yellow
//   - CNavAreaCriticalData::m_earliestOccupyTime (only used for CS)


/* table: index with bit number from live TF2 to get bitmask for TF2C */
static const uint64 s_TFNavAttrConv[] = {
	CTFNavArea::BLOCKED,                     // bit 0
	CTFNavArea::RED_SPAWN_ROOM,              // bit 1
	CTFNavArea::BLUE_SPAWN_ROOM,             // bit 2
	CTFNavArea::SPAWN_ROOM_EXIT,             // bit 3
	CTFNavArea::AMMO,                        // bit 4
	CTFNavArea::HEALTH,                      // bit 5
	CTFNavArea::CONTROL_POINT,               // bit 6
	CTFNavArea::BLUE_SENTRY,                 // bit 7
	CTFNavArea::RED_SENTRY,                  // bit 8
	CTFNavArea::UNKNOWN_BIT09,               // bit 9
	CTFNavArea::UNKNOWN_BIT10,               // bit 10
	CTFNavArea::BLUE_SETUP_GATE,             // bit 11
	CTFNavArea::RED_SETUP_GATE,              // bit 12
	CTFNavArea::BLOCKED_AFTER_POINT_CAPTURE, // bit 13
	CTFNavArea::BLOCKED_UNTIL_POINT_CAPTURE, // bit 14
	CTFNavArea::BLUE_ONE_WAY_DOOR,           // bit 15
	CTFNavArea::RED_ONE_WAY_DOOR,            // bit 16
	CTFNavArea::WITH_SECOND_POINT,           // bit 17
	CTFNavArea::WITH_THIRD_POINT,            // bit 18
	CTFNavArea::WITH_FOURTH_POINT,           // bit 19
	CTFNavArea::WITH_FIFTH_POINT,            // bit 20
	CTFNavArea::SNIPER_SPOT,                 // bit 21
	CTFNavArea::SENTRY_SPOT,                 // bit 22
	CTFNavArea::ESCAPE_ROUTE,                // bit 23
	CTFNavArea::ESCAPE_ROUTE_VISIBLE,        // bit 24
	CTFNavArea::NO_SPAWNING,                 // bit 25
	CTFNavArea::RESCUE_CLOSET,               // bit 26
	CTFNavArea::BOMB_DROP,                   // bit 27
	CTFNavArea::DOOR_NEVER_BLOCKS,           // bit 28
	CTFNavArea::DOOR_ALWAYS_BLOCKS,          // bit 29
	CTFNavArea::UNBLOCKABLE,                 // bit 30
	CTFNavArea::UNKNOWN_BIT31,               // bit 31
};
static_assert(ARRAYSIZE(s_TFNavAttrConv) == 32, "");


class CTFNavFileConverter
{
public:
	CTFNavFileConverter() :
		m_BufIn (4096, 1024 * 1024, CUtlBuffer::READ_ONLY),
		m_BufOut(4096, 1024 * 1024) {}
	
	
	bool Convert(const char *path_in, const char *path_out)
	{
		m_PathIn  = path_in;
		m_PathOut = path_out;
		
		if (!ReadInputFile())       return false;
		if (!DoMagicNumber())       return false;
		if (!DoVersionNumber())     return false;
		if (!DoSubVersionNumber())  return false;
		if (!DoBSPFileSize())       return false;
		if (!DoAnalysisFlag())      return false;
		if (!DoPlaceDirectory())    return false;
		if (!DoCustomDataPreArea()) return false;
		if (!DoNavAreas())          return false;
		if (!DoNavLadders())        return false;
		if (!DoCustomData())        return false;
		if (!WriteOutputFile())     return false;
		
		Say("Success! (\"%s\" --> \"%s\")", m_PathIn, m_PathOut);
		return true;
	}
	
	
private:
	bool ReadInputFile()
	{
		/* mimic the exact manner in which CNavMesh::Load loads nav files:
		 * only load from the BSP path as a last resort */
		if (!filesystem->ReadFile(m_PathIn, "MOD", m_BufIn) && !filesystem->ReadFile(m_PathIn, "BSP", m_BufIn)) {
			Say("Couldn't read input file \"%s\".", m_PathIn);
			return false;
		}
		return true;
	}
	
	bool DoMagicNumber()
	{
		uint32 magic;
		if (!Get(magic)) {
			Say("Reached the end of the input file before the magic number!");
			return false;
		}
		if (magic != NAV_MAGIC_NUMBER) {
			Say("Input file's magic number is invalid!\n[Valid: %08x] [File: %08x]", NAV_MAGIC_NUMBER, magic);
			return false;
		}
		Put(magic);
		return true;
	}
	
	bool DoVersionNumber()
	{
		uint32 version;
		if (!Get(version)) {
			Say("Reached the end of the input file before the version number!");
			return false;
		}
		if (version != 16) {
			Say("Input file's version number is unexpected!\n[Expected: %u] [File: %u]", 16, version);
			return false;
		}
		Put(version);
		return true;
	}
	
	bool DoSubVersionNumber()
	{
		uint32 sub_version;
		if (!Get(sub_version)) {
			Say("Reached the end of the input file before the sub-version number!");
			return false;
		}
		if (sub_version != 2) {
			Say("Input file's sub-version number is unexpected!\n[Expected: %u] [File: %u]", 2, sub_version);
			return false;
		}
		Put<uint32>(CTFNavMesh::SUB_VERSION_CURRENT);
		return true;
	}
	
	bool DoBSPFileSize()
	{
		if (!Copy<uint32>()) {
			Say("Reached the end of the input file before the BSP file size value!");
			return false;
		}
		return true;
	}
	
	bool DoAnalysisFlag()
	{
		if (!Copy<uint8>()) {
			Say("Reached the end of the input file before the BSP analysis state flag!");
			return false;
		}
		return true;
	}
	
	bool DoPlaceDirectory()
	{
		uint16 num_places;
		if (!Get(num_places)) {
			Say("Reached the end of the input file before the place directory count!");
			return false;
		}
		Put(num_places);
		
		while (num_places-- != 0) {
			uint16 len;
			char buf[256];
			
			Get(len);
			len = Min<size_t>(len, sizeof(buf));
			Put(len);
			
			m_BufIn.Get(buf, len);
			m_BufIn.Put(buf, len);
			
			if (!m_BufIn.IsValid()) {
				Say("Reached the end of the input file before finishing the place directory!");
				return false;
			}
		}
		
		if (!Copy<uint8>()) {
			Say("Reached the end of the input file before the unnamed area flag!");
			return false;
		}
		
		return true;
	}
	
	bool DoCustomDataPreArea()
	{
		Put<uint32>(CTFNavMesh::TF2C_MAGIC);
		return true;
	}
	
	bool DoNavAreas()
	{
		uint32 num_areas;
		if (!Get(num_areas)) {
			Say("Reached the end of the input file before the nav area count!");
			return false;
		}
		Put(num_areas);
		
		bool ok = true;
		while (num_areas-- != 0) {
			Copy<uint32>(); // m_id
			Copy<int32>();  // m_attributeFlags
			Copy<Vector>(); // m_nwCorner
			Copy<Vector>(); // m_seCorner
			Copy<float>();  // m_neZ
			Copy<float>();  // m_swZ
			
			/* m_connect */
			for (int dir = 0; dir < NUM_DIRECTIONS; ++dir) {
				uint32 num_connections;
				if (!Get(num_connections)) {
					ok = false;
					break;
				}
				Put(num_connections);
				while (num_connections-- != 0) {
					Copy<uint32>(); // id
				}
			}
			
			/* m_hidingSpots */
			uint8 num_hidingspots;
			if (!Get(num_hidingspots)) {
				ok = false;
				break;
			}
			Put(num_hidingspots);
			while (num_hidingspots-- != 0) {
				Copy<uint32>(); // m_id
				Copy<float>();  // m_pos.x
				Copy<float>();  // m_pos.y
				Copy<float>();  // m_pos.z
				Copy<uint8>();  // m_flags
			}
			
			/* m_spotEncounters */
			uint32 num_spotencounters;
			if (!Get(num_spotencounters)) {
				ok = false;
				break;
			}
			Put(num_spotencounters);
			while (num_spotencounters-- != 0) {
				Copy<uint32>(); // from.id
				Copy<uint8>();  // fromDir
				Copy<uint32>(); // to.id
				Copy<uint8>();  // toDir
				
				/* spots */
				uint8 num_spots;
				if (!Get(num_spots)) {
					ok = false;
					break;
				}
				Put(num_spots);
				while (num_spots-- != 0) {
					Copy<uint32>(); // id
					Copy<uint8>();  // t
				}
			}
			
			/* place index */
			Copy<uint16>();
			
			/* m_ladder */
			for (int dir = 0; dir < CNavLadder::NUM_LADDER_DIRECTIONS; ++dir) {
				uint32 num_ladders;
				if (!Get(num_ladders)) {
					ok = false;
					break;
				}
				Put(num_ladders);
				while (num_ladders-- != 0) {
					Copy<uint32>(); // id
				}
			}
			
			/* m_earliestOccupyTime */
			Put<float>(0.0f); // nav team 0 (real team 4: TF_TEAM_GREEN)
			Put<float>(0.0f); // nav team 1 (real team 5: TF_TEAM_YELLOW)
			Copy<float>();    // nav team 2 (real team 2: TF_TEAM_RED)
			Copy<float>();    // nav team 3 (real team 3: TF_TEAM_BLUE)
			
			/* m_lightIntensity */
			for (int corner = 0; corner < NUM_CORNERS; ++corner) {
				Copy<float>();
			}
			
			/* m_potentiallyVisibleAreas */
			uint32 num_pvareas;
			if (!Get(num_pvareas)) {
				ok = false;
				break;
			}
			Put(num_pvareas);
			while (num_pvareas-- != 0) {
				Copy<uint32>(); // id
				Copy<uint8>();  // attributes
			}
			
			/* m_inheritVisibilityFrom.id */
			Copy<uint32>();
			
			/* m_nTFAttributes (bit conversion!) */
			uint32 old_attr;
			Get(old_attr);
			uint64 new_attr = 0;
			for (int bit = 0; bit < 32; ++bit) {
				if ((old_attr & (1U << bit)) != 0) {
					new_attr |= s_TFNavAttrConv[bit];
				}
			}
			Put(new_attr);
			
			if (!m_BufIn.IsValid()) {
				ok = false;
				break;
			}
		}
		
		if (!ok) {
			Say("Reached the end of the input file before finishing the nav area section!");
			return false;
		}
		
		return true;
	}
	
	bool DoNavLadders()
	{
		uint32 num_ladders;
		if (!Get(num_ladders)) {
			Say("Reached the end of the input file before the nav ladder count!");
			return false;
		}
		Put(num_ladders);
		
		while (num_ladders-- != 0) {
			Copy<uint32>(); // m_id
			Copy<float>();  // m_width
			Copy<float>();  // m_top.x
			Copy<float>();  // m_top.y
			Copy<float>();  // m_top.z
			Copy<float>();  // m_bottom.x
			Copy<float>();  // m_bottom.y
			Copy<float>();  // m_bottom.z
			Copy<float>();  // m_length
			Copy<uint32>(); // m_dir
			Copy<uint32>(); // m_topForwardArea
			Copy<uint32>(); // m_topLeftArea
			Copy<uint32>(); // m_topRightArea
			Copy<uint32>(); // m_topBehindArea
			Copy<uint32>(); // m_bottomArea
			
			if (!m_BufIn.IsValid()) {
				Say("Reached the end of the input file before finishing the nav ladder section!");
				return false;
			}
		}
		
		return true;
	}
	
	bool DoCustomData()
	{
		if (m_BufIn.GetBytesRemaining() != 0) {
			Say("The input file seems to have %d extraneous bytes at the end!", m_BufIn.GetBytesRemaining());
			return false;
		}
		return true;
	}
	
	bool WriteOutputFile()
	{
		if (!filesystem->WriteFile(m_PathOut, "MOD", m_BufOut)) {
			Say("Couldn't write output file \"%s\".", m_PathOut);
			return false;
		}
		return true;
	}
	
	
	template<class T>
	bool Copy()
	{
		T val;
		if (!Get(val)) {
			return false;
		}
		Put(val);
		return true;
	}
	
	template<class T>
	bool Get(T& dst)
	{
		BufGet<T>(m_BufIn, dst);
		return m_BufIn.IsValid();
	}
	
	template<class T>
	void Put(const T& src)
	{
		BufPut<T>(m_BufOut, src);
	}
	
	
	template<class... ARGS>
	static void Say(const char *msg, ARGS&&... args)
	{
		Msg("Nav File Conversion: %s\n", CFmtStr(msg, std::forward<ARGS>(args)...).Get());
	}
	static void Say(const char *msg)
	{
		Msg("Nav File Conversion: %s\n", msg);
	}
	
	
	CUtlBuffer m_BufIn;
	CUtlBuffer m_BufOut;
	
	const char *m_PathIn  = nullptr;
	const char *m_PathOut = nullptr;
};


CON_COMMAND_F(tf2c_convert_nav_file, "Convert an existing nav file from live TF2 into one usable by TF2C. Only works while hosting a game on a listen server (any map).", FCVAR_NONE)
{
	if (args.ArgC() != 3) {
		Msg("Usage: %s <input.nav> <output.nav>\n", args[0]);
		return;
	}
	
	CTFNavFileConverter conv;
	conv.Convert(args[1], args[2]);
}
