
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

		// The client predicted the use empty map would create a new "Map #0" and added one to
		// its view of the inventory. But we're not doing that! The new map has its very own
		// map number! We need to tell the client what it should have in any slot that we know
		// is empty or contains one or more "Map #0"s.
		cInventory & Inventory = a_Player->GetInventory();
		for (int i = cInventory::invInventoryOffset; i < cInventory::invShieldOffset; i++)
		{
			cItem Item = Inventory.GetSlot(i);
			if (Item.IsEmpty() || ((Item.m_ItemType == E_ITEM_MAP) && (Item.m_ItemDamage == 0)))
			{
				Inventory.SendSlot(i);
			}
		}

		unsigned int MapID;
		if (a_World->GetMapManager().CreateMap(MapID, a_HeldItem.m_ItemDamage, FloorC(a_Player->GetPosX()), FloorC(a_Player->GetPosZ())))
		{
			// At the moment we are restricted to 16 bit map IDs because it is stored as damage on the item.
			if (MapID <= 65535)
			{
				// Replace map in the inventory.
				a_Player->ReplaceOneEquippedItemTossRest(cItem(E_ITEM_MAP, 1, static_cast<short>(MapID)));
			}
		}

		return true;
	}
} ;
