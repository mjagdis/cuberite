
#pragma once

#include "../Item.h"





class cItemMapHandler final:
	public cItemHandler
{
	using Super = cItemHandler;

public:

	using Super::Super;

	virtual void OnUpdate(cWorld * a_World, cPlayer * a_Player, const cItem & a_Item, bool a_IsEquipped) const override
	{
		a_World->GetMapManager().DoWithMap(static_cast<unsigned>(a_Item.m_ItemDamage), [a_Player, a_IsEquipped](cMap & a_Map)
			{
				a_Map.UpdateClient(a_Player, a_IsEquipped);
				return true;
			}
		);
	}
} ;
