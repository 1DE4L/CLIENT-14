/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "c14_freeze.h"

#include <base/math.h>
#include <base/vmath.h>

#include <engine/client.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/gamecore.h>
#include <game/client/components/controls.h>
#include <game/client/gameclient.h>

#include "c14_helpers.h"

void CC14FreezeHelper::Apply(CNetObj_PlayerInput *pInput, C14::CInputLocks &Locks)
{
	if(!C14::BotCanRun(GameClient()))
		return;

	// CLIENT 14 Anti-Freeze
	if(g_Config.m_ClAntiFreeze && GameClient()->m_Snap.m_pLocalCharacter)
	{
		int LocalId = GameClient()->m_Snap.m_LocalClientId;
		if(LocalId >= 0 && GameClient()->m_aClients[LocalId].m_FreezeEnd > Client()->GameTick(0))
		{
			vec2 LocalPos = GameClient()->m_LocalCharacterPos;
			int BestId = -1;
			float BestDist = 999999.0f;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(i == LocalId || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
					continue;
				if(GameClient()->m_aClients[i].m_Team == TEAM_SPECTATORS)
					continue;
				if(GameClient()->m_aClients[i].m_FreezeEnd > Client()->GameTick(0))
					continue;
				vec2 TPos = vec2(GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_X, GameClient()->m_Snap.m_aCharacters[i].m_Cur.m_Y);
				float Dist = distance(LocalPos, TPos);
				if(Dist < BestDist && Dist > 0.1f)
				{
					BestDist = Dist;
					BestId = i;
				}
			}
			if(BestId >= 0 && C14::CanClaim(Locks.m_HookOwner, C14::PRIO_FREEZE))
			{
				vec2 HookVec = vec2(GameClient()->m_Snap.m_aCharacters[BestId].m_Cur.m_X, GameClient()->m_Snap.m_aCharacters[BestId].m_Cur.m_Y) - LocalPos;
				pInput->m_Hook = 1;
				pInput->m_TargetX = (int)HookVec.x;
				pInput->m_TargetY = (int)HookVec.y;
				Locks.m_HookOwner = C14::PRIO_FREEZE;
				Locks.m_TargetOwner = C14::PRIO_FREEZE;
			}
		}
	}

	// CLIENT 14 Avoid Freeze - graduated response (gentle, not aggressive)
	//
	// Old approach: simulate all 3 directions, pick max survival. This was too
	// aggressive — a 1-tick improvement triggered a full direction reversal,
	// shoving the player around even in non-critical situations.
	//
	// New approach: graduated response with preference order:
	//   1. If current direction is safe (survival >= N): do NOTHING.
	//   2. If current direction will hit freeze: try STOPPING first (minimal
	//      disruption). Only reverse if stopping also fails AND reversing is
	//      significantly better (margin >= 2 ticks).
	//
	// This separates concerns: Avoid Freeze handles horizontal threats (walls
	// of freeze ahead), Auto Jump Save handles vertical threats (freeze below
	// that you need to jump over).
	if(g_Config.m_ClAvoidFreeze && GameClient()->m_Snap.m_pLocalCharacter)
	{
		const CCharacterCore &LocalChar = GameClient()->m_PredictedChar;
		int CurDir = pInput->m_Direction;
		int CurJump = pInput->m_Jump;

		float Speed = absolute(LocalChar.m_Vel.x);
		int N = 4;
		if(absolute(LocalChar.m_Vel.x) > 0.05f || absolute(LocalChar.m_Vel.y) > 0.05f)
		{
			if(Speed > 15.0f) N = 10;
			else if(Speed > 10.0f) N = 8;
			else if(Speed > 5.0f) N = 6;
			else N = 5;
		}

		// Step 1: Is the current direction safe?
		float MinDistCur = 999999.0f;
		int SurvivalCur = C14::SimulateStrategy(GameClient(), LocalChar, CurDir, -1, CurJump, N, MinDistCur);

		if(SurvivalCur < N)
		{
			// Current direction will hit freeze. Try graduated alternatives.

			// Step 2: Try stopping (minimal intervention).
			float MinDistStop = 999999.0f;
			int SurvivalStop = C14::SimulateStrategy(GameClient(), LocalChar, 0, -1, CurJump, N, MinDistStop);

			int BestDir = CurDir;
			int BestSurvival = SurvivalCur;

			if(SurvivalStop >= N)
			{
				// Stopping is safe — just stop, don't reverse.
				BestDir = 0;
				BestSurvival = SurvivalStop;
			}
			else
			{
				// Stopping isn't enough. Try the opposite direction.
				BestSurvival = SurvivalStop;
				BestDir = 0;

				if(CurDir != 0)
				{
					int OppDir = -CurDir;
					float MinDistOpp = 999999.0f;
					int SurvivalOpp = C14::SimulateStrategy(GameClient(), LocalChar, OppDir, -1, CurJump, N, MinDistOpp);
					// Only reverse if it is SIGNIFICANTLY better (margin >= 2).
					// This prevents jittery direction flipping for tiny gains.
					if(SurvivalOpp > BestSurvival + 1)
					{
						BestDir = OppDir;
						BestSurvival = SurvivalOpp;
					}
				}
				else
				{
					// Standing still and still hitting freeze — try both dirs.
					for(int d : {-1, 1})
					{
						float MinDistD = 999999.0f;
						int SurvivalD = C14::SimulateStrategy(GameClient(), LocalChar, d, -1, CurJump, N, MinDistD);
						if(SurvivalD > BestSurvival + 1)
						{
							BestDir = d;
							BestSurvival = SurvivalD;
						}
					}
				}
			}

			if(BestDir != CurDir && C14::CanClaim(Locks.m_DirectionOwner, C14::PRIO_FREEZE))
			{
				pInput->m_Direction = BestDir;
				Locks.m_DirectionOwner = C14::PRIO_FREEZE;
			}
		}
	}

	// CLIENT 14 Auto Jump Save - Input Gate state machine
	//
	// Bot zıplama sinyalini sadece tek bir frame için gönderir, hemen ardından
	// hattı temizleyerek oyuncunun klavye senkronizasyonunu korur.
	//
	// State machine:
	//   0: Boşta — bot aktif, çarpışma bekliyor
	//   1: Bot zıpladı — bu frame'de m_Jump=1 gönderildi
	//   2: Tahliye — m_Jump=0 yap, oyuncunun klavyesine nefes aldır
	//   (sonra 0'a dön)
	if(g_Config.m_ClAutoJumpSave && GameClient()->m_Snap.m_pLocalCharacter)
	{
		static int s_BotJumpState[NUM_DUMMIES] = {0, 0};
		const int Dummy = g_Config.m_ClDummy;
		const CCharacterCore &Src = GameClient()->m_PredictedChar;
		bool HasAirJump = !(Src.m_Jumped & 2);

		if(s_BotJumpState[Dummy] == 1)
		{
			// Bot zıpladı — hattı zorla temizle
			pInput->m_Jump = 0;
			s_BotJumpState[Dummy] = 2;
			Locks.m_JumpOwner = C14::PRIO_FREEZE;
		}
		else if(s_BotJumpState[Dummy] == 2)
		{
			// Tahliye bitti — kontrolü oyuncuya iade et
			s_BotJumpState[Dummy] = 0;
		}
		else if(HasAirJump && s_BotJumpState[Dummy] == 0)
		{
			int CurDir = pInput->m_Direction;

			// Çarpmaya tam 1 frame kala
			int TicksToFreeze = C14::SimulateToFreezeTick(GameClient(), Src, CurDir, 15);

			if(TicksToFreeze == 1)
			{
				// Oyuncu o an zıplamıyorsa bot devreye girsin
				if((pInput->m_Jump & 1) == 0 &&
					C14::CanClaim(Locks.m_JumpOwner, C14::PRIO_FREEZE))
				{
					pInput->m_Jump = 1;
					s_BotJumpState[Dummy] = 1;
					Locks.m_JumpOwner = C14::PRIO_FREEZE;
				}
			}
		}
	}
}
