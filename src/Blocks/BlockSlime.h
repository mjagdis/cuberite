#pragma once

#include "BlockHandler.h"
#include "Map.h"





class cBlockSlimeHandler final :
	public cClearMetaOnDrop<cBlockHandler>
{
public:

	using cClearMetaOnDrop<cBlockHandler>::cClearMetaOnDrop;

private:

	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		UNUSED(a_Meta);
		return cMap::eMapColor::E_MAP_COLOR_GRASS;
	}
};
