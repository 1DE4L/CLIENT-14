/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "c14_combat.h"

#include <base/math.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/console.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/client/components/controls.h>
#include <game/client/gameclient.h>

CC14Combat::CC14Combat()
{
	mem_zero(m_aAutoFireKeyDown, sizeof(m_aAutoFireKeyDown));
	mem_zero(m_aAutoFireTimer, sizeof(m_aAutoFireTimer));
	mem_zero(m_aLastHammerTick, sizeof(m_aLastHammerTick));
}

void CC14Combat::OnReset()
{
	mem_zero(m_aAutoFireKeyDown, sizeof(m_aAutoFireKeyDown));
	mem_zero(m_aAutoFireTimer, sizeof(m_aAutoFireTimer));
	mem_zero(m_aLastHammerTick, sizeof(m_aLastHammerTick));
}

void CC14Combat::ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	CKeyInputState *pState = (CKeyInputState *)pUserData;
	if(pState->m_pCombat->GameClient()->m_GameInfo.m_BugDDRaceInput && pState->m_pCombat->GameClient()->m_Snap.m_SpecInfo.m_Active)
		return;
	*pState->m_apVariables[g_Config.m_ClDummy] = pResult->GetInteger(0);
}

void CC14Combat::OnConsoleInit()
{
	static CKeyInputState s_AutoFireState = {this, {&m_aAutoFireKeyDown[0], &m_aAutoFireKeyDown[1]}};
	Console()->Register("+auto_fire", "", CFGFLAG_CLIENT, ConKeyInputState, &s_AutoFireState, "Auto fire (hold to keep firing)");
}

void CC14Combat::Apply(CNetObj_PlayerInput *pInput, C14::CInputLocks &Locks)
{
	if(!GameClient()->m_Snap.m_pLocalCharacter)
		return;

	CNetObj_PlayerInput *pLast = &GameClient()->m_Controls.m_aLastData[g_Config.m_ClDummy];

	// CLIENT 14 Auto Fire
	if(m_aAutoFireKeyDown[g_Config.m_ClDummy])
	{
		m_aAutoFireTimer[g_Config.m_ClDummy]++;
		if(m_aAutoFireTimer[g_Config.m_ClDummy] >= 1)
		{
			m_aAutoFireTimer[g_Config.m_ClDummy] = 0;
			if(C14::CanClaim(Locks.m_FireOwner, C14::PRIO_COMBAT))
			{
				pInput->m_Fire++;
				Locks.m_FireOwner = C14::PRIO_COMBAT;
			}
		}
	}
	else
	{
		m_aAutoFireTimer[g_Config.m_ClDummy] = 0;
	}

	// CLIENT 14 Freeze Unfreeze - hammer frozen teammates
	// CLIENT 14 Hammer Bot - hammer nearby enemies
	if((g_Config.m_ClFreezeUnfreeze || g_Config.m_ClHammerBot) && GameClient()->m_Snap.m_pLocalCharacter)
	{
		int LocalId = GameClient()->m_Snap.m_LocalClientId;
		// Reload cooldown from tuning (hammer_hit_fire_delay, ms -> ticks at
		// SERVER_TICK_SPEED=50). Replaces a hardcoded "> 10" tick gate.
		const int FireDelayMs = (int)GameClient()->m_aTuning[g_Config.m_ClDummy].m_HammerHitFireDelay;
		const int ReloadTicks = (FireDelayMs * SERVER_TICK_SPEED + 999) / 1000;
		// Engage range: physically grounded in the tee size (the engine hits
		// at proximity*0.5), overridable via cl_hammer_bot_range (0 = auto).
		const float HammerRange = g_Config.m_ClHammerBotRange > 0
			? (float)g_Config.m_ClHammerBotRange
			: CCharacterCore::PhysicalSize() * 2.0f;
		if(LocalId >= 0 && GameClient()->m_Snap.m_pLocalCharacter->m_Weapon == WEAPON_HAMMER &&
			Client()->GameTick(0) - m_aLastHammerTick[g_Config.m_ClDummy] > ReloadTicks)
		{
			int MyTeam = GameClient()->m_aClients[LocalId].m_Team;
			vec2 MyPos = GameClient()->m_PredictedChar.m_Pos;
			bool DidHammer = false;

			if(g_Config.m_ClFreezeUnfreeze)
			{
				int BestId = -1;
				float BestDist = 999999.0f;
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(i == LocalId || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
						continue;
					if(GameClient()->m_aClients[i].m_Team == TEAM_SPECTATORS)
						continue;
					if(GameClient()->m_aClients[i].m_Team != MyTeam)
						continue;
					if(GameClient()->m_aClients[i].m_FreezeEnd <= Client()->GameTick(0))
						continue;
					vec2 TPos = GameClient()->m_aClients[i].m_Predicted.m_Pos;
					float Dist = distance(MyPos, TPos);
					if(Dist < BestDist && Dist < HammerRange)
					{
						BestDist = Dist;
						BestId = i;
					}
				}
				if(BestId >= 0 && C14::CanClaim(Locks.m_TargetOwner, C14::PRIO_COMBAT))
				{
					vec2 HammerDir = GameClient()->m_aClients[BestId].m_Predicted.m_Pos - MyPos;
					pInput->m_TargetX = (int)HammerDir.x;
					pInput->m_TargetY = (int)HammerDir.y;
					pInput->m_Fire = pLast->m_Fire + 1;
					Locks.m_TargetOwner = C14::PRIO_COMBAT;
					Locks.m_FireOwner = C14::PRIO_COMBAT;
					m_aLastHammerTick[g_Config.m_ClDummy] = Client()->GameTick(0);
					DidHammer = true;
				}
			}

			if(!DidHammer && g_Config.m_ClHammerBot)
			{
				int BestId = -1;
				float BestDist = 999999.0f;
				for(int i = 0; i < MAX_CLIENTS; i++)
				{
					if(i == LocalId || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
						continue;
					if(GameClient()->m_aClients[i].m_Team == TEAM_SPECTATORS)
						continue;
					if(GameClient()->m_aClients[i].m_Team == MyTeam)
						continue;
					vec2 TPos = GameClient()->m_aClients[i].m_Predicted.m_Pos;
					float Dist = distance(MyPos, TPos);
					if(Dist < BestDist && Dist < HammerRange)
					{
						BestDist = Dist;
						BestId = i;
					}
				}
				if(BestId >= 0 && C14::CanClaim(Locks.m_TargetOwner, C14::PRIO_COMBAT))
				{
					vec2 HammerDir = GameClient()->m_aClients[BestId].m_Predicted.m_Pos - MyPos;
					pInput->m_TargetX = (int)HammerDir.x;
					pInput->m_TargetY = (int)HammerDir.y;
					pInput->m_Fire = pLast->m_Fire + 1;
					Locks.m_TargetOwner = C14::PRIO_COMBAT;
					Locks.m_FireOwner = C14::PRIO_COMBAT;
					m_aLastHammerTick[g_Config.m_ClDummy] = Client()->GameTick(0);
				}
			}
		}
	}
}
