
// MapManager.h





#pragma once





#include "FunctionRef.h"
#include "Map.h"




using cMapCallback = cFunctionRef<bool(cMap &)>;




// tolua_begin

/** Manages the in-game maps of a single world - Thread safe. */
class cMapManager
{
public:
	/** Creates a new map with the same contents as an existing map.
	Returns the new map or nullptr if no more maps can be created. */
	cMap * CopyMap(cMap & a_OldMap);

	// tolua_end

	cMapManager(cWorld * a_World);

	/** Returns the map with the specified ID, nullptr if out of range.
	WARNING: The returned map object is not thread safe. */
	cMap * GetMapData(unsigned int a_ID);

	/** Creates a new map. Returns nullptr on error */
	cMap * CreateMap(short a_MapType, int a_CenterX, int a_CenterY, unsigned int a_Scale = 3);

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

	typedef std::unordered_map<unsigned int, cMap> cMapList;

	cCriticalSection m_CS;

	unsigned int m_NextID;

	cMapList m_MapData;

	cWorld * m_World;

	/** How long till the map data will be saved
	Default save interval is #defined in MAP_DATA_SAVE_INTERVAL */
	unsigned int m_TicksUntilNextSave;

	/** Allocates and returns a new ID */
	unsigned int NextID(void) { return m_NextID++; }

};  // tolua_export
