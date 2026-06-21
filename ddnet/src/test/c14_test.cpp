#include "test.h"

#include <game/client/components/c14_input.h>
#include <game/client/components/c14_pure.h>

#include <gtest/gtest.h>

TEST(C14, IsFreezeTile)
{
	// Known DDNet freeze tile indices.
	EXPECT_TRUE(C14::IsFreezeTile(9));
	EXPECT_TRUE(C14::IsFreezeTile(12));
	EXPECT_TRUE(C14::IsFreezeTile(144));

	// Common non-freeze tiles: air, solids, etc.
	EXPECT_FALSE(C14::IsFreezeTile(0));
	EXPECT_FALSE(C14::IsFreezeTile(1));
	EXPECT_FALSE(C14::IsFreezeTile(2));
	EXPECT_FALSE(C14::IsFreezeTile(5));
	EXPECT_FALSE(C14::IsFreezeTile(100));
	EXPECT_FALSE(C14::IsFreezeTile(-1));
}

TEST(C14, PredictPositionStationary)
{
	CCharacterCore Core;
	Core.m_Pos = vec2(100.0f, 200.0f);
	Core.m_Vel = vec2(0.0f, 0.0f);

	// No velocity and gravity pulling down only acts on y; with vel.y=0 the
	// gravity term is 0.25*Ticks*(Ticks-1), so a stationary tee still drifts
	// down due to gravity in this open-loop predictor.
	for(int Ticks = 0; Ticks <= 10; ++Ticks)
	{
		vec2 P = C14::PredictPosition(Core, Ticks);
		EXPECT_FLOAT_EQ(P.x, 100.0f);
		const float ExpectedY = 200.0f + 0.25f * (float)(Ticks * (Ticks - 1));
		EXPECT_FLOAT_EQ(P.y, ExpectedY);
	}
}

TEST(C14, PredictPositionHorizontal)
{
	CCharacterCore Core;
	Core.m_Pos = vec2(0.0f, 0.0f);
	Core.m_Vel = vec2(5.0f, 0.0f);

	vec2 P = C14::PredictPosition(Core, 6);
	EXPECT_FLOAT_EQ(P.x, 30.0f);
	// vel.y = 0, gravity term = 0.25 * 6 * 5 = 7.5
	EXPECT_FLOAT_EQ(P.y, 7.5f);
}

TEST(C14, PredictPositionFull)
{
	CCharacterCore Core;
	Core.m_Pos = vec2(100.0f, 200.0f);
	Core.m_Vel = vec2(5.0f, -10.0f);

	vec2 P = C14::PredictPosition(Core, 6);
	EXPECT_FLOAT_EQ(P.x, 130.0f); // 100 + 5*6
	// 200 + (-10)*6 + 0.25*6*5 = 200 - 60 + 7.5 = 147.5
	EXPECT_FLOAT_EQ(P.y, 147.5f);
}

TEST(C14, PredictPositionZeroTicks)
{
	CCharacterCore Core;
	Core.m_Pos = vec2(42.0f, -7.0f);
	Core.m_Vel = vec2(99.0f, 99.0f);

	// Ticks = 0 must return the current position unchanged.
	vec2 P = C14::PredictPosition(Core, 0);
	EXPECT_FLOAT_EQ(P.x, 42.0f);
	EXPECT_FLOAT_EQ(P.y, -7.0f);
}

TEST(C14, PredictPositionOneTick)
{
	CCharacterCore Core;
	Core.m_Pos = vec2(0.0f, 0.0f);
	Core.m_Vel = vec2(4.0f, -3.0f);

	// Ticks = 1: gravity term = 0.25 * 1 * 0 = 0, so pure velocity step.
	vec2 P = C14::PredictPosition(Core, 1);
	EXPECT_FLOAT_EQ(P.x, 4.0f);
	EXPECT_FLOAT_EQ(P.y, -3.0f);
}

TEST(C14, InputPriorityCanClaim)
{
	// A bot may only write a field if its priority strictly exceeds the
	// field's current owner. This is what makes input arbitration
	// order-independent.
	EXPECT_TRUE(C14::CanClaim(C14::PRIO_NONE, C14::PRIO_MOVEMENT));
	EXPECT_TRUE(C14::CanClaim(C14::PRIO_MOVEMENT, C14::PRIO_BALANCE));
	EXPECT_TRUE(C14::CanClaim(C14::PRIO_BALANCE, C14::PRIO_AIMBOT));
	EXPECT_TRUE(C14::CanClaim(C14::PRIO_AIMBOT, C14::PRIO_COMBAT));
	EXPECT_TRUE(C14::CanClaim(C14::PRIO_COMBAT, C14::PRIO_FREEZE));

	// Equal priority must NOT override (avoids two same-tier bots fighting).
	EXPECT_FALSE(C14::CanClaim(C14::PRIO_FREEZE, C14::PRIO_FREEZE));
	EXPECT_FALSE(C14::CanClaim(C14::PRIO_BALANCE, C14::PRIO_BALANCE));

	// Lower priority must never override a higher one.
	EXPECT_FALSE(C14::CanClaim(C14::PRIO_FREEZE, C14::PRIO_MOVEMENT));
	EXPECT_FALSE(C14::CanClaim(C14::PRIO_COMBAT, C14::PRIO_BALANCE));
}

TEST(C14, InputPriorityOrdering)
{
	// The documented survival hierarchy: freeze (stay alive) beats combat
	// beats aimbot beats balance beats movement.
	EXPECT_GT(C14::PRIO_FREEZE, C14::PRIO_COMBAT);
	EXPECT_GT(C14::PRIO_COMBAT, C14::PRIO_AIMBOT);
	EXPECT_GT(C14::PRIO_AIMBOT, C14::PRIO_BALANCE);
	EXPECT_GT(C14::PRIO_BALANCE, C14::PRIO_MOVEMENT);
	EXPECT_GT(C14::PRIO_MOVEMENT, C14::PRIO_NONE);
}

TEST(C14, InputLocksStartUnclaimed)
{
	C14::CInputLocks Locks;
	EXPECT_EQ(Locks.m_DirectionOwner, C14::PRIO_NONE);
	EXPECT_EQ(Locks.m_HookOwner, C14::PRIO_NONE);
	EXPECT_EQ(Locks.m_JumpOwner, C14::PRIO_NONE);
	EXPECT_EQ(Locks.m_TargetOwner, C14::PRIO_NONE);
	EXPECT_EQ(Locks.m_FireOwner, C14::PRIO_NONE);
}
