
#pragma once

#include "../Item.h"





class cItemEmptyMapHandler final:
	public cItemHandler
{
	using Super = cItemHandler;

public:

	using Super::Super;





	virtual bool OnItemUse(
		cWorld * a_World,
		cPlayer * a_Player,
		cBlockPluginInterface & a_PluginInterface,
		const cItem & a_HeldItem,
		const Vector3i a_ClickedBlockPos,
		eBlockFace a_ClickedBlockFace
	) const override
	{
		UNUSED(a_ClickedBlockFace);

		// The map center is fixed at the central point of the 8x8 block of chunks you are standing in when you right-click it.

		const int RegionWidth = cChunkDef::Width * 8;

		int CenterX = FloorC(a_Player->GetPosX() / RegionWidth) * RegionWidth + (RegionWidth / 2);
		int CenterZ = FloorC(a_Player->GetPosZ() / RegionWidth) * RegionWidth + (RegionWidth / 2);

		unsigned short MapID;
		if (a_World->GetMapManager().CreateMap(MapID, a_HeldItem.m_ItemDamage, CenterX, CenterZ))
		{
			// Replace map in the inventory:
			a_Player->ReplaceOneEquippedItemTossRest(cItem(E_ITEM_MAP, 1, MapID));
		}
		return true;
	}
} ;
