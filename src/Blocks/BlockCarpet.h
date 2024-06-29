
// BlockCarpet.h

// Declares the cBlockCarpetHandler class representing the handler for the carpet block




#pragma once

#include "BlockHandler.h"
#include "Map.h"





class cBlockCarpetHandler final :
	public cBlockHandler
{
	using Super = cBlockHandler;

public:

	using Super::Super;

private:

	virtual bool CanBeAt(const cChunk & a_Chunk, const Vector3i a_Position, const NIBBLETYPE a_Meta) const override
	{
		return (a_Position.y > 0) && (a_Chunk.GetBlock(a_Position.addedY(-1)) != E_BLOCK_AIR);
	}





	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		switch (a_Meta)
		{
			case E_META_CARPET_WHITE: return cMap::eMapColor::E_MAP_COLOR_WHITE;
			case E_META_CARPET_ORANGE: return cMap::eMapColor::E_MAP_COLOR_ORANGE;
			case E_META_CARPET_MAGENTA: return cMap::eMapColor::E_MAP_COLOR_MAGENTA;
			case E_META_CARPET_LIGHTBLUE: return cMap::eMapColor::E_MAP_COLOR_LIGHTBLUE;
			case E_META_CARPET_YELLOW: return cMap::eMapColor::E_MAP_COLOR_YELLOW;
			case E_META_CARPET_LIGHTGREEN: return cMap::eMapColor::E_MAP_COLOR_LIGHTGREEN;
			case E_META_CARPET_PINK: return cMap::eMapColor::E_MAP_COLOR_PINK;
			case E_META_CARPET_GRAY: return cMap::eMapColor::E_MAP_COLOR_GRAY;
			case E_META_CARPET_LIGHTGRAY: return cMap::eMapColor::E_MAP_COLOR_LIGHTGRAY;
			case E_META_CARPET_CYAN: return cMap::eMapColor::E_MAP_COLOR_CYAN;
			case E_META_CARPET_PURPLE: return cMap::eMapColor::E_MAP_COLOR_PURPLE;
			case E_META_CARPET_BLUE: return cMap::eMapColor::E_MAP_COLOR_BLUE;
			case E_META_CARPET_BROWN: return cMap::eMapColor::E_MAP_COLOR_BROWN;
			case E_META_CARPET_GREEN: return cMap::eMapColor::E_MAP_COLOR_GREEN;
			case E_META_CARPET_RED: return cMap::eMapColor::E_MAP_COLOR_RED;
			case E_META_CARPET_BLACK: return cMap::eMapColor::E_MAP_COLOR_BLACK;
			default:
			{
				ASSERT(!"Unhandled meta in carpet handler!");
				return cMap::eMapColor::E_MAP_COLOR_WOOL;
			}
		}
	}
} ;




