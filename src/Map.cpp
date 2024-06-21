
// Map.cpp

#include "Globals.h"

#include "BlockEntities/BannerEntity.h"
#include "BlockInfo.h"
#include "Blocks/BlockHandler.h"
#include "Chunk.h"
#include "ClientHandle.h"
#include "Entities/Player.h"
#include "FastRandom.h"
#include "Map.h"
#include "World.h"





cMap::cMap(unsigned int a_ID, cWorld * a_World):
	m_ID(a_ID),
	m_Scale(3),
	m_CenterX(0),
	m_CenterZ(0),
	m_Dirty(false),  // This constructor is for an empty map object which will be filled by the caller with the correct values - it does not need saving.
	m_Send(false),
	m_ChangeStartX(MAP_WIDTH - 1),
	m_ChangeEndX(0),
	m_ChangeStartZ(MAP_HEIGHT - 1),
	m_ChangeEndZ(0),
	m_Locked(false),
	m_TrackingPosition(true),
	m_UnlimitedTracking(false),
	m_TrackingThreshold(DEFAULT_TRACKING_DISTANCE),
	m_FarTrackingThreshold(DEFAULT_FAR_TRACKING_DISTANCE),
	m_World(a_World),
	m_Name(fmt::format(FMT_STRING("map_{}"), a_ID))
{
}





cMap::cMap(unsigned int a_ID, short a_MapType, int a_CenterX, int a_CenterZ, cWorld * a_World, unsigned int a_Scale):
	m_ID(a_ID),
	m_Scale(a_Scale),
	m_CenterX(a_CenterX),
	m_CenterZ(a_CenterZ),
	m_Dirty(true),  // This constructor is for creating a brand new map in game, it will always need saving.
	m_Send(true),
	m_ChangeStartX(MAP_WIDTH - 1),
	m_ChangeEndX(0),
	m_ChangeStartZ(MAP_HEIGHT - 1),
	m_ChangeEndZ(0),
	m_Locked(false),
	m_TrackingPosition(a_MapType == E_EMPTY_MAP_TYPE_SIMPLE ? false : true),
	m_UnlimitedTracking(a_MapType == E_EMPTY_MAP_TYPE_EXPLORER ? true : false),
	m_TrackingThreshold(DEFAULT_TRACKING_DISTANCE),
	m_FarTrackingThreshold(DEFAULT_FAR_TRACKING_DISTANCE),
	m_World(a_World),
	m_Name(fmt::format(FMT_STRING("map_{}"), a_ID))
{
}





cMap::cMap(unsigned int a_ID, cMap & a_Map):
	m_ID(a_ID),
	m_Dirty(true),
	m_Send(true),
	m_ChangeStartX(0),
	m_ChangeEndX(MAP_WIDTH - 1),
	m_ChangeStartZ(0),
	m_ChangeEndZ(MAP_HEIGHT - 1),
	m_Name(fmt::format(FMT_STRING("map_{}"), a_ID))
{
	cCSLock Lock(m_CS);

	m_Scale = a_Map.m_Scale;
	m_CenterX = a_Map.m_CenterX;
	m_CenterZ = a_Map.m_CenterZ;
	m_Locked = a_Map.m_Locked;  // FIXME: should copying a map create an already locked map?
	m_TrackingPosition = a_Map.m_TrackingPosition;
	m_UnlimitedTracking = a_Map.m_UnlimitedTracking;
	m_TrackingThreshold = a_Map.m_TrackingThreshold;
	m_FarTrackingThreshold = a_Map.m_FarTrackingThreshold;
	m_World = a_Map.m_World;
}





