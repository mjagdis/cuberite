
#pragma once

#include "BlockHandler.h"
#include "Map.h"





class cBlockTNTHandler final :
	public cBlockHandler
{
	using Super = cBlockHandler;

public:

	using Super::Super;

private:

	virtual void OnCancelRightClick(
		cChunkInterface & a_ChunkInterface,
		cWorldInterface & a_WorldInterface,
		cPlayer & a_Player,
		const Vector3i a_BlockPos,
		eBlockFace a_BlockFace
	) const override
	{
		a_WorldInterface.SendBlockTo(a_BlockPos, a_Player);
	}





	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		UNUSED(a_Meta);
		return cMap::eMapColor::E_MAP_COLOR_FIRE;
	}
};




