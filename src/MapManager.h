
// MapManager.h





#pragma once





#include "FunctionRef.h"
#include "Map.h"




using cMapCallback = cFunctionRef<bool(cMap &)>;




// tolua_begin

/** Manages the in-game maps of a single world - Thread safe. */
class cMapManager
{
	// tolua_end

private:

	const unsigned int MAP_DATA_SAVE_INTERVAL = 6000;  // 6000 ticks or 5 minutes

public:
	// tolua_begin

	/** Creates a new map with the same contents as an existing map.
	Returns the new map or nullptr if no more maps can be created. */
	std::shared_ptr<cMap> CopyMap(cMap & a_OldMap);

	// tolua_end

	constexpr cMapManager(cWorld * a_World):
		m_World(a_World),
		m_TicksUntilNextSave(MAP_DATA_SAVE_INTERVAL)
	{
	}

	/** Creates a new map. Returns false on error otherwise the new map ID is in a_Map_ID. */
	bool CreateMap(unsigned int & a_MapID, short a_MapType, int a_CenterX, int a_CenterY, unsigned int a_Scale = 0);

	/** Calls the callback for the map with the specified ID.
	Returns true if the map was found and the callback called, false if map not found.
	Callback return value is ignored. */
	bool DoWithMap(UInt32 a_ID, cMapCallback a_Callback);  // Exported in ManualBindings.cpp

	/** Ticks each registered map */
	void TickMaps(void);

	/** Loads the map data from the disk */
	void LoadMapData(void);

	/** Saves the map data to the disk */
	void SaveMapData(void);


private:

	/** Returns the map with the specified ID, nullptr if out of range. */
	std::shared_ptr<cMap> GetMapData(unsigned int a_ID);

	typedef std::unordered_map<unsigned int, std::shared_ptr<cMap>> cMapList;

	static cCriticalSection m_CS;

	static unsigned int m_NextID;

	static cMapList m_MapData;

	cWorld * m_World;

	/** How long till the map data will be saved
	Default save interval is #defined in MAP_DATA_SAVE_INTERVAL */
	unsigned int m_TicksUntilNextSave;

	/** Allocates and returns a new ID */
	unsigned int NextID(void) { return m_NextID++; }

};  // tolua_export