void cMap::Tick()
{
	cCSLock Lock(m_CS);

#if 0
	for (auto itr = m_Decorators.begin(); itr != m_Decorators.end();)
	{
		switch (itr->first.m_Type)
		{
			case cMap::DecoratorType::PLAYER:
			{
				// Remove decorators for players that are no longer here.
				bool found = false;

				for (const auto & Client : m_ClientsInCurrentTick)
				{
					cPlayer * Player = Client->GetPlayer();

					if (itr->first.m_Id == Player->GetUniqueID())
					{
						if (Player->GetWorld() == m_World)
						{
							found = true;
							break;
						}
					}
				}

				if (!found)
				{
					itr = m_Decorators.erase(itr);
					continue;
				}

				break;
			}

			default:
			{
				break;
			}
		}

		++itr;
	}
#endif

	if (!m_ClientsInCurrentTick.empty())
	{
		// Java Edition: directional markers only spin if a Nether map is used in the Nether.
		// Bedrock Edition: directional markers always spin on a Nether map.
		// Cuberite: directional markers always spin on a Nether map.
		if (m_World->GetDimension() == dimNether)
		{
			for (auto & itr : m_Decorators)
			{
				m_Send |= itr.second.Spin();
			}
		}

		if (m_Send)
		{
			for (const auto Client : m_ClientsInCurrentTick)
			{
				if (m_ClientsWithCurrentData.find(Client) == m_ClientsWithCurrentData.end())
				{
					// LOG(fmt::format(FMT_STRING("{}: map {} send complete"), std::this_thread::get_id(), m_ID));
					Client->SendMapData(*this, 0, 0, MAP_WIDTH - 1, MAP_HEIGHT - 1);
				}
				else
				{
					// LOG(fmt::format(FMT_STRING("{}: map {} send [{}, {}] to [{}, {}]"), std::this_thread::get_id(), m_ID, m_ChangeStartX, m_ChangeStartZ, m_ChangeEndX, m_ChangeEndZ));
					Client->SendMapData(*this, m_ChangeStartX, m_ChangeStartZ, m_ChangeEndX, m_ChangeEndZ);
				}
			}

			m_ChangeStartX = MAP_WIDTH - 1;
			m_ChangeEndX = 0;
			m_ChangeStartZ = MAP_HEIGHT - 1;
			m_ChangeEndZ = 0;
			m_Send = false;

			std::swap(m_ClientsInCurrentTick, m_ClientsWithCurrentData);
		}

		m_ClientsInCurrentTick.clear();
	}
}





bool cMap::SetPixel(UInt8 a_X, UInt8 a_Z, ColorID a_Data)
{
	if ((a_X < MAP_WIDTH) && (a_Z < MAP_HEIGHT))
	{
		auto index = a_Z * MAP_WIDTH + a_X;

		if (m_Data[index] != a_Data)
		{
			m_Data[index] = a_Data;

			m_Dirty = true;
			m_Send = true;
			m_ChangeStartX = std::min(a_X, m_ChangeStartX);
			m_ChangeEndX = std::max(a_X, m_ChangeEndX);
			m_ChangeStartZ = std::min(a_Z, m_ChangeStartZ);
			m_ChangeEndZ = std::max(a_Z, m_ChangeEndZ);
			return true;
		}
	}

	return false;
}





