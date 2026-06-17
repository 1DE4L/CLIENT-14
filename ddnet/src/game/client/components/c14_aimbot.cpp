/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "c14_aimbot.h"

#include <base/math.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/gamecore.h>
#include <game/client/components/controls.h>
#include <game/client/gameclient.h>

#include "c14_helpers.h"

CC14Aimbot::CC14Aimbot()
{
	mem_zero(m_aAutoHookKeyDown, sizeof(m_aAutoHookKeyDown));
	mem_zero(m_aBotHooked, sizeof(m_aBotHooked));
	m_Target = vec2(0.0f, 0.0f);
	m_TargetId = -1;
	m_Active = false;
}

void CC14Aimbot::OnReset()
{
	mem_zero(m_aAutoHookKeyDown, sizeof(m_aAutoHookKeyDown));
	mem_zero(m_aBotHooked, sizeof(m_aBotHooked));
	m_Target = vec2(0.0f, 0.0f);
	m_TargetId = -1;
	m_Active = false;
}

struct CAimbotKeyInputState
{
	CC14Aimbot *m_pAimbot;
	int *m_apVariables[NUM_DUMMIES];
};

void CC14Aimbot::ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	CAimbotKeyInputState *pState = (CAimbotKeyInputState *)pUserData;
	if(pState->m_pAimbot->GameClient()->m_GameInfo.m_BugDDRaceInput && pState->m_pAimbot->GameClient()->m_Snap.m_SpecInfo.m_Active)
		return;
	*pState->m_apVariables[g_Config.m_ClDummy] = pResult->GetInteger(0);
}

void CC14Aimbot::OnConsoleInit()
{
	static CAimbotKeyInputState s_AutoHookState = {this, {&m_aAutoHookKeyDown[0], &m_aAutoHookKeyDown[1]}};
	Console()->Register("+auto_hook", "", CFGFLAG_CLIENT, ConKeyInputState, &s_AutoHookState, "Auto hook aimbot target");
}

bool CC14Aimbot::PredictHook(vec2 myPos, vec2 myVel, vec2 &targetPos, vec2 targetVel)
{
	const vec2 delta = targetPos - myPos;
	const vec2 deltaVel = targetVel - myVel;
	const float hookSpeed = length(targetVel) + GameClient()->m_aTuning[g_Config.m_ClDummy].m_HookFireSpeed;
	const float a = dot(deltaVel, deltaVel) - hookSpeed * hookSpeed;
	const float b = 2.0f * dot(deltaVel, delta);
	const float c = dot(delta, delta);
	const float sol = b * b - 4.0f * a * c;
	if(sol > 0.0f)
	{
		const float time = absolute(2.0f * c / (sqrtf(sol) - b));
		targetPos += targetVel * time;
		return true;
	}
	return false;
}

bool CC14Aimbot::IntersectCharacter(vec2 hookPos, vec2 lineEnd, vec2 targetPos, vec2 &newPos)
{
	vec2 closestPoint;
	if(closest_point_on_line(hookPos, lineEnd, targetPos, closestPoint))
	{
		if(distance(targetPos, closestPoint) < C14::PHYS_SIZE + 2.0f)
		{
			newPos = closestPoint;
			return true;
		}
	}
	return false;
}

