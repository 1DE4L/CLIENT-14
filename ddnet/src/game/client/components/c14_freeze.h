/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_C14_FREEZE_H
#define GAME_CLIENT_COMPONENTS_C14_FREEZE_H

#include <generated/protocol.h>

#include <game/client/component.h>

#include "c14_input.h"

class CC14FreezeHelper : public CComponent
{
public:
	CC14FreezeHelper();
	int Sizeof() const override { return sizeof(*this); }

	void OnReset() override;
	void Apply(CNetObj_PlayerInput *pInput, C14::CInputLocks &Locks);

private:
	// Auto-Jump-Save: the 35-tick jump-arc simulation is the costly part and
	// is rerun every m_ArcSimInterval ticks; the cheaper ground/freeze probes
	// still run every tick.
	int m_LastArcSimTick = -100;
	bool m_CachedJumpArcSafe = true;
	int m_ArcSimInterval = 3;
};

#endif
