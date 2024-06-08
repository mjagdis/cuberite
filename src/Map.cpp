
// Map.cpp

#include "Globals.h"

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
	m_Locked(false),
	m_TrackingPosition(true),
	m_UnlimitedTracking(false),
	m_TrackingThreshold(DEFAULT_TRACKING_DISTANCE),
	m_FarTrackingThreshold(DEFAULT_FAR_TRACKING_DISTANCE),
	m_World(a_World),
	m_Name(fmt::format(FMT_STRING("map_{}"), m_ID))
{
}





cMap::cMap(unsigned int a_ID, short a_MapType, int a_CenterX, int a_CenterZ, cWorld * a_World, unsigned int a_Scale):
	m_ID(a_ID),
	m_Scale(a_Scale),
	m_CenterX(a_CenterX),
	m_CenterZ(a_CenterZ),
	m_Dirty(true),  // This constructor is for creating a brand new map in game, it will always need saving.
	m_Locked(false),
	m_TrackingPosition(a_MapType == E_EMPTY_MAP_TYPE_SIMPLE ? false : true),
	m_UnlimitedTracking(a_MapType == E_EMPTY_MAP_TYPE_EXPLORER ? true : false),
	m_TrackingThreshold(DEFAULT_TRACKING_DISTANCE),
	m_FarTrackingThreshold(DEFAULT_FAR_TRACKING_DISTANCE),
	m_World(a_World),
	m_Name(fmt::format(FMT_STRING("map_{}"), m_ID))
{
}





cMap::cMap(unsigned int a_ID, const cMap & a_Map)
{
	*this = a_Map;
	m_ID = a_ID;
}





void cMap::Tick()
{
	for (const auto & Client : m_ClientsInCurrentTick)
	{
		Client->SendMapData(*this, 0, 0);
	}
	m_ClientsInCurrentTick.clear();
	// m_Decorators.clear();
}





void cMap::UpdateRadius(int a_PixelX, int a_PixelZ, unsigned int a_Radius)
{
	if (!m_Locked)
	{
		int PixelRadius = static_cast<int>(a_Radius / GetPixelWidth());

		// Scan decorators in this region and remove any non-manual types that
		// don't exist, are no longer ticking or have moved.
		for (auto Decorator = m_Decorators.begin(); Decorator != m_Decorators.end();)
		{
			// As far as we know everything we know is correct.
			bool exists = true;

			if (Decorator->m_Type != cMap::DecoratorType::PERSISTENT)
			{
				int PixelX = (static_cast<int>(Decorator->m_MapX) - 1) / 2;
				int PixelZ = (static_cast<int>(Decorator->m_MapZ) - 1) / 2;

				int dX = PixelX - a_PixelX;
				int dZ = PixelZ - a_PixelZ;

				int BlockX, BlockZ;

				// If it's inside our circle of awareness we'll check it.
				if ((dX * dX) + (dZ * dZ) < (PixelRadius * PixelRadius))
				{
					BlockX = m_CenterX + GetPixelWidth() * PixelX;
					BlockZ = m_CenterZ + GetPixelWidth() * PixelZ;

					int ChunkX, ChunkZ;
					cChunkDef::BlockToChunk(BlockX, BlockZ, ChunkX, ChunkZ);

					exists = m_World->DoWithChunk(ChunkX, ChunkZ, [&Decorator](cChunk & a_Chunk)
						{
							bool result = false;

							a_Chunk.DoWithEntityByID(Decorator->m_Id, [&Decorator] (cEntity & a_Entity)
								{
									return ((Decorator->m_Position == a_Entity.GetPosition()) && (Decorator->m_Yaw == a_Entity.GetYaw()));
								},
								result);

							return result;
						}
					);
				}
			}

			if (exists)
			{
				++Decorator;
			}
			else
			{
				Decorator = m_Decorators.erase(Decorator);
				m_Dirty = true;
			}
		}

		unsigned int StartX = static_cast<unsigned int>(Clamp(a_PixelX - PixelRadius, 0, MAP_WIDTH));
		unsigned int StartZ = static_cast<unsigned int>(Clamp(a_PixelZ - PixelRadius, 0, MAP_HEIGHT));

		unsigned int EndX   = static_cast<unsigned int>(Clamp(a_PixelX + PixelRadius, 0, MAP_WIDTH));
		unsigned int EndZ   = static_cast<unsigned int>(Clamp(a_PixelZ + PixelRadius, 0, MAP_HEIGHT));

		for (unsigned int X = StartX; X < EndX; ++X)
		{
			for (unsigned int Z = StartZ; Z < EndZ; ++Z)
			{
				int dX = static_cast<int>(X) - a_PixelX;
				int dZ = static_cast<int>(Z) - a_PixelZ;

				if ((dX * dX) + (dZ * dZ) < (PixelRadius * PixelRadius))
				{
					UpdatePixel(X, Z);
				}
			}
		}
	}
}





