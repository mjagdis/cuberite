
#pragma once

#include "BlockHandler.h"
#include "Map.h"
#include "Mixins.h"
#include "../EffectID.h"




class cBlockFenceGateHandler final :
	public cClearMetaOnDrop<cYawRotator<cBlockHandler, 0x03, 0x02, 0x03, 0x00, 0x01>>
{
	using Super = cClearMetaOnDrop<cYawRotator<cBlockHandler, 0x03, 0x02, 0x03, 0x00, 0x01>>;

public:

	using Super::Super;

private:

	virtual bool OnUse(
		cChunkInterface & a_ChunkInterface,
		cWorldInterface & a_WorldInterface,
		cPlayer & a_Player,
		const Vector3i a_BlockPos,
		eBlockFace a_BlockFace,
		const Vector3i a_CursorPos
	) const override
	{
		NIBBLETYPE OldMetaData = a_ChunkInterface.GetBlockMeta(a_BlockPos);
		NIBBLETYPE NewMetaData = YawToMetaData(a_Player.GetYaw());
		OldMetaData ^= 4;  // Toggle the gate

		if ((OldMetaData & 1) == (NewMetaData & 1))
		{
			// Standing in front of the gate - apply new direction
			a_ChunkInterface.SetBlockMeta(a_BlockPos, (OldMetaData & 4) | (NewMetaData & 3));
		}
		else
		{
			// Standing aside - use last direction
			a_ChunkInterface.SetBlockMeta(a_BlockPos, OldMetaData);
		}
		a_Player.GetWorld()->BroadcastSoundParticleEffect(EffectID::SFX_RANDOM_FENCE_GATE_OPEN, a_BlockPos, 0, a_Player.GetClientHandle());
		return true;
	}





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





	virtual bool IsUseable(void) const override
	{
		return true;
	}





	virtual ColourID GetMapBaseColourID(NIBBLETYPE a_Meta) const override
	{
		UNUSED(a_Meta);
		switch (m_BlockType)
		{
			case E_BLOCK_OAK_FENCE_GATE: return cMap::eMapColor::E_MAP_COLOR_OAK;
			case E_BLOCK_SPRUCE_FENCE_GATE: return cMap::eMapColor::E_MAP_COLOR_SPRUCE;
			case E_BLOCK_BIRCH_FENCE_GATE: return cMap::eMapColor::E_MAP_COLOR_BIRCH;
			case E_BLOCK_JUNGLE_FENCE_GATE: return cMap::eMapColor::E_MAP_COLOR_JUNGLE;
			case E_BLOCK_DARK_OAK_FENCE_GATE: return cMap::eMapColor::E_MAP_COLOR_DARK_OAK;
			case E_BLOCK_ACACIA_FENCE_GATE: return cMap::eMapColor::E_MAP_COLOR_ACACIA;
			default:
			{
				ASSERT(!"Unhandled blocktype in fence gate handler!");
				return cMap::eMapColor::E_MAP_COLOR_WOOD;
			}
		}
	}
} ;




