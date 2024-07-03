
#include "Globals.h"  // NOTE: MSVC stupidness requires this to be the same across all modules

#include "ItemFrame.h"
#include "Player.h"
#include "../ClientHandle.h"
#include "Chunk.h"





cItemFrame::cItemFrame(eBlockFace a_BlockFace, Vector3d a_Pos):
	Super(etItemFrame, a_BlockFace, a_Pos),
	m_Item(E_BLOCK_AIR),
	m_ItemRotation(0)
{
}





bool cItemFrame::DoTakeDamage(TakeDamageInfo & a_TDI)
{
	// Take environmental or non-player damage normally:
	if (m_Item.IsEmpty() || (a_TDI.Attacker == nullptr) || !a_TDI.Attacker->IsPlayer())
	{
		return Super::DoTakeDamage(a_TDI);
	}

	// Only pop out a pickup if attacked by a non-creative player:
	if (!static_cast<cPlayer *>(a_TDI.Attacker)->IsGameModeCreative())
	{
		// Where the pickup spawns, offset by half cPickup height to centre in the block.
		const auto SpawnPosition = GetPosition().addedY(-0.125);

		// The direction the pickup travels to simulate a pop-out effect.
		const auto FlyOutSpeed = AddFaceDirection(Vector3i(), m_Facing) * 2;

		// Spawn the frame's held item:
		GetWorld()->SpawnItemPickup(SpawnPosition, m_Item, FlyOutSpeed);
	}

	if (m_Item.m_ItemType == E_ITEM_MAP)
	{
		GetWorld()->GetMapManager().DoWithMap(static_cast<unsigned>(m_Item.m_ItemDamage), [this](cMap & a_Map)
			{
				a_Map.RemoveFrame(GetUniqueID());
				return true;
			}
		);
	}

	// In any case we have a held item and were hit by a player, so clear it:
	m_Item.Empty();
	m_ItemRotation = 0;
	a_TDI.FinalDamage = 0;
	SetInvulnerableTicks(0);
	GetWorld()->BroadcastEntityMetadata(*this);
	return false;
}





void cItemFrame::GetDrops(cItems & a_Items, cEntity * a_Killer)
{
	if (!m_Item.IsEmpty())
	{
		a_Items.push_back(m_Item);
	}

	a_Items.emplace_back(E_ITEM_ITEM_FRAME);
}





void cItemFrame::OnRightClicked(cPlayer & a_Player)
{
	Super::OnRightClicked(a_Player);

	if (!m_Item.IsEmpty())
	{
		// Item not empty, rotate, clipping values to zero to seven inclusive
		m_ItemRotation++;
		if (m_ItemRotation >= 8)
		{
			m_ItemRotation = 0;
		}
	}
	else if (!a_Player.GetEquippedItem().IsEmpty())
	{
		// Item empty, and player held item not empty - add this item to self
		m_Item = a_Player.GetEquippedItem();
		m_Item.m_ItemCount = 1;

		if (!a_Player.IsGameModeCreative())
		{
			a_Player.GetInventory().RemoveOneEquippedItem();
		}
	}

	if (m_Item.m_ItemType == E_ITEM_MAP)
	{
		GetWorld()->GetMapManager().DoWithMap(static_cast<unsigned>(m_Item.m_ItemDamage), [this](cMap & a_Map)
			{
				if (a_Map.GetWorld() == GetWorld())
				{
					a_Map.AddFrame(GetUniqueID(), GetPosition(), GetYaw());
				}

				return true;
			}
		);
	}

	GetWorld()->BroadcastEntityMetadata(*this);  // Update clients
	GetParentChunk()->MarkDirty();               // Mark chunk dirty to save rotation or item
}





void cItemFrame::SpawnOn(cClientHandle & a_ClientHandle)
{
	a_ClientHandle.SendSpawnEntity(*this);
	a_ClientHandle.SendEntityMetadata(*this);

	if (m_Item.m_ItemType == E_ITEM_MAP)
	{
		GetWorld()->GetMapManager().DoWithMap(static_cast<unsigned>(m_Item.m_ItemDamage), [this](cMap & a_Map)
			{
				if (a_Map.GetWorld() == GetWorld())
				{
					// Item IDs are dynamic and item entities may occupy the same block as
					// each other. We may already have a marker for this frame (from a save
					// or a spawn due to a past visit to the area by a player) but if so
					// it will have the same position and yaw as we have (different item
					// frames at the same position will always be facing in different
					// directions). We don't want the old marker any more.
					a_Map.RemoveFrame(GetPosition(), GetYaw());
					a_Map.AddFrame(GetUniqueID(), GetPosition(), GetYaw());
				}

				return true;
			}
		);
	}
}





void cItemFrame::Tick(std::chrono::milliseconds a_Dt, cChunk & a_Chunk)
{
	if (m_Item.m_ItemType == E_ITEM_MAP)
	{
		// All clients interested in this chunk are also going to need to see changes
		// to the framed map.
		GetWorld()->GetMapManager().DoWithMap(static_cast<unsigned>(m_Item.m_ItemDamage), [&a_Chunk](cMap & a_Map)
			{
				for (auto Client : a_Chunk.GetAllClients())
				{
					a_Map.AddClient(Client);
				}
				return true;
			}
		);
	}

	Super::Tick(a_Dt, a_Chunk);
}