void cMap::UpdateRadius(cPlayer & a_Player, unsigned int a_Radius)
{
	int PixelWidth = static_cast<int>(GetPixelWidth());

	int PixelX = static_cast<int>(a_Player.GetPosX() - m_CenterX) / PixelWidth + MAP_WIDTH  / 2;
	int PixelZ = static_cast<int>(a_Player.GetPosZ() - m_CenterZ) / PixelWidth + MAP_HEIGHT / 2;

	UpdateRadius(PixelX, PixelZ, a_Radius);
}





bool cMap::UpdatePixel(unsigned int a_X, unsigned int a_Z)
{
	int BlockX = m_CenterX + (static_cast<int>(a_X) - MAP_WIDTH  / 2) * GetPixelWidth();
	int BlockZ = m_CenterZ + (static_cast<int>(a_Z) - MAP_HEIGHT / 2) * GetPixelWidth();

	int ChunkX, ChunkZ;
	cChunkDef::BlockToChunk(BlockX, BlockZ, ChunkX, ChunkZ);

	int RelX = BlockX - (ChunkX * cChunkDef::Width);
	int RelZ = BlockZ - (ChunkZ * cChunkDef::Width);

	ASSERT(m_World != nullptr);

	ColorID PixelData;
	m_World->DoWithChunk(ChunkX, ChunkZ, [&](cChunk & a_Chunk)
		{
			if (GetDimension() == dimNether)
			{
				// TODO 2014-02-22 xdot: Nether maps

				return false;
			}

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





void cMap::UpdateClient(cPlayer * a_Player)
{
	ASSERT(a_Player != nullptr);

	m_ClientsInCurrentTick.push_back(a_Player->GetClientHandle());

	if (m_TrackingPosition && (m_UnlimitedTracking || GetTrackingDistance(a_Player) <= GetTrackingThreshold() * GetPixelWidth()))
	{
		AddDecorator(cMap::DecoratorType::PLAYER, cMap::icon::MAP_ICON_PLAYER, a_Player->GetUniqueID(), a_Player->GetPosition(), a_Player->GetYaw());
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





void cMap::AddDecorator(cMap::DecoratorType a_Type, cMap::icon a_Icon, UInt32 a_Id, const Vector3d & a_Position, int a_Yaw)
{
	int InsideWidth = (GetWidth() / 2) - 1;
	int InsideHeight = (GetHeight() / 2) - 1;

	// Center of pixel
	int PixelX = static_cast<int>(a_Position.x - GetCenterX()) / static_cast<int>(GetPixelWidth());
	int PixelZ = static_cast<int>(a_Position.z - GetCenterZ()) / static_cast<int>(GetPixelWidth());

	if ((PixelX > -InsideWidth) && (PixelX <= InsideWidth) && (PixelZ > -InsideHeight) && (PixelZ <= InsideHeight))
	{
		int Rot;

		if (GetDimension() == dimNether)
		{
			// TODO 2014-02-19 xdot: Refine
			Rot = GetRandomProvider().RandInt(15);
		}
		else
		{
			Rot = CeilC(((a_Yaw - 11.25) * 16) / 360);
		}

		m_Decorators.emplace(a_Type, a_Icon, a_Id, a_Position, a_Yaw, static_cast<unsigned>(2 * PixelX + 1), static_cast<unsigned>(2 * PixelZ + 1), Rot);
		m_Dirty = true;
	}
	else if (a_Icon == cMap::icon::MAP_ICON_PLAYER)
	{
		a_Icon = cMap::icon::MAP_ICON_PLAYER_FAR_OUTSIDE;
		if (GetTrackingDistance(a_Position) <= (GetWidth() * GetPixelWidth()) / 2 + GetFarTrackingThreshold())
		{
			a_Icon = cMap::icon::MAP_ICON_PLAYER_OUTSIDE;
		}

		// Move to border
		if (PixelX <= -InsideWidth)
		{
			PixelX = -InsideWidth;
		}
		if (PixelZ <= -InsideHeight)
		{
			PixelZ = -InsideHeight;
		}
		if (PixelX > InsideWidth)
		{
			PixelX = InsideWidth;
		}
		if (PixelZ > InsideHeight)
		{
			PixelZ = InsideHeight;
		}

		m_Decorators.emplace(a_Type, a_Icon, a_Id, a_Position, a_Yaw, static_cast<unsigned>(2 * PixelX + 1), static_cast<unsigned>(2 * PixelZ + 1), 0);
		m_Dirty = true;
	}
}





void cMap::RemoveDecorator(cMap::DecoratorType a_Type, cMap::icon a_Icon, UInt32 a_Id, const Vector3d & a_Position, int a_Yaw)
{
	auto itr = m_Decorators.find({ a_Type, a_Icon, a_Id, a_Position, a_Yaw, 0, 0, 0 });
	if (itr != m_Decorators.end())
	{
		m_Decorators.erase(itr);
		m_Dirty = true;
	}
}





