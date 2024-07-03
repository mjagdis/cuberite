
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "HangingEntity.h"
#include "BlockInfo.h"
#include "Player.h"
#include "Chunk.h"
#include "../ClientHandle.h"





cHangingEntity::cHangingEntity(eEntityType a_EntityType, eBlockFace a_Facing, Vector3d a_Pos) :
	Super(a_EntityType, a_Pos, 0.5f, 0.5f)
{
	SetFacing(a_Facing);
	SetMaxHealth(1);
	SetHealth(1);
}





void cHangingEntity::SetFacing(eBlockFace a_Facing)
{
	m_Facing = a_Facing;

	double Yaw = 0;
	double Pitch = 0;

	switch (a_Facing)
	{
		default:
		case BLOCK_FACE_ZM: Yaw = 180.0; break;
		case BLOCK_FACE_ZP: Yaw = 0.0; break;
		case BLOCK_FACE_XP: Yaw = 270.0; break;
		case BLOCK_FACE_XM: Yaw = 90.0; break;
		case BLOCK_FACE_YP: Pitch = -90.0; break;
		case BLOCK_FACE_YM: Pitch = 90.0; break;
	}

	SetYaw(Yaw);
	SetPitch(Pitch);
}





bool cHangingEntity::IsValidSupportBlock(const BLOCKTYPE a_BlockType)
{
	return cBlockInfo::IsSolid(a_BlockType) && (a_BlockType != E_BLOCK_REDSTONE_REPEATER_OFF) && (a_BlockType != E_BLOCK_REDSTONE_REPEATER_ON);
}





void cHangingEntity::KilledBy(TakeDamageInfo & a_TDI)
{
	Super::KilledBy(a_TDI);

	Destroy();
}





void cHangingEntity::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	UNUSED(a_Dt);

	// Check for a valid support block once every 64 ticks (3.2 seconds):
	if ((m_World->GetWorldTickAge() % 64_tick) != 0_tick)
	{
		return;
	}

	BLOCKTYPE Block;
	const auto SupportPosition = AddFaceDirection(cChunkDef::AbsoluteToRelative(GetPosition()), m_Facing, true);
	if (!a_Chunk.UnboundedRelGetBlockType(SupportPosition, Block) || IsValidSupportBlock(Block))
	{
		return;
	}

	// Take environmental damage, intending to self-destruct, with "take damage" handling done by child classes:
	TakeDamage(dtEnvironment, nullptr, static_cast<int>(GetMaxHealth()), 0);
}
