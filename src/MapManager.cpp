
// MapManager.cpp

#include "Globals.h"

#include "MapManager.h"

#include "World.h"
#include "WorldStorage/MapSerializer.h"





// 6000 ticks or 5 minutes
#define MAP_DATA_SAVE_INTERVAL 6000





cMapManager::cMapManager(cWorld * a_World) :
	m_NextID(0),
	m_World(a_World),
	m_TicksUntilNextSave(MAP_DATA_SAVE_INTERVAL)
{
	ASSERT(m_World != nullptr);
}





bool cMapManager::DoWithMap(UInt32 a_ID, cMapCallback a_Callback)
{
	cCSLock Lock(m_CS);
	cMap * Map = GetMapData(a_ID);

	if (Map == nullptr)
	{
		return false;
	}
	else
	{
		a_Callback(*Map);
		return true;
	}
}





void cMapManager::TickMaps()
{
	cCSLock Lock(m_CS);

	for (auto & [MapID, Map] : m_MapData)
	{
		UNUSED(MapID);

		Map.Tick();
	}

	if (m_TicksUntilNextSave == 0)
	{
		m_TicksUntilNextSave = MAP_DATA_SAVE_INTERVAL;
		SaveMapData();
	}
	else
	{
		m_TicksUntilNextSave--;
	}
}





cMap * cMapManager::GetMapData(unsigned int a_ID)
{
	try
	{
		return & m_MapData.at(a_ID);
	}
	catch (const std::out_of_range & ex)
	{
		return nullptr;
	}
}





cMap * cMapManager::CreateMap(short a_MapType, int a_CenterX, int a_CenterY, unsigned int a_Scale)
{
	cCSLock Lock(m_CS);

	unsigned int ID = NextID();
	auto [it, inserted] = m_MapData.try_emplace(ID, ID, a_MapType, a_CenterX, a_CenterY, m_World, a_Scale);

	if (!inserted)
	{
		// Either the IDs wrapped or something went horribly wrong
		LOGWARN(fmt::format(FMT_STRING("Could not craft map {} - too many maps in use?"), ID));
		return nullptr;
	}

	return &it->second;
}





cMap * cMapManager::CopyMap(cMap & a_OldMap)
{
	cCSLock Lock(m_CS);

	unsigned int ID = NextID();
	auto [it, inserted] = m_MapData.try_emplace(ID, ID, a_OldMap);

	if (!inserted)
	{
		// Either the IDs wrapped or something went horribly wrong
		LOGWARN(fmt::format(FMT_STRING("Could not craft map {} - too many maps in use?"), ID));
		return nullptr;
	}

	return &it->second;
}





void cMapManager::LoadMapData(void)
{
	cCSLock Lock(m_CS);

	cIDCountSerializer IDSerializer(m_World->GetDataPath());

	if (!IDSerializer.Load())
	{
		return;
	}

	m_NextID = IDSerializer.GetMapCount();

	m_MapData.clear();

	for (unsigned int i = 0; i < m_NextID; ++i)
	{
		auto [it, inserted] = m_MapData.try_emplace(i, i, m_World);

		cMapSerializer Serializer(m_World->GetDataPath(), &it->second);
		Serializer.Load();

		// A just-loaded map isn't dirty even though we just "changed" it
		it->second.m_Dirty = false;
	}
}





void cMapManager::SaveMapData(void)
{
	cCSLock Lock(m_CS);

	if (m_MapData.empty())
	{
		return;
	}

	// FIXME: We don't clean up orphaned map saves. This could waste a lot of disk eventually.
	bool changes = false;
	for (auto & [MapID, Map] : m_MapData)
	{
		if (Map.m_Dirty)
		{
			cMapSerializer Serializer(m_World->GetDataPath(), &Map);

			Map.m_Dirty = false;

			if (Serializer.Save())
			{
				changes = true;
			}
			else
			{
				Map.m_Dirty = true;
				LOGWARN("Could not save map #%i", MapID);
			}
		}
	}

	if (changes)
	{
		cIDCountSerializer IDSerializer(m_World->GetDataPath());

		IDSerializer.SetMapCount(m_NextID);

		if (!IDSerializer.Save())
		{
			LOGERROR("Could not save idcounts.dat");
		}
	}
}