bool CC14Aimbot::HitScanHook(vec2 myPos, vec2 targetPos, vec2 scanDir)
{
	vec2 exDir = normalize(scanDir);
	const float hookLen = GameClient()->m_aTuning[g_Config.m_ClDummy].m_HookLength;
	const float hookSpeed = GameClient()->m_aTuning[g_Config.m_ClDummy].m_HookFireSpeed;
	vec2 finishPos = myPos + exDir * (hookLen - C14::PHYS_SIZE * 1.5f);
	vec2 oldPos = myPos + exDir * C14::PHYS_SIZE * 1.5f;
	vec2 newPos = oldPos;
	bool doBreak = false;
	do
	{
		oldPos = newPos;
		newPos = oldPos + exDir * hookSpeed;
		if(distance(myPos, newPos) > hookLen)
		{
			newPos = myPos + normalize(newPos - myPos) * hookLen;
			doBreak = true;
		}
		int teleNr = 0;
		vec2 colPos = finishPos;
		const int hit = GameClient()->Collision()->IntersectLineTeleHook(oldPos, newPos, &colPos, nullptr, &teleNr);
		if(IntersectCharacter(oldPos, colPos, targetPos, finishPos))
			return true;
		if(hit)
			break;
		newPos.x = round_to_int(newPos.x);
		newPos.y = round_to_int(newPos.y);
		if(oldPos == newPos)
			break;
		exDir.x = round_to_int(exDir.x * 256.0f) / 256.0f;
		exDir.y = round_to_int(exDir.y * 256.0f) / 256.0f;
	} while(!doBreak);
	return false;
}

vec2 CC14Aimbot::EdgeScan(vec2 myPos, vec2 myVel, vec2 targetPos, vec2 targetVel, bool &visible)
{
	visible = false;
	vec2 predictedTarget = targetPos;
	if(!PredictHook(myPos, myVel, predictedTarget, targetVel))
		return vec2(0, 0);
	if(HitScanHook(myPos, predictedTarget, predictedTarget - myPos))
	{
		visible = true;
		return predictedTarget - myPos;
	}
	static constexpr int MAX_HP = 32;
	vec2 hitPoints[MAX_HP];
	int hitCount = 0;
	const float visAngle = atan2(predictedTarget.y - myPos.y, predictedTarget.x - myPos.x) + pi * 0.5f;
	for(float i = visAngle; i < pi + visAngle; i += 0.2f)
	{
		if(hitCount >= MAX_HP)
			break;
		vec2 pos = vec2(round_to_int(predictedTarget.x + cosf(i) * C14::PHYS_SIZE),
			round_to_int(predictedTarget.y + sinf(i) * C14::PHYS_SIZE));
		vec2 dir = pos - myPos;
		if(HitScanHook(myPos, predictedTarget, dir))
		{
			hitPoints[hitCount] = dir;
			hitCount++;
		}
	}
	if(hitCount > 0)
	{
		visible = true;
		return hitPoints[(hitCount - 1) / 2];
	}
	return vec2(0, 0);
}

bool CC14Aimbot::InFov(float fovDeg, vec2 dir)
{
	if(fovDeg >= 360.0f)
		return true;
	const float diff = absolute(atan2(sin(angle(dir) - angle(GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy])),
		cos(angle(dir) - angle(GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy])))) * (180.0f / pi);
	return diff <= fovDeg * 0.5f;
}

int CC14Aimbot::GetClosestId(float fovDeg, float range)
{
	const vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
	const vec2 MouseDir = GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy];
	float BestScore = 999999.0f;
	int ClosestID = -1;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == GameClient()->m_Snap.m_LocalClientId)
			continue;
		if(!GameClient()->m_Snap.m_aCharacters[i].m_Active)
			continue;
		vec2 Position = GameClient()->m_aClients[i].m_Predicted.m_Pos;
		if(!InFov(fovDeg, Position - Pos))
			continue;

		float score;
		if(g_Config.m_ClAimbotTarget == 0)
		{
			vec2 aimDir = Position - Pos;
			float angleDiff = absolute(atan2(sin(angle(aimDir) - angle(MouseDir)),
				cos(angle(aimDir) - angle(MouseDir))));
			score = angleDiff;
		}
		else
		{
			score = distance(Pos, Position);
		}

		if(score < BestScore)
		{
			BestScore = score;
			ClosestID = i;
		}
	}
	return ClosestID;
}

