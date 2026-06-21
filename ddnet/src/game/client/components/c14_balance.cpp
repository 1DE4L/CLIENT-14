/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "c14_balance.h"

#include <base/math.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/console.h>
#include <engine/shared/config.h>

#include <game/client/components/controls.h>
#include <game/client/gameclient.h>

#include "c14_helpers.h"

CC14Balance::CC14Balance()
{
	mem_zero(m_aKeyDown, sizeof(m_aKeyDown));
	m_LastDirection = 0;
}

void CC14Balance::OnReset()
{
	mem_zero(m_aKeyDown, sizeof(m_aKeyDown));
	m_LastDirection = 0;
}

void CC14Balance::ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	CKeyInputState *pState = (CKeyInputState *)pUserData;
	if(pState->m_pBalance->GameClient()->m_GameInfo.m_BugDDRaceInput && pState->m_pBalance->GameClient()->m_Snap.m_SpecInfo.m_Active)
		return;
	*pState->m_apVariables[g_Config.m_ClDummy] = pResult->GetInteger(0);
}

void CC14Balance::OnConsoleInit()
{
	static CKeyInputState s_State = {this, {&m_aKeyDown[0], &m_aKeyDown[1]}};
	Console()->Register("+balance", "", CFGFLAG_CLIENT, ConKeyInputState, &s_State, "Predict nearest player and align horizontally");
}

void CC14Balance::Apply(CNetObj_PlayerInput *pInput, C14::CInputLocks &Locks)
{
	if(!C14::BotCanRun(GameClient()))
		return;

	if(!m_aKeyDown[g_Config.m_ClDummy] && !g_Config.m_ClBalanceBot)
	{
		m_HasTarget = false;
		m_LastDirection = 0;
		return;
	}

	int LocalId = GameClient()->m_Snap.m_LocalClientId;
	if(LocalId < 0 || !GameClient()->m_Snap.m_pLocalCharacter)
		return;

	vec2 MyPos = GameClient()->m_PredictedChar.m_Pos;
	int BestId = -1;
	float BestDist = 999999.0f;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == LocalId || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
			continue;
		vec2 TPos = GameClient()->m_aClients[i].m_Predicted.m_Pos;
		float Dist = distance(MyPos, TPos);
		if(Dist < BestDist)
		{
			BestDist = Dist;
			BestId = i;
		}
	}

	if(BestId < 0)
		return;

	// Predict where the target will be in N ticks using the same physics
	// constants as the engine. Horizontal: linear extrapolation with current
	// velocity. Vertical: cumulative gravity (0.5 per tick).
	const CCharacterCore &TargetSource = GameClient()->m_aClients[BestId].m_Predicted;
	vec2 PredictedTargetPos = C14::PredictPosition(TargetSource, 6);
	float DeltaX = PredictedTargetPos.x - MyPos.x;

	// Lock-on mode: when we're close enough to the target's predicted X,
	// freeze our current X as a target line and use a velocity-corrected
	// PID to stay glued to it. This lets the tee "stand" on top of the
	// target instead of jittering past it.
	const float LockRange = 6.0f;
	if(absolute(DeltaX) <= LockRange)
	{
		if(!m_HasTarget)
		{
			m_TargetX = MyPos.x;
			m_HasTarget = true;
		}
	}
	else
	{
		m_HasTarget = false;
	}

	if(m_HasTarget)
	{
		// PID-style velocity-corrected hold:
		//   DesiredVelX = (TargetX - CurrentX) * Gain   (P term, pull toward target)
		//   Error       = DesiredVelX - VelX            (velocity cancellation)
		// Use pulse-width modulation: 0/±1 input chosen by the sign of Error.
		float CurrentX = MyPos.x;
		float VelX = GameClient()->m_PredictedChar.m_Vel.x;
		const float Gain = 0.5f;
		float DesiredVelX = (m_TargetX - CurrentX) * Gain;
		float Error = DesiredVelX - VelX;
		const float DeadZone = 0.1f;
		int NewDir = 0;
		if(Error > DeadZone) NewDir = 1;
		else if(Error < -DeadZone) NewDir = -1;
		m_LastDirection = NewDir;
		if(C14::CanClaim(Locks.m_DirectionOwner, C14::PRIO_BALANCE))
		{
			pInput->m_Direction = NewDir;
			Locks.m_DirectionOwner = C14::PRIO_BALANCE;
		}
		return;
	}

	// Tracking mode: hysteresis-driven seek towards the predicted X.
	const float OnThreshold = 8.0f;
	const float OffThreshold = -3.0f;
	int NewDir2 = 0;
	if(m_LastDirection > 0)
	{
		if(DeltaX > OffThreshold) NewDir2 = 1;
		else NewDir2 = 0;
	}
	else if(m_LastDirection < 0)
	{
		if(DeltaX < -OffThreshold) NewDir2 = -1;
		else NewDir2 = 0;
	}
	else
	{
		if(DeltaX > OnThreshold) NewDir2 = 1;
		else if(DeltaX < -OnThreshold) NewDir2 = -1;
		else NewDir2 = 0;
	}
	m_LastDirection = NewDir2;
	if(C14::CanClaim(Locks.m_DirectionOwner, C14::PRIO_BALANCE))
	{
		pInput->m_Direction = NewDir2;
		Locks.m_DirectionOwner = C14::PRIO_BALANCE;
	}
}
