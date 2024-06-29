
#pragma once

#include "BlockHandler.h"
#include "Map.h"
#include "Mixins.h"




class cBlockStairsHandler final :
	public cClearMetaOnDrop<cYawRotator<cBlockHandler, 0x03, 0x03, 0x00, 0x02, 0x01, true>>
{
	using Super = cClearMetaOnDrop<cYawRotator<cBlockHandler, 0x03, 0x03, 0x00, 0x02, 0x01, true>>;

public:

	using Super::Super;

	static bool IsAnyStairType(BLOCKTYPE a_Block)
	{
		switch (a_Block)
		{
			case E_BLOCK_SANDSTONE_STAIRS:
			case E_BLOCK_BIRCH_WOOD_STAIRS:
			case E_BLOCK_QUARTZ_STAIRS:
			case E_BLOCK_JUNGLE_WOOD_STAIRS:
			case E_BLOCK_RED_SANDSTONE_STAIRS:
			case E_BLOCK_COBBLESTONE_STAIRS:
			case E_BLOCK_STONE_BRICK_STAIRS:
			case E_BLOCK_OAK_WOOD_STAIRS:
			case E_BLOCK_ACACIA_WOOD_STAIRS:
			case E_BLOCK_PURPUR_STAIRS:
			case E_BLOCK_DARK_OAK_WOOD_STAIRS:
			case E_BLOCK_BRICK_STAIRS:
			case E_BLOCK_NETHER_BRICK_STAIRS:
			case E_BLOCK_SPRUCE_WOOD_STAIRS:
				return true;
			default:
			{
				return false;
			}
		}
	}

private:

	virtual NIBBLETYPE MetaMirrorXZ(NIBBLETYPE a_Meta) const override
	{
		// Toggle bit 3:
		return (a_Meta & 0x0b) | ((~a_Meta) & 0x04);
	}


	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		UNUSED(a_Meta);
		switch (m_BlockType)
		{
			case E_BLOCK_SANDSTONE_STAIRS: return cMap::eMapColor::E_MAP_COLOR_SAND;
			case E_BLOCK_BIRCH_WOOD_STAIRS: return cMap::eMapColor::E_MAP_COLOR_BIRCH;
			case E_BLOCK_QUARTZ_STAIRS: return cMap::eMapColor::E_MAP_COLOR_QUARTZ;
			case E_BLOCK_JUNGLE_WOOD_STAIRS: return cMap::eMapColor::E_MAP_COLOR_JUNGLE;
			case E_BLOCK_RED_SANDSTONE_STAIRS: return cMap::eMapColor::E_MAP_COLOR_ORANGE;
			case E_BLOCK_COBBLESTONE_STAIRS:
			case E_BLOCK_STONE_BRICK_STAIRS: return cMap::eMapColor::E_MAP_COLOR_STONE;
			case E_BLOCK_OAK_WOOD_STAIRS: return cMap::eMapColor::E_MAP_COLOR_OAK;
			case E_BLOCK_ACACIA_WOOD_STAIRS: return cMap::eMapColor::E_MAP_COLOR_ACACIA;
			case E_BLOCK_PURPUR_STAIRS: return cMap::eMapColor::E_MAP_COLOR_MAGENTA;
			case E_BLOCK_DARK_OAK_WOOD_STAIRS: return cMap::eMapColor::E_MAP_COLOR_DARK_OAK;
			case E_BLOCK_BRICK_STAIRS: return cMap::eMapColor::E_MAP_COLOR_RED;
			case E_BLOCK_NETHER_BRICK_STAIRS: return cMap::eMapColor::E_MAP_COLOR_NETHER;
			case E_BLOCK_SPRUCE_WOOD_STAIRS: return cMap::eMapColor::E_MAP_COLOR_SPRUCE;
			default:
			{
				ASSERT(!"Unhandled blocktype in stairs handler!");
				return cMap::eMapColor::E_MAP_COLOR_STONE;
			}
		}
	}





	/** EXCEPTION a.k.a. why is this removed:
	This collision-detection is actually more accurate than the client, but since the client itself
	sends inaccurate / sparse data, it's easier to just err on the side of the client and keep the
	two in sync by assuming that if a player hit ANY of the stair's bounding cube, it counts as the ground. */
	#if 0
	bool IsInsideBlock(Vector3d a_RelPosition, const BLOCKTYPE a_BlockType, const NIBBLETYPE a_BlockMeta)
	{
		if (a_BlockMeta & 0x4)  // upside down
		{
			return true;
		}
		else if ((a_BlockMeta & 0x3) == 0)  // tall side is east (+X)
		{
			return (a_RelPosition.y < ((a_RelPosition.x > 0.5) ? 1.0 : 0.5));
		}
		else if ((a_BlockMeta & 0x3) == 1)  // tall side is west (-X)
		{
			return (a_RelPosition.y < ((a_RelPosition.x < 0.5) ? 1.0 : 0.5));
		}
		else if ((a_BlockMeta & 0x3) == 2)  // tall side is south (+Z)
		{
			return (a_RelPosition.y < ((a_RelPosition.z > 0.5) ? 1.0 : 0.5));
		}
		else if ((a_BlockMeta & 0x3) == 3)  // tall side is north (-Z)
		{
			return (a_RelPosition.y < ((a_RelPosition.z < 0.5) ? 1.0 : 0.5));
		}
		return false;
	}
	#endif

} ;




