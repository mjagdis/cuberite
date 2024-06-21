
// BlockStandingBanner.h

#pragma once

#include "../BlockInfo.h"
#include "BlockEntity.h"
#include "../BlockEntities/BannerEntity.h"




class cBlockStandingBannerHandler final :
	public cBlockEntityHandler
{
	using Super = cBlockEntityHandler;

public:

	using Super::Super;

	virtual cItems ConvertToPickups(const NIBBLETYPE a_BlockMeta, const cItem * const a_Tool) const override
	{
		// Drops handled by the block entity:
		return {};
	}





	virtual bool CanBeAt(const cChunk & a_Chunk, const Vector3i a_Position, const NIBBLETYPE a_Meta) const override
	{
		if (a_Position.y < 1)
		{
			return false;
		}

		return cBlockInfo::IsSolid(a_Chunk.GetBlock(a_Position.addedY(-1)));
	}





	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		UNUSED(a_Meta);
		return 0;
	}





	virtual bool OnUse(
		cChunkInterface & a_ChunkInterface,
		cWorldInterface & a_WorldInterface,
		cPlayer & a_Player,
		const Vector3i a_BlockPos,
		eBlockFace a_BlockFace,
		const Vector3i a_CursorPos
	) const override
	{
		auto & m_Item = a_Player.GetEquippedItem();

		if (m_Item.m_ItemType == E_ITEM_MAP)
		{
			unsigned char Color = 15;
			AString CustomName = "";

			a_WorldInterface.DoWithBlockEntityAt(a_BlockPos, [&Color, &CustomName](cBlockEntity & a_BlockEntity)
			{
				ASSERT((a_BlockEntity.GetBlockType() == E_BLOCK_WALL_BANNER)
					|| (a_BlockEntity.GetBlockType() == E_BLOCK_STANDING_BANNER));

				cBannerEntity & BannerEntity = static_cast<cBannerEntity &>(a_BlockEntity);
				Color = BannerEntity.GetBaseColor();
				CustomName = BannerEntity.GetCustomName();
				return false;
			});

			a_Player.GetWorld()->GetMapManager().DoWithMap(static_cast<unsigned>(m_Item.m_ItemDamage), [&a_BlockPos, Color, CustomName](cMap & a_Map)
				{
					a_Map.AddBanner(Color, a_BlockPos, CustomName);
					return false;
				}
			);
		}

		return false;
	}
} ;
