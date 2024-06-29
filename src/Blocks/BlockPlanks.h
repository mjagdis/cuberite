
#pragma once

#include "BlockHandler.h"
#include "Map.h"





class cBlockPlanksHandler final :
	public cBlockHandler
{
	using Super = cBlockHandler;

public:

	using Super::Super;

private:

	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		switch (a_Meta)
		{
			case E_META_PLANKS_BIRCH: return cMap::eMapColor::E_MAP_COLOR_BIRCH;
			case E_META_PLANKS_JUNGLE: return cMap::eMapColor::E_MAP_COLOR_JUNGLE;
			case E_META_PLANKS_OAK: return cMap::eMapColor::E_MAP_COLOR_OAK;
			case E_META_PLANKS_ACACIA: return cMap::eMapColor::E_MAP_COLOR_ACACIA;
			case E_META_PLANKS_DARK_OAK: return cMap::eMapColor::E_MAP_COLOR_DARK_OAK;
			case E_META_PLANKS_SPRUCE: return cMap::eMapColor::E_MAP_COLOR_SPRUCE;
			default:
			{
				ASSERT(!"Unhandled meta in planks handler!");
				return cMap::eMapColor::E_MAP_COLOR_WOOD;
			}
		}
	}
} ;