void cMap::UpdateRadius(const cPlayer * a_Player)
{
	int PixelWidth = static_cast<int>(GetPixelWidth());

	int PlayerPixelX = static_cast<int>(a_Player->GetPosX() - m_CenterX) / PixelWidth + MAP_WIDTH  / 2;
	int PlayerPixelZ = static_cast<int>(a_Player->GetPosZ() - m_CenterZ) / PixelWidth + MAP_HEIGHT / 2;
	int PixelRadius = static_cast<int>(DEFAULT_RADIUS / GetPixelWidth());

	// Scan decorators in this region and remove any non-player, non-manual types that
	// don't exist, are no longer ticking or have moved.
	for (auto itr = m_Decorators.begin(); itr != m_Decorators.end();)
	{
		// As far as we know everything we know is correct.
		bool exists = true;

		if (itr->first.m_Type != DecoratorType::PERSISTENT)
		{
			int PixelX = (static_cast<int>(itr->second.m_MapX) - 1) / 2;
			int PixelZ = (static_cast<int>(itr->second.m_MapZ) - 1) / 2;

			int dX = PixelX - PlayerPixelX;
			int dZ = PixelZ - PlayerPixelZ;

			int BlockX, BlockZ;

			// If it's inside our circle of awareness we'll check it.
			if ((dX * dX) + (dZ * dZ) < (PixelRadius * PixelRadius))
			{
				BlockX = m_CenterX + GetPixelWidth() * PixelX;
				BlockZ = m_CenterZ + GetPixelWidth() * PixelZ;

				int ChunkX, ChunkZ;
				cChunkDef::BlockToChunk(BlockX, BlockZ, ChunkX, ChunkZ);

				exists = m_World->DoWithChunk(ChunkX, ChunkZ, [&itr](cChunk & a_Chunk)
					{
						// This can be checked so it doesn't exist unless we can find it.
						bool result = false;

						if ((itr->first.m_Type == DecoratorType::PLAYER) || (itr->first.m_Type == DecoratorType::FRAME))
						{
							a_Chunk.DoWithEntityByID(itr->first.m_Id, [&itr] (cEntity & a_Entity)
								{
									// Player markers update as they move so we only need to know
									// player exists. Frame markers are static until we remove
									// them here and could have been broken and placed elsewhere
									// since we last checked.
									bool res = a_Entity.IsPlayer() || ((itr->second.m_Position == a_Entity.GetPosition()) && (itr->second.m_Yaw == a_Entity.GetYaw()));
									// LOG(fmt::format(FMT_STRING("type={} id={} dpos {}/{} epos {}/{} res {}"), itr->first.m_Type, itr->first.m_Id, itr->second.m_Position, itr->second.m_Yaw, a_Entity.GetPosition(), a_Entity.GetYaw(), res));
									return res;
								},
								result);

							// LOG(fmt::format(FMT_STRING("type={} id={} result {}"), itr->first.m_Type, itr->first.m_Id, result));
						}
						else if (itr->first.m_Type == DecoratorType::BANNER)
						{
							// Banner markers are block entities so we just check that the block
							// here is a banner with the expected colour and name.
							auto BlockEntity = a_Chunk.GetBlockEntity(itr->second.m_Position.Floor());
							result = (BlockEntity
							&& ((BlockEntity->GetBlockType() == E_BLOCK_WALL_BANNER) || (BlockEntity->GetBlockType() == E_BLOCK_STANDING_BANNER))
							&& (itr->second.m_Icon == BannerColourToIcon(static_cast<cBannerEntity *>(BlockEntity)->GetBaseColor()))
							&& !itr->second.m_Name.compare(static_cast<cBannerEntity *>(BlockEntity)->GetCustomName()));
						}

						return result;
					}
				);
			}
		}

		if (exists)
		{
			++itr;
		}
		else
		{
			// LOG(fmt::format(FMT_STRING("remove decorator type={} id={} icon={} position={} yaw={}"), itr->first.m_Type, itr->first.m_Id, itr->second.m_Icon, itr->second.m_Position, itr->second.m_Yaw));
			itr = m_Decorators.erase(itr);
			m_Dirty = true;
			m_Send = true;
		}
	}

	UInt8 StartX = static_cast<UInt8>(Clamp(PlayerPixelX - PixelRadius, 0, MAP_WIDTH));
	UInt8 StartZ = static_cast<UInt8>(Clamp(PlayerPixelZ - PixelRadius, 0, MAP_HEIGHT));

	UInt8 EndX   = static_cast<UInt8>(Clamp(PlayerPixelX + PixelRadius, 0, MAP_WIDTH));
	UInt8 EndZ   = static_cast<UInt8>(Clamp(PlayerPixelZ + PixelRadius, 0, MAP_HEIGHT));

	// LOG(fmt::format(FMT_STRING("map {} redraw [{}, {}] to [{}, {}]"), m_ID, StartX, StartZ, EndX, EndZ));

	for (UInt8 X = StartX; X < EndX; ++X)
	{
		for (UInt8 Z = StartZ; Z < EndZ; ++Z)
		{
			int dX = static_cast<int>(X) - PlayerPixelX;
			int dZ = static_cast<int>(Z) - PlayerPixelZ;

			if ((dX * dX) + (dZ * dZ) < (PixelRadius * PixelRadius))
			{
				UpdatePixel(X, Z);
			}
		}
	}
}





