
// Map.cpp

#include <cstdint>

#include "Globals.h"

#include "BlockEntities/BannerEntity.h"
#include "BlockInfo.h"
#include "Blocks/BlockHandler.h"
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
	m_Radius((a_World->GetDimension() != dimNether) ? DEFAULT_RADIUS : DEFAULT_RADIUS / 2),
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
	m_Name(fmt::format(FMT_STRING("map_{}"), a_ID)),
	m_Data { 0, }
{
}





cMap::cMap(unsigned int a_ID, short a_MapType, int a_CenterX, int a_CenterZ, cWorld * a_World, unsigned int a_Scale):
	m_ID(a_ID),
	m_Scale(a_Scale),
	m_CenterX(FAST_FLOOR_DIV(a_CenterX, cChunkDef::Width * 8) * cChunkDef::Width * 8 + cChunkDef::Width * 4),
	m_CenterZ(FAST_FLOOR_DIV(a_CenterZ, cChunkDef::Width * 8) * cChunkDef::Width * 8 + cChunkDef::Width * 4),
	m_Radius((a_World->GetDimension() != dimNether) ? DEFAULT_RADIUS : DEFAULT_RADIUS / 2),
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
	m_Name(fmt::format(FMT_STRING("map_{}"), a_ID)),
	m_Data { 0, }
{
}





