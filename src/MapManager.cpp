
// MapManager.cpp

#include "Globals.h"

#include "MapManager.h"

#include "World.h"
#include "WorldStorage/MapSerializer.h"





cCriticalSection cMapManager::m_CS;

unsigned int cMapManager::m_NextID = 0;

cMapManager::cMapList cMapManager::m_MapData;





bool cMapManager::DoWithMap(UInt32 a_ID, cMapCallback a_Callback)
{
	std::shared_ptr<cMap> Map = GetMapData(a_ID);

	if (Map)
	{
		cCSLock Lock(Map->m_CS);

		a_Callback(*Map);
		return true;
	}

	return false;
}





void cMapManager::TickMaps()
{
	cCSLock Lock(m_CS);

	for (auto & [MapID, Map] : m_MapData)
	{
		UNUSED(MapID);

		if (Map->GetWorld() == m_World)
		{
			Map->Tick();
		}
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





std::shared_ptr<cMap> cMapManager::GetMapData(unsigned int a_ID)
{
	cCSLock Lock(m_CS);
	auto itr = m_MapData.find(a_ID);
	return (itr != m_MapData.end() ? itr->second : nullptr);
}





bool cMapManager::CreateMap(unsigned int & a_MapID, cMap::eMapIcon a_MapType, int a_X, int a_Z, unsigned int a_Scale)
{
	cCSLock Lock(m_CS);

	a_MapID = NextID();

	auto Map = std::make_shared<cMap>(a_MapID, a_MapType, a_X, a_Z, m_World, a_Scale);
	auto [it, inserted] = m_MapData.try_emplace(a_MapID, Map);
	UNUSED(it);

	if (!inserted)
	{
		// Either the IDs wrapped or something went horribly wrong
		LOGWARN(fmt::format(FMT_STRING("Could not craft map {} - too many maps in use?"), a_MapID));
		return false;
	}

	if ((a_MapType != cMap::eMapIcon::E_MAP_ICON_NONE) && (a_MapType != cMap::eMapIcon::E_MAP_ICON_WHITE_ARROW))
	{
		Map->AddDecorator(cMap::DecoratorType::PERSISTENT, 0, a_MapType, { a_X, 0, a_Z }, 180, "");
		Map->FillAndSketch();
	}

	return true;
}





std::shared_ptr<cMap> cMapManager::CopyMap(cMap & a_OldMap)
{
	cCSLock Lock(m_CS);

	unsigned int ID = NextID();

	std::shared_ptr<cMap> Map;
	{
		cCSLock Lock(a_OldMap.m_CS);
		Map = std::make_shared<cMap>(ID, a_OldMap);
	}

	auto [it, inserted] = m_MapData.try_emplace(ID, Map);
	UNUSED(it);

	if (!inserted)
	{
		// Either the IDs wrapped or something went horribly wrong
		LOGWARN(fmt::format(FMT_STRING("Could not craft map {} - too many maps in use?"), ID));
		return nullptr;
	}

	return Map;
}





void cMapManager::LoadMapData(void)
{
	cCSLock Lock(m_CS);

	const AStringVector Files = cFile::GetFolderContents(cMapSerializer::GetDataPath(m_World->GetDataPath()));

	for (const auto & File : Files)
	{
		if (File.compare(0, 4, "map_") || File.compare(File.size() - 4, 4, ".dat"))
		{
			continue;
		}

		unsigned int MapID;
		try
		{
			MapID = std::stoul(File.substr(4, File.size() - 8));
		}
		catch (...)
		{
			continue;
		}

		auto Map = std::make_shared<cMap>(MapID, m_World);
		auto [it, inserted] = m_MapData.try_emplace(MapID, Map);
		Map = it->second;

		if (!inserted)
		{
			LOGWARN(fmt::format(FMT_STRING("MapManager: ID {} already used by {} ignoring {}"), MapID, Map->m_World->GetName(), File));
			continue;
		}

		cMapSerializer Serializer(m_World->GetDataPath(), Map.get());
		if (Serializer.Load())
		{
			// A just-loaded map isn't dirty even though we just "changed" it.
			Map->m_Dirty = false;

			// But it will need to be sent unconditionally to any existing clients.
			Map->m_Send = true;
		}

		m_NextID = std::max(m_NextID, MapID + 1);
	}
}





void cMapManager::SaveMapData(void)
{
	cCSLock Lock(m_CS);

	bool changes = false;
	unsigned int NextID = 0;

	if (!m_MapData.empty())
	{
		// FIXME: We don't clean up orphaned map saves. This could waste a lot of disk eventually.
		for (auto [MapID, Map] : m_MapData)
		{
			if (Map->m_Dirty && (Map->GetWorld() == m_World))
			{
				NextID = std::max(NextID, MapID);

				cMapSerializer Serializer(m_World->GetDataPath(), Map.get());

				Map->m_Dirty = false;

				if (Serializer.Save())
				{
					changes = true;
				}
				else
				{
					Map->m_Dirty = true;
					LOGWARN("Could not save map #%i", MapID);
				}
			}
		}
	}

	if (changes)
	{
		cIDCountSerializer IDSerializer(m_World->GetDataPath());

		IDSerializer.SetMapCount(NextID);

		if (!IDSerializer.Save())
		{
			LOGERROR("Could not save idcounts.dat");
		}
	}
}





