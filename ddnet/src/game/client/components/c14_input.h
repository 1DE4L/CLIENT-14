/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_C14_INPUT_H
#define GAME_CLIENT_COMPONENTS_C14_INPUT_H

#include <generated/protocol.h>

namespace C14
{
	// CLIENT 14 input arbitration.
	//
	// Every bot helper writes into the same CNetObj_PlayerInput that gets sent
	// to the server. Without coordination, later bots blindly overwrite fields
	// written by earlier ones (e.g. balance and avoid-freeze both drive
	// m_Direction, and the last Apply call wins regardless of importance).
	//
	// CInputLocks records, per input field, the priority level that currently
	// "owns" it. A bot may only write a field if its own priority is strictly
	// higher than the field's current owner. This makes the resolution
	// order-independent and lets survival-critical bots (freeze) override
	// cosmetic ones (balance/movement) instead of the reverse.
	//
	// Priority ordering (high -> low):
	//   PRIO_FREEZE   - anti/avoid freeze, auto jump save (staying alive)
	//   PRIO_COMBAT   - hammer bot, auto fire
	//   PRIO_AIMBOT   - aim correction, auto hook
	//   PRIO_BALANCE  - horizontal alignment
	//   PRIO_MOVEMENT - quick stop
	enum EInputPriority
	{
		PRIO_NONE = 0,
		PRIO_MOVEMENT = 20,
		PRIO_BALANCE = 40,
		PRIO_AIMBOT = 60,
		PRIO_COMBAT = 80,
		PRIO_FREEZE = 100,
	};

	struct CInputLocks
	{
		int m_DirectionOwner = PRIO_NONE;
		int m_HookOwner = PRIO_NONE;
		int m_JumpOwner = PRIO_NONE;
		int m_TargetOwner = PRIO_NONE;
		int m_FireOwner = PRIO_NONE;
	};

	inline bool CanClaim(int CurrentOwner, int MyPrio) { return MyPrio > CurrentOwner; }
}

#endif