bool cMap::UpdatePixel(UInt8 a_X, UInt8 a_Z)
{
	ASSERT(m_World != nullptr);

	int BlockX = m_CenterX + (static_cast<int>(a_X) - MAP_WIDTH  / 2) * GetPixelWidth();
	int BlockZ = m_CenterZ + (static_cast<int>(a_Z) - MAP_HEIGHT / 2) * GetPixelWidth();

	int ChunkX, ChunkZ;
	cChunkDef::BlockToChunk(BlockX, BlockZ, ChunkX, ChunkZ);

	int RelX = BlockX - (ChunkX * cChunkDef::Width);
	int RelZ = BlockZ - (ChunkZ * cChunkDef::Width);

	ColorID PixelData;
	m_World->DoWithChunk(ChunkX, ChunkZ, [&](cChunk & a_Chunk)
		{
			static const std::array<unsigned char, 4> BrightnessID = { { 3, 0, 1, 2 } };  // Darkest to lightest
			BLOCKTYPE TargetBlock;
			NIBBLETYPE TargetMeta;

			auto Height = a_Chunk.GetHeight(RelX, RelZ);
			auto ChunkHeight = cChunkDef::Height;
			a_Chunk.GetBlockTypeMeta(RelX, Height, RelZ, TargetBlock, TargetMeta);
			auto ColourID = cBlockHandler::For(TargetBlock).GetMapBaseColourID(TargetMeta);

			if (IsBlockWater(TargetBlock))
			{
				ChunkHeight /= 4;
				while (((--Height) != -1) && IsBlockWater(a_Chunk.GetBlock(RelX, Height, RelZ)))
				{
					continue;
				}
			}
			else if (ColourID == 0)
			{
				while (((--Height) != -1) && ((ColourID = cBlockHandler::For(a_Chunk.GetBlock(RelX, Height, RelZ)).GetMapBaseColourID(a_Chunk.GetMeta(RelX, Height, RelZ))) == 0))
				{
					continue;
				}
			}

			// Multiply base color ID by 4 and add brightness ID
			const int BrightnessIDSize = static_cast<int>(BrightnessID.size());
			PixelData = ColourID * 4 + BrightnessID[static_cast<size_t>(Clamp<int>((BrightnessIDSize * Height) / ChunkHeight, 0, BrightnessIDSize - 1))];
			return false;
		}
	);

	SetPixel(a_X, a_Z, PixelData);

	return true;
}





void cMap::UpdateClient(const cPlayer * a_Player, bool a_UpdateMap)
{
	ASSERT(a_Player != nullptr);

	if (a_Player->GetWorld() == m_World)
	{
		if (m_TrackingPosition && (m_UnlimitedTracking || GetTrackingDistance(a_Player->GetPosition()) <= GetTrackingThreshold() * GetPixelWidth()))
		{
			AddDecorator(DecoratorType::PLAYER, a_Player->GetUniqueID(), eMapIcon::E_MAP_ICON_PLAYER, a_Player->GetPosition(), a_Player->GetYaw(), "");
		}
		else
		{
			RemoveDecorator(DecoratorType::PLAYER, a_Player->GetUniqueID());
		}
	}

	if (a_UpdateMap)
	{
		if (!m_Locked && a_Player->GetWorld() == m_World)
		{
			UpdateRadius(a_Player);
		}

		m_ClientsInCurrentTick.emplace(a_Player->GetClientHandle());

		// If the client wasn't getting updates previously (perhaps because the map was in an
		// item frame nearby) we need to force an update to refresh what it has even if we
		// didn't change anything. The map may have been changed via other clones while we
		// weren't looking.
		// m_Send |= (m_ClientsWithCurrentData.find(a_Player->GetClientHandle()) == m_ClientsWithCurrentData.end());
		// FIXME: if the player changes world the client seems to discard maps?
		m_Send = true;
	}
}





eDimension cMap::GetDimension(void) const
{
	ASSERT(m_World != nullptr);
	return m_World->GetDimension();
}





void cMap::SetLocked(bool a_OnOff)
{
	m_Dirty |= (m_Locked != a_OnOff);
	m_Locked = a_OnOff;
}





void cMap::SetTrackingPosition(bool a_OnOff)
{
	m_Dirty |= (m_TrackingPosition != a_OnOff);
	m_TrackingPosition = a_OnOff;
}





void cMap::SetUnlimitedTracking(bool a_OnOff)
{
	m_Dirty |= (m_UnlimitedTracking != a_OnOff);
	m_UnlimitedTracking = a_OnOff;
}





void cMap::SetTrackingThreshold(unsigned int a_Threshold)
{
	m_Dirty |= (m_TrackingThreshold != a_Threshold);
	m_TrackingThreshold = a_Threshold;
}





