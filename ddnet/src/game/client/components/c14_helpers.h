/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_C14_HELPERS_H
#define GAME_CLIENT_COMPONENTS_C14_HELPERS_H

#include <base/math.h>
#include <base/vmath.h>

#include <algorithm>

#include <game/collision.h>
#include <game/gamecore.h>
#include <game/client/gameclient.h>

#include "c14_pure.h"

namespace C14
{

static constexpr float PHYS_SIZE = 28.0f;

inline int GetTile(CCollision *pCol, float x, float y)
{
	if(!pCol->GameLayer())
		return 0;
	int Nx = std::clamp(round_to_int(x) / 32, 0, pCol->GetWidth() - 1);
	int Ny = std::clamp(round_to_int(y) / 32, 0, pCol->GetHeight() - 1);
	int idx = Ny * pCol->GetWidth() + Nx;
	int tile = pCol->GameLayer()[idx].m_Index;
	if(tile != 0)
		return tile;
	if(pCol->FrontLayer())
		return pCol->FrontLayer()[idx].m_Index;
	return 0;
}

inline bool HasFreeze(CCollision *pCol, float x, float y)
{
	return IsFreezeTile(GetTile(pCol, x, y));
}

inline bool Grounded(CCollision *pCol, vec2 pos)
{
	if(pCol->CheckPoint(pos.x + PHYS_SIZE / 2, pos.y + PHYS_SIZE / 2 + 5))
		return true;
	if(pCol->CheckPoint(pos.x - PHYS_SIZE / 2, pos.y + PHYS_SIZE / 2 + 5))
		return true;
	int r = pCol->GetMoveRestrictions(pos + vec2(0, PHYS_SIZE / 2 + 4), 0.0f);
	if(r & CANTMOVE_DOWN)
		return true;
	return false;
}

inline bool BotCanRun(const CGameClient *pGC)
{
	if(pGC->m_Snap.m_SpecInfo.m_Active)
		return false;
	if(pGC->IsWorldPaused())
		return false;
	if(!pGC->m_Snap.m_pLocalCharacter)
		return false;
	return true;
}

	int SimulateStrategy(CGameClient *pGC, const CCharacterCore &Source,
		int SimDir, int SimHook, int SimJump, int N, float &OutMinDistToFreeze);

} // namespace C14

#endif
