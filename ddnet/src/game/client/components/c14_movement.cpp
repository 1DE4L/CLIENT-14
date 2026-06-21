/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "c14_movement.h"

#include <base/math.h>
#include <base/vmath.h>

#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/client/components/controls.h>
#include <game/client/gameclient.h>

#include "c14_helpers.h"

CC14Movement::CC14Movement()
{
	mem_zero(m_aWasMoving, sizeof(m_aWasMoving));
	mem_zero(m_aQuickStopTicks, sizeof(m_aQuickStopTicks));
	mem_zero(m_aChainBrakeTicks, sizeof(m_aChainBrakeTicks));
}

void CC14Movement::OnReset()
{
	mem_zero(m_aWasMoving, sizeof(m_aWasMoving));
	mem_zero(m_aQuickStopTicks, sizeof(m_aQuickStopTicks));
	mem_zero(m_aChainBrakeTicks, sizeof(m_aChainBrakeTicks));
}

void CC14Movement::Apply(CNetObj_PlayerInput *pInput, C14::CInputLocks &Locks)
{
	if(!C14::BotCanRun(GameClient()))
		return;

	if(g_Config.m_ClQuickStop)
	{
		float VelX = GameClient()->m_PredictedChar.m_Vel.x;
		float VelY = GameClient()->m_PredictedChar.m_Vel.y;
		vec2 Pos = GameClient()->m_PredictedChar.m_Pos;
		bool OnGround = C14::Grounded(GameClient()->Collision(), Pos);
		if(!g_Config.m_ClQuickStopGround || OnGround)
		{
			bool IsMoving = (GameClient()->m_Controls.m_aInputDirectionLeft[g_Config.m_ClDummy] || GameClient()->m_Controls.m_aInputDirectionRight[g_Config.m_ClDummy]);
			if(IsMoving)
				m_aWasMoving[g_Config.m_ClDummy] = true;
			if(!IsMoving && m_aWasMoving[g_Config.m_ClDummy])
			{
				m_aWasMoving[g_Config.m_ClDummy] = false;
				// Scale the brake duration with the current horizontal speed:
				// faster tee -> longer counter-input. Clamped to [3, 12] ticks.
				float AbsVelAtRelease = absolute(VelX);
				int Ticks = (int)round(AbsVelAtRelease / 3.0f);
				m_aQuickStopTicks[g_Config.m_ClDummy] = std::clamp(Ticks, 3, 12);
			}
			if(m_aQuickStopTicks[g_Config.m_ClDummy] > 0)
			{
				m_aQuickStopTicks[g_Config.m_ClDummy]--;

				float AbsVel = absolute(VelX);
				// Strength gates the brake: only counter-steer when moving
				// fast enough that a hard brake is meaningful. Keeping the
				// threshold high avoids the left/right jitter and the
				// micro-braking that disturbed PvP aim/movement.
				float Strength = std::clamp(AbsVel / 5.0f, 0.0f, 1.0f);
				if(Strength > 0.5f && C14::CanClaim(Locks.m_DirectionOwner, C14::PRIO_MOVEMENT))
				{
					pInput->m_Direction = VelX > 0 ? -1 : 1;
					Locks.m_DirectionOwner = C14::PRIO_MOVEMENT;
				}
			}
			if(GameClient()->m_Snap.m_pLocalCharacter && GameClient()->m_Snap.m_pLocalCharacter->m_HookState == 2)
			{
				vec2 HookPos = vec2(GameClient()->m_Snap.m_pLocalCharacter->m_HookX, GameClient()->m_Snap.m_pLocalCharacter->m_HookY);
				vec2 HookDir = HookPos - Pos;
				float HookLen = length(HookDir);
				float AbsVelX = absolute(VelX);

				// Chain-hook aware braking:
				//  - Only counter-steer horizontally when the hook has a
				//    meaningful horizontal component (we are being pulled
				//    sideways). A purely vertical hook (swing) should not have
				//    its X momentum killed.
				//  - Stop braking once we are close to the anchor, so the tee
				//    does not overshoot and oscillate back-and-forth.
				//  - Keep the brake duration speed-scaled like the ground
				//    quick-stop, instead of forcing >= 4 ticks every frame.
				const float HookHorizRatio = (HookLen > 1.0f) ? absolute(HookDir.x) / HookLen : 0.0f;
				const bool HookIsHorizontal = HookHorizRatio > 0.5f;
				const bool NearAnchor = HookLen < CCharacterCore::PhysicalSize() * 1.5f;

				if(C14::CanClaim(Locks.m_DirectionOwner, C14::PRIO_MOVEMENT))
				{
					if(HookIsHorizontal && !NearAnchor && AbsVelX > 0.3f)
					{
						pInput->m_Direction = VelX > 0 ? -1 : 1;
						Locks.m_DirectionOwner = C14::PRIO_MOVEMENT;
						if(m_aChainBrakeTicks[g_Config.m_ClDummy] <= 0)
						{
							int Ticks = (int)round(AbsVelX / 3.0f);
							m_aChainBrakeTicks[g_Config.m_ClDummy] = std::clamp(Ticks, 2, 8);
						}
					}
					else if(VelY < -3.0f && OnGround)
					{
						pInput->m_Direction = VelX > 0 ? -1 : 1;
						Locks.m_DirectionOwner = C14::PRIO_MOVEMENT;
					}
				}

				if(m_aChainBrakeTicks[g_Config.m_ClDummy] > 0)
					m_aChainBrakeTicks[g_Config.m_ClDummy]--;
			}
			else
			{
				// Hook released/idle: clear chain-brake state so a future hook
				// starts with a fresh brake budget.
				m_aChainBrakeTicks[g_Config.m_ClDummy] = 0;
			}
		}
	}
}