void CC14Aimbot::Apply(CNetObj_PlayerInput *pInput)
{
	if(!C14::BotCanRun(GameClient()))
		return;

	// Throttled target scan: heavy EdgeScan every 2 ticks, not every input
	if(Client()->GameTick(0) - m_LastScanTick >= 2)
	{
		m_LastScanTick = Client()->GameTick(0);
		m_Active = false;
		m_TargetId = -1;
		if(g_Config.m_ClAimbot && GameClient()->m_Snap.m_pGameInfoObj && !GameClient()->m_Snap.m_SpecInfo.m_Active && GameClient()->m_Snap.m_pLocalCharacter)
		{
		vec2 MyPos = GameClient()->m_PredictedChar.m_Pos;
		vec2 MyVel = GameClient()->m_PredictedChar.m_Vel;
		int BestId = GetClosestId((float)g_Config.m_ClAimbotFov, 400.0f);
		if(BestId >= 0)
		{
			vec2 TargetPos = GameClient()->m_aClients[BestId].m_Predicted.m_Pos;
			vec2 TargetVel = GameClient()->m_aClients[BestId].m_Predicted.m_Vel;
			bool visible = false;
			vec2 AimDir = EdgeScan(MyPos, MyVel, TargetPos, TargetVel, visible);
			if(visible && (AimDir.x != 0.0f || AimDir.y != 0.0f))
			{
				const float CameraMaxDist = 200.0f;
				const float FollowFactor = (g_Config.m_ClDyncam ? g_Config.m_ClDyncamFollowFactor : g_Config.m_ClMouseFollowfactor) / 100.0f;
				const float DeadZone = g_Config.m_ClDyncam ? g_Config.m_ClDyncamDeadzone : g_Config.m_ClMouseDeadzone;
				const float MaxDist = g_Config.m_ClMouseMaxDistance;
				const float MouseMax = minimum((FollowFactor != 0.0f ? CameraMaxDist / FollowFactor + DeadZone : MaxDist), MaxDist);
				float aimLen = length(AimDir);
				if(aimLen > 0.001f)
				{
					AimDir = normalize_pre_length(AimDir, aimLen) * MouseMax;
					AimDir = vec2(round_to_int(AimDir.x), round_to_int(AimDir.y));
				}

				m_Active = true;
				m_Target = AimDir;
				m_TargetId = BestId;
				if(!g_Config.m_ClAimbotSilent)
				{
					GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy] = AimDir;
					GameClient()->m_Controls.ClampMousePos();
				}
			}
		}
	}
	}

	// CLIENT 14 Silent Aim
	if(g_Config.m_ClAimbot && g_Config.m_ClAimbotSilent && m_Active)
	{
		pInput->m_TargetX = (int)m_Target.x;
		pInput->m_TargetY = (int)m_Target.y;
	}

	// CLIENT 14 Auto Hook
	if(m_aAutoHookKeyDown[g_Config.m_ClDummy] && m_Active && m_TargetId >= 0 && GameClient()->m_Snap.m_pLocalCharacter)
	{
		int LocalId = GameClient()->m_Snap.m_LocalClientId;
		if(LocalId < 0 || GameClient()->m_aClients[LocalId].m_FreezeEnd <= Client()->GameTick(0))
		{
			vec2 MyPos = GameClient()->m_PredictedChar.m_Pos;
			vec2 TargetPos = GameClient()->m_aClients[m_TargetId].m_Predicted.m_Pos;
			float Dist = distance(MyPos, TargetPos);
			if(Dist < GameClient()->m_aTuning[g_Config.m_ClDummy].m_HookLength)
			{
				pInput->m_Hook = 1;
				pInput->m_TargetX = (int)(TargetPos.x - MyPos.x);
				pInput->m_TargetY = (int)(TargetPos.y - MyPos.y);
				m_aBotHooked[g_Config.m_ClDummy] = true;
			}
		}
	}
	else if(m_aBotHooked[g_Config.m_ClDummy] && !m_aAutoHookKeyDown[g_Config.m_ClDummy])
	{
		pInput->m_Hook = 0;
		m_aBotHooked[g_Config.m_ClDummy] = false;
	}
}
