/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "c14_helpers.h"

namespace C14
{

int SimulateStrategy(CGameClient *pGC, const CCharacterCore &Source,
	int SimDir, int SimHook, int SimJump, int N, float &OutMinDistToFreeze)
{
	CCollision *pCol = pGC->Collision();
	CWorldCore World;
	CCharacterCore Core;

	Core = Source;
	Core.Init(&World, pCol, nullptr);

	if(!pGC->m_Snap.m_pLocalCharacter)
	{
		OutMinDistToFreeze = 999999.0f;
		return N;
	}

	int LocalId = pGC->m_Snap.m_LocalClientId;
	if(LocalId >= 0 && LocalId < MAX_CLIENTS)
		World.m_apCharacters[LocalId] = &Core;
	Core.m_Id = LocalId;

	Core.m_Input.m_Direction = SimDir;
	if(SimHook == 0)
	{
		Core.m_Input.m_Hook = 0;
		Core.m_HookState = HOOK_IDLE;
	}

	if(SimJump == 1)
		Core.m_Input.m_Jump = 1;
	else
		Core.m_Input.m_Jump = 0;

	float MinDist = 999999.0f;
	int SurvivalTicks = N;

	for(int t = 0; t < N; t++)
	{
		if(t > 0 && SimJump == 1)
			Core.m_Input.m_Jump = 0;

		Core.Tick(true, false);
		Core.Move();

		vec2 P = Core.m_Pos;
		float Half = PHYS_SIZE / 2.0f;

		bool HitFreeze = HasFreeze(pCol, P.x - Half + 1, P.y) ||
				 HasFreeze(pCol, P.x + Half - 1, P.y) ||
				 HasFreeze(pCol, P.x, P.y - Half + 1) ||
				 HasFreeze(pCol, P.x, P.y + Half - 1) ||
				 HasFreeze(pCol, P.x, P.y);

		if(HitFreeze)
		{
			SurvivalTicks = t;
			MinDist = 0.0f;
			break;
		}

		int TileX = round_to_int(P.x) / 32;
		int TileY = round_to_int(P.y) / 32;
		for(int dx = -2; dx <= 2; dx++)
		{
			for(int dy = -2; dy <= 2; dy++)
			{
				float TargetX = (TileX + dx) * 32.0f + 16.0f;
				float TargetY = (TileY + dy) * 32.0f + 16.0f;
				if(HasFreeze(pCol, TargetX, TargetY))
				{
					float Dist = distance(P, vec2(TargetX, TargetY)) - Half - 16.0f;
					if(Dist < 0.0f)
						Dist = 0.0f;
					if(Dist < MinDist)
						MinDist = Dist;
				}
			}
		}
	}

	OutMinDistToFreeze = MinDist;
	return SurvivalTicks;
}

} // namespace C14
