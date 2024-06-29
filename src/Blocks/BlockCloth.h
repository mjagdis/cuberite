
#pragma once

#include "BlockHandler.h"
#include "Map.h"





class cBlockClothHandler final :
	public cBlockHandler
{
public:

	using cBlockHandler::cBlockHandler;

private:

	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		switch (a_Meta)
		{
			case E_META_WOOL_WHITE: return cMap::eMapColor::E_MAP_COLOR_WHITE;
			case E_META_WOOL_ORANGE: return cMap::eMapColor::E_MAP_COLOR_ORANGE;
			case E_META_WOOL_MAGENTA: return cMap::eMapColor::E_MAP_COLOR_MAGENTA;
			case E_META_WOOL_LIGHTBLUE: return cMap::eMapColor::E_MAP_COLOR_LIGHTBLUE;
			case E_META_WOOL_YELLOW: return cMap::eMapColor::E_MAP_COLOR_YELLOW;
			case E_META_WOOL_LIGHTGREEN: return cMap::eMapColor::E_MAP_COLOR_LIGHTGREEN;
			case E_META_WOOL_PINK: return cMap::eMapColor::E_MAP_COLOR_PINK;
			case E_META_WOOL_GRAY: return cMap::eMapColor::E_MAP_COLOR_GRAY;
			case E_META_WOOL_LIGHTGRAY: return cMap::eMapColor::E_MAP_COLOR_LIGHTGRAY;
			case E_META_WOOL_CYAN: return cMap::eMapColor::E_MAP_COLOR_CYAN;
			case E_META_WOOL_PURPLE: return cMap::eMapColor::E_MAP_COLOR_PURPLE;
			case E_META_WOOL_BLUE: return cMap::eMapColor::E_MAP_COLOR_BLUE;
			case E_META_WOOL_BROWN: return cMap::eMapColor::E_MAP_COLOR_BROWN;
			case E_META_WOOL_GREEN: return cMap::eMapColor::E_MAP_COLOR_GREEN;
			case E_META_WOOL_RED: return cMap::eMapColor::E_MAP_COLOR_RED;
			case E_META_WOOL_BLACK: return cMap::eMapColor::E_MAP_COLOR_BLACK;
			default:
			{
				ASSERT(!"Unhandled meta in wool handler!");
				return cMap::eMapColor::E_MAP_COLOR_WOOL;
			}
		}
	}
} ;