void cMap::SetFarTrackingThreshold(unsigned int a_Threshold)
{
	m_Dirty |= (m_FarTrackingThreshold != a_Threshold);
	m_FarTrackingThreshold = a_Threshold;
}





void cMap::SetPosition(int a_CenterX, int a_CenterZ)
{
	m_CenterX = a_CenterX;
	m_CenterZ = a_CenterZ;
}





void cMap::AddDecorator(DecoratorType a_Type, UInt32 a_Id, eMapIcon a_Icon, const Vector3d & a_Position, double a_Yaw, AString a_Name)
{
	int InsideWidth = ((MAP_WIDTH / 2) - 1);
	int InsideHeight = ((MAP_HEIGHT / 2) - 1);

	// Center of pixel
	int PixelX = (a_Position.x - GetCenterX()) / static_cast<int>(GetPixelWidth());
	int PixelZ = (a_Position.z - GetCenterZ()) / static_cast<int>(GetPixelWidth());

	if ((PixelX <= -InsideWidth) || (PixelX > InsideWidth) || (PixelZ <= -InsideHeight) || (PixelZ > InsideHeight))
	{
		// No decorators outside the map boundaries except for players which can be
		// clipped to the edges and represented by "outside" and "far outside" icons.
		if (a_Type != DecoratorType::PLAYER)
		{
			LOG(fmt::format(FMT_STRING("Outside borders [{}, {}]"), PixelX, PixelZ));
			return;
		}

		// Bedrock Edition: the pointer remains as an arrow but shrinks until the player
		// is near the area shown on the map.
		// FIXME: add a map option (and / or world / server default) to keep using an arrow.
		a_Icon = eMapIcon::E_MAP_ICON_PLAYER_FAR_OUTSIDE;
		if (GetTrackingDistance(a_Position) <= (GetWidth() * GetPixelWidth()) / 2 + GetFarTrackingThreshold())
		{
			a_Icon = eMapIcon::E_MAP_ICON_PLAYER_OUTSIDE;
		}

		// Move to border
		PixelX = std::clamp(PixelX, -InsideWidth, InsideWidth);
		PixelZ = std::clamp(PixelZ, -InsideHeight, InsideHeight);
	}

	signed char MapX = static_cast<signed char>(2 * PixelX + 1);
	signed char MapZ = static_cast<signed char>(2 * PixelZ + 1);

	// Banner markers have rotation fixed.
	if (a_Type == DecoratorType::BANNER)
	{
		a_Yaw = 180;
	}

	// Players and persistent (placed by plugins) markers are movable. Others (item frames
	// and banners) are simply added and only removed when a region redraw notes that they
	// are no longer present. However the banner at a particular location could have been
	// replaced with one of a different colour. Since banner IDs are generated internally
	// based on their position we can simply treat banners as "movable" (they never move but
	// their colour / icon could be different). Therefore only item frames aren't updatable.
	if (a_Type != DecoratorType::FRAME)
	{
		auto itr = m_Decorators.find({ a_Type, a_Id });
		if (itr != m_Decorators.end())
		{
			// The map needs SAVING if there are changes to the decorator.
			m_Dirty |= (a_Position != itr->second.m_Position) || (a_Yaw != itr->second.m_Yaw) || (a_Icon != itr->second.m_Icon);

			// The map needs SENDING if there are changes to the decorator's representation on the map.
			m_Send |= itr->second.Update(a_Icon, a_Position, a_Yaw, MapX, MapZ, (m_World->GetDimension() == dimNether), a_Name);
			return;
		}
	}

	m_Decorators.emplace(std::piecewise_construct,
		std::forward_as_tuple(a_Type, a_Id),
		std::forward_as_tuple(a_Icon, a_Position, a_Yaw, MapX, MapZ, a_Name));

	// If we placed a new decorator the map needs sending...
	m_Send = true;

	// ...and saving if the decorator wasn't a player marker.
	m_Dirty = (a_Type != DecoratorType::PLAYER);
}





void cMap::RemoveDecorator(cMap::DecoratorType a_Type, UInt32 a_Id)
{
	if (m_Decorators.erase({ a_Type, a_Id }) > 0)
	{
		// If we removed one or more decorators the map needs sending...
		m_Send = true;

		// ...and saving if the decorator wasn't a player marker.
		m_Dirty = (a_Type != DecoratorType::PLAYER);
	}
}





