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

CC14FreezeHelper::CC14FreezeHelper() = default;

void CC14FreezeHelper::OnReset()
{
	m_LastArcSimTick = -100;
	m_CachedJumpArcSafe = true;
}

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
				// Only hook non-frozen players: a frozen tee cannot pull us
				// out, hooking them is useless and wastes the hook cooldown.
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

	// CLIENT 14 Avoid Freeze - predictive physics simulation (every tick)
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

		float DefaultMinDist = 999999.0f;
		int DefaultSurvival = C14::SimulateStrategy(GameClient(), LocalChar, CurDir, -1, CurJump, N, DefaultMinDist);

		if(DefaultSurvival < N)
		{
			int BestSurvival = DefaultSurvival;
			float BestMinDist = DefaultMinDist;
			int BestDir = CurDir;

			for(int d : {-1, 0, 1})
			{
				if(d == CurDir)
					continue;

				float MinDist = 999999.0f;
				int Survival = C14::SimulateStrategy(GameClient(), LocalChar, d, -1, CurJump, N, MinDist);

				if(Survival > BestSurvival || (Survival == BestSurvival && MinDist > BestMinDist))
				{
					BestSurvival = Survival;
					BestMinDist = MinDist;
					BestDir = d;
				}
			}

			if(C14::CanClaim(Locks.m_DirectionOwner, C14::PRIO_FREEZE))
			{
				pInput->m_Direction = BestDir;
				Locks.m_DirectionOwner = C14::PRIO_FREEZE;
			}
		}
	}

	// CLIENT 14 Auto Jump Save
	if(g_Config.m_ClAutoJumpSave)
	{
		vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
		vec2 Vel = GameClient()->m_PredictedChar.m_Vel;
		CCollision *pCol = GameClient()->Collision();
		bool OnGround = C14::Grounded(pCol, Pos);
		int CurDir = pInput->m_Direction;
		float VelX = Vel.x;

		int JumpDir = CurDir;
		if(JumpDir == 0)
		{
			if(VelX > 0.05f) JumpDir = 1;
			else if(VelX < -0.05f) JumpDir = -1;
		}
		if(JumpDir == 0) JumpDir = 1;

		bool fB = C14::HasFreeze(pCol, Pos.x, Pos.y + 18.0f) ||
			  C14::HasFreeze(pCol, Pos.x - 10.0f, Pos.y + 18.0f) ||
			  C14::HasFreeze(pCol, Pos.x + 10.0f, Pos.y + 18.0f);

		float Reach = 32.0f;

		float h[5] = {Pos.y + 14.0f, Pos.y + 4.0f, Pos.y - 4.0f, Pos.y - 12.0f, Pos.y - 20.0f};
		bool fF = false;
		if(JumpDir != 0)
		{
			for(int hi = 0; hi < 5; hi++)
			{
				if(C14::HasFreeze(pCol, Pos.x + JumpDir * Reach, h[hi]))
				{
					fF = true;
					break;
				}
			}
		}

		bool fA = C14::HasFreeze(pCol, Pos.x, Pos.y - 16.0f);

		bool NeedJump = fB || fF;

		bool HeadClear = !pCol->CheckPoint(Pos.x, Pos.y - C14::PHYS_SIZE - 4);

		bool LandingSafe = true;
		if(NeedJump)
		{
			LandingSafe = !C14::HasFreeze(pCol, Pos.x + JumpDir * 32.0f - 8.0f, Pos.y + 50.0f)
				  && !C14::HasFreeze(pCol, Pos.x + JumpDir * 32.0f + 8.0f, Pos.y + 50.0f)
				  && !C14::HasFreeze(pCol, Pos.x + JumpDir * 56.0f - 8.0f, Pos.y + 70.0f)
				  && !C14::HasFreeze(pCol, Pos.x + JumpDir * 56.0f + 8.0f, Pos.y + 70.0f);
		}

		bool DiagonalSafe = true;
		if(NeedJump)
		{
			float diagX = JumpDir * 32.0f;
			float diagY = 32.0f;
			if(C14::HasFreeze(pCol, Pos.x + diagX, Pos.y + diagY) ||
			   C14::HasFreeze(pCol, Pos.x + diagX, Pos.y - diagY))
			{
				DiagonalSafe = false;
			}
		}

		// Throttled jump-arc simulation: rerun every m_ArcSimInterval ticks,
		// reuse the cached verdict in between (the threat landscape does not
		// change much within a few ticks while grounded).
		if(NeedJump && OnGround && (Client()->GameTick(0) - m_LastArcSimTick >= m_ArcSimInterval || Client()->GameTick(0) < m_LastArcSimTick))
		{
			m_LastArcSimTick = Client()->GameTick(0);
			vec2 SimPos = Pos;
			vec2 SimVel = Vel;
			SimVel.y = -13.2f;

			m_CachedJumpArcSafe = true;
			for(int t = 0; t < 35; t++)
			{
				SimVel.y += 0.5f;
				SimPos += SimVel;

				if(C14::HasFreeze(pCol, SimPos.x, SimPos.y) ||
				   C14::HasFreeze(pCol, SimPos.x + 6.0f, SimPos.y) ||
				   C14::HasFreeze(pCol, SimPos.x - 6.0f, SimPos.y) ||
				   C14::HasFreeze(pCol, SimPos.x, SimPos.y + 10.0f) ||
				   C14::HasFreeze(pCol, SimPos.x, SimPos.y - 10.0f))
				{
					m_CachedJumpArcSafe = false;
					break;
				}

				if(C14::Grounded(pCol, SimPos))
					break;
			}
		}
		else if(!NeedJump || !OnGround)
		{
			// Reset the cache when there is no pending jump so a stale "unsafe"
			// verdict does not block a future jump after the situation clears.
			m_CachedJumpArcSafe = true;
			m_LastArcSimTick = Client()->GameTick(0);
		}
		bool JumpArcSafe = m_CachedJumpArcSafe;

		bool PlayerPressingJump = (pInput->m_Jump & 1) != 0;

		if(!PlayerPressingJump)
		{
			if(NeedJump && OnGround && !fA && HeadClear && LandingSafe && JumpArcSafe && DiagonalSafe &&
				C14::CanClaim(Locks.m_JumpOwner, C14::PRIO_FREEZE))
			{
				pInput->m_Jump |= 1;
				Locks.m_JumpOwner = C14::PRIO_FREEZE;
			}
			if(fA && !OnGround && C14::CanClaim(Locks.m_JumpOwner, C14::PRIO_FREEZE))
			{
				pInput->m_Jump &= ~1;
				Locks.m_JumpOwner = C14::PRIO_FREEZE;
			}
		}

		if(!JumpArcSafe || !LandingSafe || !DiagonalSafe)
		{
			if(fF && !fB && C14::CanClaim(Locks.m_DirectionOwner, C14::PRIO_FREEZE))
			{
				int RevDir = -CurDir;
				if(RevDir != 0)
				{
					bool RevSafe = !C14::HasFreeze(pCol, Pos.x + RevDir * 32.0f, Pos.y);
					if(RevSafe)
						pInput->m_Direction = RevDir;
					else
						pInput->m_Direction = 0;
				}
				else
				{
					pInput->m_Direction = 0;
				}
				Locks.m_DirectionOwner = C14::PRIO_FREEZE;
			}
		}
	}
}
