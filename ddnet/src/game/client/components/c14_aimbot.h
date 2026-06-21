/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_C14_AIMBOT_H
#define GAME_CLIENT_COMPONENTS_C14_AIMBOT_H

#include <base/vmath.h>

#include <engine/client.h>
#include <engine/console.h>

#include <game/client/component.h>

#include "c14_input.h"

class CC14Aimbot : public CComponent
{
public:
	CC14Aimbot();
	int Sizeof() const override { return sizeof(*this); }

	void OnConsoleInit() override;
	void OnReset() override;

	void Apply(CNetObj_PlayerInput *pInput, C14::CInputLocks &Locks);
	int TargetId() const { return m_TargetId; }

private:
	bool PredictHook(vec2 myPos, vec2 myVel, vec2 &targetPos, vec2 targetVel);
	bool HitScanHook(vec2 myPos, vec2 targetPos, vec2 scanDir);
	bool IntersectCharacter(vec2 hookPos, vec2 lineEnd, vec2 targetPos, vec2 &newPos);
	vec2 EdgeScan(vec2 myPos, vec2 myVel, vec2 targetPos, vec2 targetVel, bool &visible);
	bool InFov(float fovDeg, vec2 dir);
	int GetClosestId(float fovDeg, float range);

	static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData);

	vec2 m_Target;
	int m_TargetId;
	bool m_Active;
	int m_LastScanTick = -100;

	int m_aAutoHookKeyDown[NUM_DUMMIES];
	bool m_aBotHooked[NUM_DUMMIES];
	// Once auto-hook fires, the hooked target ID is remembered here so the
	// hook can be maintained independently of the aimbot scan (which flickers
	// every 2 ticks and when EdgeScan temporarily loses visibility).
	int m_aHookedTarget[NUM_DUMMIES];
};

#endif
