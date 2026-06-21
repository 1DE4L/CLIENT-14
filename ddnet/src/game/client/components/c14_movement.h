/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_C14_MOVEMENT_H
#define GAME_CLIENT_COMPONENTS_C14_MOVEMENT_H

#include <engine/client.h>

#include <game/client/component.h>

#include "c14_input.h"

class CC14Movement : public CComponent
{
public:
	CC14Movement();
	int Sizeof() const override { return sizeof(*this); }

	void OnReset() override;
	void Apply(CNetObj_PlayerInput *pInput, C14::CInputLocks &Locks);

private:
	int m_aWasMoving[NUM_DUMMIES];
	int m_aQuickStopTicks[NUM_DUMMIES];
	// Chain-hook brake state: tracks the tick at which the tee should stop
	// counter-steering so it does not oscillate past the hook anchor point.
	int m_aChainBrakeTicks[NUM_DUMMIES];
};

#endif
