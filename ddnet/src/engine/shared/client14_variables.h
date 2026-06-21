/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

// This file can be included several times.

#ifndef MACRO_CONFIG_INT
#error "The config macros must be defined"
// This helps IDEs properly syntax highlight the uses of the macro below.
#define MACRO_CONFIG_INT(Name, ScriptName, Def, Min, Max, Save, Desc)
#define MACRO_CONFIG_COL(Name, ScriptName, Def, Save, Desc)
#define MACRO_CONFIG_STR(Name, ScriptName, Len, Def, Save, Desc)
#endif

// CLIENT 14 client-side helper features
MACRO_CONFIG_INT(ClAimbot, cl_aimbot, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Enable aimbot (auto-aim assist)")
MACRO_CONFIG_INT(ClAimbotFov, cl_aimbot_fov, 30, 5, 180, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Aimbot field of view")
MACRO_CONFIG_INT(ClAimbotSilent, cl_aimbot_silent, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Silent aimbot (hide aim corrections)")
MACRO_CONFIG_INT(ClAimbotTarget, cl_aimbot_target, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Target priority: 0=closest to crosshair, 1=closest to player")

MACRO_CONFIG_INT(ClQuickStop, cl_quick_stop, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Quick stop - brake faster when releasing movement keys")
MACRO_CONFIG_INT(ClQuickStopGround, cl_quick_stop_ground, 1, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Only quick stop when grounded")

MACRO_CONFIG_INT(ClAntiFreeze, cl_anti_freeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto hook nearby player when frozen")
MACRO_CONFIG_INT(ClAvoidFreeze, cl_avoid_freeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Avoid freeze tiles by auto-jumping")
MACRO_CONFIG_INT(ClAutoJumpSave, cl_auto_jump_save, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto jump when about to land on freeze tile")

MACRO_CONFIG_INT(ClAutoHook, cl_auto_hook, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto hook when aimbot target is in range")
MACRO_CONFIG_INT(ClHammerBot, cl_hammer_bot, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto hammer nearby enemies")
MACRO_CONFIG_INT(ClHammerBotRange, cl_hammer_bot_range, 0, 0, 400, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Hammer bot engage range in pixels (0 = auto, derived from tee physical size)")
MACRO_CONFIG_INT(ClFreezeUnfreeze, cl_freeze_unfreeze, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Auto hammer frozen teammates to unfreeze them")
MACRO_CONFIG_INT(ClBalanceBot, cl_balance_bot, 0, 0, 1, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Always-on balance bot (align horizontally with nearest player). Also bindable as +balance (hold)")
