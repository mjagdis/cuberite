
// ChunkStay.cpp

// Implements the cChunkStay class representing a base for classes that keep chunks loaded

#include "Globals.h"
#include "ChunkStay.h"
#include "ChunkMap.h"





cChunkStay::~cChunkStay()
{
	// It's been disabled first, right? Right?
	ASSERT(m_ChunkMap == nullptr);

	m_Chunks.clear();
	m_OutstandingChunks.clear();
}





void cChunkStay::Clear(void)
{
	ASSERT(m_ChunkMap == nullptr);
	m_Chunks.clear();
}





void cChunkStay::AddEnabled(int a_ChunkX, int a_ChunkZ)
{
	ASSERT(m_ChunkMap != nullptr);

	cCSLock Lock(m_ChunkMap->m_CSChunks);

	auto & Chunk = m_ChunkMap->GetChunk(a_ChunkX, a_ChunkZ);

	{
		auto [itr, inserted] = m_Chunks.emplace(a_ChunkX, a_ChunkZ);
		UNUSED(itr);

		if (inserted)
		{
			Chunk.Stay(true);
		}
	}

	{
		auto [itr, inserted] = m_OutstandingChunks.emplace(a_ChunkX, a_ChunkZ);
		UNUSED(itr);

		if (inserted)
		{
			if (Chunk.IsValid() && ChunkAvailable(a_ChunkX, a_ChunkZ))
			{
				// The ChunkStay wants to be deactivated, disable it and bail out:
				Disable();
			}
		}
	}
}





void cChunkStay::RemoveEnabled(int a_ChunkX, int a_ChunkZ)
{
	ASSERT(m_ChunkMap != nullptr);

	cCSLock Lock(m_ChunkMap->m_CSChunks);

	if (m_Chunks.erase({ a_ChunkX, a_ChunkZ }) != 0)
	{
		m_OutstandingChunks.erase({ a_ChunkX, a_ChunkZ });

		const auto Chunk = m_ChunkMap->FindChunk(a_ChunkX, a_ChunkZ);
		ASSERT(Chunk != nullptr);  // Stayed chunks cannot unload

		Chunk->Stay(false);
	}
}





void cChunkStay::Enable(cChunkMap & a_ChunkMap)
{
	cCSLock Lock(a_ChunkMap.m_CSChunks);

	// Enabling a ChunkStay that has already been enabled but not disabled again is an error.
	ASSERT(m_ChunkMap == nullptr);

	m_OutstandingChunks = m_Chunks;
	m_ChunkMap = &a_ChunkMap;

	a_ChunkMap.m_ChunkStays.push_back(this);

	// Schedule all chunks to be loaded / generated, and mark each as locked:
	for (auto & itr : m_OutstandingChunks)
	{
		auto & Chunk = a_ChunkMap.GetChunk(itr.m_ChunkX, itr.m_ChunkZ);

		Chunk.Stay(true);

		if (Chunk.IsValid())
		{
			if (ChunkAvailable(itr.m_ChunkX, itr.m_ChunkZ))
			{
				// The ChunkStay wants to be deactivated, disable it and bail out:
				Disable();
				break;
			}
		}
	}
}





void cChunkStay::Disable(void)
{
	cCSLock Lock(m_ChunkMap->m_CSChunks);

	if (m_ChunkMap)
	{
		// Remove from the list of active chunkstays:
		bool Found = false;
		for (auto itr = m_ChunkMap->m_ChunkStays.begin(), end = m_ChunkMap->m_ChunkStays.end(); itr != end; ++itr)
		{
			if (*itr == this)
			{
				m_ChunkMap->m_ChunkStays.erase(itr);
				Found = true;
				break;
			}
		}

		if (Found)
		{
			// Unmark all contained chunks:
			for (auto & itr : m_OutstandingChunks)
			{
				const auto Chunk = m_ChunkMap->FindChunk(itr.m_ChunkX, itr.m_ChunkZ);
				ASSERT(Chunk != nullptr);  // Stayed chunks cannot unload

				Chunk->Stay(false);
			}

			m_OutstandingChunks.clear();

			OnDisabled();
		}

		m_ChunkMap = nullptr;
	}
}





bool cChunkStay::ChunkAvailable(int a_ChunkX, int a_ChunkZ)
{
	if (m_OutstandingChunks.erase({ a_ChunkX, a_ChunkZ }))
	{
		OnChunkAvailable(a_ChunkX, a_ChunkZ);

		if (m_OutstandingChunks.empty())
		{
			return OnAllChunksAvailable();
		}
	}

	return false;
}
