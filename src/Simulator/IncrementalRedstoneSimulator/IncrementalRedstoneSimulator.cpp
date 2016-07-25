

#include "Globals.h"

#include "IncrementalRedstoneSimulator.h"
#include "Chunk.h"

#include "CommandBlockHandler.h"
#include "DoorHandler.h"
#include "RedstoneHandler.h"
#include "RedstoneTorchHandler.h"
#include "RedstoneWireHandler.h"
#include "RedstoneRepeaterHandler.h"
#include "RedstoneToggleHandler.h"
#include "SolidBlockHandler.h"
#include "RedstoneLampHandler.h"
#include "RedstoneBlockHandler.h"
#include "PistonHandler.h"
#include "SmallGateHandler.h"
#include "NoteBlockHandler.h"
#include "TNTHandler.h"
#include "PoweredRailHandler.h"
#include "PressurePlateHandler.h"
#include "TripwireHookHandler.h"
#include "DropSpenserHandler.h"
#include "RedstoneComparatorHandler.h"
#include "TrappedChestHandler.h"





cRedstoneHandler * cIncrementalRedstoneSimulator::CreateComponent(cWorld & a_World, BLOCKTYPE a_BlockType, cIncrementalRedstoneSimulatorChunkData * a_Data)
{
	switch (a_BlockType)
	{
		case E_BLOCK_ACTIVATOR_RAIL:
		case E_BLOCK_DETECTOR_RAIL:
		case E_BLOCK_POWERED_RAIL:
		{
			static thread_local cPoweredRailHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_ACTIVE_COMPARATOR:
		case E_BLOCK_INACTIVE_COMPARATOR:
		{
			static thread_local cRedstoneComparatorHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_DISPENSER:
		case E_BLOCK_DROPPER:
		{
			static thread_local cDropSpenserHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_HEAVY_WEIGHTED_PRESSURE_PLATE:
		case E_BLOCK_LIGHT_WEIGHTED_PRESSURE_PLATE:
		case E_BLOCK_STONE_PRESSURE_PLATE:
		case E_BLOCK_WOODEN_PRESSURE_PLATE:
		{
			static thread_local cPressurePlateHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_FENCE_GATE:
		case E_BLOCK_IRON_TRAPDOOR:
		case E_BLOCK_TRAPDOOR:
		{
			static thread_local cSmallGateHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_REDSTONE_LAMP_OFF:
		case E_BLOCK_REDSTONE_LAMP_ON:
		{
			static thread_local cRedstoneLampHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_REDSTONE_REPEATER_OFF:
		case E_BLOCK_REDSTONE_REPEATER_ON:
		{
			static thread_local cRedstoneRepeaterHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_REDSTONE_TORCH_OFF:
		case E_BLOCK_REDSTONE_TORCH_ON:
		{
			static thread_local cRedstoneTorchHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_PISTON:
		case E_BLOCK_STICKY_PISTON:
		{
			static thread_local cPistonHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_LEVER:
		case E_BLOCK_STONE_BUTTON:
		case E_BLOCK_WOODEN_BUTTON:
		{
			static thread_local cRedstoneToggleHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_BLOCK_OF_REDSTONE:
		{
			static thread_local cRedstoneBlockHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_COMMAND_BLOCK:
		{
			static thread_local cCommandBlockHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_NOTE_BLOCK:
		{
			static thread_local cNoteBlockHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_REDSTONE_WIRE:
		{
			static thread_local cRedstoneWireHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_TNT:
		{
			static thread_local cTNTHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_TRAPPED_CHEST:
		{
			static thread_local cTrappedChestHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		case E_BLOCK_TRIPWIRE_HOOK:
		{
			static thread_local cTripwireHookHandler ComponentHandler(a_World);
			return &ComponentHandler;
		}
		default:
		{
			if (cBlockDoorHandler::IsDoorBlockType(a_BlockType))
			{
				static thread_local cDoorHandler ComponentHandler(a_World);
				return &ComponentHandler;
			}

			if (cBlockInfo::FullyOccupiesVoxel(a_BlockType))
			{
				static thread_local cSolidBlockHandler ComponentHandler(a_World);
				return &ComponentHandler;
			}
			return nullptr;
		}
	}
}





void cIncrementalRedstoneSimulator::Simulate(float a_dt)
{
	for (auto & DelayInfo : m_Data.m_MechanismDelays)
	{
		if ((--DelayInfo.second.first) == 0)
		{
			m_Data.GetActiveBlocks().emplace_back(DelayInfo.first);
		}
	}

	// Build our work queue
	cVector3iArray WorkQueue;
	std::swap(WorkQueue, m_Data.GetActiveBlocks());

	// Process the work queue
	while (!WorkQueue.empty())
	{
		// Grab the first element and remove it from the list
		Vector3i CurrentLocation = WorkQueue.back();
		WorkQueue.pop_back();

		BLOCKTYPE CurrentBlock;
		NIBBLETYPE CurrentMeta;
		if (!m_World.GetBlockTypeMeta(CurrentLocation.x, CurrentLocation.y, CurrentLocation.z, CurrentBlock, CurrentMeta))
		{
			continue;
		}

		auto CurrentHandler = cIncrementalRedstoneSimulator::CreateComponent(m_World, CurrentBlock, &m_Data);
		if (CurrentHandler == nullptr)  // Block at CurrentPosition doesn't have a corresponding redstone handler
		{
			// Clean up cached PowerData for CurrentPosition
			static_cast<cIncrementalRedstoneSimulator *>(m_World.GetRedstoneSimulator())->GetChunkData()->ErasePowerData(CurrentLocation);

			continue;
		}

		cRedstoneHandler::PoweringData Power;
		for (const auto & Location : CurrentHandler->GetValidSourcePositions(CurrentLocation, CurrentBlock, CurrentMeta))
		{
			if (!cChunk::IsValidHeight(Location.y))
			{
				continue;
			}
			BLOCKTYPE PotentialBlock;
			NIBBLETYPE PotentialMeta;
			m_World.GetBlockTypeMeta(Location.x, Location.y, Location.z, PotentialBlock, PotentialMeta);

			auto PotentialSourceHandler = cIncrementalRedstoneSimulator::CreateComponent(m_World, PotentialBlock, &m_Data);
			if (PotentialSourceHandler == nullptr)
			{
				continue;
			}

			decltype(Power) PotentialPower(PotentialBlock, PotentialSourceHandler->GetPowerDeliveredToPosition(Location, PotentialBlock, PotentialMeta, CurrentLocation, CurrentBlock));
			Power = std::max(Power, PotentialPower);
		}

		// Inform the handler to update
		cVector3iArray Updates = CurrentHandler->Update(CurrentLocation, CurrentBlock, CurrentMeta, Power);
		WorkQueue.insert(WorkQueue.end(), Updates.begin(), Updates.end());

		if (IsAlwaysTicked(CurrentBlock))
		{
			m_Data.GetActiveBlocks().emplace_back(CurrentLocation);
		}
	}
}
