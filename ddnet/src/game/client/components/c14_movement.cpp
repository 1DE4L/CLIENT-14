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
}

void CC14Movement::OnReset()
{
	mem_zero(m_aWasMoving, sizeof(m_aWasMoving));
	mem_zero(m_aQuickStopTicks, sizeof(m_aQuickStopTicks));
}

void CC14Movement::Apply(CNetObj_PlayerInput *pInput)
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
				m_aQuickStopTicks[g_Config.m_ClDummy] = 6;
			}
			if(m_aQuickStopTicks[g_Config.m_ClDummy] > 0)
			{
				m_aQuickStopTicks[g_Config.m_ClDummy]--;

				float AbsVel = absolute(VelX);
				if(AbsVel > 0.1f)
				{
					float Strength = std::clamp(AbsVel / 5.0f, 0.0f, 1.0f);
					if(Strength > 0.3f)
						pInput->m_Direction = VelX > 0 ? -1 : 1;
				}
			}
			if(GameClient()->m_Snap.m_pLocalCharacter && GameClient()->m_Snap.m_pLocalCharacter->m_HookState == 2)
			{
				vec2 HookPos = vec2(GameClient()->m_Snap.m_pLocalCharacter->m_HookX, GameClient()->m_Snap.m_pLocalCharacter->m_HookY);
				vec2 HookDir = HookPos - Pos;
				float HookLen = length(HookDir);
				float AbsVelX = absolute(VelX);

				if(HookLen > 1.0f && absolute(HookDir.x) > 0.3f * HookLen)
				{
					if(AbsVelX > 0.3f)
						pInput->m_Direction = VelX > 0 ? -1 : 1;
				}
				else if(AbsVelX > 0.3f)
				{
					pInput->m_Direction = VelX > 0 ? -1 : 1;
				}
				m_aQuickStopTicks[g_Config.m_ClDummy] = maximum(m_aQuickStopTicks[g_Config.m_ClDummy], 4);

				if(VelY < -3.0f && OnGround)
					pInput->m_Direction = VelX > 0 ? -1 : 1;
			}
		}
	}
}