cMap::cMap(unsigned int a_ID, cMap & a_Map):
	m_ID(a_ID),
	m_Scale(a_Map.m_Scale),
	m_CenterX(a_Map.m_CenterX),
	m_CenterZ(a_Map.m_CenterZ),
	m_Radius(a_Map.m_Radius),
	m_Dirty(true),
	m_Send(true),
	m_ChangeStartX(0),
	m_ChangeEndX(MAP_WIDTH - 1),
	m_ChangeStartZ(0),
	m_ChangeEndZ(MAP_HEIGHT - 1),
	m_Locked(a_Map.m_Locked),  // FIXME: should copying a map create an already locked map?
	m_TrackingPosition(a_Map.m_TrackingPosition),
	m_UnlimitedTracking(a_Map.m_UnlimitedTracking),
	m_TrackingThreshold(a_Map.m_TrackingThreshold),
	m_FarTrackingThreshold(a_Map.m_FarTrackingThreshold),
	m_World(a_Map.m_World),
	m_Name(fmt::format(FMT_STRING("map_{}"), a_ID))
{
	ASSERT(a_Map.m_CS.IsLocked());

	memcpy(m_Data, a_Map.m_Data, sizeof(m_Data) / sizeof(m_Data[0]));
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





bool cMap::SetPixel(int a_X, int a_Z, ColorID a_Data)
{
	if ((a_X >= 0) && (a_X < MAP_WIDTH) && (a_Z >= 0) && (a_Z < MAP_HEIGHT))
	{
		auto index = a_Z * MAP_WIDTH + a_X;

		if (m_Data[index] != a_Data)
		{
			m_Data[index] = a_Data;

			m_Dirty = true;
			m_Send = true;
			m_ChangeStartX = std::min(static_cast<UInt8>(a_X), m_ChangeStartX);
			m_ChangeEndX = std::max(static_cast<UInt8>(a_X), m_ChangeEndX);
			m_ChangeStartZ = std::min(static_cast<UInt8>(a_Z), m_ChangeStartZ);
			m_ChangeEndZ = std::max(static_cast<UInt8>(a_Z), m_ChangeEndZ);
			return true;
		}
	}

	return false;
}





bool cMap::InRange(int a_CenterX, int a_CenterZ, int a_TargetX, int a_TargetZ) const
{
	int dX = a_TargetX - a_CenterX;
	int dZ = a_TargetZ - a_CenterZ;

	return ((dX * dX) + (dZ * dZ) < m_Radius * m_Radius);
}





void cMap::UpdateRadius(const cPlayer * a_Player)
{
	Vector3i Center = a_Player->GetPosition();

	// Take a box around the player position, clip it to the edge of the map then convert to chunk coords.
	// Note that this implies extending the box edges outwards to the nearest chunk boundary so there may
	// then be portions of the box that are off-map.
	cChunkCoords StartChunk = cChunkDef::BlockToChunk(
		{
			Clamp(Center.x - m_Radius, m_CenterX - GetPixelWidth() * MAP_WIDTH / 2, m_CenterX + GetPixelWidth() * MAP_WIDTH / 2),
			0,
			Clamp(Center.z - m_Radius, m_CenterZ - GetPixelWidth() * MAP_HEIGHT / 2, m_CenterZ + GetPixelWidth() * MAP_HEIGHT / 2)
		}
	);
	cChunkCoords EndChunk = cChunkDef::BlockToChunk(
		{
			Clamp(Center.x + m_Radius, m_CenterX - GetPixelWidth() * MAP_WIDTH / 2, m_CenterX + GetPixelWidth() * MAP_WIDTH / 2),
			0,
			Clamp(Center.z + m_Radius, m_CenterZ - GetPixelWidth() * MAP_HEIGHT / 2, m_CenterZ + GetPixelWidth() * MAP_HEIGHT / 2)
		}
	);

	// For each chunk within that box...
	for (int ChunkX = StartChunk.m_ChunkX; ChunkX <= EndChunk.m_ChunkX; ++ChunkX)
	{
		for (int ChunkZ = StartChunk.m_ChunkZ; ChunkZ <= EndChunk.m_ChunkZ; ++ChunkZ)
		{
			m_World->DoWithChunk(ChunkX, ChunkZ, [this, Center](cChunk & a_Chunk)
				{
					// Scan decorators in this region and remove any non-player, non-manual types that
					// don't exist or have moved (in which case there is another decorator for it).
					for (auto itr = m_Decorators.begin(); itr != m_Decorators.end();)
					{
						bool exists = true;

						cChunkCoords DecoratorChunk = cChunkDef::BlockToChunk(itr->second.m_Position);

						// While the InRange area might extend outside the map area that needn't
						// concern us since there should not be any decorators that aren't actually
						// on the map.
						if ((DecoratorChunk.m_ChunkX == a_Chunk.GetPosX()) && (DecoratorChunk.m_ChunkZ == a_Chunk.GetPosZ())
						&& InRange(Center.x, Center.z, itr->second.m_Position.x, itr->second.m_Position.z))
						{
							switch (itr->first.m_Type)
							{
								case DecoratorType::PLAYER:
								case DecoratorType::FRAME:
								{
									// Player markers update as they move so we only need to know
									// the player exists. Frame markers are static until we remove
									// them here and could have been broken and placed elsewhere
									// since we last checked.
									exists = false;
									a_Chunk.DoWithEntityByID(itr->first.m_Id, [&itr] (cEntity & a_Entity)
										{
											return a_Entity.IsPlayer()
											|| (itr->second.m_Position == (a_Entity.GetPosition().Floor()) && (std::abs(itr->second.m_Yaw - a_Entity.GetYaw()) < 22.5));
										},
										exists
									);
									break;
								}

								case DecoratorType::BANNER:
								{
									// Banner markers are block entities so we just check that the block
									// here is a banner with the expected colour and name.
									auto BlockEntity = a_Chunk.GetBlockEntity(itr->second.m_Position.Floor());
									exists = (BlockEntity
									&& ((BlockEntity->GetBlockType() == E_BLOCK_WALL_BANNER) || (BlockEntity->GetBlockType() == E_BLOCK_STANDING_BANNER))
									&& (itr->second.m_Icon == BannerColourToIcon(static_cast<cBannerEntity *>(BlockEntity)->GetBaseColor()))
									&& !itr->second.m_Name.compare(static_cast<cBannerEntity *>(BlockEntity)->GetCustomName()));
									break;
								}

								default:
								{
									exists = true;
									break;
								}
							}
						}

						if (exists)
						{
							++itr;
						}
						else
						{
							itr = m_Decorators.erase(itr);
							m_Dirty = true;
							m_Send = true;
						}
					}

					// The position of the corner of the chunk to which chunk-relative positions refer.
					int BaseX = a_Chunk.GetPosX() * cChunkDef::Width;
					int BaseZ = a_Chunk.GetPosZ() * cChunkDef::Width;

					// Scan the blocks for this chunk and update.
					// Java Edition: the colour of a map pixel matches the colour of the most common
					// opaque block that it covers.
					// Bedrock Edition: the colour of a map pixel matches the colour of the top left
					// opaque block of the blocks covered by the pixel.
					// Cuberite: as Bedrock Edition.
					for (int RelX = 0; RelX < cChunkDef::Width; RelX += GetPixelWidth())
					{
						for (int RelZ = 0; RelZ < cChunkDef::Width; RelZ += GetPixelWidth())
						{
							if (InRange(Center.x, Center.z, BaseX + RelX, BaseZ + RelZ))
							{
								int a_X = (BaseX + RelX - m_CenterX) / GetPixelWidth() + MAP_WIDTH / 2;
								int a_Z = (BaseZ + RelZ - m_CenterZ) / GetPixelWidth() + MAP_HEIGHT / 2;

								// SetPixel checks that the point is within the map bounds so we don't
								// need to worry that aligning the range with chunk borders may have
								// extended it off the map edge.
								SetPixel(a_X, a_Z, PixelColour(a_Chunk, RelX, RelZ));
							}
						}
					}

					return true;
				}
			);
		}
	}
}





ColourID cMap::PixelColour(cChunk & a_Chunk, int a_RelX, int a_RelZ) const
{
	static const std::array<unsigned char, 4> BrightnessID = { { 3, 0, 1, 2 } };  // Darkest to lightest

	auto Height = a_Chunk.GetHeight(a_RelX, a_RelZ);
	auto HeightRange = cChunkDef::Height;

	BLOCKTYPE TargetBlock;
	BLOCKMETATYPE TargetMeta;
	a_Chunk.GetBlockTypeMeta(a_RelX, Height, a_RelZ, TargetBlock, TargetMeta);
	auto ColourID = cBlockHandler::For(TargetBlock).GetMapBaseColourID(TargetMeta);

	// Descend through the blocks looking for the first block that has a non-transparent colour.
	while ((ColourID == E_MAP_COLOR_TRANSPARENT) && (--Height != -1))
	{
		a_Chunk.GetBlockTypeMeta(a_RelX, Height, a_RelZ, TargetBlock, TargetMeta);
		ColourID = cBlockHandler::For(TargetBlock).GetMapBaseColourID(TargetMeta);
	}

	// Descend through water blocks ignoring transparent blocks and adjusting the brightness
	// scaling as we go. The colour remains the colour of the top water block but gets darker
	// the further we descend.
	if (IsBlockWater(TargetBlock) || cBlockInfo::IsTransparent(TargetBlock))
	{
		// We can see a maximum of number of blocks down when mapping. Beyond that it's just
		// uniformly dark.
		HeightRange = DEFAULT_WATER_DEPTH;

		auto Surface = Height;
		while ((IsBlockWater(TargetBlock) || cBlockInfo::IsTransparent(TargetBlock)) && (--Height != -1) && (Surface - Height < HeightRange - 1))
		{
			TargetBlock = a_Chunk.GetBlock(a_RelX, Height, a_RelZ);
		}

		// The brightness of the water is based on the depth of water rather than the absolute
		// height of the sea floor (which we may not even have reached).
		Height = HeightRange - 1 - (Surface - Height);
	}

	// Multiply base color ID by 4 and add brightness ID
	const int BrightnessIDSize = static_cast<int>(BrightnessID.size());
	return ColourID * 4 + BrightnessID[static_cast<size_t>(Clamp<int>((BrightnessIDSize * Height) / HeightRange, 0, BrightnessIDSize - 1))];
}





void cMap::UpdateClient(const cPlayer * a_Player, bool a_UpdateMap)
{
	ASSERT(a_Player != nullptr);

	if (a_Player->GetWorld() == m_World)
	{
		if (m_TrackingPosition && (m_UnlimitedTracking || GetTrackingDistance(a_Player->GetPosition()) <= GetTrackingThreshold() * GetPixelWidth()))
		{
			AddDecorator(DecoratorType::PLAYER, a_Player->GetUniqueID(), eMapIcon::E_MAP_ICON_WHITE_ARROW, a_Player->GetPosition(), a_Player->GetYaw(), "");
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
	int MapX = static_cast<int>(FAST_FLOOR_DIV(256.0 * (a_Position.x - GetCenterX()) / GetPixelWidth(), MAP_WIDTH));
	int MapZ = static_cast<int>(FAST_FLOOR_DIV(256.0 * (a_Position.z - GetCenterZ()) / GetPixelWidth(), MAP_HEIGHT));

	if ((MapX < INT8_MIN) || (MapX > INT8_MAX) || (MapZ < INT8_MIN) || (MapZ > INT8_MAX))
	{
		// No decorators outside the map boundaries except for players which can be
		// clipped to the edges and represented by "outside" and "far outside" icons.
		if (a_Type != DecoratorType::PLAYER)
		{
			LOG(fmt::format(FMT_STRING("Outside borders [{}, {}]"), MapX, MapZ));
			return;
		}

		// Bedrock Edition: the pointer remains as an arrow but shrinks until the player
		// is near the area shown on the map.
		// FIXME: add a map option (and / or world / server default) to keep using an arrow.
		a_Icon = eMapIcon::E_MAP_ICON_SMALL_WHITE_CIRCLE;
		if (GetTrackingDistance(a_Position) <= (GetWidth() * GetPixelWidth()) / 2 + GetFarTrackingThreshold())
		{
			a_Icon = eMapIcon::E_MAP_ICON_WHITE_CIRCLE;
		}

		// Move to border and disable rotation.
		MapX = std::clamp(MapX, INT8_MIN, INT8_MAX);
		MapZ = std::clamp(MapZ, INT8_MIN, INT8_MAX);
		a_Yaw = 180.0;
	}

	// Banner markers have rotation fixed.
	if (a_Type == DecoratorType::BANNER)
	{
		a_Yaw = 180.0;
	}

	// We know the map coords fit in an Int8. Let's avoid warnings about narrowing.
	Int8 MapX8 = static_cast<Int8>(MapX);
	Int8 MapZ8 = static_cast<Int8>(MapZ);

	if (a_Type != DecoratorType::FRAME)
	{
		// Players and persistent (placed by plugins) markers are movable. Others (item frames
		// and banners) are simply added and only removed when a region redraw notes that they
		// are no longer present. However the banner at a particular location could have been
		// replaced with one of a different colour. Since banner IDs are generated internally
		// based on their position we can simply treat banners as "movable" (they never move but
		// their colour / icon could be different). Therefore only item frames aren't updatable.
		auto itr = m_Decorators.find({ a_Type, a_Id });
		if (itr != m_Decorators.end())
		{
			bool Changes = itr->second.Update(a_Icon, a_Position, a_Yaw, MapX8, MapZ8, (m_World->GetDimension() == dimNether), a_Name);

			// The map needs sending if the representation of the decorator changed.
			m_Send |= Changes;

			// The map needs saving if there were changes to a non-player marker and
			// the position of the marked entity has changed by at least a block.
			m_Dirty |= (Changes && (a_Type != DecoratorType::PLAYER) && !itr->second.m_Position.EqualsEps(a_Position, 1.0));

			return;
		}
	}

	m_Decorators.emplace(std::piecewise_construct,
		std::forward_as_tuple(a_Type, a_Id),
		std::forward_as_tuple(a_Icon, a_Position, a_Yaw, MapX8, MapZ8, a_Name));

	// If we placed a new decorator the map needs sending...
	m_Send = true;

	// ...and saving if the decorator wasn't a player marker.
	m_Dirty |= (a_Type != DecoratorType::PLAYER);

	// LOG(fmt::format(FMT_STRING("map {} add marker type {} id {} icon {} pos {} yaw {} mapx {} mapz {} send {} dirty {}"), m_ID, a_Type, a_Id, a_Icon, a_Position, a_Yaw, MapX8, MapZ8, m_Send, m_Dirty));
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





void cMap::RemoveFrame(const Vector3d & a_Position, double a_Yaw)
{
	Vector3i Position = a_Position.Floor();

	for (auto itr = m_Decorators.begin(); itr != m_Decorators.end(); ++itr)
	{
		if ((itr->first.m_Type == DecoratorType::FRAME) && (itr->second.m_Position == Position) && (itr->second.m_Yaw == a_Yaw))
		{
			// There should be only one.
			m_Decorators.erase(itr);
			break;
		}
	}
}
