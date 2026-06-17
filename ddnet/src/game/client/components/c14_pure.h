/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_C14_PURE_H
#define GAME_CLIENT_COMPONENTS_C14_PURE_H

#include <base/vmath.h>

#include <game/gamecore.h>

namespace C14
{
	// Pure (dependency-free) helpers used by the CLIENT 14 bot modules.
	// Kept in a separate header so they can be unit-tested without pulling in
	// the full client (gameclient.h), which is not available to the testrunner.

	// DDNet freeze tile indices (game/front layer): TILE_FREEZE and friends.
	inline bool IsFreezeTile(int t)
	{
		return t == 9 || t == 12 || t == 144;
	}

	// Position extrapolation matching the engine's per-tick integration
	// (horizontal: linear in velocity, vertical: linear + cumulative gravity
	// of 0.5/tick). Used by the balance bot to predict the target's future X.
	inline vec2 PredictPosition(const CCharacterCore &Source, int Ticks)
	{
		vec2 P = Source.m_Pos;
		P.x += Source.m_Vel.x * Ticks;
		P.y += Source.m_Vel.y * Ticks + 0.5f * 0.5f * (float)(Ticks * (Ticks - 1));
		return P;
	}
} // namespace C14

#endif
