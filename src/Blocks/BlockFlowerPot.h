
#pragma once

#include "Map.h"





class cBlockFlowerPotHandler final :
	public cBlockEntityHandler
{
	using Super = cBlockEntityHandler;

public:

	using Super::Super;

private:

	virtual cItems ConvertToPickups(const NIBBLETYPE a_BlockMeta, const cItem * const a_Tool) const override
	{
		return cItem(E_ITEM_FLOWER_POT, 1, 0);
	}





	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		UNUSED(a_Meta);
		return cMap::eMapColor::E_MAP_COLOR_TERRACOTTA;
	}
} ;




